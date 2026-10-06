/**
 * Kyoto module/catalog rules for DreamShare.
 * Canonical worker entry point: ../worker.js
 *
 * The worker passes its environment/dependencies explicitly so this module
 * does not rely on cross-module globals.
 */
async function handleKyotoModule(action, body, sess, env, deps) {
  const STORAGE = deps.STORAGE;
  const KYOTO_API_VERSION = deps.KYOTO_API_VERSION;
  const KYOTO_MODULE_FORMAT = deps.KYOTO_MODULE_FORMAT;
  const isSuper = deps.isSuper;
  const communityIndex = deps.communityIndex;
  const kv = env.DREAMSHARE_KV;
  if (!kv) return { ok: false, error: "DREAMSHARE_KV missing", code: "no-kv" };

  if (action === "module_list" || action === "community" || action === "catalog") {
    const face = String(body.face || body.filter || "").toLowerCase();
    let list = [];
    try { list = JSON.parse((await kv.get("module-index")) || "[]"); } catch (_) { list = []; }
    if (!Array.isArray(list)) list = [];
    if (face) list = list.filter((m) => String(m.face || "").toLowerCase() === face);
    if (!face || face === "kyoto") {
      try {
        const ci = await communityIndex(env);
        for (const p of (ci || []).slice(0, 80)) {
          if (!list.some(x => x.id === p.id)) list.push({ id:p.id, name:p.name, face:"kyoto", author:p.author, at:p.updated||p.created||0, community:true });
        }
      } catch (_) {}
    }
    list = list.sort((a,b) => (b.at||0) - (a.at||0)).slice(0, 100);
    return { ok:true, modules:list, community:list, storage:STORAGE, kyotoApi:KYOTO_API_VERSION };
  }

  if (action === "module_get" || action === "community_get") {
    const id = String(body.id || body.community || "").replace(/[^a-zA-Z0-9_-]/g, "").slice(0, 48);
    if (!id) return { ok:false, error:"missing id" };
    const raw = await kv.get("module:" + id);
    if (!raw) {
      const ci = await kv.get("community-instrument:" + id, "json");
      if (ci) return { ok:true, module:ci, instrument:ci };
      return { ok:false, error:"not found" };
    }
    const module = JSON.parse(raw);
    try {
      const index = JSON.parse((await kv.get("module-index")) || "[]");
      const hit = index.find((m) => m && m.id === id);
      if (hit) { hit.downloads = (hit.downloads || 0) + 1; await kv.put("module-index", JSON.stringify(index.slice(-200))); }
    } catch (_) {}
    return { ok:true, module };
  }

  if (action === "module_delete" || action === "catalog_delete" || action === "module_remove") {
    if (!isSuper(sess.user) || sess.role !== "super")
      return { ok:false, error:"admin only" };
    const id = String(body.id || "").replace(/[^a-zA-Z0-9_-]/g, "").slice(0, 48);
    if (!id) return { ok:false, error:"missing id" };
    const raw = await kv.get("module:" + id);
    if (!raw) return { ok:false, error:"not found" };
    const doc = JSON.parse(raw);
    await kv.delete("module:" + id);
    let index = [];
    try { index = JSON.parse((await kv.get("module-index")) || "[]"); } catch (_) { index = []; }
    index = Array.isArray(index) ? index.filter(m => m && m.id !== id) : [];
    await kv.put("module-index", JSON.stringify(index.slice(-200)));
    try {
      const ci = await kv.get("community-instruments-v1", "json");
      if (Array.isArray(ci)) await kv.put("community-instruments-v1", JSON.stringify(ci.filter(x => x && x.id !== id)));
      await kv.delete("community-instrument:" + id);
    } catch (_) {}
    const ownerKey = "user-modules:" + String(doc.author || "").toLowerCase();
    if (ownerKey !== "user-modules:") {
      const owned = JSON.parse((await kv.get(ownerKey)) || "[]").filter(x => x !== id);
      await kv.put(ownerKey, JSON.stringify(owned.slice(-100)));
    }
    return { ok:true, deleted:id };
  }

  if (action === "module_publish" || action === "community_publish") {
    let mod = body.module;
    if (!mod && body.state) {
      mod = typeof body.state === "string" ? JSON.parse(body.state) : body.state;
      if (mod && typeof mod === "object") {
        mod.name = mod.name || body.name;
        mod.face = mod.face || "kyoto";
        mod.format = mod.format || KYOTO_MODULE_FORMAT;
      }
    }
    if (typeof mod === "string") { try { mod = JSON.parse(mod); } catch (_) { return {ok:false,error:"bad module json"}; } }
    if (!mod || typeof mod !== "object") return {ok:false,error:"module required"};
    const format = mod.format || KYOTO_MODULE_FORMAT;
    const okFormat = format === "kyoteppah-module-1" || format === "kyoteppah-effect-1";
    const face = String(mod.face || body.face || "kyoto").toLowerCase();
    const okFace = ["kyoto","fx","chain","effect"].includes(face);
    if (!okFormat || !okFace) return {ok:false,error:"bad module"};
    const id = Date.now().toString(36) + Math.random().toString(36).slice(2,6);
    const theme = String(mod.theme || body.theme || "trippah").toLowerCase().replace(/[^a-z0-9]/g, "").slice(0,32) || "trippah";
    const doc = {
      format, id,
      name:String(mod.name || body.name || "untitled").replace(/[<>]/g,"").trim().slice(0,48)||"untitled",
      face, author:sess.user, theme, grid:Number(mod.grid)||0, free:!!mod.free,
      steps:Array.isArray(mod.steps)?mod.steps.slice(0,16):[],
      slots:Array.isArray(mod.slots)?mod.slots.slice(0,32):[],
      chainLevels: mod.chainLevels && typeof mod.chainLevels === "object" ? {
        enabled: mod.chainLevels.enabled === true,
        levels: Array.isArray(mod.chainLevels.levels) ? mod.chainLevels.levels.slice(0,33).map(v =>
          typeof v === "number" && Number.isFinite(v) ? Math.max(0, Math.min(1, v)) : 1) : []
      } : null,
      widgets:Array.isArray(mod.widgets)?mod.widgets.slice(0,80):[],
      bricks:Array.isArray(mod.bricks)?mod.bricks.slice(0,24):[],
      slotCursor:Number(mod.slotCursor)||0,
      nextUid:Number(mod.nextUid)||1,
      // Full machine reconstruction payload: playground/body/parts/connections/theme/macro data.
      machineDesign: (mod.machineDesign && typeof mod.machineDesign === "object") ? mod.machineDesign : null,
      instrument:face === "kyoto" ? (mod.instrument || null) : null,
      description:String(body.description||mod.description||"").replace(/[<>]/g,"").trim().slice(0,280),
      at:Date.now()
    };
    if (JSON.stringify(doc).length > 180000) return {ok:false,error:"too large"};
    await kv.put("module:"+id, JSON.stringify(doc));
    let index=[];
    try { index=JSON.parse((await kv.get("module-index"))||"[]"); } catch (_) { index=[]; }
    if (!Array.isArray(index)) index=[];
    index.push({id,name:doc.name,face:doc.face,author:doc.author,at:doc.at,downloads:0});
    await kv.put("module-index", JSON.stringify(index.slice(-200)));
    const ownedKey="user-modules:"+String(sess.user||"").toLowerCase();
    const owned=JSON.parse((await kv.get(ownedKey))||"[]"); owned.push(id);
    await kv.put(ownedKey, JSON.stringify(owned.slice(-100)));
    return {ok:true,id,module:doc};
  }
  return null;
}

export { handleKyotoModule };

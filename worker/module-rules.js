/**
 * Splice into the authenticated action switch of the DreamShare worker,
 * before the final "unknown action" return.
 * Binding: DREAMSHARE_KV.
 * Keys: module:{id}  module-index  user-modules:{name}
 *
 * Faces: chain (machine), fx (effect plugin), effect (stacked FX, one block).
 */
export async function handleKyotoModule(action, body, sess, env) {
  const kv = env.DREAMSHARE_KV;
  if (!kv) return { ok: false, error: "DREAMSHARE_KV missing" };
  if (action === "module_list") {
    const face = String(body.face || "");
    let list = JSON.parse((await kv.get("module-index")) || "[]");
    if (face) list = list.filter((m) => m.face === face);
    return { ok: true, modules: list.slice(-80).reverse() };
  }
  if (action === "module_get") {
    const id = String(body.id || "").replace(/[^a-zA-Z0-9_-]/g, "").slice(0, 40);
    const raw = id && (await kv.get("module:" + id));
    if (!raw) return { ok: false, error: "not found" };
    return { ok: true, module: JSON.parse(raw) };
  }
  if (action === "module_publish") {
    const mod = body.module;
    const format = mod && mod.format;
    const face = mod && mod.face;
    const okFormat = format === "kyoteppah-module-1" || format === "kyoteppah-effect-1";
    const okFace = face === "kyoto" || face === "fx" || face === "chain" || face === "effect";
    if (!mod || !okFormat || !okFace) return { ok: false, error: "bad module" };
    const id = Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
    const doc = {
      format,
      id,
      name: String(mod.name || "untitled").slice(0, 48),
      face,
      author: sess.user,
      theme: String(mod.theme || "trippah").slice(0, 32),
      grid: Number(mod.grid) || 0,
      steps: Array.isArray(mod.steps) ? mod.steps.slice(0, 12) : [],
      slots: Array.isArray(mod.slots) ? mod.slots.slice(0, 12) : [],
      widgets: Array.isArray(mod.widgets) ? mod.widgets.slice(0, 80) : [],
      at: Date.now(),
    };
    if (JSON.stringify(doc).length > 180000) return { ok: false, error: "too large" };
    await kv.put("module:" + id, JSON.stringify(doc));
    const index = JSON.parse((await kv.get("module-index")) || "[]");
    index.push({ id, name: doc.name, face: doc.face, author: doc.author, at: doc.at });
    await kv.put("module-index", JSON.stringify(index.slice(-200)));
    const ownedKey = "user-modules:" + String(sess.user || "").toLowerCase();
    const owned = JSON.parse((await kv.get(ownedKey)) || "[]");
    owned.push(id);
    await kv.put(ownedKey, JSON.stringify(owned.slice(-100)));
    return { ok: true, id };
  }
  return null;
}

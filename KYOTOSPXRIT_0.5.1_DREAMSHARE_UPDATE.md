# KyotoSpxrit 0.5.1 — DreamShare Socials + Native Discord #general

## What this update does

DreamShare chat / threads already lived inside KyotoSpxrit (SocialRail live chat, ThreadBoard Reddit/4chan-style board, catalog, friends/DMs). This release finishes the Socials replacement the previous pass started:

1. **Native Discord #general in the VST**  
   - New **DISCORD** rail tab next to CHAT / SOCIALS.  
   - Pulls Kyoto server `#general` through the worker bot (`discord_messages`).  
   - SEND posts into `#general` as the bot, attributed to your DreamShare username (`discord_send`).  
   - The VST **never** holds `DISCORD_BOT_TOKEN` — only the Cloudflare worker does.  
   - Messages are painted as SocialRail bubbles with the **DIS** tag (same path as the existing Discord bridge).

2. **Live chat → Discord mirror**  
   - Ordinary DreamShare `chat_send` lines are best-effort mirrored into `#general` so the Discord room and the VST stay in sync.

3. **Worker API surface (deploy `worker.js`)**  
   - `discord_status` | `discord_channels` | `discord_messages` | `discord_send` | `discord_react`  
   - Fixed guild/channel: Kyoto `#general`  
     `https://discord.com/channels/1518252339864014929/1518252340707197000`  
   - Secret required: `DISCORD_BOT_TOKEN` (and optional `DISCORD_GUILD_ID`).

4. **Client API** (`Source/DreamApi.h` / `.cpp`)  
   - `getDiscordMessages`, `sendDiscordMessage`, `getDiscordStatus`.

5. **Thread board / chat UI**  
   - Existing DreamBoard ThreadBoard (NEW / ACTIVE / FILES, quote `>>No.`, attachments, plugin chips) remains the threads surface.  
   - Chat rail is the DreamShare-style bubble feed; Discord reuses that feed with DIS tags.

## Deploy steps

```bash
# Worker (this repo root)
npx wrangler deploy
npx wrangler secret put DISCORD_BOT_TOKEN
# optional:
npx wrangler secret put DISCORD_GUILD_ID

# Discord bot intents still required (Message Content + Server Members + Presence)
# See discord/README.md for the separate presence/bridge worker if you use it.
```

## Build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

GitHub Actions workflow `Build KYOTRIPPAH` still produces the VST3 zips.

## QOL

- Version bumped to **0.5.1** in CMakeLists.txt.  
- Worker copies kept in sync: `worker.js`, `WORKER_DREAMSHARE.js`, `worker/WORKER_DREAMSHARE.js`, `JV_WORKER.js`.  
- Discord rail is a first-class tab instead of a hidden bridge-only path.  
- Feed refresh button respects CHAT vs DISCORD mode.

## Notes / limits

- Discord lite is **single-channel** (`#general` only) by design — no multi-channel browser in the VST.  
- If `DISCORD_BOT_TOKEN` is missing, the DISCORD tab shows a clear status error; CHAT / THREADS / SOCIALS keep working.  
- JUCE is AGPLv3 unless you hold a commercial licence.

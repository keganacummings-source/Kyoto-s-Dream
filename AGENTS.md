# AGENTS.md — Base44 dev environment notes

## What this project actually is
- **Native VST3 plugins** (KYOTO synth + KYOTRIPPAH FX), C++17/JUCE 8.0.6 via CMake. Built by the GitHub Actions workflow (`.github/workflows/main.yml`) on Windows + macOS runners only. They cannot be built or shown in the Base44 browser preview.
- **DreamShare API** — a Cloudflare Worker (`worker.js` + `module-rules.js` at repo root; `worker/` holds identical canonical copies). This is the only piece that runs in the sandbox preview, served on port 3000.

## Running locally (Base44)
- `docker compose -f docker-compose.base44.yml up -d` starts `npx wrangler dev` (wrangler 4, local mode) from `node:22` with the repo bind-mounted at `/app`.
- `wrangler.toml` binds `DREAMSHARE_KV` with a fake ID — wrangler local mode simulates KV in `.wrangler/state` (gitignored). **No Cloudflare account or credentials are needed for local dev.**
- Wrangler live-reloads on `worker.js` / `module-rules.js` edits — no container restart needed for worker changes; restart only for dependency/compose changes.
- Dependencies: `package.json` pins `wrangler` and is installed at container startup; `node_modules/` is gitignored.

## Data storage quirks
- The worker's source of truth is the **public extendsclass.com JSON bins** (`abffdbc` feed, `ffedede` users) — shared with the production deployment. Logins/chats created in the local preview hit that live shared store; accounts are created on first login (no separate register). Super-admins: Trippah, Goonr.
- KV-backed features (module catalog) run against the local simulation and are NOT shared with production.
- The production worker lives at `https://dreamshare-api.keganacummings.workers.dev`; the VST hardcodes that URL.

## Verify it works
```bash
curl -s http://localhost:3000/ | head -c 300          # feed JSON, "storage":"durable-v4"
curl -s -X POST localhost:3000/ -H 'Content-Type: application/json' \
  -d '{"action":"login","user":"dev_test","pass":"x"}'  # returns token
```
Container healthcheck: `node -e "fetch('http://127.0.0.1:3000/')..."` (no curl/wget in node image).

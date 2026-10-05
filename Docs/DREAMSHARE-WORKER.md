# DreamShare Worker update

`WORKER_DREAMSHARE.js` is the supplied DreamShare worker plus one backward-compatible addition:

- `image_part` accepts authenticated image chunks.
- `GET /?image=<threadId>` reconstructs an image from R2/KV.
- Thread payloads now expose `hasImage`, `imageUrl`, and `imageMime`.

The existing login, presence, thread, comment, reaction, preset, and WAV API actions are unchanged.

For production image sharing, bind `DREAMSHARE_R2` (recommended) or `DREAMSHARE_KV` to the worker. The native VST will refuse image upload if neither storage binding is available.

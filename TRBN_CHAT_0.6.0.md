# Origin tags KYTO / TRBN

| Discord server | Guild ID | Chat badge |
|----------------|----------|------------|
| Kyoto | 1518252339864014929 | **KYTO** |
| TurboNerdos | 1440066181624234106 | **TRBN** |

Bridge `relayIn` sets `tag` from `guild_id`. DreamShare `chat_send` persists `tag` on the message. VST draws the badge from `originTag`.

Redeploy `discord/worker.js` + main DreamShare worker after push.

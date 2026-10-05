# Kyoto's Dream — Cloudflare R2 + KV Setup (Dummies Guide)

This guide sets up the two Cloudflare storage bindings used by the Kyoto's Dream DreamShare Worker:

- `DREAMSHARE_KV` — accounts, sessions/presence, DreamShare feed data, Community Instruments, and other small JSON state.
- `DREAMSHARE_R2` — large WAV/image/VST release files. R2 is strongly recommended for media.

Cloudflare currently lets you create and bind both resources from the dashboard. KV is a globally replicated key-value store, while R2 is object storage. See the official Cloudflare docs linked at the end of this guide.

## Part 1 — What you need before starting

You need:

1. A Cloudflare account.
2. Your existing DreamShare Worker.
3. The file `WORKER_DREAMSHARE.js` from this project.
4. Access to **Workers & Pages** and **R2** in the same Cloudflare account.

Do not create API keys for the VST. The Worker talks to KV/R2 through Cloudflare bindings, so the VST never receives your Cloudflare storage credentials.

## Part 2 — Create the KV namespace

### Step 1

Open the Cloudflare Dashboard.

Go to:

**Workers & Pages → KV**

Choose **Create namespace**.

### Step 2

For the namespace name, use something easy to recognize, for example:

`kyotos-dream-kv`

The exact namespace title does not have to match the binding name.

### Step 3

Create it.

You now have a KV namespace. Cloudflare assigns it an ID automatically.

You do **not** put that ID into the VST.

## Part 3 — Bind KV to DreamShare

This is the important part.

### Step 1

Go to:

**Workers & Pages → your DreamShare Worker**

Your current Worker is the one serving:

`dreamshare-api.keganacummings.workers.dev`

### Step 2

Open:

**Settings → Bindings**

### Step 3

Click:

**Add → KV Namespace**

### Step 4

For the variable/binding name, type EXACTLY:

`DREAMSHARE_KV`

Do not use:

- `dreamshare_kv`
- `KV`
- `KYOTO_KV`
- `COMMUNITY_KV`

The Worker code specifically looks for `env.DREAMSHARE_KV`.

### Step 5

Select the KV namespace you created.

Click **Add** / **Save**.

### Step 6

Click **Deploy** when Cloudflare asks you to deploy the Worker.

## Part 4 — Why KV matters to Kyoto's Dream

The Worker uses KV for things such as:

- Dream accounts
- account/session data
- online presence
- DreamShare feed state
- Community Instrument index
- individual Community Instrument records

The Community Instrument system specifically uses these KV keys:

`community-instruments-v1`

and:

`community-instrument:<id>`

If `DREAMSHARE_KV` is missing, Community Instruments will not work.

## Part 5 — Create the R2 bucket

Now we create the large-file storage.

### Step 1

In Cloudflare Dashboard, open:

**R2 Object Storage**

### Step 2

Click:

**Create bucket**

### Step 3

Give it a simple lowercase name. For example:

`kyotos-dream-media`

R2 bucket names can use lowercase letters, numbers, and hyphens.

### Step 4

Choose the location/storage options Cloudflare offers.

For this project, the normal/default choice is fine unless you have a specific data-location requirement.

### Step 5

Click **Create bucket**.

## Part 6 — Bind R2 to DreamShare

### Step 1

Return to:

**Workers & Pages → your DreamShare Worker**

### Step 2

Open:

**Settings → Bindings**

### Step 3

Click:

**Add → R2 Bucket**

### Step 4

For the binding/variable name type EXACTLY:

`DREAMSHARE_R2`

Again, spelling matters because the Worker checks `env.DREAMSHARE_R2`.

### Step 5

Select your bucket:

`kyotos-dream-media`

(or whatever name you chose).

### Step 6

Save the binding and click **Deploy**.

## Part 7 — Do NOT make the R2 bucket public yet

Leave the bucket private.

The Worker is designed to read/write the objects through its R2 binding.

You do not need to expose your entire bucket to the public internet just to make DreamShare work.

This is especially important because the bucket can contain:

- WAV files
- images
- VST release ZIPs
- other DreamShare media

## Part 8 — Test that both bindings exist

Your Worker contains a diagnostics response that reports whether storage bindings are present.

Open your Worker URL with:

`?health=1`

If the Worker version in this project is deployed, the diagnostic response should report KV/R2 availability.

You want to see the equivalent of:

```json
{
  "kv": true,
  "r2": true
}
```

The exact response can contain additional fields.

If `kv` says `false`:

- check the binding name is exactly `DREAMSHARE_KV`
- check the correct namespace is selected
- deploy the Worker again

If `r2` says `false`:

- check the binding name is exactly `DREAMSHARE_R2`
- check the correct bucket is selected
- deploy the Worker again

## Part 9 — Test Community Instruments

After KV is working:

1. Log into Kyoto's Dream.
2. Build an instrument.
3. Save it.
4. Give it a custom UI/theme.
5. Open Community Instruments.
6. Upload it.

The Worker should create/update the Community Instruments index in KV.

The VST should then be able to request:

`?community=instruments`

and receive the public list.

## Part 10 — Test WAV/image sharing

After R2 is working:

1. Open DreamShare.
2. Post a normal text thread.
3. Post a WAV.
4. Post a PNG/JPG/GIF image.

R2 is the preferred storage location for the larger media objects.

If R2 is missing, the Worker can fall back to KV for some media operations, but that is not the setup you want for a production DreamShare system.

## Part 11 — Put the Kyoto's Dream release ZIP in R2

The Worker also knows about a VST release object.

When we configure the production release, the release ZIP can be stored in R2 and served through the Worker.

The Worker expects its configured release key, rather than exposing the entire R2 bucket.

For the current source, check the `VST_RELEASE` configuration near the top of `WORKER_DREAMSHARE.js` before uploading the release.

## Part 12 — The easiest setup: Dashboard only

If you are a beginner, you do NOT need Wrangler for the first setup.

Use this exact sequence:

1. Create KV namespace.
2. Open Worker.
3. Settings → Bindings.
4. Add KV.
5. Name it `DREAMSHARE_KV`.
6. Select the namespace.
7. Add R2.
8. Name it `DREAMSHARE_R2`.
9. Select the R2 bucket.
10. Deploy.
11. Test the Worker.
12. Test login.
13. Test Community Instruments.
14. Test WAV upload.
15. Test image upload.

## Part 13 — Optional Wrangler method

If you later want command-line deployment, Cloudflare's current Wrangler commands are:

```bash
npx wrangler login
npx wrangler kv namespace create kyotos-dream-kv
npx wrangler r2 bucket create kyotos-dream-media
```

Then bind the generated KV namespace and R2 bucket in your Wrangler configuration.

Example shape:

```jsonc
{
  "kv_namespaces": [
    {
      "binding": "DREAMSHARE_KV",
      "id": "YOUR_KV_NAMESPACE_ID"
    }
  ],
  "r2_buckets": [
    {
      "binding": "DREAMSHARE_R2",
      "bucket_name": "kyotos-dream-media"
    }
  ]
}
```

Do not copy the placeholder ID literally. Use the ID Cloudflare gives you.

## Part 14 — If something says “binding missing”

This is almost always one of three things:

### Problem A — Wrong name

Correct:

`DREAMSHARE_KV`

Incorrect:

`dreamshare-kv`

### Problem B — You forgot Deploy

Cloudflare binding changes do not become active merely because you selected them in Settings.

Click **Deploy**.

### Problem C — Wrong Worker

Make sure you attached the bindings to the Worker actually serving:

`dreamshare-api.keganacummings.workers.dev`

not another test Worker.

## Part 15 — Security rules for beginners

Never put any of these inside the VST:

- Cloudflare API token
- R2 access key
- R2 secret key
- Cloudflare account API key

The VST only talks to your public DreamShare Worker.

The Worker talks to R2/KV using Cloudflare bindings.

That is the correct architecture.

## Official Cloudflare references

Cloudflare's current KV documentation explains both namespace creation and dashboard bindings:

https://developers.cloudflare.com/kv/concepts/kv-namespaces/

Cloudflare's KV getting-started guide covers creation and binding:

https://developers.cloudflare.com/kv/get-started/

Cloudflare's R2 bucket documentation covers dashboard bucket creation:

https://developers.cloudflare.com/r2/buckets/create-buckets/

Cloudflare's R2 Workers documentation covers binding a bucket to a Worker:

https://developers.cloudflare.com/r2/api/workers/workers-api-usage/

Cloudflare's bindings documentation explains why the Worker can access KV/R2 without putting storage credentials into the VST:

https://developers.cloudflare.com/workers/runtime-apis/bindings/

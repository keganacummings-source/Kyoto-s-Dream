#include "DreamAPI.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace kyoto
{
DreamAPI::DreamAPI(juce::String e)
    : endpoint(e.trimCharactersAtEnd("/"))
{
}

juce::var DreamAPI::postJson(
    const juce::var& body,
    const juce::String& token,
    juce::String& error)
{
    juce::DynamicObject::Ptr obj(body.getDynamicObject());

    if (!obj)
    {
        error = "invalid request";
        return {};
    }

    juce::URL u(endpoint);
    auto json = juce::JSON::toString(body);

    auto opt =
        juce::URL::InputStreamOptions(
            juce::URL::ParameterHandling::inPostData)
        .withConnectionTimeoutMs(12000)
        .withExtraHeaders(
            "Content-Type: application/json\r\n"
            "Accept: application/json\r\n"
            + (token.isNotEmpty()
                ? "X-SW-Token: " + token + "\r\n"
                : ""));

    auto in = u.withPOSTData(json).createInputStream(opt);

    if (!in)
    {
        error = "API connection failed";
        return {};
    }

    auto text = in->readEntireStreamAsString();
    auto v = juce::JSON::parse(text);

    if (v.isVoid())
    {
        error = "API returned invalid JSON";
        return {};
    }

    if (auto* d = v.getDynamicObject())
    {
        if (d->hasProperty("error")
            && d->getProperty("ok") == false)
        {
            error =
                d->getProperty("error").toString();
        }
    }

    return v;
}

DreamSession DreamAPI::login(
    const juce::String& u,
    const juce::String& p)
{
    return registerAccount(u, p);
}

DreamSession DreamAPI::registerAccount(
    const juce::String& u,
    const juce::String& p)
{
    DreamSession s;

    auto* o = new juce::DynamicObject();

    // The DreamShare worker intentionally creates an account
    // on first login, so the same endpoint handles both
    // login and registration.
    o->setProperty("action", "login");
    o->setProperty("user", u);
    o->setProperty("pass", p);

    juce::String e;
    auto v = postJson(
        juce::var(o),
        {},
        e);

    if (auto* d = v.getDynamicObject())
    {
        s.ok =
            (bool) d->getProperty("ok");

        s.token =
            d->getProperty("token").toString();

        s.user =
            d->getProperty("user").toString();

        s.role =
            d->getProperty("role").toString();

        s.theme =
            d->getProperty("theme").toString();

        s.onlineCount =
            (int) d->getProperty("onlineCount");
    }

    s.error = e;
    return s;
}

bool DreamAPI::feed(
    std::vector<DreamThread>& out,
    int& onlineCount)
{
    auto u = juce::URL(endpoint);

    auto opt =
        juce::URL::InputStreamOptions(
            juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000);

    auto in = u.createInputStream(opt);

    if (!in)
        return false;

    auto v =
        juce::JSON::parse(
            in->readEntireStreamAsString());

    auto* d = v.getDynamicObject();

    if (!d)
        return false;

    onlineCount =
        (int) d->getProperty("onlineCount");

    out.clear();

    auto a =
        d->getProperty("threads");

    if (auto* arr = a.getArray())
    {
        for (auto& x : *arr)
        {
            if (auto* t =
                    x.getDynamicObject())
            {
                DreamThread q;

                q.id =
                    t->getProperty("id")
                        .toString();

                q.user =
                    t->getProperty("user")
                        .toString();

                q.title =
                    t->getProperty("title")
                        .toString();

                q.text =
                    t->getProperty("text")
                        .toString();

                q.audioUrl =
                    t->getProperty("audioUrl")
                        .toString();

                q.imageUrl =
                    t->getProperty("imageUrl")
                        .toString();

                q.hasAudio =
                    (bool) t->getProperty("hasAudio");

                q.hasImage =
                    (bool) t->getProperty("hasImage")
                    || q.imageUrl.isNotEmpty();

                q.at =
                    (int64_t) t->getProperty("at");

                out.push_back(q);
            }
        }
    }

    return true;
}

bool DreamAPI::postText(
    const juce::String& t,
    const juce::String& text,
    const juce::String& title)
{
    auto* o = new juce::DynamicObject();

    o->setProperty("action", "create_thread");
    o->setProperty("token", t);
    o->setProperty("text", text);
    o->setProperty("title", title);

    juce::String e;

    auto v =
        postJson(
            juce::var(o),
            t,
            e);

    return v.getDynamicObject()
        && (bool) v.getDynamicObject()
            ->getProperty("ok");
}

bool DreamAPI::postAudio(
    const juce::String& t,
    const juce::File& f,
    const juce::String& text)
{
    if (!f.existsAsFile())
        return false;

    const auto fileBytes =
        f.getSize();

    constexpr int64_t maxAudio =
        75LL * 1024LL * 1024LL;

    constexpr int chunkBytes =
        4 * 1024 * 1024;

    if (fileBytes <= 0
        || fileBytes > maxAudio)
        return false;

    const int parts =
        (int) ((fileBytes + chunkBytes - 1)
               / chunkBytes);

    if (parts < 1 || parts > 20)
        return false;

    // The Worker accepts raw WAV pieces at ?op=audio_part.
    // Keep the identifier short and limited to safe characters.
    const auto upload =
        "vstwav-" +
        juce::String(
            juce::Time::getCurrentTime()
                .toMilliseconds());

    juce::FileInputStream input(f);

    if (!input.openedOk())
        return false;

    juce::String sink;

    for (int index = 0;
         index < parts;
         ++index)
    {
        const auto remaining =
            fileBytes
            - static_cast<int64_t>(index)
                * chunkBytes;

        const int bytesThisPart =
            (int) juce::jmin<int64_t>(
                chunkBytes,
                remaining);

        juce::MemoryBlock bytes(
            (size_t) bytesThisPart);

        if (input.read(
                bytes.getData(),
                bytesThisPart)
            != bytesThisPart)
        {
            return false;
        }

        auto partUrl =
            juce::URL(endpoint)
                .withParameter(
                    "op",
                    "audio_part")
                .withPOSTData(bytes);

        auto options =
            juce::URL::InputStreamOptions(
                juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(30000)
            .withExtraHeaders(
                "Content-Type: audio/wav\r\n"
                "X-SW-Token: " + t + "\r\n"
                "X-SW-Upload: " + upload + "\r\n"
                "X-SW-Index: "
                    + juce::String(index)
                    + "\r\n"
                "X-SW-Parts: "
                    + juce::String(parts)
                    + "\r\n");

        auto response =
            partUrl.createInputStream(options);

        if (!response)
            return false;

        auto result =
            juce::JSON::parse(
                response->readEntireStreamAsString());

        auto* d =
            result.getDynamicObject();

        if (!d
            || !(bool) d->getProperty("ok"))
        {
            return false;
        }

        sink =
            d->getProperty("sink")
                .toString();
    }

    if (sink.isEmpty())
        return false;

    auto* o = new juce::DynamicObject();

    o->setProperty(
        "action",
        "create_thread");

    o->setProperty(
        "token",
        t);

    o->setProperty(
        "text",
        text);

    o->setProperty(
        "title",
        f.getFileNameWithoutExtension());

    o->setProperty(
        "hasAudio",
        true);

    o->setProperty(
        "audioId",
        "a" + upload);

    o->setProperty(
        "audioStore",
        sink);

    o->setProperty(
        "audioUpload",
        upload);

    o->setProperty(
        "audioParts",
        parts);

    o->setProperty(
        "audioBytes",
        fileBytes);

    o->setProperty(
        "audioMime",
        "audio/wav");

    juce::String e;

    auto v =
        postJson(
            juce::var(o),
            t,
            e);

    return v.getDynamicObject()
        && (bool) v.getDynamicObject()
            ->getProperty("ok");
}

bool DreamAPI::postImage(
    const juce::String& t,
    const juce::File& f,
    const juce::String& text)
{
    auto bytes = f.loadFileAsData();

    if (bytes.isEmpty())
        return false;

    auto b64 =
        bytes.toBase64Encoding();

    const int chunk = 600000;
    const int parts =
        (b64.length() + chunk - 1)
        / chunk;

    if (parts > 20)
        return false;

    auto upload =
        "img" +
        juce::String(
            juce::Time::getCurrentTime()
                .toMilliseconds());

    auto mime =
        f.hasFileExtension("png")
            ? "image/png"
            : f.hasFileExtension("jpg")
                || f.hasFileExtension("jpeg")
                    ? "image/jpeg"
                    : f.hasFileExtension("gif")
                        ? "image/gif"
                        : "application/octet-stream";

    juce::String sink = "bin";

    for (int i = 0; i < parts; ++i)
    {
        auto* o =
            new juce::DynamicObject();

        o->setProperty(
            "action",
            "image_part");

        o->setProperty(
            "token",
            t);

        o->setProperty(
            "upload",
            upload);

        o->setProperty(
            "index",
            i);

        o->setProperty(
            "parts",
            parts);

        o->setProperty(
            "mime",
            mime);

        o->setProperty(
            "b64",
            b64.substring(
                i * chunk,
                juce::jmin(
                    b64.length(),
                    (i + 1) * chunk)));

        juce::String e;

        auto v =
            postJson(
                juce::var(o),
                t,
                e);

        auto* d =
            v.getDynamicObject();

        if (!d
            || (bool) d->getProperty("ok") != true)
            return false;

        sink =
            d->getProperty("sink")
                .toString();
    }

    auto* o =
        new juce::DynamicObject();

    o->setProperty(
        "action",
        "create_thread");

    o->setProperty(
        "token",
        t);

    o->setProperty(
        "text",
        text);

    o->setProperty(
        "title",
        f.getFileNameWithoutExtension());

    o->setProperty(
        "hasImage",
        true);

    o->setProperty(
        "imageStore",
        sink);

    o->setProperty(
        "imageUpload",
        upload);

    o->setProperty(
        "imageParts",
        parts);

    o->setProperty(
        "imageBytes",
        (int64_t) bytes.getSize());

    o->setProperty(
        "imageMime",
        mime);

    juce::String e;

    auto v =
        postJson(
            juce::var(o),
            t,
            e);

    return v.getDynamicObject()
        && (bool) v.getDynamicObject()
            ->getProperty("ok");
}

bool DreamAPI::communityList(
    std::vector<CommunityInstrument>& out)
{
    out.clear();

    auto u =
        juce::URL(endpoint)
            .withParameter(
                "community",
                "instruments");

    auto opt =
        juce::URL::InputStreamOptions(
            juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000);

    auto in =
        u.createInputStream(opt);

    if (!in)
        return false;

    auto v =
        juce::JSON::parse(
            in->readEntireStreamAsString());

    auto* d =
        v.getDynamicObject();

    if (!d)
        return false;

    if (auto* a =
            d->getProperty("instruments")
                .getArray())
    {
        for (auto& x : *a)
        {
            if (auto* o =
                    x.getDynamicObject())
            {
                CommunityInstrument q;

                q.id =
                    o->getProperty("id").toString();

                q.name =
                    o->getProperty("name").toString();

                q.author =
                    o->getProperty("author").toString();

                q.description =
                    o->getProperty("description")
                        .toString();

                q.created =
                    o->getProperty("created")
                        .toString();

                q.downloads =
                    (int) o->getProperty("downloads");

                out.push_back(q);
            }
        }
    }

    return true;
}

bool DreamAPI::communityGet(
    const juce::String& id,
    CommunityInstrument& out)
{
    auto u =
        juce::URL(endpoint)
            .withParameter(
                "community",
                id);

    auto opt =
        juce::URL::InputStreamOptions(
            juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000);

    auto in =
        u.createInputStream(opt);

    if (!in)
        return false;

    auto v =
        juce::JSON::parse(
            in->readEntireStreamAsString());

    auto* o =
        v.getDynamicObject();

    if (!o
        || !(bool) o->getProperty("ok"))
        return false;

    out.id =
        o->getProperty("id").toString();

    out.name =
        o->getProperty("name").toString();

    out.author =
        o->getProperty("author").toString();

    out.description =
        o->getProperty("description").toString();

    out.created =
        o->getProperty("created").toString();

    out.downloads =
        (int) o->getProperty("downloads");

    out.state =
        o->getProperty("state");

    return out.state.isObject();
}

bool DreamAPI::setTheme(
    const juce::String& t,
    const juce::String& theme)
{
    auto* o =
        new juce::DynamicObject();

    o->setProperty(
        "action",
        "set_theme");

    o->setProperty(
        "token",
        t);

    o->setProperty(
        "theme",
        theme);

    juce::String e;

    auto v =
        postJson(
            juce::var(o),
            t,
            e);

    auto* d =
        v.getDynamicObject();

    return d
        && (bool) d->getProperty("ok");
}

bool DreamAPI::communityPublish(
    const juce::String& t,
    const CommunityInstrument& item)
{
    auto* o =
        new juce::DynamicObject();

    o->setProperty(
        "action",
        "community_publish");

    o->setProperty(
        "token",
        t);

    o->setProperty(
        "name",
        item.name);

    o->setProperty(
        "description",
        item.description);

    o->setProperty(
        "state",
        item.state);

    juce::String err;

    auto v =
        postJson(
            juce::var(o),
            t,
            err);

    auto* d =
        v.getDynamicObject();

    return d
        && (bool) d->getProperty("ok");
}

} // namespace kyoto

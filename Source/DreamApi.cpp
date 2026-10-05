#include "DreamApi.h"

namespace kt
{
static const char* kEndpoint = "https://dreamshare-api.keganacummings.workers.dev/";

static juce::String readUrl(const juce::URL& url)
{
    auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                                             .withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n")
                                             .withConnectionTimeoutMs(12000));
    if (stream == nullptr)
        return {};
    return stream->readEntireStreamAsString();
}

DreamResult postAction(const juce::String& action, juce::var body, const juce::String& token)
{
    DreamResult r;
    if (! body.isObject())
        body = juce::var(new juce::DynamicObject());
    body.getDynamicObject()->setProperty("action", action);
    if (token.isNotEmpty())
        body.getDynamicObject()->setProperty("token", token);
    const auto text = juce::JSON::toString(body);
    const auto raw = readUrl(juce::URL(kEndpoint).withPOSTData(text));
    r.raw = raw;
    auto parsed = juce::JSON::parse(raw);
    if (auto* o = parsed.getDynamicObject())
    {
        r.ok = (bool) o->getProperty("ok");
        r.error = o->getProperty("error").toString();
        r.token = o->getProperty("token").toString();
        r.user = o->getProperty("user").toString();
        r.body = o->getProperty("body").toString();
        if (r.body.isEmpty() && o->hasProperty("feed"))
            r.body = juce::JSON::toString(o->getProperty("feed"));
        if (r.body.isEmpty() && o->hasProperty("messages"))
            r.body = juce::JSON::toString(o->getProperty("messages"));
    }
    else
        r.error = raw.isEmpty() ? "DreamShare did not answer" : "Bad DreamShare response";
    return r;
}

DreamResult login(const juce::String& user, const juce::String& pass)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("user", user);
    o->setProperty("pass", pass);
    return postAction("login", juce::var(o), {});
}

DreamResult sendChat(const juce::String& token, const juce::String& text)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("text", text);
    return postAction("chat", juce::var(o), token);
}

DreamResult getFeed(const juce::String& token)
{
    // Prefer GET-style feed for simplicity; fall back to action
    DreamResult r;
    const auto raw = readUrl(juce::URL(kEndpoint).withParameter("feed", "1")
                                                 .withParameter("token", token));
    r.raw = raw;
    if (raw.isNotEmpty())
    {
        auto parsed = juce::JSON::parse(raw);
        if (auto* arr = parsed.getArray())
        {
            juce::String log;
            for (auto& item : *arr)
            {
                if (auto* m = item.getDynamicObject())
                {
                    log << m->getProperty("user").toString() << ": "
                        << m->getProperty("text").toString() << "\n";
                }
            }
            r.ok = true;
            r.body = log;
            return r;
        }
        if (auto* o = parsed.getDynamicObject())
        {
            r.ok = (bool) o->getProperty("ok");
            r.body = o->getProperty("body").toString();
            if (r.body.isEmpty())
                r.body = juce::JSON::toString(o->getProperty("messages"));
            return r;
        }
    }
    return postAction("feed", juce::var(new juce::DynamicObject()), token);
}

DreamResult getCatalog(const juce::String& token)
{
    return postAction("community", juce::var(new juce::DynamicObject()), token);
}

DreamResult publishModule(const juce::String& token, const juce::String& name, const juce::String& jsonBody)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("name", name);
    o->setProperty("module", juce::JSON::parse(jsonBody));
    return postAction("community_publish", juce::var(o), token);
}
}

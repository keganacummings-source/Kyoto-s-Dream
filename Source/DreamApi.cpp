#include "DreamApi.h"

namespace kt
{
static const char* kEndpoint = "https://dreamshare-api.keganacummings.workers.dev/";

static juce::String readUrl(const juce::URL& url, const juce::String& extraHeaders = {})
{
    auto opts = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                    .withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n" + extraHeaders)
                    .withConnectionTimeoutMs(15000);
    auto stream = url.createInputStream(opts);
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
    r.parsed = juce::JSON::parse(raw);
    if (auto* o = r.parsed.getDynamicObject())
    {
        r.ok = (bool) o->getProperty("ok");
        r.error = o->getProperty("error").toString();
        r.token = o->getProperty("token").toString();
        r.user = o->getProperty("user").toString();
        if (o->hasProperty("chat"))
            r.body = juce::JSON::toString(o->getProperty("chat"));
        else if (o->hasProperty("modules"))
            r.body = juce::JSON::toString(o->getProperty("modules"));
        else if (o->hasProperty("threads"))
            r.body = juce::JSON::toString(o->getProperty("threads"));
        else
            r.body = o->getProperty("body").toString();
        if (! r.ok && r.error.isEmpty() && raw.isNotEmpty())
            r.error = "DreamShare rejected " + action;
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
    return postAction("chat_send", juce::var(o), token);
}

DreamResult getFeed(const juce::String& token)
{
    return postAction("chat_list", juce::var(new juce::DynamicObject()), token);
}

DreamResult getThreads(const juce::String& token)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("limit", 20);
    return postAction("list_threads", juce::var(o), token);
}

DreamResult getCatalog(const juce::String& token)
{
    return postAction("module_list", juce::var(new juce::DynamicObject()), token);
}

DreamResult getModule(const juce::String& token, const juce::String& id)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("id", id);
    return postAction("module_get", juce::var(o), token);
}

DreamResult publishModule(const juce::String& token, const juce::String& name, const juce::String& jsonBody)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("name", name);
    o->setProperty("module", juce::JSON::parse(jsonBody));
    return postAction("module_publish", juce::var(o), token);
}
}

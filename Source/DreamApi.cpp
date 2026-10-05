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
    }
    else
        r.error = raw.isEmpty() ? "DreamShare did not answer" : "Bad DreamShare response";
    return r;
}

juce::var getFeed()
{
    const auto raw = readUrl(juce::URL(kEndpoint).withParameter("feed", "1"));
    return juce::JSON::parse(raw);
}
}

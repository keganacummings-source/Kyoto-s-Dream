#include "UpdateService.h"

namespace kyoto
{
    UpdateInfo UpdateService::check(const juce::String& current)
    {
        UpdateInfo i;

        auto options =
            juce::URL::InputStreamOptions(
                juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(8000);

        auto in = juce::URL(manifestUrl).createInputStream(options);

        if (!in)
            return i;

        auto v = juce::JSON::parse(
            in->readEntireStreamAsString());

        if (auto* d = v.getDynamicObject())
        {
            i.version = d->getProperty("version").toString();
            i.url = d->getProperty("package").toString();
            i.notes = d->getProperty("notes").toString();
            i.available =
                i.version.isNotEmpty() &&
                i.version != current;
        }

        return i;
    }

    juce::File UpdateService::download(
        const juce::String& url)
    {
        auto dir =
            juce::File::getSpecialLocation(
                juce::File::tempDirectory)
                .getChildFile("KyotoSpxrit");

        dir.createDirectory();

        auto file = dir.getChildFile("update.zip");

        auto options =
            juce::URL::InputStreamOptions(
                juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(30000);

        if (auto in =
                juce::URL(url).createInputStream(options))
        {
            file.replaceWithData(nullptr, 0);
            file.create();

            juce::FileOutputStream out(file);

            if (out.openedOk())
            {
                out.writeFromInputStream(*in, -1);
                out.flush();
                return file;
            }
        }

        return {};
    }
}

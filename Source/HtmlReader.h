#pragma once
#include <JuceHeader.h>
#include "Themes.h"

// ── Hidden HTML reader ──────────────────────────────────────────────────
// No button, no menu item, no tooltip.  The only way to trigger this overlay
// is to physically drop an .html file inside the .vst3 bundle directory.  When
// the editor constructs it scans the bundle; if a file is found the content
// is displayed full-screen.  Esc or clicking the dimmed border dismisses it.
struct HtmlOverlay : public juce::Component
{
    explicit HtmlOverlay(const kt::ThemePalette& t) : theme(t)
    {
        setOpaque(false);
        content.setMultiLine(true);
        content.setReadOnly(true);
        content.setScrollbarsShown(true);
        content.setCaretVisible(false);
        content.setFont(juce::Font(15.f));
        addAndMakeVisible(content);

        closeBtn.setButtonText(juce::CharPointer_UTF8("\xc3\x97"));  // ×
        closeBtn.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeBtn);

        setTheme(t);
    }

    void setTheme(const kt::ThemePalette& t)
    {
        theme = t;
        content.setColour(juce::TextEditor::backgroundColourId, kt::c(theme.bg).brighter(0.03f));
        content.setColour(juce::TextEditor::textColourId, kt::c(theme.text));
        content.setColour(juce::TextEditor::outlineColourId, kt::c(theme.border));
        content.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
        closeBtn.setColour(juce::TextButton::buttonColourId, kt::c(theme.panel));
        closeBtn.setColour(juce::TextButton::textColourOffId, kt::c(theme.text));
        closeBtn.setColour(juce::TextButton::buttonOnColourId, kt::c(theme.accent));
        repaint();
    }

    void setContent(const juce::String& html)
    {
        content.setText(stripHtml(html), juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        // Dim the plugin UI behind the overlay.
        g.setColour(kt::c(theme.bg).withAlpha(0.88f));
        g.fillAll();

        auto panel = getLocalBounds().reduced(24).toFloat();
        g.setColour(kt::c(theme.panel));
        g.fillRoundedRectangle(panel, 14.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(panel, 14.f, 1.5f);

        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 16.f, true));
        g.drawText(juce::String("  ") + fileName, panel.removeFromTop(42.f),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        closeBtn.setBounds(getWidth() - 52, 32, 30, 30);
        content.setBounds(40, 74, getWidth() - 80, getHeight() - 98);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Click on the dimmed margin (outside the inner panel) closes the overlay.
        auto inner = getLocalBounds().reduced(24);
        if (! inner.contains(e.getPosition()))
            setVisible(false);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            setVisible(false);
            return true;
        }
        return false;
    }

    juce::String fileName;

private:
    static juce::String stripHtml(const juce::String& html)
    {
        juce::String out;
        juce::String tag;
        bool inTag = false;
        auto flushTag = [&]()
        {
            const auto t = tag.toLowerCase();
            if (t.startsWith("br") || t == "/p" || t == "/div" || t == "/tr" || t == "/li"
                || t == "/h1" || t == "/h2" || t == "/h3" || t == "/table")
                out += "\n";
            else if (t == "li" || t.startsWith("li "))
                out += "\n  • ";
            else if (t == "h1" || t.startsWith("h1 ") || t == "h2" || t.startsWith("h2 ")
                     || t == "h3" || t.startsWith("h3 "))
                out += "\n\n";
            else if (t == "p" || t.startsWith("p ") || t == "div" || t.startsWith("div "))
                out += "\n";
            else if (t == "tr" || t.startsWith("tr "))
                out += "\n";
            else if (t == "td" || t.startsWith("td ") || t == "th" || t.startsWith("th "))
                out += "  ";
            tag.clear();
        };
        for (int i = 0; i < html.length(); ++i)
        {
            const auto ch = html[i];
            if (ch == '<') { inTag = true; tag.clear(); continue; }
            if (ch == '>') { inTag = false; flushTag(); continue; }
            if (inTag) { tag += ch; continue; }
            out += ch;
        }
        out = out.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">")
                 .replace("&nbsp;", " ").replace("\r\n", "\n").replace("\r", "\n");
        while (out.contains("\n\n\n"))
            out = out.replace("\n\n\n", "\n\n");
        return out.trim();
    }

    kt::ThemePalette theme;
    juce::TextEditor content;
    juce::TextButton closeBtn;
};

// Walks up from the plugin binary to the .vst3 bundle root and searches it
// recursively for .html / .htm files.  Returns the first match or an invalid File.
inline juce::File ktFindHtmlInBundle()
{
    auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    // binary → platform dir → Contents → bundle root
    auto contents = exe.getParentDirectory().getParentDirectory();
    auto bundleRoot = contents.getParentDirectory();

    // If the path doesn't look like a bundle, fall back to searching the
    // binary's parent and grandparent directories.
    juce::Array<juce::File> dirs;
    if (bundleRoot.exists() && bundleRoot.isDirectory())
        dirs.add(bundleRoot);
    dirs.add(contents);
    dirs.add(exe.getParentDirectory());

    for (auto& dir : dirs)
    {
        if (! dir.isDirectory()) continue;
        auto html = dir.findChildFiles(juce::File::findFiles, true, "*.html");
        auto htm  = dir.findChildFiles(juce::File::findFiles, true, "*.htm");
        html.addArray(htm);
        if (html.size() > 0)
            return html[0];
    }
    return {};
}

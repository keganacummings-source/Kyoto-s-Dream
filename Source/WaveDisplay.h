#pragma once
#include "PluginProcessor.h"
#include "Themes.h"

// Reusable visualization surface. The processor owns the lock-free-ish sample ring;
// this component only consumes a small snapshot at UI rate and never asks the audio thread
// to calculate a waveform during paint().
class WaveDisplay final : public juce::Component, private juce::Timer
{
public:
    enum class Mode { Oscilloscope, StereoScope, Spectrum, Spectrogram, GhostWave, BrokenWave, VectorXY,
                      Bars, Radial, Lissajous, Mirror, Particles, Terrain };
    static constexpr int kModeCount = 13;
    static const char* modeName(int i)
    {
        static const char* n[] = { "OSCILLOSCOPE", "STEREO SCOPE", "SPECTRUM", "SPECTROGRAM", "GHOST WAVE", "BROKEN WAVE", "VECTOR / XY",
                                   "BAR METER", "RADIAL", "LISSAJOUS", "MIRROR", "PARTICLES", "TERRAIN" };
        return n[juce::jlimit(0, kModeCount - 1, i)];
    }
    Mode getMode() const { return mode; }
    explicit WaveDisplay(KyotoAudioProcessor& p) : processor(p) { startTimerHz(20); }
    void setMode(Mode m) { mode = m; repaint(); }
    void setTheme(const kt::ThemePalette& t) { theme = t; repaint(); }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(4.f);
        g.setColour(kt::c(theme.bg).brighter(0.02f)); g.fillRoundedRectangle(r, theme.cornerRadius - 2.f);
        g.setColour(kt::c(theme.border).withAlpha(0.70f)); g.drawRoundedRectangle(r, theme.cornerRadius - 2.f, 1.f);
        float samples[256] {};
        processor.copyScope(samples, 256);
        const auto accent = kt::c(theme.accent);
        if ((int) mode >= (int) Mode::Bars)
        {
            paintExtra(g, r.reduced(6.f), samples, accent);
            g.setColour(kt::c(theme.muted));
            g.setFont(kt::font(theme, 8.f, true));
            g.drawText(modeName((int) mode), r.getX() + 8, r.getBottom() - 15.f, r.getWidth() - 16.f, 11, juce::Justification::centred);
            return;
        }
        g.setColour(accent.withAlpha(mode == Mode::BrokenWave ? 0.72f : 0.9f));
        juce::Path p;
        const float mid = r.getCentreY();
        p.startNewSubPath(r.getX() + 6.f, mid);
        for (int i = 0; i < 256; ++i)
        {
            float x = r.getX() + 6.f + (r.getWidth() - 12.f) * (float)i / 255.f;
            float y = samples[i];
            if (mode == Mode::BrokenWave) y = std::round(y * 8.f) / 8.f;
            if (mode == Mode::Spectrogram) y = std::sin((float)i * 0.08f + std::abs(y) * 4.f) * std::abs(y);
            if (mode == Mode::StereoScope || mode == Mode::VectorXY) y *= 0.72f;
            p.lineTo(x, mid - y * r.getHeight() * 0.38f);
        }
        g.strokePath(p, juce::PathStrokeType(1.4f));
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 8.f, true));
        g.drawText(modeName((int) mode), r.getX()+8, r.getBottom()-15.f, r.getWidth()-16.f, 11, juce::Justification::centred);
    }
private:
    void paintExtra(juce::Graphics& g, juce::Rectangle<float> r, const float* s, juce::Colour accent) const
    {
        const float cx = r.getCentreX(), cy = r.getCentreY();
        switch (mode)
        {
            case Mode::Bars:
                for (int b = 0; b < 32; ++b)
                {
                    float a = 0.f;
                    for (int k = 0; k < 8; ++k) a = juce::jmax(a, std::abs(s[b * 8 + k]));
                    const float h = juce::jlimit(0.02f, 1.f, a * 1.6f) * (r.getHeight() - 14.f);
                    const float w = r.getWidth() / 32.f;
                    g.setColour(accent.withAlpha(0.85f));
                    g.fillRect(r.getX() + (float) b * w + 1.f, r.getBottom() - 14.f - h, w - 2.f, h);
                }
                break;
            case Mode::Radial:
            {
                juce::Path p;
                const float base = juce::jmin(r.getWidth(), r.getHeight()) * 0.28f;
                for (int i = 0; i <= 128; ++i)
                {
                    const float ang = juce::MathConstants<float>::twoPi * (float) i / 128.f;
                    const float rad = base + s[(i % 128) * 2] * base * 0.9f;
                    const float x = cx + std::cos(ang) * rad, y = cy + std::sin(ang) * rad;
                    if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
                }
                p.closeSubPath();
                g.setColour(accent.withAlpha(0.9f)); g.strokePath(p, juce::PathStrokeType(1.4f));
                break;
            }
            case Mode::Lissajous:
            {
                juce::Path p;
                const float sc = juce::jmin(r.getWidth(), r.getHeight()) * 0.42f;
                for (int i = 0; i < 255; ++i)
                {
                    const float x = cx + s[i] * sc, y = cy + s[i + 1] * sc;
                    if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
                }
                g.setColour(accent.withAlpha(0.9f)); g.strokePath(p, juce::PathStrokeType(1.2f));
                break;
            }
            case Mode::Mirror:
            {
                juce::Path top, bot;
                for (int i = 0; i < 256; ++i)
                {
                    const float x = r.getX() + r.getWidth() * (float) i / 255.f;
                    const float h = std::abs(s[i]) * r.getHeight() * 0.42f;
                    if (i == 0) { top.startNewSubPath(x, cy - h); bot.startNewSubPath(x, cy + h); }
                    else { top.lineTo(x, cy - h); bot.lineTo(x, cy + h); }
                }
                g.setColour(accent.withAlpha(0.9f)); g.strokePath(top, juce::PathStrokeType(1.4f));
                g.setColour(accent.withAlpha(0.45f)); g.strokePath(bot, juce::PathStrokeType(1.4f));
                break;
            }
            case Mode::Particles:
                for (int i = 0; i < 64; ++i)
                {
                    const float a = std::abs(s[i * 4]);
                    const float x = r.getX() + r.getWidth() * (float) i / 63.f;
                    const float y = cy + std::sin((float) i * 1.7f) * r.getHeight() * 0.3f * (0.3f + a);
                    const float d = 2.f + a * 9.f;
                    g.setColour(accent.withAlpha(0.35f + a * 0.6f));
                    g.fillEllipse(x - d * 0.5f, y - d * 0.5f, d, d);
                }
                break;
            case Mode::Terrain:
            {
                for (int row = 0; row < 5; ++row)
                {
                    juce::Path p;
                    const float yo = r.getY() + r.getHeight() * (0.25f + 0.15f * (float) row);
                    for (int i = 0; i < 64; ++i)
                    {
                        const float x = r.getX() + r.getWidth() * (float) i / 63.f;
                        const float y = yo - std::abs(s[(i * 4 + row * 11) % 256]) * r.getHeight() * 0.22f;
                        if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
                    }
                    g.setColour(accent.withAlpha(0.25f + 0.15f * (float) row));
                    g.strokePath(p, juce::PathStrokeType(1.2f));
                }
                break;
            }
            default: break;
        }
    }
    void timerCallback() override { repaint(); }
    KyotoAudioProcessor& processor;
    Mode mode = Mode::Oscilloscope;
    kt::ThemePalette theme = kt::kThemes[0];
};

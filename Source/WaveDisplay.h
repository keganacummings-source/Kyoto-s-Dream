#pragma once
#include "PluginProcessor.h"
#include "Themes.h"

// Reusable visualization surface. The processor owns the lock-free-ish sample ring;
// this component only consumes a small snapshot at UI rate and never asks the audio thread
// to calculate a waveform during paint().
//
// Visualizer modes 0-6 are the original line-based scopes. Modes 7+ are audio-responsive
// visualizers rebuilt from open-source concepts (spectrum-analyser bars, radial displays,
// particle fields, LED VU meters, blocky waveform bars, ripple rings) and recoloured to
// follow the active kt::ThemePalette.
class WaveDisplay final : public juce::Component, private juce::Timer
{
public:
    enum class Mode {
        Oscilloscope, StereoScope, Spectrum, Spectrogram, GhostWave, BrokenWave, VectorXY,
        Bars, Radial, Particles, PeakMeter, WaveformBars, Ripple
    };

    static constexpr int kModeCount = 13;

    explicit WaveDisplay(KyotoAudioProcessor& p) : processor(p) { startTimerHz(30); }
    void setMode(Mode m) { mode = m; repaint(); }
    void setModeInt(int m) { mode = (Mode) juce::jlimit(0, kModeCount - 1, m); repaint(); }
    int getModeInt() const { return (int) mode; }
    void setTheme(const kt::ThemePalette& t) { theme = t; repaint(); }

    // Simple in-place radix-2 FFT (n must be a power of two). Used by Bars and Radial
    // to turn the time-domain scope snapshot into a frequency magnitude spectrum.
    static void fft(float* re, float* im, int n)
    {
        for (int i = 1, j = 0; i < n; ++i)
        {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); }
        }
        for (int len = 2; len <= n; len <<= 1)
        {
            const float ang = -6.28318530718f / (float) len;
            const float wRe = std::cos(ang), wIm = std::sin(ang);
            for (int i = 0; i < n; i += len)
            {
                float curRe = 1.f, curIm = 0.f;
                for (int k = 0; k < len / 2; ++k)
                {
                    const float tRe = curRe * re[i + k + len / 2] - curIm * im[i + k + len / 2];
                    const float tIm = curRe * im[i + k + len / 2] + curIm * re[i + k + len / 2];
                    re[i + k + len / 2] = re[i + k] - tRe;
                    im[i + k + len / 2] = im[i + k] - tIm;
                    re[i + k] += tRe;
                    im[i + k] += tIm;
                    const float nRe = curRe * wRe - curIm * wIm;
                    curIm = curRe * wIm + curIm * wRe;
                    curRe = nRe;
                }
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(4.f);
        g.setColour(kt::c(theme.bg).brighter(0.02f)); g.fillRoundedRectangle(r, theme.cornerRadius - 2.f);
        g.setColour(kt::c(theme.border).withAlpha(0.70f)); g.drawRoundedRectangle(r, theme.cornerRadius - 2.f, 1.f);
        float samples[256] {};
        processor.copyScope(samples, 256);
        const auto accent = kt::c(theme.accent);
        const auto muted = kt::c(theme.muted);
        const auto panel = kt::c(theme.panel);

        switch (mode)
        {
        case Mode::Bars:         paintBars(g, r, samples, accent, muted, panel);        break;
        case Mode::Radial:       paintRadial(g, r, samples, accent, muted, panel);      break;
        case Mode::Particles:     paintParticles(g, r, samples, accent, muted, panel);  break;
        case Mode::PeakMeter:     paintPeakMeter(g, r, samples, accent, muted, panel);   break;
        case Mode::WaveformBars:  paintWaveformBars(g, r, samples, accent, muted, panel); break;
        case Mode::Ripple:        paintRipple(g, r, samples, accent, muted, panel);     break;
        case Mode::Oscilloscope:
        case Mode::StereoScope:
        case Mode::Spectrum:
        case Mode::Spectrogram:
        case Mode::GhostWave:
        case Mode::BrokenWave:
        case Mode::VectorXY:      paintLineScope(g, r, samples, accent, muted);         break;
        }

        g.setColour(muted);
        g.setFont(kt::font(theme, 8.f, true));
        g.drawText(modeLabel(), r.getX()+8, r.getBottom()-15.f, r.getWidth()-16.f, 11, juce::Justification::centred);
    }
private:
    void timerCallback() override { repaint(); }

    // ---- Original line-based scopes (modes 0-6) ----
    void paintLineScope(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                        juce::Colour accent, juce::Colour muted)
    {
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
        juce::ignoreUnused(muted);
    }

    // ---- Mode 7: Bars — classic spectrum-analyser vertical bars ----
    // Concept: open-source "bar spectrum" visualisers (e.g. projectM, butterchurn).
    // Rebuilt with a 256-point FFT and theme-coloured gradient bars.
    void paintBars(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                   juce::Colour accent, juce::Colour muted, juce::Colour panel)
    {
        float re[256], im[256] {};
        for (int i = 0; i < 256; ++i) { re[i] = samples[i]; im[i] = 0.f; }
        fft(re, im, 256);

        constexpr int kBars = 32;
        const float barW = (r.getWidth() - 12.f) / kBars;
        const float baseY = r.getBottom() - 18.f;
        const float maxH = r.getHeight() - 24.f;

        for (int i = 0; i < kBars; ++i)
        {
            // Map bars logarithmically across the lower 3/4 of the spectrum (musical range).
            const int lo = (int) std::pow((float) i / kBars, 2.0f) * 96 + 1;
            const int hi = (int) std::pow((float)(i + 1) / kBars, 2.0f) * 96 + 1;
            float mag = 0.f;
            for (int j = lo; j <= hi && j < 128; ++j)
                mag = juce::jmax(mag, std::sqrt(re[j] * re[j] + im[j] * im[j]));
            mag = juce::jlimit(0.f, 1.f, mag * 3.5f);
            // Smooth peak hold
            barPeaks[i] = juce::jmax(mag, barPeaks[i] * 0.92f);

            const float h = mag * maxH;
            const float x = r.getX() + 6.f + i * barW;
            auto bar = juce::Rectangle<float>(x + 1.f, baseY - h, barW - 2.f, h);
            juce::ColourGradient grad(accent.brighter(0.3f), x, bar.getY(),
                                      accent.darker(0.4f), x, baseY, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(bar, 2.f);

            // Peak hold marker
            const float peakY = baseY - barPeaks[i] * maxH;
            g.setColour(accent.brighter(0.5f).withAlpha(0.85f));
            g.fillRoundedRectangle(juce::Rectangle<float>(x + 1.f, peakY - 2.f, barW - 2.f, 2.f), 1.f);
        }
        g.setColour(muted.withAlpha(0.4f));
        g.drawLine(r.getX() + 6.f, baseY, r.getRight() - 6.f, baseY, 1.f);
        juce::ignoreUnused(panel);
    }

    // ---- Mode 8: Radial — circular spectrum bars emanating from centre ----
    // Concept: open-source "radial visualiser" (e.g. VSXu, Milkdrop circular modes).
    void paintRadial(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                     juce::Colour accent, juce::Colour muted, juce::Colour panel)
    {
        float re[256], im[256] {};
        for (int i = 0; i < 256; ++i) { re[i] = samples[i]; im[i] = 0.f; }
        fft(re, im, 256);

        const auto cx = r.getCentreX();
        const auto cy = r.getCentreY();
        const float innerR = juce::jmin(r.getWidth(), r.getHeight()) * 0.14f;
        const float maxR = juce::jmin(r.getWidth(), r.getHeight()) * 0.42f;
        constexpr int kBars = 64;

        // RMS for the glow ring
        float rms = 0.f;
        for (int i = 0; i < 256; ++i) rms += samples[i] * samples[i];
        rms = std::sqrt(rms / 256.f);

        g.setColour(panel.brighter(0.05f));
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.f, innerR * 2.f);
        g.setColour(accent.withAlpha(0.15f + rms * 0.4f));
        g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.f, innerR * 2.f, 1.5f);

        for (int i = 0; i < kBars; ++i)
        {
            const int bin = (int) std::pow((float) i / kBars, 1.6f) * 96 + 1;
            const float mag = juce::jlimit(0.f, 1.f, std::sqrt(re[bin] * re[bin] + im[bin] * im[bin]) * 4.f);
            const float ang = (float) i / kBars * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
            const float r0 = innerR + 2.f;
            const float r1 = innerR + 2.f + mag * (maxR - innerR - 2.f);
            const float x0 = cx + std::cos(ang) * r0, y0 = cy + std::sin(ang) * r0;
            const float x1 = cx + std::cos(ang) * r1, y1 = cy + std::sin(ang) * r1;
            g.setColour(accent.withAlpha(0.5f + mag * 0.5f));
            g.drawLine(x0, y0, x1, y1, 2.f);
        }
        juce::ignoreUnused(muted);
    }

    // ---- Mode 9: Particles — particle field that drifts and reacts to audio energy ----
    // Concept: open-source particle visualisers (e.g. audio-reactive shader demos).
    void paintParticles(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                        juce::Colour accent, juce::Colour muted, juce::Colour panel)
    {
        float energy = 0.f;
        for (int i = 0; i < 256; ++i) energy += std::abs(samples[i]);
        energy = juce::jlimit(0.f, 1.f, energy / 128.f);

        const auto cx = r.getCentreX(), cy = r.getCentreY();
        constexpr int kParticles = 40;
        const float maxR = juce::jmin(r.getWidth(), r.getHeight()) * 0.42f;

        for (int i = 0; i < kParticles; ++i)
        {
            const float seedAng = (float) i / kParticles * juce::MathConstants<float>::twoPi;
            const float drift = std::sin(particlePhase + i * 0.3f) * 0.5f + 0.5f;
            const float radius = maxR * (0.2f + drift * 0.8f) * (0.6f + energy * 0.8f);
            const float ang = seedAng + particlePhase * 0.3f;
            const float px = cx + std::cos(ang) * radius;
            const float py = cy + std::sin(ang) * radius;
            const float size = 2.f + energy * 6.f + std::sin(particlePhase + i) * 1.5f;
            g.setColour(accent.withAlpha(0.3f + energy * 0.5f));
            g.fillEllipse(px - size * 0.5f, py - size * 0.5f, size, size);
        }
        // Central glow
        g.setColour(accent.withAlpha(0.08f + energy * 0.15f));
        g.fillEllipse(cx - maxR * 0.3f, cy - maxR * 0.3f, maxR * 0.6f, maxR * 0.6f);
        particlePhase += 0.04f + energy * 0.06f;
        juce::ignoreUnused(muted, panel);
    }

    // ---- Mode 10: PeakMeter — LED-style VU/peak meter with gradient segments ----
    // Concept: open-source LED VU meter plugins (e.g. melda MAnalyzer, zita-mu1).
    void paintPeakMeter(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                       juce::Colour accent, juce::Colour muted, juce::Colour panel)
    {
        float peak = 0.f;
        for (int i = 0; i < 256; ++i) peak = juce::jmax(peak, std::abs(samples[i]));
        peakHold = juce::jmax(peak, peakHold * 0.95f);

        constexpr int kSegs = 14;
        const float segH = (r.getHeight() - 24.f) / kSegs;
        const float meterW = r.getWidth() * 0.35f;
        const float meterX = r.getCentreX() - meterW * 0.5f;
        const float baseY = r.getBottom() - 18.f;

        for (int i = 0; i < kSegs; ++i)
        {
            const float thresh = (float)(i + 1) / kSegs;
            const bool lit = peak >= thresh - 0.02f;
            const float y = baseY - (i + 1) * segH;
            auto seg = juce::Rectangle<float>(meterX, y + 1.f, meterW, segH - 2.f);
            // Green->yellow->red gradient as you go up
            juce::Colour segCol;
            if (i < kSegs * 0.6f)      segCol = accent.brighter(0.2f);
            else if (i < kSegs * 0.85f) segCol = accent.brighter(0.5f);
            else                       segCol = accent.contrasting(0.3f);
            g.setColour(lit ? segCol : panel.darker(0.1f));
            g.fillRoundedRectangle(seg, 2.f);
            if (! lit)
            {
                g.setColour(muted.withAlpha(0.2f));
                g.drawRoundedRectangle(seg, 2.f, 0.5f);
            }
        }

        // Peak hold marker line
        const float peakY = baseY - peakHold * (r.getHeight() - 24.f);
        g.setColour(accent.contrasting(0.4f).withAlpha(0.9f));
        g.drawLine(meterX - 4.f, peakY, meterX + meterW + 4.f, peakY, 1.5f);

        // RMS bar on the right
        float rms = 0.f;
        for (int i = 0; i < 256; ++i) rms += samples[i] * samples[i];
        rms = std::sqrt(rms / 256.f);
        const float rmsH = rms * (r.getHeight() - 24.f);
        g.setColour(accent.withAlpha(0.4f));
        g.fillRoundedRectangle(juce::Rectangle<float>(meterX + meterW + 8.f, baseY - rmsH, 6.f, rmsH), 2.f);
    }

    // ---- Mode 11: WaveformBars — blocky quantized waveform bars (DAW overview style) ----
    // Concept: open-source waveform-overview displays (e.g. audacity, kwave).
    void paintWaveformBars(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                           juce::Colour accent, juce::Colour muted, juce::Colour panel)
    {
        constexpr int kBars = 48;
        const float barW = (r.getWidth() - 12.f) / kBars;
        const float mid = r.getCentreY() - 4.f;
        const float maxH = r.getHeight() * 0.34f;
        const int samplesPerBar = 256 / kBars;

        for (int i = 0; i < kBars; ++i)
        {
            float peak = 0.f;
            for (int j = 0; j < samplesPerBar; ++j)
                peak = juce::jmax(peak, std::abs(samples[i * samplesPerBar + j]));
            const float h = peak * maxH;
            const float x = r.getX() + 6.f + i * barW;
            g.setColour(accent.withAlpha(0.85f));
            g.fillRoundedRectangle(juce::Rectangle<float>(x + 0.5f, mid - h, barW - 1.f, h * 2.f), 1.5f);
        }
        g.setColour(muted.withAlpha(0.3f));
        g.drawLine(r.getX() + 6.f, mid, r.getRight() - 6.f, mid, 0.5f);
        juce::ignoreUnused(panel);
    }

    // ---- Mode 12: Ripple — concentric ripple rings that pulse with audio energy ----
    // Concept: open-source ripple/wave visualisers (e.g. audio-reactive canvas demos).
    void paintRipple(juce::Graphics& g, juce::Rectangle<float> r, const float* samples,
                     juce::Colour accent, juce::Colour muted, juce::Colour panel)
    {
        float energy = 0.f;
        for (int i = 0; i < 256; ++i) energy += std::abs(samples[i]);
        energy = juce::jlimit(0.f, 1.f, energy / 128.f);

        const auto cx = r.getCentreX(), cy = r.getCentreY();
        const float maxR = juce::jmin(r.getWidth(), r.getHeight()) * 0.46f;

        for (int i = 0; i < kRipples; ++i)
        {
            rippleRadius[i] += 0.6f + energy * 2.5f;
            if (rippleRadius[i] > maxR)
            {
                rippleRadius[i] = 4.f;
                rippleAlpha[i] = 1.f;
            }
            const float a = rippleAlpha[i] * (1.f - rippleRadius[i] / maxR);
            g.setColour(accent.withAlpha(a * 0.7f));
            g.drawEllipse(cx - rippleRadius[i], cy - rippleRadius[i],
                          rippleRadius[i] * 2.f, rippleRadius[i] * 2.f, 1.5f);
            rippleAlpha[i] = juce::jmax(0.1f, rippleAlpha[i] - 0.005f);
        }
        // Centre dot
        const float dotSize = 4.f + energy * 10.f;
        g.setColour(accent.withAlpha(0.8f));
        g.fillEllipse(cx - dotSize * 0.5f, cy - dotSize * 0.5f, dotSize, dotSize);
        juce::ignoreUnused(muted, panel);
    }

    const char* modeLabel() const
    {
        const char* labels[] = {
            "OSCILLOSCOPE", "STEREO SCOPE", "SPECTRUM", "SPECTROGRAM", "GHOST WAVE",
            "BROKEN WAVE", "VECTOR / XY",
            "SPECTRUM BARS", "RADIAL SPECTRUM", "PARTICLE FIELD",
            "PEAK METER", "WAVEFORM BARS", "RIPPLE"
        };
        return labels[(int) mode];
    }

    KyotoAudioProcessor& processor;
    Mode mode = Mode::Oscilloscope;
    kt::ThemePalette theme = kt::kThemes[0];
    float barPeaks[32] {};
    float peakHold = 0.f;
    float particlePhase = 0.f;
    static constexpr int kRipples = 6;
    float rippleRadius[kRipples] { 4.f, 30.f, 60.f, 90.f, 120.f, 150.f };
    float rippleAlpha[kRipples] { 1.f, 0.8f, 0.6f, 0.4f, 0.3f, 0.2f };
};

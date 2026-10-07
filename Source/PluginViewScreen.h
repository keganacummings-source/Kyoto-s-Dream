#pragma once
#include <map>
#include "PluginEditor.h"
#include "FxCatalog.h"
#include "HardwareInternals.h"
#include <cmath>

static_assert(sizeof(hb::kInternals) / sizeof(hb::kInternals[0]) == (size_t) pb::kShellCount, "every template needs hardware internals");

// Pluggin View - the viewer surface. While the editor is in plugin view this overlay covers the
// whole window and shows only the built pluggin running live: its shell, its placed bays with
// real values, its live waveform - plus exactly two controls, BACK and GEEK. The Geek button
// melts moving transparent spots into the top cover so the internal hardware (motherboard,
// battery, caps, fan...) shows through; hovering a part highlights it and reads out its settings.
class PluginViewScreen final : public juce::Component, private juce::Timer
{
public:
    explicit PluginViewScreen(KyotoAudioProcessorEditor& e) : editor(e)
    {
        backBtn.setButtonText("< BACK");
        geekBtn.setButtonText("GEEK");
        geekBtn.setClickingTogglesState(true);
        addAndMakeVisible(backBtn);
        addAndMakeVisible(geekBtn);
        backBtn.onClick = [this] { editor.setPluginView(false); };
        geekBtn.onClick = [this]
        {
            geekMode = geekBtn.getToggleState();
            if (! geekMode) geekHot = -1;
        };
        startTimerHz(20); // keep low; FL hates heavy UI timers on close
    }

    // Called from the editor destructor — must be public (Timer is a private base).
    void shutdownViewer()
    {
        stopTimer();
        setVisible(false);
        dragNode = {};
        heldKey = {};
        geekMode = false;
        geekHot = -1;
    }

    void paint(juce::Graphics& g) override
    {
        const auto& theme = editor.machineDesign.palette();
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& shell = pb::kShells[shellIdx];
        const auto accent = kt::c(theme.accent);
        const auto ink = kt::c(theme.text);
        const auto muted = kt::c(theme.muted);

        g.fillAll(kt::c(theme.bg));

        const auto caseR = pluginCase();
        const float bodyRadius = pb::shellRadius(shell);

        // Case body - this template's own hardware shell: silhouette, ears/handles/feet, trim pattern and tint.
        pb::paintShellBody(g, caseR, shell, theme, true);

        // Corner screws (pulled in for the cut-corner silhouettes).
        const float so = (shell.shape == 1 || shell.shape == 5) ? 26.f : 14.f;
        g.setColour(muted.withAlpha(0.6f));
        for (auto p : { juce::Point<float>(caseR.getX() + so, caseR.getY() + so),
                        juce::Point<float>(caseR.getRight() - so, caseR.getY() + so),
                        juce::Point<float>(caseR.getX() + so, caseR.getBottom() - so),
                        juce::Point<float>(caseR.getRight() - so, caseR.getBottom() - so) })
        {
            g.fillEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f);
            g.setColour(kt::c(theme.bg).withAlpha(0.7f));
            g.drawLine(p.x - 2.f, p.y - 2.f, p.x + 2.f, p.y + 2.f, 1.f);
            g.setColour(muted.withAlpha(0.6f));
        }

        // Shell wordmark + live LED on the case.
        g.setColour(accent);
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText(juce::String(shell.name).toUpperCase(), caseR.getX() + 24, (int) caseR.getY() + 10, 240, 18, juce::Justification::centredLeft);
        g.setColour(muted);
        g.setFont(kt::font(theme, 9.f));
        g.drawText("PLUGGIN  -  LIVE", caseR.getX() + 24, (int) caseR.getY() + 27, 240, 13, juce::Justification::centredLeft);
        const float pulse = 0.45f + 0.4f * std::sin(animPhase * 2.f);
        g.setColour(accent.withAlpha(pulse));
        g.fillEllipse(caseR.getRight() - 30.f, caseR.getY() + 12.f, 9.f, 9.f);
        g.setColour(accent.withAlpha(0.35f * pulse));
        g.drawEllipse(caseR.getRight() - 34.f, caseR.getY() + 8.f, 17.f, 17.f, 1.2f);

        const auto face = pb::faceRect(caseR);
        const auto interior = pluginInterior();
        const bool geekOn = geekReveal > 0.01f;

        // 1) Internal hardware underneath the top cover.
        if (geekOn) { drawInternals(g, interior); drawPartHardware(g, face, shell, interior); }

        // 2) The top cover: faceplate, bay wiring and the placed parts running live.
        //    Geek mode keeps the finished, textured shell dominant and lets the hidden hardware read
        //    through it, so the machine still looks like the real thing while it is being explained.
        const float cover = 1.f - 0.45f * geekReveal;
        g.setColour(kt::c(theme.bg).withAlpha(0.95f * cover));
        g.fillRoundedRectangle(face, bodyRadius);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(face, bodyRadius, 1.3f);
        if (geekOn) g.beginTransparencyLayer(juce::jmax(0.35f, cover));
        const auto shellClip = pb::casePath(caseR, shell);
        for (int i = 0; i < shell.slotCount; ++i)
            if (shell.slots[i].kind == pb::SlotKind::Board)
                pb::paintScreenBezel(g, pb::slotRect(face, shell.slots[i]).reduced(3.f), pb::boardScreenTypeOf(editor.proc.uiState, shell.screenStyle), theme);
        drawBayWiring(g, face, shell);
        drawPlacedParts(g, face, shell, shellClip);
        if (geekOn) g.endTransparencyLayer();

        // 3) Geek x-ray overlay: signal path wires, layout detail, scan band, labels.
        if (geekOn)
        {
            drawSignalPath(g, face, shell, accent);
            drawLayoutDetail(g, face, shell, accent);
            drawXrayOverlay(g, interior, accent);
        }
    }

    void resized() override
    {
        backBtn.setBounds(24, 20, 116, 38);
        geekBtn.setBounds(148, 20, 116, 38);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        if (! geekMode || geekReveal < 0.5f) return;
        const int hit = hitHardwarePart(e.position);
        if (hit != geekHot) { geekHot = hit; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (geekHot != -1) { geekHot = -1; repaint(); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex)];
        auto face = pb::faceRect(pluginCase());
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            auto r = pb::slotRect(face, shell.slots[bay]);
            if (! r.contains(e.position)) continue;
            const auto kind = node.getProperty("kind").toString();
            if (kind == "key")
            {
                editor.proc.noteOn((int) node.getProperty("note", 60), 0.9f);
                editor.proc.triggerSample();
                heldKey = node;
            }
            else if (kind == "dial" || kind == "slider")
            {
                dragNode = node;
                dragStartY = e.position.y;
                dragStartValue = liveParamValue(node);
            }
            return;
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragNode.isValid())
        {
            auto* param = liveParam(dragNode);
            if (param != nullptr)
            {
                const float v = juce::jlimit(0.f, 1.f, dragStartValue + (dragStartY - e.position.y) * 0.005f);
                param->setValueNotifyingHost(v);
                repaint();
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (heldKey.isValid()) { editor.proc.noteOff((int) heldKey.getProperty("note", 60)); heldKey = {}; }
        dragNode = {};
    }

private:
    void timerCallback() override
    {
        // Editor may be tearing down; never touch it after close starts.
        if (editor.editorClosing)
        {
            stopTimer();
            return;
        }
        const bool active = editor.inPluginView();
        if (active != isVisible()) { setVisible(active); if (active) toFront(false); }
        if (active && getBounds() != editor.getLocalBounds()) setBounds(editor.getLocalBounds());
        if (! active)
        {
            geekMode = false;
            geekBtn.setToggleState(false, juce::dontSendNotification);
            geekHot = -1;
            geekReveal = juce::jmax(0.f, geekReveal - 0.1f);
            return; // no repaint when hidden — saves CPU and avoids host teardown races
        }
        animPhase += 0.04f;
        if (animPhase > juce::MathConstants<float>::twoPi) animPhase -= juce::MathConstants<float>::twoPi;
        geekReveal = juce::jlimit(0.f, 1.f, geekReveal + (geekMode ? 0.055f : -0.055f));
        repaint();
    }

    juce::Rectangle<float> pluginCase() const
    {
        auto b = getLocalBounds().toFloat();
        return { b.getX() + 22.f, b.getY() + 66.f, b.getWidth() - 44.f, b.getHeight() - 88.f };
    }

    juce::Rectangle<float> pluginInterior() const
    {
        auto face = pb::faceRect(pluginCase());
        return face.reduced(face.getWidth() * 0.02f, face.getHeight() * 0.035f);
    }

    // The shell's hidden hardware, laid out randomly on the builder grid and cached per
    // (shell, seed) so it never jitters between frames.
    const juce::Array<juce::Rectangle<float>>& internalsNormalized() const
    {
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto seed = (juce::uint32) (int) editor.proc.uiState.getProperty("internalsSeed", 1);
        if (! internalsReady || cacheShell != shellIdx || cacheSeed != seed)
        {
            hb::placeInternals(shellIdx, seed, internalsCache);
            cacheShell = shellIdx;
            cacheSeed = seed;
            internalsReady = true;
        }
        return internalsCache;
    }

    juce::Rectangle<float> internalRect(juce::Rectangle<float> interior, int index) const
    {
        const auto& rects = internalsNormalized();
        if (index < 0 || index >= rects.size()) return {};
        return hb::gridRect(interior, rects[index]);
    }


    // Visual enhancer: draw the live signal path (wire order) between bays.
    void drawSignalPath(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell, juce::Colour accent) const
    {
        const auto orderStr = editor.proc.uiState.getProperty("wireOrder").toString();
        if (orderStr.isEmpty()) return;

        juce::Array<juce::Point<float>> pts;
        // Motherboard centre first.
        if (shell.slotCount > 0 && shell.slots[0].kind == pb::SlotKind::Board)
            pts.add(pb::slotRect(face, shell.slots[0]).getCentre());
        else
            pts.add(face.getCentre());

        juce::StringArray parts;
        parts.addTokens(orderStr, ",", "");
        for (const auto& token : parts)
        {
            const int dsp = token.trim().getIntValue();
            // Find shell bay that owns this DSP slot.
            for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
            {
                auto node = editor.proc.uiState.getChild(i);
                if (! node.hasType("w")) continue;
                if ((int) node.getProperty("slot", -1) != dsp) continue;
                if ((int) node.getProperty("satellite", 0) != 0) continue;
                const int bay = (int) node.getProperty("shellSlot", -1);
                if (bay < 0 || bay >= shell.slotCount) continue;
                pts.add(pb::slotRect(face, shell.slots[bay]).getCentre());
                break;
            }
        }

        if (pts.size() < 2) return;

        juce::Path path;
        path.startNewSubPath(pts[0]);
        for (int i = 1; i < pts.size(); ++i)
        {
            const auto a = pts[i - 1], b = pts[i];
            const float mx = 0.5f * (a.x + b.x);
            path.cubicTo(mx, a.y, mx, b.y, b.x, b.y);
        }

        // Glow trace
        g.setColour(accent.withAlpha(0.12f * geekReveal));
        g.strokePath(path, juce::PathStrokeType(6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.55f * geekReveal));
        g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Animated packet — O(points) only, no path flattening
        if (pts.size() >= 2)
        {
            const float u = 0.5f + 0.5f * std::sin(animPhase * 1.7f);
            const float fidx = u * (float) (pts.size() - 1);
            const int i0 = juce::jlimit(0, pts.size() - 2, (int) fidx);
            const float frac = fidx - (float) i0;
            const auto p = pts[i0] + (pts[i0 + 1] - pts[i0]) * frac;
            g.setColour(accent.withAlpha(0.9f * geekReveal));
            g.fillEllipse(p.x - 3.5f, p.y - 3.5f, 7.f, 7.f);
            g.setColour(juce::Colours::white.withAlpha(0.35f * geekReveal));
            g.fillEllipse(p.x - 1.5f, p.y - 1.5f, 3.f, 3.f);
        }

        // Stage index badges
        g.setFont(kt::font(editor.machineDesign.palette(), 8.f, true));
        for (int i = 1; i < pts.size(); ++i)
        {
            g.setColour(accent.withAlpha(0.75f * geekReveal));
            g.fillRoundedRectangle(pts[i].x - 8.f, pts[i].y - 18.f, 16.f, 12.f, 3.f);
            g.setColour(kt::c(editor.machineDesign.palette().bg).withAlpha(0.9f));
            g.drawText(juce::String(i), (int) pts[i].x - 8, (int) pts[i].y - 18, 16, 12, juce::Justification::centred);
        }
    }

    // Extra layout detail: bay outlines, satellite dial markers, screen tap labels.
    void drawLayoutDetail(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell, juce::Colour accent) const
    {
        const auto& theme = editor.machineDesign.palette();
        g.setFont(kt::font(theme, 8.f, true));
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            auto r = pb::slotRect(face, shell.slots[bay]);
            const bool sat = (int) node.getProperty("satellite", 0) != 0;
            const auto kind = node.getProperty("kind").toString();

            if (sat)
            {
                // Satellite control dial — ring highlight
                g.setColour(accent.withAlpha(0.5f * geekReveal));
                g.drawEllipse(r.reduced(2.f), 1.4f);
                g.setColour(kt::c(theme.text).withAlpha(0.8f * geekReveal));
                g.drawFittedText(node.getProperty("label").toString(), r.reduced(4.f).toNearestInt(), juce::Justification::centred, 2);
            }
            else if (kind == "wave" || kind == "board")
            {
                const int tap = (int) node.getProperty("tapAfter", 0);
                g.setColour(accent.withAlpha(0.65f * geekReveal));
                g.drawRoundedRectangle(r.reduced(1.f), 4.f, 1.2f);
                g.setColour(kt::c(theme.text).withAlpha(0.85f * geekReveal));
                g.drawText("TAP@" + juce::String(tap), r.getX() + 4.f, r.getY() + 2.f, r.getWidth() - 8.f, 12.f, juce::Justification::centredLeft);
            }
            else
            {
                const int dsp = (int) node.getProperty("slot", -1);
                if (dsp > 0)
                {
                    g.setColour(accent.withAlpha(0.25f * geekReveal));
                    g.drawRoundedRectangle(r.reduced(0.5f), 5.f, 1.f);
                }
            }
        }
    }

    void drawInternals(juce::Graphics& g, juce::Rectangle<float> interior) const
    {
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& parts = hb::kInternals[shellIdx];
        for (int i = 0; i < parts.count; ++i)
            hb::drawHardwarePart(g, editor.machineDesign.palette(), parts.parts[i], internalRect(interior, i), animPhase, i == geekHot);
    }

    // Per-part hardware: small PCB under each part. Cheap fills only; skip when barely visible.
    void drawPartHardware(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell, juce::Rectangle<float> interior) const
    {
        if (geekReveal < 0.08f) return;
        juce::ignoreUnused(interior);
        const auto& theme = editor.machineDesign.palette();
        const auto accent = kt::c(theme.accent);
        const auto peg = kt::c(theme.peg);
        const auto muted = kt::c(theme.muted);
        const auto ink = kt::c(theme.text);
        const auto boardCentre = shell.slots[0].kind == pb::SlotKind::Board ? pb::slotRect(face, shell.slots[0]).getCentre() : face.getCentre();
        const float a = geekReveal;

        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay <= 0 || bay >= shell.slotCount) continue;
            auto r = pb::slotRect(face, shell.slots[bay]);
            if (r.getWidth() < 10.f || r.getHeight() < 10.f) continue;

            // Small PCB beneath the part
            auto pcb = r.reduced(4.f);
            g.setColour(peg.darker(0.3f).withAlpha(0.6f * geekReveal));
            g.fillRoundedRectangle(pcb, 3.f);
            g.setColour(accent.withAlpha(0.3f * geekReveal));
            g.drawRoundedRectangle(pcb, 3.f, 0.8f);

            // Chip in the center of the PCB
            auto chip = pcb.withSizeKeepingCentre(juce::jmin(pcb.getWidth() * 0.5f, 24.f), juce::jmin(pcb.getHeight() * 0.5f, 18.f));
            g.setColour(accent.withAlpha(0.5f * geekReveal));
            g.fillRoundedRectangle(chip, 2.f);
            g.setColour(ink.withAlpha(0.6f * geekReveal));
            g.setFont(kt::font(theme, 6.f, true));
            g.drawText("IC", chip, juce::Justification::centred);

            // Chip pins
            const int pins = juce::jmax(3, (int) (chip.getWidth() / 4.f));
            g.setColour(muted.withAlpha(0.4f * geekReveal));
            for (int p = 0; p < pins; ++p)
            {
                const float px = chip.getX() + 2.f + (float) p * (chip.getWidth() - 4.f) / (float) juce::jmax(1, pins - 1);
                g.fillRect(px - 0.5f, chip.getBottom(), 1.5f, 3.f);
                g.fillRect(px - 0.5f, chip.getY() - 3.f, 1.5f, 3.f);
            }

            // Trace wire from this part's hardware to the motherboard
            const auto partCentre = r.getCentre();
            juce::Path trace;
            trace.startNewSubPath(partCentre.x, partCentre.y);
            const float midX = (partCentre.x + boardCentre.x) * 0.5f;
            trace.cubicTo(midX, partCentre.y, midX, boardCentre.y, boardCentre.x, boardCentre.y);
            g.setColour(accent.withAlpha(0.25f * geekReveal));
            g.strokePath(trace, juce::PathStrokeType(1.2f));

            // Solder dots at the trace endpoints
            g.setColour(accent.withAlpha(0.5f * geekReveal));
            g.fillEllipse(partCentre.x - 2.f, partCentre.y - 2.f, 4.f, 4.f);
            g.fillEllipse(boardCentre.x - 2.f, boardCentre.y - 2.f, 4.f, 4.f);
        }
    }

    void drawXrayOverlay(juce::Graphics& g, juce::Rectangle<float> interior, juce::Colour accent) const
    {
        const auto& theme = editor.machineDesign.palette();
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& parts = hb::kInternals[shellIdx];
        const float reveal = geekReveal;

        // Slow scan band sweeping across the open machine.
        {
            g.saveState();
            g.reduceClipRegion(interior.toNearestInt());
            const float t = animPhase / juce::MathConstants<float>::twoPi;
            const float bandW = interior.getWidth() * 0.22f;
            const float sx = interior.getX() - bandW + t * (interior.getWidth() + 2.f * bandW);
            juce::ColourGradient grad(accent.withAlpha(0.f), sx - bandW, 0.f, accent.withAlpha(0.f), sx + bandW, 0.f, false);
            grad.addColour(0.5, accent.withAlpha(0.16f * reveal));
            g.setGradientFill(grad);
            g.fillRect(juce::Rectangle<float>(sx - bandW, interior.getY(), bandW * 2.f, interior.getHeight()));
            g.restoreState();
        }

        // Faint frame around the readable area.
        g.setColour(accent.withAlpha(0.22f * reveal));
        g.drawRoundedRectangle(interior.reduced(1.f), 8.f, 1.f);

        // Steady labels, one per internal part, so a passive viewer can read the machine without hovering.
        g.setFont(kt::font(theme, 10.f, true));
        for (int i = 0; i < parts.count; ++i)
        {
            const auto pr = internalRect(interior, i);
            const juce::String name = parts.parts[i].name;
            const float w = juce::jmin(interior.getWidth() - 8.f, (float) name.length() * 7.0f + 16.f);
            const float x = juce::jlimit(interior.getX() + 4.f, juce::jmax(interior.getX() + 4.f, interior.getRight() - w - 4.f), pr.getX() + 3.f);
            const float y = juce::jlimit(interior.getY() + 4.f, juce::jmax(interior.getY() + 4.f, interior.getBottom() - 22.f), pr.getY() + 3.f);
            auto pill = juce::Rectangle<float>(x, y, w, 17.f);
            const bool hot = (i == geekHot);
            g.setColour(kt::c(theme.bg).withAlpha((hot ? 0.92f : 0.78f) * reveal));
            g.fillRoundedRectangle(pill, 8.f);
            g.setColour(accent.withAlpha((hot ? 0.95f : 0.45f) * reveal));
            g.drawRoundedRectangle(pill, 8.f, hot ? 1.6f : 1.f);
            g.setColour(kt::c(theme.text).withAlpha(reveal));
            g.drawText(name, pill, juce::Justification::centred, true);
            if (hot)
            {
                g.setColour(accent.withAlpha(0.9f));
                g.drawRoundedRectangle(pr.expanded(2.f), 6.f, 2.f);
            }
        }

        // Caption.
        g.setColour(kt::c(theme.muted).withAlpha(0.9f * reveal));
        g.setFont(kt::font(theme, 11.f));
        g.drawText("X-RAY  -  hover a part for its live readout", interior.withTrimmedTop(interior.getHeight() - 22.f).toNearestInt(), juce::Justification::centred, true);

        if (geekHot >= 0 && geekHot < parts.count)
            drawTooltip(g, interior, geekHot, internalRect(interior, geekHot));
    }

    void punchHole(juce::Graphics& g, juce::Rectangle<float> interior, float cx, float cy, float r, juce::Colour accent) const
    {
        if (r < 1.f) return;
        g.saveState();
        juce::Path hole;
        hole.addEllipse(cx - r, cy - r, r * 2.f, r * 2.f);
        g.reduceClipRegion(hole);
        drawInternals(g, interior);
        g.restoreState();
        g.setColour(accent.withAlpha(0.35f));
        g.drawEllipse(cx - r, cy - r, r * 2.f, r * 2.f, 1.4f);
        g.setColour(accent.withAlpha(0.12f));
        g.drawEllipse(cx - r - 4.f, cy - r - 4.f, r * 2.f + 8.f, r * 2.f + 8.f, 1.f);
    }

    int hitHardwarePart(juce::Point<float> pos) const
    {
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& parts = hb::kInternals[shellIdx];
        const auto interior = pluginInterior();
        for (int i = 0; i < parts.count; ++i)
            if (internalRect(interior, i).contains(pos)) return i;
        return -1;
    }

    juce::RangedAudioParameter* liveParam(const juce::ValueTree& node) const
    {
        const int slot = (int) node.getProperty("slot", -1);
        if (slot < 0) return nullptr;
        const auto id = "s" + juce::String(slot + 1).paddedLeft('0', 2) + node.getProperty("param").toString();
        return editor.proc.apvts.getParameter(id);
    }

    float liveParamValue(const juce::ValueTree& node) const
    {
        auto* p = liveParam(node);
        return p != nullptr ? p->getValue() : 0.f;
    }

    static juce::String fxNameFor(int type)
    {
        if (type == KyotoAudioProcessor::kMixType) return "MASTER MIX";
        if (type == KyotoAudioProcessor::kBreakType) return "CHAIN BREAK";
        if (type >= 0 && type < kt::kFxCount) return kt::kFx[type].name;
        return "FX";
    }

    int liveTypeValue(const juce::ValueTree& node) const
    {
        const int slot = (int) node.getProperty("slot", -1);
        if (slot < 0) return -1;
        auto* p = editor.proc.apvts.getParameter("s" + juce::String(slot + 1).paddedLeft('0', 2) + "type");
        return p != nullptr ? (int) std::lround(p->convertFrom0to1(p->getValue())) : -1;
    }

    juce::String pct(float v) const { return juce::String(juce::roundToInt(v * 100.f)) + "%"; }

    int activeBayCount() const
    {
        int n = 0;
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (node.hasType("w") && (int) node.getProperty("shellSlot", -1) > 0) ++n;
        }
        return n;
    }

    // Live settings readout for a hovered internal part.
    juce::String liveSettingsFor(const hb::HardwarePart& part) const
    {
        auto& apvts = editor.proc.apvts;
        if (part.kind == hb::PartKind::Board || part.kind == hb::PartKind::Battery || part.kind == hb::PartKind::Psu)
        {
            juce::String line;
            if (auto* type = apvts.getParameter("s01type"))
                line << "FX: " << fxNameFor((int) std::lround(type->convertFrom0to1(type->getValue()))) << "   ";
            if (auto* on = apvts.getParameter("s01on"))
                line << (on->getValue() >= 0.5f ? "ON" : "STANDBY") << "   ";
            if (auto* mix = apvts.getParameter("s01mix"))
                line << "MIX " << pct(mix->getValue());
            line << "   BAYS " << activeBayCount();
            return line;
        }
        return {};
    }

    void drawTooltip(juce::Graphics& g, juce::Rectangle<float> interior, int partIndex, juce::Rectangle<float> partRect) const
    {
        const auto& theme = editor.machineDesign.palette();
        const auto accent = kt::c(theme.accent);
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& part = hb::kInternals[shellIdx].parts[partIndex];
        const auto settings = liveSettingsFor(part);
        const float cardW = juce::jmin(300.f, interior.getWidth() * 0.7f);
        const float cardH = settings.isEmpty() ? 62.f : 80.f;
        auto card = juce::Rectangle<float>(partRect.getCentreX() - cardW * 0.5f, partRect.getY() - cardH - 12.f, cardW, cardH);
        if (card.getY() < interior.getY()) card.setPosition(card.getX(), partRect.getBottom() + 12.f);
        card.setX(juce::jlimit(interior.getX(), interior.getRight() - card.getWidth(), card.getX()));
        card.setY(juce::jlimit(interior.getY(), interior.getBottom() - card.getHeight(), card.getY()));

        g.setColour(kt::c(theme.panel).withAlpha(0.97f));
        g.fillRoundedRectangle(card, 10.f);
        g.setColour(accent);
        g.fillRoundedRectangle(card.getX(), card.getY(), 3.5f, card.getHeight(), 2.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(card, 10.f, 1.f);
        g.setColour(accent);
        g.setFont(kt::font(theme, 12.5f, true));
        g.drawText(part.name, (int) card.getX() + 12, (int) card.getY() + 7, (int) card.getWidth() - 24, 16, juce::Justification::centredLeft);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 10.f));
        g.drawFittedText(part.blurb, (int) card.getX() + 12, (int) card.getY() + 23, (int) card.getWidth() - 24, settings.isEmpty() ? 32 : 20, juce::Justification::topLeft, 2);
        if (settings.isNotEmpty())
        {
            g.setColour(kt::c(theme.text));
            g.setFont(kt::font(theme, 10.f, true));
            g.drawText(settings, (int) card.getX() + 12, (int) card.getBottom() - 22, (int) card.getWidth() - 24, 16, juce::Justification::centredLeft);
        }
    }

    void drawBayWiring(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell) const
    {
        // Every part is wired into the part it was connected to (the chain), not just to the motherboard.
        std::map<int, juce::Point<float>> centre;
        std::map<int, int> parentOf;
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            centre[bay] = pb::slotRect(face, shell.slots[bay]).getCentre();
            parentOf[bay] = (int) node.getProperty("parent", -1);
        }
        if (centre.find(0) == centre.end()) return;
        const auto accent = kt::c(editor.machineDesign.palette().accent);
        for (auto& entry : centre)
        {
            const int bay = entry.first;
            if (bay == 0) continue;
            int par = parentOf[bay];
            if (par < 0 || par == bay || centre.find(par) == centre.end())
                continue; // no automatic motherboard wire
            const auto from = centre[par];
            const auto to = entry.second;
            juce::Path wire;
            wire.startNewSubPath(from);
            wire.cubicTo(from.x, (from.y + to.y) * 0.5f, to.x, (from.y + to.y) * 0.5f, to.x, to.y);
            g.setColour(accent.withAlpha(0.30f));
            g.strokePath(wire, juce::PathStrokeType(1.6f));
            g.setColour(accent.withAlpha(0.8f));
            g.fillEllipse(to.x - 2.5f, to.y - 2.5f, 5.f, 5.f);
        }
    }

    void drawPlacedParts(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell, const juce::Path& shellClip) const
    {
        const auto& theme = editor.machineDesign.palette();
        const auto accent = kt::c(theme.accent);
        const auto ink = kt::c(theme.text);
        const auto muted = kt::c(theme.muted);
        bool placedAny = false;
        juce::ignoreUnused(shellClip);

        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            auto r = pb::slotRect(face, shell.slots[bay]);
            if (r.getWidth() < 8.f || r.getHeight() < 8.f) continue;
            placedAny = true;
            const auto kind = node.getProperty("kind").toString();
            const auto label = node.getProperty("label").toString();
            const auto skinId = node.getProperty("skin").toString();
            const int variant = kt::plastic::styleVariantFor(node);
            const float v = liveParamValue(node);

            if (kind == "board")
            {
                kt::plastic::fillPlasticBody(g, r.reduced(2.f), 10.f,
                    kt::plastic::colsFor(theme, skinId, kt::plastic::tierOf(r.getWidth(), r.getHeight())), false);
                float live[64] {};
                editor.proc.copyScope(live, 64);
                // Upsample-ish: paintScreenFace expects 128 — pad
                float live128[128] {};
                for (int s = 0; s < 128; ++s) live128[s] = live[s >> 1];
                const int st = pb::boardScreenTypeOf(editor.proc.uiState, shell.screenStyle);
                pb::paintScreenFace(g, r.reduced(6.f), st, theme, live128, 128, juce::String("SCREEN  -  ") + pb::kScreenTypes[st], animPhase);
            }
            else if (kind == "dial")
                kt::plastic::drawDial(g, r.reduced(2.f), theme, skinId, v, false, variant);
            else if (kind == "slider")
                kt::plastic::drawFader(g, r.reduced(2.f), theme, skinId, v, false, r.getWidth() > r.getHeight() * 1.2f);
            else if (kind == "key")
                kt::plastic::drawKey(g, r.reduced(2.f), theme, skinId, false, false, label);
            else if (kind == "button")
                kt::plastic::drawButton(g, r.reduced(2.f), theme, skinId, false, false, label);
            else if (kind == "wave" || kind == "stack")
                kt::plastic::drawWaveFrame(g, r.reduced(2.f), theme, skinId, false, kind == "stack");
            else if (kind == "cosmetic")
                kt::plastic::drawCosmetic(g, r.reduced(2.f), theme, skinId, node.getProperty("style").toString(), false, label);
            else
                kt::plastic::drawDial(g, r.reduced(2.f), theme, skinId, v, false, variant);
        }

        if (! placedAny)
        {
            const auto& md = editor.machineDesign;
            g.setColour(muted);
            g.setFont(kt::font(theme, 12.f));
            g.drawText("No bays filled yet - parts placed in the Plugin Builder show up here, live.",
                       rtoInt(face).removeFromTop(juce::jmax(20, (int) (face.getHeight() * 0.08f))),
                       juce::Justification::centred);
            const float sx = face.getWidth() / (float) juce::jmax(1, md.playgroundWidth);
            const float sy = face.getHeight() / (float) juce::jmax(1, md.playgroundHeight);
            for (const auto& part : md.modules)
            {
                auto q = juce::Rectangle<float>(face.getX() + part.bounds.getX() * sx, face.getY() + part.bounds.getY() * sy,
                                               part.bounds.getWidth() * sx, part.bounds.getHeight() * sy);
                const float loX = face.getX() + 4.f, hiX = juce::jmax(loX, face.getRight() - q.getWidth() - 4.f);
                const float loY = face.getY() + 4.f, hiY = juce::jmax(loY, face.getBottom() - q.getHeight() - 4.f);
                q.setX(juce::jlimit(loX, hiX, q.getX()));
                q.setY(juce::jlimit(loY, hiY, q.getY()));
                kt::plastic::fillPlasticBody(g, q, 10.f,
                    kt::plastic::colsFor(theme, {}, kt::plastic::tierOf(q.getWidth(), q.getHeight())), false);
                g.setColour(ink);
                g.setFont(kt::font(theme, 9.f, true));
                g.drawFittedText(part.type, q.reduced(7.f).toNearestInt(), juce::Justification::centred, 1);
            }
        }
    }

    static juce::Rectangle<int> rtoInt(juce::Rectangle<float> r) { return r.toNearestInt(); }

    KyotoAudioProcessorEditor& editor;
    juce::TextButton backBtn, geekBtn;
    bool geekMode = false;
    float geekReveal = 0.f;
    float animPhase = 0.f;
    int geekHot = -1;
    juce::ValueTree dragNode, heldKey;
    float dragStartValue = 0.f, dragStartY = 0.f;
    mutable juce::Array<juce::Rectangle<float>> internalsCache;
    mutable int cacheShell = -1;
    mutable juce::uint32 cacheSeed = 0;
    mutable bool internalsReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginViewScreen)
};

// Installs the viewer overlay once the editor is parented into a host window.
// The overlay is added as a hidden child; it only becomes visible inside Plugin View.
inline void KyotoAudioProcessorEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();
    if (editorClosing)
        return;
    // Host may detach the editor (close/minimize). Do not build overlays while unparented.
    if (getPeer() == nullptr && getParentComponent() == nullptr)
        return;
    if (viewScreen == nullptr)
    {
        viewScreen = new PluginViewScreen(*this);
        addChildComponent(viewScreen);
        viewScreen->setBounds(getLocalBounds());
        viewScreen->resized();
        viewScreen->toFront(false);
    }
}
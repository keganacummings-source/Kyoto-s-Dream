#pragma once
#include <JuceHeader.h>
#include "FxCatalog.h"

// Wire graph: DSP order, dial→effect bindings, and control menus for the builder.
namespace kt {
namespace wire {

enum class Ctrl : int
{
    Mix = 0, Amount, Tone, Motion, Shape,
    Decay, Time, Hz, Strength, Feedback,
    Count
};

inline const char* ctrlId(Ctrl c)
{
    switch (c)
    {
        case Ctrl::Mix: return "mix";
        case Ctrl::Amount: case Ctrl::Decay: case Ctrl::Strength: return "amt";
        case Ctrl::Tone: case Ctrl::Hz: return "tone";
        case Ctrl::Motion: case Ctrl::Time: return "mot";
        case Ctrl::Shape: case Ctrl::Feedback: return "shp";
        default: return "mix";
    }
}

inline const char* ctrlLabel(Ctrl c)
{
    switch (c)
    {
        case Ctrl::Mix: return "Mix %";
        case Ctrl::Amount: return "Amount";
        case Ctrl::Tone: return "Tone";
        case Ctrl::Motion: return "Motion";
        case Ctrl::Shape: return "Shape";
        case Ctrl::Decay: return "Decay / Tail";
        case Ctrl::Time: return "Time / Delay";
        case Ctrl::Hz: return "Cutoff (Hz feel)";
        case Ctrl::Strength: return "Strength";
        case Ctrl::Feedback: return "Feedback";
        default: return "Mix %";
    }
}

inline Ctrl ctrlFromId(const juce::String& id)
{
    const auto s = id.toLowerCase();
    if (s == "decay") return Ctrl::Decay;
    if (s == "time") return Ctrl::Time;
    if (s == "hz") return Ctrl::Hz;
    if (s == "strength") return Ctrl::Strength;
    if (s == "feedback") return Ctrl::Feedback;
    if (s == "amt" || s == "amount") return Ctrl::Amount;
    if (s == "tone") return Ctrl::Tone;
    if (s == "mot" || s == "motion") return Ctrl::Motion;
    if (s == "shp" || s == "shape") return Ctrl::Shape;
    return Ctrl::Mix;
}

// Family: 0 Delay 1 Reverb 2 Stereo 3 Modulation 4 Filter/EQ 5 Drive 6 Dynamics 7 Character
inline juce::Array<Ctrl> controlsForFx(int fxIndex)
{
    juce::Array<Ctrl> out;
    out.add(Ctrl::Mix);
    out.add(Ctrl::Amount);
    int family = 7;
    if (fxIndex >= 0 && fxIndex < kFxCount)
        family = kFx[fxIndex].family;

    auto add = [&out](Ctrl c) { if (! out.contains(c)) out.add(c); };

    switch (family)
    {
        case 0: // Delay
            add(Ctrl::Time); add(Ctrl::Feedback); add(Ctrl::Tone); break;
        case 1: // Reverb
            add(Ctrl::Decay); add(Ctrl::Tone); add(Ctrl::Shape); break;
        case 4: // Filter / EQ
            add(Ctrl::Hz); add(Ctrl::Strength); add(Ctrl::Shape); break;
        case 3: // Modulation
            add(Ctrl::Motion); add(Ctrl::Tone); add(Ctrl::Shape); break;
        case 6: // Dynamics
            add(Ctrl::Strength); add(Ctrl::Tone); add(Ctrl::Shape); break;
        case 5: // Drive
            add(Ctrl::Strength); add(Ctrl::Tone); add(Ctrl::Shape); break;
        default:
            add(Ctrl::Tone); add(Ctrl::Motion); add(Ctrl::Shape); break;
    }
    return out;
}

inline void rebuildOrder(juce::ValueTree& ui, juce::Array<int>& orderedSlots)
{
    orderedSlots.clear();
    struct Node {
        int slot = -1, parentShell = 0, wiredInto = -1, shell = -1;
        juce::String kind;
        bool isScreen = false, satellite = false;
    };
    juce::Array<Node> nodes;
    for (int i = 0; i < ui.getNumChildren(); ++i)
    {
        auto c = ui.getChild(i);
        if (! c.hasType("w")) continue;
        Node n;
        n.slot = (int) c.getProperty("slot", -1);
        n.parentShell = (int) c.getProperty("parent", 0);
        n.wiredInto = (int) c.getProperty("wiredInto", -1);
        n.shell = (int) c.getProperty("shellSlot", -1);
        n.kind = c.getProperty("kind").toString();
        n.isScreen = (n.kind == "wave" || n.kind == "board");
        n.satellite = (int) c.getProperty("satellite", 0) != 0;
        if (n.slot >= 0) nodes.add(n);
    }

    // Topological-ish: nodes wired into X should appear after X.
    // Multiple passes of "emit if parent already emitted or parent is motherboard".
    juce::Array<int> emitted;
    auto tryEmit = [&](const Node& n) {
        if (n.isScreen || n.satellite || n.slot <= 0) return false;
        if (emitted.contains(n.slot)) return false;
        if (n.wiredInto >= 0 && n.wiredInto > 0 && ! emitted.contains(n.wiredInto) && n.wiredInto != n.slot)
        {
            // parent dsp not yet in list — wait unless parent not present
            bool parentExists = false;
            for (const auto& o : nodes)
                if (o.slot == n.wiredInto && ! o.isScreen && ! o.satellite) parentExists = true;
            if (parentExists) return false;
        }
        emitted.add(n.slot);
        return true;
    };

    for (int pass = 0; pass < 64; ++pass)
    {
        bool any = false;
        // Prefer lower shell indices for stability when no wire
        juce::Array<Node> sorted = nodes;
        std::stable_sort(sorted.begin(), sorted.end(), [](const Node& a, const Node& b) {
            if (a.wiredInto != b.wiredInto) return a.wiredInto < b.wiredInto;
            return a.slot < b.slot;
        });
        for (const auto& n : sorted)
            if (tryEmit(n)) any = true;
        if (! any) break;
    }
    // Anything still missing (cycles / unwired) append by slot index
    juce::Array<Node> rest = nodes;
    std::stable_sort(rest.begin(), rest.end(), [](const Node& a, const Node& b) { return a.slot < b.slot; });
    for (const auto& n : rest)
    {
        if (n.isScreen || n.satellite || n.slot <= 0) continue;
        if (! emitted.contains(n.slot)) emitted.add(n.slot);
    }
    orderedSlots = emitted;

    int lastDsp = 0;
    juce::Array<Node> bySlot = nodes;
    std::stable_sort(bySlot.begin(), bySlot.end(), [](const Node& a, const Node& b) { return a.slot < b.slot; });
    // Walk in wire order for tapAfter on screens
    lastDsp = 0;
    for (int s : orderedSlots) lastDsp = s; // default end
    // Assign each screen the max wired slot among modules with smaller shell y/order before it
    for (const auto& n : nodes)
    {
        if (! n.isScreen) continue;
        int tap = 0;
        for (int s : orderedSlots)
        {
            // screens after parts that share earlier parent chain: use last emitted before this shell index
            for (const auto& o : nodes)
                if (o.slot == s && o.shell < n.shell) tap = s;
        }
        for (int i = 0; i < ui.getNumChildren(); ++i)
        {
            auto c = ui.getChild(i);
            if (! c.hasType("w")) continue;
            if ((int) c.getProperty("shellSlot", -1) == n.shell && c.getProperty("kind").toString() == n.kind)
            {
                c.setProperty("tapAfter", tap, nullptr);
                break;
            }
        }
    }

    juce::String orderStr;
    for (int i = 0; i < orderedSlots.size(); ++i)
    {
        if (i) orderStr << ",";
        orderStr << orderedSlots[i];
    }
    ui.setProperty("wireOrder", orderStr, nullptr);
}

inline juce::String apvtsSuffixForBind(const juce::String& bindCtrl)
{
    return juce::String(ctrlId(ctrlFromId(bindCtrl)));
}

} // namespace wire
} // namespace kt

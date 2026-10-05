#include "FxChain.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace kyoto
{
namespace
{
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clamp01 (float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float lerpExp (float a, float b, float t) { return a * std::pow (b / a, t); }

// Bit-level finite check so it keeps working even under -ffast-math.
inline float san (float v)
{
    std::uint32_t b; std::memcpy (&b, &v, 4);
    if ((b & 0x7f800000u) == 0x7f800000u) return 0.0f;
    return v > 64.0f ? 64.0f : (v < -64.0f ? -64.0f : v);
}
inline float onePole (float hz, float sr) { return 1.0f - std::exp (-kTwoPi * std::min (hz, sr * 0.45f) / sr); }
inline float dbToGain (float db) { return std::exp (db * 0.11512925f); }

enum class Kind : std::uint8_t
{
    Drive, Chorus, Delay, Reverb, Width, HighPass, LowPass, Tremolo, AutoPan, Flanger,
    Phaser, Crush, Vibrato, Slap, Telephone, Tape, Stutter, Formant, BrightPlate, Comb,
    Compressor, Warmth, Fade, Grain, Saturate, Rotary, Echo, Air, Grit, Haze,
    Choir, Pulse, Ink, Limiter, Tilt, Bass, Presence, Notch, MonoBass, Room,
    Hall, Exciter, Duck, Shimmer, Focus, Gain, Sheen, Bloom, Swirl, Ping,
    Drift, Snap, Body, AirLift, SubBoost, RingMod, DeepPhase, LongEcho, Freeze, Vinyl,
    Radio, Underwater, Glass, Cream, Sparkle, Clean, Stage, Wire, Shelf, SleepGate,
    Acid, GhostDelay, BoneEQ, Cinder, Orbit, Pluck, DrumGlue, HatAir, DarkHall, TightComp,
    OpenComp, DeEss, SideAir, Flutter, Gate16, OctaveDown, OctaveUp, Resonant, Smear, Dust
};

using K = Kind;
enum class Fam : std::uint8_t { Sat, Filt, Mod, Phase, Delay, Verb, Dyn, AmpMod, Stereo, Crush, Ring, OctDn, OctUp, Util };

// 200 effect slots -> algorithm kind. Position in this table == effect index.
const Kind kRecipes[kNumEffects] = {
        K::Drive, K::Chorus, K::Delay, K::Reverb, K::Width, K::HighPass, K::LowPass, K::Tremolo, K::AutoPan, K::Flanger,
        K::Phaser, K::Crush, K::Vibrato, K::Slap, K::Telephone, K::Tape, K::Stutter, K::Formant, K::BrightPlate, K::Comb,
        K::Compressor, K::Warmth, K::Delay, K::Fade, K::Grain, K::Saturate, K::Chorus, K::Rotary, K::Echo, K::Delay,
        K::Air, K::Grit, K::Haze, K::Grit, K::Phaser, K::Choir, K::Delay, K::Reverb, K::Pulse, K::Haze,
        K::Tape, K::Ink, K::Limiter, K::Saturate, K::Tilt, K::Bass, K::Air, K::Presence, K::Haze, K::Notch,
        K::Notch, K::Width, K::MonoBass, K::Room, K::Hall, K::Exciter, K::Warmth, K::Haze, K::Duck, K::Shimmer,
        K::Chorus, K::Focus, K::Gain, K::Width, K::Sheen, K::Saturate, K::Grit, K::Bloom, K::Swirl, K::Ping,
        K::Drift, K::Snap, K::Body, K::AirLift, K::SubBoost, K::Notch, K::RingMod, K::DeepPhase, K::LongEcho, K::Freeze,
        K::Vinyl, K::Radio, K::Underwater, K::Glass, K::Pulse, K::AutoPan, K::Cream, K::Reverb, K::Compressor, K::Limiter,
        K::Drive, K::Air, K::Bass, K::Tilt, K::Width, K::MonoBass, K::LowPass, K::HighPass, K::Sparkle, K::Warmth,
        K::Compressor, K::Warmth, K::Bass, K::Reverb, K::Width, K::Limiter, K::Tilt, K::Clean, K::Stage, K::Wire,
        K::Compressor, K::Shelf, K::Gain, K::SubBoost, K::Presence, K::Notch, K::Notch, K::Air, K::SleepGate, K::Room,
        K::Acid, K::GhostDelay, K::BoneEQ, K::Warmth, K::Cinder, K::Shelf, K::Glass, K::Drive, K::Hall, K::Room,
        K::Echo, K::Tape, K::Reverb, K::Delay, K::Orbit, K::Choir, K::Pluck, K::DrumGlue, K::Bass, K::HatAir,
        K::Snap, K::Room, K::DarkHall, K::BrightPlate, K::Flanger, K::Tremolo, K::Chorus, K::TightComp, K::OpenComp, K::DeEss,
        K::MonoBass, K::SideAir, K::Crush, K::Tape, K::Flutter, K::Gate16, K::Shimmer, K::OctaveDown, K::OctaveUp, K::Resonant,
        K::Smear, K::Dust, K::Tape, K::Limiter, K::Saturate, K::Air, K::Width, K::Hall, K::Room, K::Delay,
        K::Phaser, K::Chorus, K::Vibrato, K::AutoPan, K::Tremolo, K::LowPass, K::HighPass, K::Drive, K::Grit, K::Haze,
        K::Rotary, K::Saturate, K::Chorus, K::Echo, K::Delay, K::Air, K::Grit, K::Phaser, K::Choir, K::Haze,
        K::Ink, K::Tape, K::Reverb, K::Pulse, K::Delay, K::Orbit, K::Choir, K::BrightPlate, K::Comb, K::Formant,
};

struct KindInfo { Fam fam; int cat; float nat; };

KindInfo infoFor (Kind k)
{
    using K = Kind;
    switch (k)
    {
        case K::Drive: case K::Saturate: case K::Warmth: case K::Cream: case K::Ink: case K::Cinder:
        case K::Vinyl: case K::Grit: case K::Snap: case K::Acid: case K::Dust: case K::Exciter:
            return { Fam::Sat, 0, k == K::Exciter || k == K::Dust || k == K::Vinyl ? 0.6f : 1.0f };
        case K::LowPass: case K::HighPass: case K::Notch: case K::Presence: case K::Tilt: case K::Shelf:
        case K::Bass: case K::SubBoost: case K::Body: case K::Focus: case K::Air: case K::AirLift:
        case K::HatAir: case K::SideAir: case K::Wire: case K::Sheen: case K::Sparkle: case K::BoneEQ:
        case K::Formant: case K::DeEss: case K::Telephone: case K::Radio: case K::Underwater:
        case K::Haze: case K::Glass: case K::Clean: case K::Resonant:
            return { Fam::Filt, 1, 1.0f };
        case K::Chorus: case K::Flanger: case K::Vibrato: case K::Rotary: case K::Swirl: case K::Drift:
        case K::Orbit: case K::Flutter: case K::Tape: case K::Grain:
            return { Fam::Mod, 2, (k == K::Vibrato || k == K::Tape || k == K::Flutter || k == K::Drift) ? 1.0f : 0.55f };
        case K::Phaser: case K::DeepPhase: return { Fam::Phase, 2, 0.6f };
        case K::Delay: case K::Echo: case K::Slap: case K::Ping: case K::LongEcho: case K::GhostDelay:
            return { Fam::Delay, 3, 0.5f };
        case K::Pluck: case K::Comb: case K::Stutter: case K::Freeze: case K::Smear:
            return { Fam::Delay, 3, k == K::Stutter ? 1.0f : 0.6f };
        case K::Reverb: case K::Room: case K::Hall: case K::BrightPlate: case K::DarkHall: case K::Stage:
        case K::Bloom: case K::Choir: case K::Shimmer:
            return { Fam::Verb, 4, 0.4f };
        case K::Compressor: case K::DrumGlue: case K::TightComp: case K::OpenComp: case K::Limiter: case K::Duck:
            return { Fam::Dyn, 5, 1.0f };
        case K::Tremolo: case K::Pulse: case K::Gate16: case K::SleepGate: case K::AutoPan: case K::Fade:
            return { Fam::AmpMod, 2, 1.0f };
        case K::Width: case K::MonoBass: return { Fam::Stereo, 6, 1.0f };
        case K::Gain:    return { Fam::Util, 6, 1.0f };
        case K::Crush:   return { Fam::Crush, 7, 1.0f };
        case K::RingMod: return { Fam::Ring, 7, 0.8f };
        case K::OctaveDown: return { Fam::OctDn, 7, 0.8f };
        case K::OctaveUp:   return { Fam::OctUp, 7, 0.8f };
    }
    return { Fam::Util, 6, 1.0f };
}

// Each effect gets a stratified "voicing": effects of the same kind are spread
// evenly across u0 so no two siblings share the same tuning.
struct Voicing { float u0, u1, u2, u3; };

struct VoicingTable
{
    Voicing v[kNumEffects];
    VoicingTable()
    {
        int count[128] = {}, seen[128] = {};
        for (int i = 0; i < kNumEffects; ++i) ++count[(int) kRecipes[i]];
        for (int i = 0; i < kNumEffects; ++i)
        {
            const int k = (int) kRecipes[i];
            const int n = seen[k]++;
            const float u0 = (n + 0.5f) / (float) count[k];
            auto fr = [] (float x) { return x - std::floor (x); };
            v[i] = { u0, fr (n * 0.381966f + 0.27f + k * 0.113f), fr (n * 0.754878f + 0.61f), fr (n * 0.569840f + 0.43f + k * 0.071f) };
        }
    }
};
const VoicingTable& voicings() { static const VoicingTable t; return t; }

// ---------------------------------------------------------------------------
enum class Bq { LP, HP, BP, Peak, LowShelf, HighShelf };

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] = {}, z2[2] = {};

    void set (Bq mode, float sr, float f, float q, float gainDb)
    {
        f = std::clamp (f, 15.0f, sr * 0.45f);
        q = std::clamp (q, 0.2f, 18.0f);
        const float w = kTwoPi * f / sr, cw = std::cos (w), sw = std::sin (w), al = sw / (2.0f * q);
        float nb0 = 1, nb1 = 0, nb2 = 0, na0 = 1, na1 = 0, na2 = 0;
        const float A = std::pow (10.0f, gainDb / 40.0f);
        switch (mode)
        {
            case Bq::LP: nb0 = (1 - cw) * 0.5f; nb1 = 1 - cw; nb2 = nb0; na0 = 1 + al; na1 = -2 * cw; na2 = 1 - al; break;
            case Bq::HP: nb0 = (1 + cw) * 0.5f; nb1 = -(1 + cw); nb2 = nb0; na0 = 1 + al; na1 = -2 * cw; na2 = 1 - al; break;
            case Bq::BP: nb0 = al; nb1 = 0; nb2 = -al; na0 = 1 + al; na1 = -2 * cw; na2 = 1 - al; break;
            case Bq::Peak:
                nb0 = 1 + al * A; nb1 = -2 * cw; nb2 = 1 - al * A;
                na0 = 1 + al / A; na1 = -2 * cw; na2 = 1 - al / A; break;
            case Bq::LowShelf:
            {
                const float s = 2.0f * std::sqrt (A) * al;
                nb0 = A * ((A + 1) - (A - 1) * cw + s); nb1 = 2 * A * ((A - 1) - (A + 1) * cw); nb2 = A * ((A + 1) - (A - 1) * cw - s);
                na0 = (A + 1) + (A - 1) * cw + s; na1 = -2 * ((A - 1) + (A + 1) * cw); na2 = (A + 1) + (A - 1) * cw - s; break;
            }
            case Bq::HighShelf:
            {
                const float s = 2.0f * std::sqrt (A) * al;
                nb0 = A * ((A + 1) + (A - 1) * cw + s); nb1 = -2 * A * ((A - 1) + (A + 1) * cw); nb2 = A * ((A + 1) + (A - 1) * cw - s);
                na0 = (A + 1) - (A - 1) * cw + s; na1 = 2 * ((A - 1) - (A + 1) * cw); na2 = (A + 1) - (A - 1) * cw - s; break;
            }
        }
        const float inv = 1.0f / na0;
        b0 = nb0 * inv; b1 = nb1 * inv; b2 = nb2 * inv; a1 = na1 * inv; a2 = na2 * inv;
    }
    inline float run (int ch, float x)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void clear() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
};

struct Ctx
{
    float sr; Kind kind; KindInfo ki; Voicing v;
    float amt, tone, mot, shp;
};

constexpr int kBlock = 32;
constexpr int kAllpass = 6;

struct Slot
{
    int effect = -1;
    float gate = 0, amt = 0.5f, tone = 0.5f, mot = 0.5f, mixp = 0.5f, shp = 0.5f;

    Biquad bq[3];
    float lp[2][4] = {};
    float ph[3] = {};
    float env[2] = {};
    float held[2] = {};
    float cnt = 0, flip = 1, lastSign = 0, rnd = 0, rndTarget = 0, rndTimer = 0;
    float apState[2][kAllpass] = {};
    std::uint32_t rng = 12345u;

    // memory: delay line (2 * D) or reverb network carved from the same block
    std::vector<float> mem;
    std::size_t D = 0, wp = 0;
    std::size_t cbOff[2][4] = {}, cbLen[2][4] = {}, cbPos[2][4] = {};
    float cbLp[2][4] = {};
    std::size_t apOff[2][2] = {}, apLen[2][2] = {}, apPos[2][2] = {};
    std::size_t capPos = 0, cycle = 0, cycleN = 0;

    float noise() { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) * (1.0f / 8388608.0f) - 1.0f; }

    void clearState (float sr)
    {
        for (auto& b : bq) b.clear();
        std::memset (lp, 0, sizeof lp); std::memset (ph, 0, sizeof ph); std::memset (env, 0, sizeof env);
        std::memset (held, 0, sizeof held); std::memset (apState, 0, sizeof apState);
        std::memset (cbLp, 0, sizeof cbLp);
        cnt = 0; flip = 1; lastSign = 0; rnd = rndTarget = rndTimer = 0; wp = 0; capPos = 0; cycle = 0; cycleN = 0;
        std::fill (mem.begin(), mem.end(), 0.0f);
        std::memset (cbPos, 0, sizeof cbPos); std::memset (apPos, 0, sizeof apPos);
        (void) sr;
    }

    // Delay line access (channel ch). d = delay in samples, linear interpolation.
    inline float readDelay (int ch, float d) const
    {
        d = std::clamp (d, 1.0f, (float) (D - 3));
        const float rp = (float) wp - d;
        float fl = std::floor (rp);
        const float fr = rp - fl;
        long long i0 = (long long) fl;
        const long long Dl = (long long) D;
        i0 = ((i0 % Dl) + Dl) % Dl;
        const long long i1 = (i0 + 1) % Dl;
        const float* base = mem.data() + (std::size_t) ch * D;
        return base[i0] + (base[i1] - base[i0]) * fr;
    }
    inline void writeDelay (int ch, float v) { mem[(std::size_t) ch * D + wp] = v; }
    inline void advance() { if (++wp >= D) wp = 0; }

    // Reverb network setup. scale stretches room size.
    void setupVerb (float sr, float scale)
    {
        static const int cl[4] = { 1116, 1188, 1277, 1356 }, cr[4] = { 1139, 1211, 1300, 1379 };
        static const int al[2] = { 556, 441 };
        std::size_t off = 0;
        for (int c = 0; c < 4; ++c)
        {
            cbLen[0][c] = std::max<std::size_t> (16, (std::size_t) (cl[c] * scale * sr / 44100.0f));
            cbLen[1][c] = std::max<std::size_t> (16, (std::size_t) (cr[c] * scale * sr / 44100.0f));
            for (int ch = 0; ch < 2; ++ch) { cbOff[ch][c] = off; off += cbLen[ch][c]; }
        }
        for (int a = 0; a < 2; ++a)
            for (int ch = 0; ch < 2; ++ch)
            {
                apLen[ch][a] = std::max<std::size_t> (8, (std::size_t) ((al[a] + ch * 23) * std::sqrt (scale) * sr / 44100.0f));
                apOff[ch][a] = off; off += apLen[ch][a];
            }
        if (off > mem.size()) { for (int c = 0; c < 4; ++c) for (int ch = 0; ch < 2; ++ch) cbLen[ch][c] = 16; off = 0; } // safety
    }
};

float vowelF (int vowel, int formant)
{
    static const float t[5][3] = { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 350, 2000, 2800 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
    return t[vowel][formant];
}

} // namespace

// ===========================================================================
struct FxChain::Impl
{
    double sr = 44100.0;
    std::array<Slot, kMaxFxSlots> slots;
    float dcIn[2] = {}, dcOut[2] = {};
    float limGain = 1.0f;

    void prepare (double rate)
    {
        sr = (rate >= 8000.0 && rate <= 768000.0) ? rate : 44100.0;
        const std::size_t D = (std::size_t) (0.5 * sr) + 8;
        for (auto& s : slots)
        {
            s.D = D;
            s.mem.assign (2 * D, 0.0f);
            s.effect = -1; s.gate = 0;
            s.clearState ((float) sr);
        }
        dcIn[0] = dcIn[1] = dcOut[0] = dcOut[1] = 0; limGain = 1.0f;
    }

    void reset()
    {
        for (auto& s : slots) { s.effect = -1; s.gate = 0; s.clearState ((float) sr); }
        dcIn[0] = dcIn[1] = dcOut[0] = dcOut[1] = 0; limGain = 1.0f;
    }

    // ---- algorithm families --------------------------------------------------
    static void runSat (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runFilt (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runMod (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runPhase (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runDelay (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runVerb (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runDyn (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runAmpMod (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);
    static void runMisc (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n);

    void processSlot (Slot& s, const FxSlotParams& p, float* L, float* R, int numSamples)
    {
        const bool valid = p.effect >= 0 && p.effect < kNumEffects;
        const float gateTarget = (valid && p.on) ? 1.0f : 0.0f;
        if (!valid && s.gate < 1.0e-4f) { s.effect = -1; return; }

        if (valid && p.effect != s.effect)
        {
            s.effect = p.effect;
            s.clearState ((float) sr);
            s.amt = p.amount; s.tone = p.tone; s.mot = p.motion; s.mixp = p.mix; s.shp = p.shape;
            s.gate = 0;
            if (infoFor (kRecipes[s.effect]).fam == Fam::Verb)
            {
                const auto v = voicings().v[s.effect];
                const Kind k = kRecipes[s.effect];
                float scale = lerp (0.7f, 1.6f, v.u0);
                if (k == Kind::Room) scale = lerp (0.45f, 0.95f, v.u0);
                else if (k == Kind::Hall || k == Kind::DarkHall) scale = lerp (1.2f, 2.0f, v.u0);
                else if (k == Kind::BrightPlate) scale = lerp (0.6f, 1.2f, v.u0);
                s.setupVerb ((float) sr, scale);
            }
        }
        if (s.effect < 0) return;

        const Kind kind = kRecipes[s.effect];
        const KindInfo ki = infoFor (kind);
        const Voicing v = voicings().v[s.effect];

        float inL[kBlock], inR[kBlock], wL[kBlock], wR[kBlock];

        for (int start = 0; start < numSamples; start += kBlock)
        {
            const int n = std::min (kBlock, numSamples - start);
            s.amt  += (p.amount - s.amt)  * 0.15f;
            s.tone += (p.tone   - s.tone) * 0.15f;
            s.mot  += (p.motion - s.mot)  * 0.15f;
            s.mixp += (p.mix    - s.mixp) * 0.15f;
            s.shp  += (p.shape  - s.shp)  * 0.15f;
            s.gate += (gateTarget - s.gate) * 0.12f;

            float* l = L + start;
            float* r = R + start;
            std::memcpy (inL, l, sizeof (float) * (std::size_t) n);
            std::memcpy (inR, r, sizeof (float) * (std::size_t) n);
            std::memcpy (wL, inL, sizeof (float) * (std::size_t) n);
            std::memcpy (wR, inR, sizeof (float) * (std::size_t) n);

            const Ctx c { (float) sr, kind, ki, v, clamp01 (s.amt), clamp01 (s.tone), clamp01 (s.mot), clamp01 (s.shp) };
            switch (ki.fam)
            {
                case Fam::Sat:    runSat (s, c, inL, inR, wL, wR, n); break;
                case Fam::Filt:   runFilt (s, c, inL, inR, wL, wR, n); break;
                case Fam::Mod:    runMod (s, c, inL, inR, wL, wR, n); break;
                case Fam::Phase:  runPhase (s, c, inL, inR, wL, wR, n); break;
                case Fam::Delay:  runDelay (s, c, inL, inR, wL, wR, n); break;
                case Fam::Verb:   runVerb (s, c, inL, inR, wL, wR, n); break;
                case Fam::Dyn:    runDyn (s, c, inL, inR, wL, wR, n); break;
                case Fam::AmpMod: runAmpMod (s, c, inL, inR, wL, wR, n); break;
                default:          runMisc (s, c, inL, inR, wL, wR, n); break;
            }

            const float m = std::min (1.0f, ki.nat * std::min (2.0f, s.mixp * 2.0f)) * s.gate;
            for (int i = 0; i < n; ++i)
            {
                l[i] = san (inL[i] + (san (wL[i]) - inL[i]) * m);
                r[i] = san (inR[i] + (san (wR[i]) - inR[i]) * m);
            }
        }
    }

    void process (float* left, float* right, int numSamples, const FxSlotParams* sp, int ns)
    {
        if (left == nullptr || numSamples <= 0) return;
        const bool stereo = right != nullptr;

        // work in fixed scratch so mono hosts take the same path
        float tmpL[kBlock * 16], tmpR[kBlock * 16];
        for (int start = 0; start < numSamples; start += kBlock * 16)
        {
            const int n = std::min (kBlock * 16, numSamples - start);
            for (int i = 0; i < n; ++i) { tmpL[i] = san (left[start + i]); tmpR[i] = stereo ? san (right[start + i]) : tmpL[i]; }

            for (int k = 0; k < std::min (ns, kMaxFxSlots); ++k)
                processSlot (slots[(std::size_t) k], sp[k], tmpL, tmpR, n);

            const float dcC = std::exp (-kTwoPi * 5.0f / (float) sr);
            const float rel = onePole (6.0f, (float) sr);
            for (int i = 0; i < n; ++i)
            {
                const float l = tmpL[i], r = tmpR[i];
                const float yl = l - dcIn[0] + dcC * dcOut[0], yr = r - dcIn[1] + dcC * dcOut[1];
                dcIn[0] = l; dcIn[1] = r; dcOut[0] = san (yl); dcOut[1] = san (yr);
                const float pk = std::max (std::abs (yl), std::abs (yr));
                const float tg = pk > 0.98f ? 0.98f / pk : 1.0f;
                if (tg < limGain) limGain = tg; else limGain += (tg - limGain) * rel;
                left[start + i] = san (yl * limGain);
                if (stereo) right[start + i] = san (yr * limGain);
            }
        }
    }
};

// ---------------------------------------------------------------------------
// SATURATION
void FxChain::Impl::runSat (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    const float drive = lerpExp (1.2f, 16.0f, c.amt) * lerp (0.7f, 1.35f, c.v.u0);
    const float bias = (c.shp - 0.5f) * 0.7f;
    const float post = onePole (lerpExp (1200.0f, 17000.0f, c.tone) * lerp (0.7f, 1.2f, c.v.u1), c.sr);
    const float comp = 1.0f / (0.55f + 0.45f * std::sqrt (drive));
    const float hp = onePole (lerpExp (1200.0f, 5500.0f, c.v.u0), c.sr);

    // Transfer curve morphs across the family by u0/u2 so siblings do not sound alike.
    const float morph = c.v.u0 * 3.0f;
    const int   m0 = std::min (2, (int) morph);
    const float mf = morph - (float) m0;
    auto curve = [] (int t, float x) -> float
    {
        switch (t)
        {
            case 0:  return std::tanh (x);                                   // smooth
            case 1:  return x / (1.0f + std::abs (x));                       // soft/tube-ish
            case 2:  return x > 0 ? std::tanh (x * 1.4f) : std::tanh (x * 0.55f) * 1.3f; // asymmetric
            default: return std::sin (std::clamp (x, -1.5707f, 1.5707f));   // sine saturator
        }
    };
    auto shaper = [&] (float x) -> float
    {
        switch (c.kind)
        {
            case K::Ink: case K::Cinder: case K::Vinyl:   // asymmetric: even harmonics
                return curve (2, x) * (0.8f + 0.2f * mf) + curve (m0 == 2 ? 1 : 0, x) * (0.2f * (1.0f - mf));
            case K::Acid:                                  // foldback
            {
                float y = x * (0.8f + 0.8f * c.v.u2); for (int i = 0; i < 5 && std::abs (y) > 1.0f; ++i) y = (y > 0 ? 2.0f : -2.0f) - y; return y;
            }
            case K::Grit: case K::Snap:                    // hard, bright clip
                return std::clamp (x * (1.0f + c.v.u2), -0.9f, 0.9f) * (1.0f + 0.1f * std::sin (x * (2.0f + 6.0f * c.v.u0)));
            case K::Dust:                                  // sample-crunch
            {   const float st = 4.0f + 36.0f * (1.0f - c.amt) * (0.4f + c.v.u0); return std::round (x * st) / st; }
            default: return curve (m0, x) * (1.0f - mf) + curve (m0 + 1, x) * mf;
        }
    };
    // pre-EQ colour (different hump per effect) fed into the shaper
    s.bq[0].set (Bq::Peak, c.sr, lerpExp (180.0f, 3500.0f, c.v.u1), 0.9f, (c.v.u2 - 0.5f) * 14.0f * (0.4f + c.amt));
    const float b0 = shaper (bias * drive);

    for (int i = 0; i < n; ++i)
    {
        float in[2] = { iL[i], iR[i] }, out[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            float x = s.bq[0].run (ch, in[ch]);
            if (c.kind == K::Exciter) { s.lp[ch][1] += hp * (x - s.lp[ch][1]); x = x - s.lp[ch][1]; }
            float y = (shaper (x * drive + bias * drive) - b0) * comp;
            if (c.kind == K::Exciter) y = in[ch] + y * c.amt * 0.9f;
            if (c.kind == K::Dust || c.kind == K::Vinyl)
            {
                const float r = s.noise();
                if (std::abs (r) > 0.9985f) y += r * 0.35f * c.amt; // crackle
                y += s.noise() * 0.004f * c.amt;
            }
            if (c.kind == K::Warmth || c.kind == K::Vinyl || c.kind == K::Cream)
            {   // low-mid body
                s.lp[ch][2] += onePole (200.0f, c.sr) * (in[ch] - s.lp[ch][2]);
                y = y * 0.8f + s.lp[ch][2] * 0.2f * (1.0f + c.amt);
            }
            s.lp[ch][0] += post * (y - s.lp[ch][0]);
            out[ch] = s.lp[ch][0];
        }
        oL[i] = out[0]; oR[i] = out[1];
    }
}

// ---------------------------------------------------------------------------
// FILTERS / EQ
void FxChain::Impl::runFilt (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    const float u0 = c.v.u0, u1 = c.v.u1, u2v = c.v.u2;
    const float toneShift = std::pow (2.0f, (c.tone - 0.5f) * 2.0f);
    int stages = 1; bool sat = false, side = false, deess = false;
    float gGlass = 0;

    auto q = [&] (float lo, float hi) { return lerp (lo, hi, clamp01 (u1 * 0.5f + c.shp * 0.5f)); };

    switch (c.kind)
    {
        case K::LowPass:  s.bq[0].set (Bq::LP, c.sr, lerpExp (14000.0f, 250.0f, c.amt) * lerp (1.3f, 0.7f, u0) * toneShift, q (0.6f, 2.2f), 0); break;
        case K::HighPass: s.bq[0].set (Bq::HP, c.sr, lerpExp (30.0f, 2500.0f, c.amt) * lerp (0.6f, 1.4f, u0) * toneShift, q (0.6f, 1.8f), 0); break;
        case K::Notch:    s.bq[0].set (Bq::Peak, c.sr, lerpExp (250.0f, 7000.0f, u0) * toneShift, q (2.0f, 10.0f), -26.0f * c.amt); break;
        case K::Presence: s.bq[0].set (Bq::Peak, c.sr, lerpExp (1800.0f, 5500.0f, u0) * toneShift, q (0.7f, 1.8f), 11.0f * c.amt); break;
        case K::Tilt:
            s.bq[0].set (Bq::LowShelf, c.sr, lerpExp (300.0f, 900.0f, u0), 0.7f, -9.0f * (c.amt - 0.5f) * 2.0f * toneShift);
            s.bq[1].set (Bq::HighShelf, c.sr, lerpExp (1500.0f, 4000.0f, u0), 0.7f, 9.0f * (c.amt - 0.5f) * 2.0f);
            stages = 2; break;
        case K::Shelf:    s.bq[0].set (Bq::HighShelf, c.sr, lerpExp (1200.0f, 5000.0f, u0) * toneShift, 0.35f, 8.0f * c.amt); break;
        case K::Bass:     s.bq[0].set (Bq::LowShelf, c.sr, lerpExp (70.0f, 320.0f, u0) * toneShift, q (0.5f, 1.2f), 12.0f * c.amt); break;
        case K::SubBoost: s.bq[0].set (Bq::Peak, c.sr, lerpExp (28.0f, 70.0f, u0) * toneShift, q (1.0f, 2.2f), 12.0f * c.amt); break;
        case K::Body:     s.bq[0].set (Bq::Peak, c.sr, lerpExp (110.0f, 340.0f, u0) * toneShift, q (0.8f, 2.0f), 9.0f * c.amt); break;
        case K::Focus:    s.bq[0].set (Bq::Peak, c.sr, lerpExp (600.0f, 2000.0f, u0) * toneShift, q (0.9f, 2.5f), 8.0f * c.amt); break;
        case K::Air:      s.bq[0].set (Bq::HighShelf, c.sr, lerpExp (3500.0f, 14000.0f, u0) * toneShift, lerp (0.5f, 1.2f, u1), 13.0f * c.amt);
                          s.bq[1].set (Bq::Peak, c.sr, lerpExp (8000.0f, 16000.0f, u2v), 1.5f, 5.0f * c.amt * u1); stages = 2; break;
        case K::AirLift:  s.bq[0].set (Bq::HighShelf, c.sr, lerpExp (4500.0f, 9000.0f, u0) * toneShift, 0.6f, 10.0f * c.amt); break;
        case K::HatAir:   s.bq[0].set (Bq::HighShelf, c.sr, lerpExp (8500.0f, 14000.0f, u0) * toneShift, 0.9f, 11.0f * c.amt);
                          s.bq[1].set (Bq::HP, c.sr, 200.0f, 0.7f, 0); stages = 2; break;
        case K::SideAir:  s.bq[0].set (Bq::HighShelf, c.sr, lerpExp (3000.0f, 9000.0f, u0) * toneShift, 0.7f, 14.0f * c.amt); side = true; break;
        case K::Wire:     s.bq[0].set (Bq::Peak, c.sr, lerpExp (2500.0f, 7500.0f, u0) * toneShift, q (2.0f, 5.0f), 11.0f * c.amt); sat = true; break;
        case K::Sheen:    s.bq[0].set (Bq::HighShelf, c.sr, lerpExp (2500.0f, 6500.0f, u0) * toneShift, 0.5f, 7.0f * c.amt);
                          s.bq[1].set (Bq::Peak, c.sr, lerpExp (9000.0f, 13000.0f, u1), 1.2f, 5.0f * c.amt); stages = 2; break;
        case K::Sparkle:  s.bq[0].set (Bq::Peak, c.sr, lerpExp (7500.0f, 14500.0f, u0) * toneShift, q (1.0f, 3.0f), 12.0f * c.amt); break;
        case K::BoneEQ:   s.bq[0].set (Bq::Peak, c.sr, lerpExp (300.0f, 1000.0f, u0) * toneShift, q (0.8f, 1.8f), -10.0f * c.amt);
                          s.bq[1].set (Bq::HighShelf, c.sr, 5000.0f, 0.7f, 5.0f * c.amt); stages = 2; break;
        case K::Formant:
        {
            const float pos = clamp01 (c.tone * 0.7f + u0 * 0.3f) * 4.0f;
            const int a = std::min (3, (int) pos); const float fr = pos - (float) a;
            for (int f = 0; f < 3; ++f)
                s.bq[f].set (Bq::Peak, c.sr, lerp (vowelF (a, f), vowelF (a + 1, f), fr), q (4.0f, 9.0f), (f == 0 ? 15.0f : f == 1 ? 13.0f : 9.0f) * c.amt);
            stages = 3; break;
        }
        case K::DeEss: s.bq[0].set (Bq::HP, c.sr, lerpExp (4500.0f, 9000.0f, u0) * toneShift, 0.8f, 0); deess = true; break;
        case K::Telephone:
            s.bq[0].set (Bq::HP, c.sr, lerpExp (300.0f, 750.0f, u0) * toneShift, 0.9f, 0);
            s.bq[1].set (Bq::LP, c.sr, lerpExp (2600.0f, 4200.0f, u1), 0.9f, 0);
            s.bq[2].set (Bq::Peak, c.sr, lerpExp (1000.0f, 2200.0f, u0), 2.0f, 6.0f * c.amt); stages = 3; sat = true; break;
        case K::Radio:
            s.bq[0].set (Bq::HP, c.sr, lerpExp (140.0f, 420.0f, u0) * toneShift, 0.8f, 0);
            s.bq[1].set (Bq::LP, c.sr, lerpExp (4200.0f, 7500.0f, u1), 0.8f, 0);
            s.bq[2].set (Bq::Peak, c.sr, lerpExp (1500.0f, 3200.0f, u0), 1.3f, 5.0f * c.amt); stages = 3; sat = true; break;
        case K::Underwater:
        {
            const float lfo = std::sin (kTwoPi * s.ph[0]);
            s.ph[0] += n * lerpExp (0.15f, 1.4f, c.mot) / c.sr; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;
            s.bq[0].set (Bq::LP, c.sr, lerpExp (1800.0f, 220.0f, c.amt) * lerp (0.7f, 1.4f, u0) * toneShift * (1.0f + 0.45f * lfo * c.mot), q (0.8f, 3.0f), 0); break;
        }
        case K::Haze:  s.bq[0].set (Bq::LP, c.sr, lerpExp (9000.0f, 1800.0f, c.amt) * lerp (0.4f, 2.0f, u0) * toneShift, lerp (0.5f, 1.8f, u1), 0);
                       s.bq[1].set (Bq::Peak, c.sr, lerpExp (300.0f, 3000.0f, u2v), 0.8f, -6.0f * c.amt * (u1 + 0.3f)); stages = 2; break;
        case K::Glass:
            s.bq[0].set (Bq::HP, c.sr, lerpExp (500.0f, 2200.0f, u0), 0.7f, 0);
            s.bq[1].set (Bq::Peak, c.sr, lerpExp (3200.0f, 9500.0f, u1) * toneShift, q (3.0f, 8.0f), 14.0f * c.amt); stages = 2; gGlass = 1; break;
        case K::Clean: s.bq[0].set (Bq::HP, c.sr, lerpExp (22.0f, 140.0f, u0 * 0.5f + c.amt * 0.5f), 0.707f, 0);
                       s.bq[1].set (Bq::Peak, c.sr, lerpExp (180.0f, 420.0f, u1), 1.0f, -5.0f * c.amt); stages = 2; break;
        case K::Resonant:
        {
            const float lfo = 0.5f + 0.5f * std::sin (kTwoPi * s.ph[0]);
            s.ph[0] += n * lerpExp (0.2f, 3.0f, c.mot) / c.sr; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;
            s.bq[0].set (Bq::Peak, c.sr, lerpExp (250.0f, 4500.0f, clamp01 (u0 * 0.5f + lfo * 0.5f)) * toneShift, q (3.0f, 10.0f), 16.0f * c.amt); break;
        }
        default: break;
    }
    (void) gGlass;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { iL[i], iR[i] }, y[2];
        float mid = 0, sd = 0;
        if (side) { mid = 0.5f * (x[0] + x[1]); sd = 0.5f * (x[0] - x[1]); x[0] = sd; x[1] = sd; }
        for (int ch = 0; ch < 2; ++ch)
        {
            float v = x[ch];
            if (deess)
            {
                const float hp = s.bq[0].run (ch, v);
                s.env[ch] += (std::abs (hp) > s.env[ch] ? 0.3f : 0.002f) * (std::abs (hp) - s.env[ch]);
                const float thr = lerp (0.2f, 0.02f, c.amt);
                const float red = clamp01 (1.0f - thr / std::max (s.env[ch], 1.0e-4f));
                v = v - hp * red * 0.95f;
            }
            else
                for (int st = 0; st < stages; ++st) v = s.bq[st].run (ch, v);
            if (sat) v = std::tanh (v * (1.0f + c.amt * 1.5f)) / (1.0f + c.amt * 0.5f);
            y[ch] = v;
        }
        if (side) { oL[i] = mid + y[0]; oR[i] = mid - y[0]; }
        else { oL[i] = y[0]; oR[i] = y[1]; }
    }
}

// ---------------------------------------------------------------------------
// MODULATED DELAY (chorus family)
void FxChain::Impl::runMod (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    float rateLo = 0.2f, rateHi = 1.8f, centre = 16.0f, depth = 6.0f, fb = 0.0f, phaseOff = 0.25f, wet = 1.0f;
    switch (c.kind)
    {
        case K::Chorus:  rateLo = 0.15f; rateHi = 1.6f;  centre = 17.0f; depth = 3.0f + 9.0f * c.amt; phaseOff = 0.25f; break;
        case K::Flanger: rateLo = 0.05f; rateHi = 1.2f;  centre = 2.8f;  depth = 1.0f + 3.0f * c.amt; fb = (c.shp - 0.3f) * 1.1f; phaseOff = 0.2f; break;
        case K::Vibrato: rateLo = 3.5f;  rateHi = 7.5f;  centre = 6.0f;  depth = 0.5f + 5.5f * c.amt;  phaseOff = 0.0f; break;
        case K::Rotary:  rateLo = 0.7f;  rateHi = 6.5f;  centre = 3.0f;  depth = 0.4f + 2.2f * c.amt;  phaseOff = 0.5f; break;
        case K::Swirl:   rateLo = 0.08f; rateHi = 0.9f;  centre = 24.0f; depth = 5.0f + 14.0f * c.amt; phaseOff = 0.5f; break;
        case K::Drift:   rateLo = 0.05f; rateHi = 0.6f;  centre = 12.0f; depth = 0.5f + 4.0f * c.amt;  phaseOff = 0.33f; break;
        case K::Orbit:   rateLo = 0.1f;  rateHi = 0.8f;  centre = 30.0f; depth = 4.0f + 16.0f * c.amt; phaseOff = 0.75f; fb = 0.25f * c.shp; break;
        case K::Flutter: rateLo = 7.0f;  rateHi = 18.0f; centre = 5.0f;  depth = 0.2f + 1.6f * c.amt;  phaseOff = 0.13f; break;
        case K::Tape:    rateLo = 0.4f;  rateHi = 2.5f;  centre = 10.0f; depth = 0.3f + 2.4f * c.amt;  phaseOff = 0.08f; break;
        case K::Grain:   rateLo = 4.0f;  rateHi = 22.0f; centre = 22.0f; depth = 4.0f + 16.0f * c.amt; phaseOff = 0.5f; break;
        default: break;
    }
    const float rate = lerpExp (rateLo, rateHi, c.v.u0) * std::pow (2.0f, (c.mot - 0.5f) * 3.0f);
    const float inc = rate / c.sr;
    const float sec = 0.001f * c.sr;
    const float lpC = onePole (lerpExp (1500.0f, 16000.0f, c.tone), c.sr);
    const float tapeDrive = 1.0f + c.amt * 2.0f;
    fb = std::clamp (fb, -0.85f, 0.85f);

    for (int i = 0; i < n; ++i)
    {
        float lfoL, lfoR;
        if (c.kind == K::Drift || c.kind == K::Grain)
        {   // smoothed random walk
            s.rndTimer -= 1.0f;
            if (s.rndTimer <= 0.0f) { s.rndTarget = s.noise(); s.rndTimer = c.sr / std::max (0.5f, rate); }
            s.rnd += (s.rndTarget - s.rnd) * (c.kind == K::Grain ? 0.01f : 0.0004f * rate * 10.0f);
            lfoL = s.rnd; lfoR = -s.rnd * 0.8f;
        }
        else
        {
            lfoL = std::sin (kTwoPi * s.ph[0]);
            lfoR = std::sin (kTwoPi * (s.ph[0] + phaseOff));
        }
        s.ph[0] += inc; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;

        const float dL = (centre + depth * lfoL) * sec, dR = (centre + depth * lfoR) * sec;
        float wl = s.readDelay (0, dL), wr = s.readDelay (1, dR);
        s.writeDelay (0, san (iL[i] + wl * fb));
        s.writeDelay (1, san (iR[i] + wr * fb));
        s.advance();

        if (c.kind == K::Tape)
        {
            wl = std::tanh (wl * tapeDrive) / tapeDrive; wr = std::tanh (wr * tapeDrive) / tapeDrive;
            s.lp[0][0] += lpC * (wl - s.lp[0][0]); s.lp[1][0] += lpC * (wr - s.lp[1][0]);
            wl = s.lp[0][0]; wr = s.lp[1][0];
        }
        if (c.kind == K::Rotary)
        {   // amplitude component of the horn
            wl *= 0.75f + 0.25f * lfoL; wr *= 0.75f + 0.25f * lfoR;
        }
        if (c.kind == K::Orbit)
        {
            const float d2 = (centre * 1.7f - depth * lfoL) * sec;
            wl = 0.6f * wl + 0.4f * s.readDelay (0, d2); wr = 0.6f * wr + 0.4f * s.readDelay (1, d2 * 0.93f);
        }
        oL[i] = wl * wet; oR[i] = wr * wet;
    }
}

// ---------------------------------------------------------------------------
// PHASER
void FxChain::Impl::runPhase (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    const bool deep = c.kind == Kind::DeepPhase;
    const int stages = deep ? 6 : 4;
    const float rate = lerpExp (0.05f, deep ? 1.2f : 2.5f, c.v.u0) * std::pow (2.0f, (c.mot - 0.5f) * 3.0f);
    const float fLo = lerpExp (150.0f, 500.0f, c.v.u1), fHi = lerpExp (1800.0f, 6000.0f, c.v.u2) * std::pow (2.0f, (c.tone - 0.5f) * 1.5f);
    const float fb = (deep ? 0.35f : 0.1f) + c.shp * (deep ? 0.55f : 0.45f);
    const float depth = 0.3f + 0.7f * c.amt;

    for (int i = 0; i < n; ++i)
    {
        const float x[2] = { iL[i], iR[i] }; float y[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            const float lfo = 0.5f + 0.5f * std::sin (kTwoPi * (s.ph[0] + (ch ? 0.18f : 0.0f)));
            const float f = fLo * std::pow (fHi / fLo, lfo * depth);
            const float t = std::tan (kPi * std::min (f, c.sr * 0.4f) / c.sr);
            const float a = (t - 1.0f) / (t + 1.0f);
            float v = x[ch] + s.lp[ch][3] * fb;
            for (int st = 0; st < stages; ++st)
            {
                const float out = a * v + s.apState[ch][st];
                s.apState[ch][st] = v - a * out;
                v = out;
            }
            s.lp[ch][3] = san (v);
            y[ch] = 0.5f * (x[ch] + v);
        }
        s.ph[0] += rate / c.sr; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;
        oL[i] = y[0]; oR[i] = y[1];
    }
}

// ---------------------------------------------------------------------------
// DELAYS, resonators, stutter
void FxChain::Impl::runDelay (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    float t0 = 90, t1 = 480, fbMax = 0.85f, lvl = 1.0f; bool ping = false;
    switch (c.kind)
    {
        case K::Delay: t0 = 90;  t1 = 480; break;
        case K::Echo:  t0 = 180; t1 = 495; fbMax = 0.8f; break;
        case K::Slap:  t0 = 30;  t1 = 125; fbMax = 0.35f; break;
        case K::Ping:  t0 = 140; t1 = 420; ping = true; break;
        case K::LongEcho: t0 = 330; t1 = 495; fbMax = 0.8f; break;
        case K::GhostDelay: t0 = 220; t1 = 460; fbMax = 0.86f; ping = true; break;
        default: break;
    }
    const float sec = 0.001f * c.sr;
    const float wobble = (c.mot - 0.5f) * 2.0f;
    const float lpC = onePole (lerpExp (900.0f, 16000.0f, c.tone), c.sr);

    // ---- stutter / beat repeat ------------------------------------------------
    if (c.kind == K::Stutter)
    {
        const std::size_t N = std::max<std::size_t> (64, std::min<std::size_t> (s.D - 8, (std::size_t) (lerpExp (35.0f, 260.0f, c.v.u0) * sec * std::pow (2.0f, (0.5f - c.mot) * 1.5f))));
        const std::size_t C = N * (std::size_t) (2 + (int) (c.shp * 3.0f));
        const std::size_t xf = std::max<std::size_t> (8, (std::size_t) (0.002f * c.sr));
        for (int i = 0; i < n; ++i)
        {
            float l = iL[i], r = iR[i];
            if (s.cycle < N) { s.mem[s.cycle] = l; s.mem[s.D + s.cycle] = r; }
            float ol = l, orr = r;
            if (s.cycle >= N)
            {
                const std::size_t rp = (s.cycle - N) % N;
                float g = 1.0f;
                if (rp < xf) g = (float) rp / (float) xf;
                else if (N - rp < xf) g = (float) (N - rp) / (float) xf;
                const float rl = s.mem[rp], rr = s.mem[s.D + rp];
                const float k = (0.4f + 0.6f * c.amt) * g;
                ol = l + (rl - l) * k; orr = r + (rr - r) * k;
            }
            if (++s.cycle >= C) s.cycle = 0;
            oL[i] = ol; oR[i] = orr;
        }
        return;
    }

    // ---- diffusion smear ------------------------------------------------------
    if (c.kind == K::Smear)
    {
        const float d1 = lerpExp (7.0f, 40.0f, c.v.u0) * sec, d2 = d1 * 1.61f;
        const float g = 0.45f + 0.35f * c.amt;
        for (int i = 0; i < n; ++i)
        {
            float in[2] = { iL[i], iR[i] }, out[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = in[ch] + s.lp[ch][3] * (0.45f + 0.4f * c.shp);
                const float a = s.readDelay (ch, d1);
                const float y1 = -g * x + a;
                s.writeDelay (ch, san (x + g * a));
                const float b = s.readDelay (ch, d2 + 0.0f);
                (void) b;
                s.lp[ch][0] += lpC * (y1 - s.lp[ch][0]);
                s.lp[ch][3] = san (s.lp[ch][0]);
                out[ch] = y1 * 0.6f + s.lp[ch][0] * 0.4f;
            }
            s.advance();
            oL[i] = out[0]; oR[i] = out[1];
        }
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        float lfo = 0.0f;
        if (std::abs (wobble) > 0.02f || c.kind == K::Echo)
        {
            lfo = std::sin (kTwoPi * s.ph[0]);
            s.ph[0] += 0.7f / c.sr; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;
        }
        float timeMs = lerpExp (t0, t1, c.v.u0);
        float fb = c.amt * fbMax;
        float dl, dr;
        const float wob = lfo * (c.kind == K::Echo ? 0.6f : 1.0f) * std::abs (wobble) * 2.5f * sec;

        if (c.kind == K::Pluck)
        {   // Karplus-Strong style resonator
            const float f0 = lerpExp (80.0f, 800.0f, c.v.u0);
            dl = dr = c.sr / f0;
            fb = 0.9f + 0.098f * c.amt;
        }
        else if (c.kind == K::Comb)
        {
            dl = lerpExp (0.6f, 14.0f, c.v.u0) * sec; dr = dl * (1.0f + 0.03f * c.v.u1);
            fb = (0.25f + 0.65f * c.amt) * (c.shp < 0.5f ? 1.0f : -1.0f);
        }
        else if (c.kind == K::Freeze)
        {
            dl = lerpExp (60.0f, 150.0f, c.v.u0) * sec; dr = dl * 1.07f;
            fb = 0.7f + 0.285f * c.amt;
        }
        else
        {
            dl = timeMs * sec * (1.0f + (c.shp - 0.5f) * 0.0f) + wob;
            dr = ping ? dl * (c.kind == K::GhostDelay ? 1.5f : 1.0f) : dl * (1.0f + 0.02f * c.v.u1);
        }
        fb = std::clamp (fb, -0.99f, 0.99f);

        float rl = s.readDelay (0, dl), rr = s.readDelay (1, dr);
        // damping in the loop
        s.lp[0][0] += lpC * (rl - s.lp[0][0]); s.lp[1][0] += lpC * (rr - s.lp[1][0]);
        float fl = s.lp[0][0], fr = s.lp[1][0];
        if (c.kind == K::Pluck || c.kind == K::Comb || c.kind == K::Freeze) { fl = 0.5f * (rl + fl); fr = 0.5f * (rr + fr); }

        float wl = iL[i], wr = iR[i];
        if (ping)
        {
            const float mono = 0.5f * (wl + wr);
            s.writeDelay (0, san (mono + fr * fb));
            s.writeDelay (1, san (fl * fb));
        }
        else
        {
            s.writeDelay (0, san (wl + fl * fb));
            s.writeDelay (1, san (wr + fr * fb));
        }
        s.advance();

        float ol = rl, orr = rr;
        if (c.kind == K::GhostDelay) { ol = 0.65f * rl + 0.35f * s.readDelay (0, dl * 0.5f); orr = 0.65f * rr + 0.35f * s.readDelay (1, dr * 0.5f); }
        if (c.kind == K::Pluck || c.kind == K::Comb) { ol = rl * 0.9f; orr = rr * 0.9f; }
        oL[i] = ol * lvl; oR[i] = orr * lvl;
        if (c.kind == K::Comb || c.kind == K::Pluck || c.kind == K::Freeze) { oL[i] = ol + iL[i] * 0.0f; oR[i] = orr; }
    }
}

// ---------------------------------------------------------------------------
// REVERBS
void FxChain::Impl::runVerb (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    float decayBase = 0.72f, decayRange = 0.26f;
    switch (c.kind)
    {
        case K::Room: decayBase = 0.6f; decayRange = 0.2f; break;
        case K::Hall: case K::DarkHall: decayBase = 0.78f; decayRange = 0.2f; break;
        case K::Stage: decayBase = 0.68f; decayRange = 0.22f; break;
        case K::Bloom: decayBase = 0.8f; decayRange = 0.18f; break;
        default: break;
    }
    const float g = std::min (0.985f, decayBase + decayRange * c.amt);
    float dampHz = lerpExp (1500.0f, 14000.0f, c.tone);
    if (c.kind == K::DarkHall) dampHz *= 0.35f;
    if (c.kind == K::BrightPlate || c.kind == K::Shimmer) dampHz = std::min (16000.0f, dampHz * 1.8f);
    const float damp = onePole (dampHz, c.sr);
    const float width = 0.4f + 0.6f * c.shp;
    const float bloomC = onePole (lerpExp (30.0f, 2.0f, c.v.u0), c.sr);

    if (c.kind == K::Choir)
    {
        const float pos = clamp01 (c.v.u0 * 0.6f + c.tone * 0.4f) * 4.0f;
        const int a = std::min (3, (int) pos); const float fr = pos - (float) a;
        for (int f = 0; f < 3; ++f) s.bq[f].set (Bq::BP, c.sr, lerp (vowelF (a, f), vowelF (a + 1, f), fr), 6.0f, 0);
    }

    for (int i = 0; i < n; ++i)
    {
        float in = 0.5f * (iL[i] + iR[i]);
        if (c.kind == K::Bloom) { s.lp[0][1] += bloomC * (in - s.lp[0][1]); in = s.lp[0][1] * 1.4f; }
        if (c.kind == K::Choir) { float v = 0; for (int f = 0; f < 3; ++f) v += s.bq[f].run (0, in) * (f == 0 ? 1.0f : 0.7f); in = in * 0.3f + v * 1.4f; }
        if (c.kind == K::Shimmer)
        {   // octave-up component (rectified, DC removed) fed into the tank
            const float rect = std::abs (in); s.lp[0][2] += 0.002f * (rect - s.lp[0][2]);
            in = in * 0.75f + (rect - s.lp[0][2]) * 0.5f;
        }
        float out[2] = { 0, 0 };
        for (int ch = 0; ch < 2; ++ch)
        {
            float sum = 0;
            for (int cb = 0; cb < 4; ++cb)
            {
                float* line = s.mem.data() + s.cbOff[ch][cb];
                const std::size_t len = s.cbLen[ch][cb]; std::size_t& pos = s.cbPos[ch][cb];
                const float d = line[pos];
                s.cbLp[ch][cb] += damp * (d - s.cbLp[ch][cb]);
                line[pos] = san (in * 0.28f + s.cbLp[ch][cb] * g);
                if (++pos >= len) pos = 0;
                sum += d;
            }
            for (int a = 0; a < 2; ++a)
            {
                float* line = s.mem.data() + s.apOff[ch][a];
                const std::size_t len = s.apLen[ch][a]; std::size_t& pos = s.apPos[ch][a];
                const float b = line[pos];
                const float y = -sum + b;
                line[pos] = san (sum + 0.5f * b);
                if (++pos >= len) pos = 0;
                sum = y;
            }
            out[ch] = sum;
        }
        const float m = 0.5f * (out[0] + out[1]), sd = 0.5f * (out[0] - out[1]);
        oL[i] = (m + sd * width * 2.0f) * 0.9f; oR[i] = (m - sd * width * 2.0f) * 0.9f;
    }
}

// ---------------------------------------------------------------------------
// DYNAMICS
void FxChain::Impl::runDyn (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    float atk = 10, rel = 120, ratio = 2.5f;
    switch (c.kind)
    {
        case K::Compressor: atk = 10; rel = 120; ratio = 2.0f + 4.0f * c.shp; break;
        case K::DrumGlue:   atk = 30; rel = 240; ratio = 3.0f; break;
        case K::TightComp:  atk = 3;  rel = 60;  ratio = 4.0f + 4.0f * c.shp; break;
        case K::OpenComp:   atk = 28; rel = 380; ratio = 1.6f + 1.2f * c.shp; break;
        case K::Limiter:    atk = 0.2f; rel = 70; ratio = 20.0f; break;
        case K::Duck:       atk = 5;  rel = 160; ratio = 6.0f; break;
        default: break;
    }
    atk *= lerp (0.35f, 2.2f, c.v.u1); rel *= lerp (0.4f, 2.4f, c.v.u2) * std::pow (2.0f, (0.5f - c.mot) * 1.5f);
    ratio *= lerp (0.7f, 1.5f, c.v.u3);
    const float thrDb = lerp (-4.0f, -34.0f, c.amt) + (c.v.u0 - 0.5f) * 18.0f;
    const float ac = 1.0f - std::exp (-1.0f / (atk * 0.001f * c.sr)), rc = 1.0f - std::exp (-1.0f / (rel * 0.001f * c.sr));
    const float makeup = dbToGain (-thrDb * (1.0f - 1.0f / ratio) * 0.5f * (0.4f + 0.6f * c.tone) * 2.0f);

    for (int i = 0; i < n; ++i)
    {
        const float lvl = std::max (std::abs (iL[i]), std::abs (iR[i]));
        float& e = s.env[0];
        e += (lvl > e ? ac : rc) * (lvl - e);
        float gain = 1.0f;
        if (c.kind == K::Duck)
            gain = 1.0f - c.amt * std::min (0.9f, e * 3.0f);
        else
        {
            const float db = 20.0f * std::log10 (e + 1.0e-7f);
            const float over = db - thrDb;
            if (over > 0.0f) gain = dbToGain (-over * (1.0f - 1.0f / ratio));
            gain *= (c.kind == K::Limiter ? 1.0f : makeup);
        }
        oL[i] = iL[i] * gain; oR[i] = iR[i] * gain;
    }
}

// ---------------------------------------------------------------------------
// AMPLITUDE MODULATION / GATES / PAN
void FxChain::Impl::runAmpMod (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    using K = Kind;
    float rLo = 1.5f, rHi = 12.0f;
    switch (c.kind)
    {
        case K::Tremolo:   rLo = 1.5f;  rHi = 12.0f; break;
        case K::Pulse:     rLo = 1.0f;  rHi = 9.0f;  break;
        case K::Gate16:    rLo = 3.0f;  rHi = 16.0f; break;
        case K::SleepGate: rLo = 0.3f;  rHi = 2.0f;  break;
        case K::AutoPan:   rLo = 0.15f; rHi = 5.0f;  break;
        case K::Fade:      rLo = 0.05f; rHi = 0.45f; break;
        default: break;
    }
    const float rate = lerpExp (rLo, rHi, c.v.u0) * std::pow (2.0f, (c.mot - 0.5f) * 3.0f);
    const std::uint32_t pattern = (std::uint32_t) (c.v.u2 * 65535.0f) | 0x0111u;
    const float smooth = onePole (lerpExp (60.0f, 2000.0f, c.tone), c.sr);

    for (int i = 0; i < n; ++i)
    {
        const float ph = s.ph[0];
        float gl = 1.0f, gr = 1.0f;
        switch (c.kind)
        {
            case K::Tremolo: { const float m = 0.5f - 0.5f * std::cos (kTwoPi * ph); gl = gr = 1.0f - c.amt * m; break; }
            case K::Pulse:   { const float m = 0.5f + 0.5f * std::tanh (4.0f * (1.0f + 4.0f * c.shp) * std::sin (kTwoPi * ph)) ; gl = gr = 1.0f - c.amt * m; break; }
            case K::Gate16:
            {
                const int step = (int) (ph * 16.0f) & 15;
                const float target = ((pattern >> step) & 1u) ? 1.0f : 1.0f - c.amt;
                s.lp[0][0] += smooth * (target - s.lp[0][0]); gl = gr = s.lp[0][0]; break;
            }
            case K::SleepGate:
            {
                const float target = ph < 0.5f ? 1.0f : 1.0f - 0.95f * c.amt;
                s.lp[0][0] += smooth * 0.2f * (target - s.lp[0][0]); gl = gr = s.lp[0][0]; break;
            }
            case K::AutoPan:
            {
                const float th = (0.5f + 0.5f * std::sin (kTwoPi * ph)) * 0.5f * kPi * 0.5f; // 0..pi/4 swing
                const float pos = 0.25f * kPi + (th - 0.125f * kPi) * 4.0f * c.amt;
                gl = std::cos (pos) * 1.4142f; gr = std::sin (pos) * 1.4142f; break;
            }
            case K::Fade: { const float m = 0.5f - 0.5f * std::cos (kTwoPi * ph); gl = gr = 1.0f - c.amt * (1.0f - m) * 0.95f; break; }
            default: break;
        }
        s.ph[0] += rate / c.sr; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;
        oL[i] = iL[i] * gl; oR[i] = iR[i] * gr;
    }
}

// ---------------------------------------------------------------------------
// STEREO / UTILITY / LO-FI / PITCH-ISH
void FxChain::Impl::runMisc (Slot& s, const Ctx& c, const float* iL, const float* iR, float* oL, float* oR, int n)
{
    switch (c.ki.fam)
    {
        case Fam::Stereo:
        {
            if (c.kind == Kind::Width)
            {
                const float w = 0.2f + 2.4f * c.amt * lerp (0.8f, 1.2f, c.v.u0);
                const float hp = onePole (lerpExp (60.0f, 400.0f, c.v.u1), c.sr);
                for (int i = 0; i < n; ++i)
                {
                    const float m = 0.5f * (iL[i] + iR[i]); float sd = 0.5f * (iL[i] - iR[i]);
                    s.lp[0][0] += hp * (sd - s.lp[0][0]);
                    const float sdHp = sd - s.lp[0][0] * (1.0f - c.tone) ; // tone keeps more low side
                    oL[i] = m + sdHp * w; oR[i] = m - sdHp * w;
                }
            }
            else // MonoBass
            {
                const float f = lerpExp (70.0f, 300.0f, c.v.u0) * std::pow (2.0f, (c.tone - 0.5f) * 1.5f);
                const float lc = onePole (f, c.sr);
                for (int i = 0; i < n; ++i)
                {
                    const float m = 0.5f * (iL[i] + iR[i]);
                    s.lp[0][0] += lc * (m - s.lp[0][0]);
                    const float lowL = s.lp[0][1] + lc * (iL[i] - s.lp[0][1]); s.lp[0][1] = lowL;
                    const float lowR = s.lp[1][1] + lc * (iR[i] - s.lp[1][1]); s.lp[1][1] = lowR;
                    const float monoLow = 0.5f * (lowL + lowR);
                    oL[i] = iL[i] - lowL + monoLow * (1.0f + 0.2f * c.shp);
                    oR[i] = iR[i] - lowR + monoLow * (1.0f + 0.2f * c.shp);
                }
            }
            break;
        }
        case Fam::Util:
        {
            const float g = dbToGain ((c.amt - 0.5f) * 24.0f);
            const float bal = (c.shp - 0.5f) * 2.0f;
            const float gl = g * (bal > 0 ? 1.0f - bal : 1.0f), gr = g * (bal < 0 ? 1.0f + bal : 1.0f);
            // each utility gain has its own character: a tilt and a touch of saturation
            const float tiltC = onePole (lerpExp (300.0f, 2500.0f, c.v.u0), c.sr);
            const float tilt = (c.v.u1 - 0.5f) * 1.2f, satK = 0.05f + 0.5f * c.v.u2 * c.v.u2;
            for (int i = 0; i < n; ++i)
            {
                float in[2] = { iL[i], iR[i] }, out[2];
                for (int ch = 0; ch < 2; ++ch)
                {
                    s.lp[ch][0] += tiltC * (in[ch] - s.lp[ch][0]);
                    float y = in[ch] + (s.lp[ch][0] - (in[ch] - s.lp[ch][0])) * tilt * 0.5f;
                    y = std::tanh (y * (1.0f + satK)) / (1.0f + satK * 0.8f);
                    out[ch] = y * (ch == 0 ? gl : gr);
                }
                oL[i] = out[0]; oR[i] = out[1];
            }
            break;
        }
        case Fam::Crush:
        {
            const float bits = lerp (15.0f, 2.5f, std::pow (c.amt, 0.8f)) + (c.v.u0 - 0.5f) * 1.5f;
            const float steps = std::pow (2.0f, std::max (1.5f, bits));
            const float hold = 1.0f + std::pow (c.amt, 2.0f) * (2.0f + 30.0f * c.v.u1) * (0.3f + c.mot);
            const float lc = onePole (lerpExp (1500.0f, 18000.0f, c.tone), c.sr);
            for (int i = 0; i < n; ++i)
            {
                s.cnt += 1.0f;
                if (s.cnt >= hold)
                {
                    s.cnt -= hold;
                    const float dith = s.noise() * c.shp * 0.5f / steps;
                    s.held[0] = std::round ((iL[i] + dith) * steps) / steps;
                    s.held[1] = std::round ((iR[i] + dith) * steps) / steps;
                }
                s.lp[0][0] += lc * (s.held[0] - s.lp[0][0]); s.lp[1][0] += lc * (s.held[1] - s.lp[1][0]);
                oL[i] = s.lp[0][0]; oR[i] = s.lp[1][0];
            }
            break;
        }
        case Fam::Ring:
        {
            const float f = lerpExp (35.0f, 2400.0f, c.v.u0) * std::pow (2.0f, (c.tone - 0.5f) * 3.0f);
            for (int i = 0; i < n; ++i)
            {
                const float car = std::sin (kTwoPi * s.ph[0]);
                s.ph[0] += f / c.sr; if (s.ph[0] >= 1.0f) s.ph[0] -= 1.0f;
                const float mixed = c.shp < 0.5f ? car : (car > 0 ? 1.0f : -1.0f);
                oL[i] = iL[i] * (1.0f - c.amt) + iL[i] * mixed * c.amt;
                oR[i] = iR[i] * (1.0f - c.amt) + iR[i] * mixed * c.amt;
            }
            break;
        }
        case Fam::OctDn:
        {   // analogue-style octave divider: flip-flop on rising zero-crossings of low-passed input
            const float lc = onePole (lerpExp (500.0f, 1400.0f, c.v.u0), c.sr);
            const float sm = onePole (lerpExp (400.0f, 1500.0f, c.tone), c.sr);
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (iL[i] + iR[i]);
                s.lp[0][0] += lc * (m - s.lp[0][0]);
                const float sign = s.lp[0][0] > 0.002f ? 1.0f : (s.lp[0][0] < -0.002f ? -1.0f : s.lastSign);
                if (sign > 0 && s.lastSign < 0) s.flip = -s.flip;
                s.lastSign = sign;
                const float sub = s.flip * std::abs (s.lp[0][0]) * (1.0f + 1.2f * c.amt);
                s.lp[0][1] += sm * (sub - s.lp[0][1]);
                oL[i] = iL[i] * (1.0f - 0.5f * c.amt) + s.lp[0][1] * 1.6f;
                oR[i] = iR[i] * (1.0f - 0.5f * c.amt) + s.lp[0][1] * 1.6f;
            }
            break;
        }
        case Fam::OctUp:
        {   // full-wave rectification octave-up fuzz
            const float hc = onePole (lerpExp (150.0f, 500.0f, c.v.u0), c.sr);
            const float lc = onePole (lerpExp (3000.0f, 12000.0f, c.tone), c.sr);
            for (int i = 0; i < n; ++i)
            {
                float in[2] = { iL[i], iR[i] }, out[2];
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float r = std::abs (in[ch]) * 2.0f;
                    s.lp[ch][0] += hc * (r - s.lp[ch][0]);
                    float y = r - s.lp[ch][0];
                    s.lp[ch][1] += lc * (y - s.lp[ch][1]);
                    out[ch] = in[ch] * (1.0f - 0.5f * c.amt) + std::tanh (s.lp[ch][1] * (1.0f + 2.0f * c.shp)) * c.amt;
                }
                oL[i] = out[0]; oR[i] = out[1];
            }
            break;
        }
        default: break;
    }
}

// ===========================================================================
FxChain::FxChain() : impl (new Impl) {}
FxChain::~FxChain() = default;
void FxChain::prepare (double sampleRate) { impl->prepare (sampleRate); }
void FxChain::reset() { impl->reset(); }
void FxChain::process (float* l, float* r, int n, const FxSlotParams* s, int ns) { impl->process (l, r, n, s, ns); }

int FxChain::categoryOf (int effect)
{
    if (effect < 0 || effect >= kNumEffects) return 6;
    return infoFor (kRecipes[effect]).cat;
}
const char* FxChain::categoryName (int c)
{
    static const char* n[] = { "Saturation", "Filter & EQ", "Modulation", "Delay & Resonator", "Reverb", "Dynamics", "Stereo & Utility", "Lo-fi & Pitch" };
    return n[std::clamp (c, 0, 7)];
}
} // namespace kyoto

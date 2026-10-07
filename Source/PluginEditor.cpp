#include "PluginEditor.h"
#include "DreamApi.h"
#include "FxCatalog.h"
#include <thread>
#include <array>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace
{
static juce::var propertyOr(const juce::DynamicObject* object,
                            const juce::Identifier& propertyName,
                            const juce::var& fallback)
{
    return object != nullptr && object->hasProperty(propertyName)
        ? object->getProperty(propertyName)
        : fallback;
}

class KyotoLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void setTheme(const kt::ThemePalette& t) { theme = t; }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return kt::font(theme, buttonHeight < 30 ? 12.5f : 13.5f, true);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return kt::font(theme, 13.0f);
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return kt::font(theme, 12.0f);
    }

    juce::Font getPopupMenuFont() override
    {
        return kt::font(theme, 13.0f);
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        auto r = juce::Rectangle<float>(0.5f, 0.5f, (float) width - 1.f, (float) height - 1.f);
        g.setColour(kt::c(theme.panel).brighter(0.04f));
        g.fillRoundedRectangle(r, 8.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(r, 8.f, 1.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.08f));
        g.fillRoundedRectangle(r.withSizeKeepingCentre(r.getWidth() * 0.96f, r.getHeight() * 0.96f), 6.f);
    }

    void drawPopupMenuItem(juce::Graphics& g, int width, int height, bool isSeparator, bool isActive,
                           bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKey)
    {
        auto r = juce::Rectangle<float>(1.f, 1.f, (float) width - 2.f, (float) height - 2.f);
        if (isSeparator)
        {
            g.setColour(kt::c(theme.border).withAlpha(0.5f));
            g.drawHorizontalLine((int) (r.getCentreY()), r.getX() + 8.f, r.getRight() - 8.f);
            return;
        }
        if (isHighlighted && isActive)
        {
            g.setColour(kt::c(theme.accent).withAlpha(0.22f));
            g.fillRoundedRectangle(r, 5.f);
            g.setColour(kt::c(theme.accent).withAlpha(0.6f));
            g.drawRoundedRectangle(r, 5.f, 1.f);
        }
        g.setColour(isActive ? kt::c(theme.text) : kt::c(theme.muted));
        g.setFont(kt::font(theme, 13.f));
        g.drawText(text, r.reduced(10.f, 0.f).toNearestInt(), juce::Justification::centredLeft);
        if (isTicked)
        {
            g.setColour(kt::c(theme.accent));
            g.fillEllipse(r.getRight() - 18.f, r.getCentreY() - 3.f, 6.f, 6.f);
        }
        if (hasSubMenu)
        {
            g.setColour(kt::c(theme.accent).withAlpha(0.7f));
            juce::Path arrow;
            arrow.startNewSubPath(r.getRight() - 14.f, r.getCentreY() - 4.f);
            arrow.lineTo(r.getRight() - 8.f, r.getCentreY());
            arrow.lineTo(r.getRight() - 14.f, r.getCentreY() + 4.f);
            g.strokePath(arrow, juce::PathStrokeType(1.5f));
        }
        juce::ignoreUnused(shortcutKey);
    }

    juce::Font getSliderPopupFont(juce::Slider&) override
    {
        return kt::font(theme, 12.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto r = button.getLocalBounds().toFloat().reduced(0.8f);
        auto fill = kt::c(theme.panel).brighter(isMouseOver ? 0.13f : 0.06f);
        if (button.getToggleState()) fill = kt::c(theme.accent).withAlpha(isButtonDown ? 0.38f : 0.25f);
        if (isButtonDown) fill = fill.brighter(0.10f);
        g.setColour(fill);
        g.fillRoundedRectangle(r, 8.f);
        g.setColour(kt::c(theme.border).withAlpha(0.95f));
        g.drawRoundedRectangle(r, 8.f, 1.f);
        if (button.getToggleState())
        {
            g.setColour(kt::c(theme.accent).withAlpha(0.75f));
            g.drawRoundedRectangle(r.reduced(2.f), 6.f, 1.f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, button.getHeight() < 30 ? 12.5f : 13.5f, true));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(6, 2), juce::Justification::centred, 1);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.f, (float)height - 1.f);
        g.setColour(kt::c(theme.panel).brighter(0.05f));
        g.fillRoundedRectangle(r, 7.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(r, 7.f, 1.f);
        // The ComboBox child Label draws the text; painting it here doubles it.
        juce::ignoreUnused(box);
        g.setColour(kt::c(theme.accent));
        juce::Path p;
        p.startNewSubPath((float)width - 17.f, height * 0.42f);
        p.lineTo((float)width - 11.f, height * 0.42f);
        p.lineTo((float)width - 14.f, height * 0.62f);
        p.closeSubPath();
        g.fillPath(p);
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        g.setColour(editor.hasKeyboardFocus(true) ? kt::c(theme.accent).withAlpha(0.9f) : kt::c(theme.border));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.f, (float)height - 1.f, 7.f, 1.f);
    }

private:
    kt::ThemePalette theme = kt::kThemes[0];
};

static KyotoLookAndFeel kLookAndFeel;

juce::String effectTokenFor(const juce::String& id)
{
    return "[KYOTRIPPAH_EFFECT:" + id + "]";
}

juce::String nativeKey(const juce::String& web)
{
    if (web == "amount") return "amt";
    if (web == "motion") return "mot";
    if (web == "shape") return "shp";
    return web;
}

void drawThemeSprite(juce::Graphics& g, const kt::ThemePalette& t, const juce::String& id, float x, float y, float scale, float phase)
{
    auto a = kt::c(t.accent).withAlpha(0.10f);
    g.setColour(a);
    auto S=[&](float v){return v*scale;};
    if (id=="trippah") { g.fillEllipse(x-S(10),y-S(22),S(20),S(30)); g.fillEllipse(x-S(30),y-S(8),S(24),S(14)); g.fillEllipse(x+S(6),y-S(8),S(24),S(14)); g.fillRect(x-S(3),y+S(8),S(6),S(22)); }
    else if (id=="goonr") { for(int i=0;i<5;i++){float yy=y+S((i-2)*12); g.drawLine(x-S(24),yy,x+S(24),yy,1.2f); g.drawLine(x+S((i-2)*9),y-S(24),x+S((i-2)*9),y+S(24),1.0f);} }
    else if (id=="abyss"||id=="cobalt") { g.drawEllipse(x-S(24),y-S(24),S(48),S(48),2.f); g.drawEllipse(x-S(9),y-S(9),S(18),S(18),2.f); g.fillEllipse(x+std::cos(phase)*S(28)-S(4),y+std::sin(phase)*S(28)-S(4),S(8),S(8)); }
    else if (id=="amber"||id=="honey") { for(int i=0;i<6;i++){float ang=i*1.047f+phase*.15f; g.drawLine(x,y,x+std::cos(ang)*S(28),y+std::sin(ang)*S(28),1.5f);} g.drawEllipse(x-S(10),y-S(10),S(20),S(20),2.f); }
    else if (id=="bloodmoon"||id=="wine") { g.drawEllipse(x-S(20),y-S(20),S(40),S(40),3.f); g.fillEllipse(x-S(7),y-S(24),S(28),S(28)); }
    else if (id=="ember"||id=="rust") { juce::Path p; p.startNewSubPath(x,y+S(24)); p.quadraticTo(x-S(28),y,x,y-S(28)); p.quadraticTo(x+S(22),y-S(4),x,y+S(24)); g.strokePath(p,juce::PathStrokeType(3.f)); }
    else if (id=="fog"||id=="lagoon") { for(int i=0;i<3;i++) g.drawEllipse(x-S(28-i*8),y+S(i*8),S(40),S(18),2.f); }
    else if (id=="graphite"||id=="steel") { for(int i=-2;i<=2;i++){g.drawLine(x-S(26),y+S(i*12),x+S(26),y+S(i*12),1.f);g.drawLine(x+S(i*12),y-S(26),x+S(i*12),y+S(26),1.f);} }
    else if (id=="ice") { juce::Path p; for(int i=0;i<6;i++){float ang=i*1.047f; auto q=juce::Point<float>(x+std::cos(ang)*S(25),y+std::sin(ang)*S(25)); if(i==0)p.startNewSubPath(q);else p.lineTo(q);} p.closeSubPath(); g.strokePath(p,juce::PathStrokeType(2.f)); }
    else if (id=="ink"||id=="void") { g.drawEllipse(x-S(22),y-S(14),S(44),S(28),2.5f); g.fillEllipse(x-S(6),y-S(6),S(12),S(12)); }
    else if (id=="lilac"||id=="plum") { for(int i=0;i<5;i++){float ang=i*1.256f+phase*.1f; g.fillEllipse(x+std::cos(ang)*S(18)-S(7),y+std::sin(ang)*S(18)-S(12),S(14),S(24));} }
    else if (id=="mint"||id=="pine") { g.drawLine(x-S(22),y+S(24),x+S(22),y-S(24),2.f); for(int i=0;i<5;i++){float yy=y+S(18-i*9);g.drawLine(x+S((i%2?-1:1)*4),yy,x+S((i%2?-1:1)*22),yy-S(10),1.5f);} }
    else if (id=="neon") { for(int i=1;i<=3;i++) g.drawEllipse(x-S(10*i),y-S(10*i),S(20*i),S(20*i),1.8f); }
    else { for(int i=0;i<8;i++){float ang=i*0.785f+phase*.08f; g.fillEllipse(x+std::cos(ang)*S(24)-S(3),y+std::sin(ang)*S(24)-S(3),S(6),S(6));} }
}
}

CanvasWidget::CanvasWidget(KyotoAudioProcessor& p, juce::ValueTree n)
    : proc(p), node(std::move(n))
{
    const auto k = node.getProperty("kind").toString();
    if (k == "slider") kind = Kind::Slider;
    else if (k == "key") kind = Kind::Key;
    else if (k == "wave") kind = Kind::Wave;
    else if (k == "stack") kind = Kind::Stack;
    else if (k == "board") kind = Kind::Board;
    else if (k == "cosmetic") kind = Kind::Cosmetic;
    else if (k == "button") kind = Kind::Button;
    else if (k == "sound") kind = Kind::Sound;
    else kind = Kind::Dial;

    caption.setText(node.getProperty("label").toString(), juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(kt::font(theme, 12.f, true));
    caption.setInterceptsMouseClicks(false, false);
    // These parts already paint their own captions.
    caption.setVisible(kind != Kind::Key && kind != Kind::Board && kind != Kind::Cosmetic);
    addChildComponent(caption);

    if (kind == Kind::Wave)
    {
        waveDisplay = std::make_unique<WaveDisplay>(proc);
        waveDisplay->setTheme(theme);
        addAndMakeVisible(*waveDisplay);
    }
    if (kind == Kind::Dial || kind == Kind::Slider)
    {
        int slot = (int) node.getProperty("slot", -1);
        // Satellite dials bound to a host effect use bindFx + bindSuffix.
        const int bindFx = (int) node.getProperty("bindFx", -1);
        auto key = node.getProperty("param").toString();
        const auto bindSuffix = node.getProperty("bindSuffix").toString();
        if (bindFx >= 0 && bindSuffix.isNotEmpty())
        {
            slot = bindFx;
            key = bindSuffix;
        }
        else if (key.isEmpty())
            key = "mix"; // default control on an effect dial
        auto id = slot < 0 ? key : ("s" + juce::String(slot + 1).paddedLeft('0', 2) + key);
        const auto style = node.getProperty("style").toString();
        slider.setSliderStyle(kind == Kind::Slider
                                   ? (style == "hfader" ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical)
                                   : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRange(0.0, 1.0, 0.001);
        slider.setTextValueSuffix(" %");
        // Plastic artwork is painted underneath; keep the native slider mostly invisible.
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(slider);
        if (proc.apvts.getParameter(id) != nullptr)
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, slider);
        slider.onDragStart = [this] { if (onSelect) onSelect(); };
        slider.onValueChange = [this] { repaint(); };
    }
}

void CanvasWidget::setTheme(const kt::ThemePalette& t)
{
    theme = t;
    caption.setFont(kt::font(theme, 12.f, true));
    caption.setColour(juce::Label::textColourId, kt::c(theme.text));
    slider.setColour(juce::Slider::rotarySliderFillColourId, kt::c(theme.accent));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, kt::c(theme.border));
    slider.setColour(juce::Slider::thumbColourId, kt::c(theme.accent));
    slider.setColour(juce::Slider::trackColourId, kt::c(theme.border));
    slider.setColour(juce::Slider::backgroundColourId, kt::c(theme.panel));
    // Modular piece skins override the theme accent for this part only.
    if (auto* skin = kt::partSkinById(node.getProperty("skin").toString()))
    {
        if (skin->accentHex != nullptr)
        {
            const auto accent = juce::Colour::fromString(skin->accentHex);
            slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
            slider.setColour(juce::Slider::thumbColourId, accent);
        }
    }
    if (waveDisplay) waveDisplay->setTheme(theme);
    repaint();
}

void CanvasWidget::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    const auto skinId = node.getProperty("skin").toString();
    const auto label = node.getProperty("label").toString();
    const int variant = kt::plastic::styleVariantFor(node);
    const float val = (float) slider.getValue();

    if (kind == Kind::Key)
    {
        kt::plastic::drawKey(g, bounds, theme, skinId, isMouseButtonDown(), selected, label);
    }
    else if (kind == Kind::Button)
    {
        const bool on = node.getProperty("on", false);
        kt::plastic::drawButton(g, bounds, theme, skinId, on, selected, label);
    }
    else if (kind == Kind::Board)
    {
        // Plastic bezel around live screen glass
        kt::plastic::fillPlasticBody(g, bounds, juce::jlimit(6.f, 14.f, theme.cornerRadius + 2.f),
                                     kt::plastic::colsFor(theme, skinId, kt::plastic::tierOf(bounds.getWidth(), bounds.getHeight())),
                                     selected);
        float live[128] {};
        proc.copyScope(live, 128);
        const int st = juce::jlimit(0, pb::kScreenTypeCount - 1, (int) node.getProperty("screenType", 0));
        // Cheap phase: quantize to reduce repaint noise when static
        const float phase = (float) ((juce::Time::getMillisecondCounter() / 32) % 4000) * 0.004f;
        pb::paintScreenFace(g, bounds.reduced(7.f), st, theme, live, 128, juce::String("SCREEN  -  ") + pb::kScreenTypes[st], phase);
    }
    else if (kind == Kind::Cosmetic)
    {
        kt::plastic::drawCosmetic(g, bounds, theme, skinId, node.getProperty("style").toString(), selected, label);
    }
    else if (kind == Kind::Wave || kind == Kind::Stack)
    {
        kt::plastic::drawWaveFrame(g, bounds, theme, skinId, selected, kind == Kind::Stack);
        // Lightweight waveform (32 segments, not 64) — scope only when visible size allows
        auto screen = bounds.reduced(10.f).withTrimmedBottom(16.f);
        if (screen.getHeight() > 12.f && screen.getWidth() > 20.f)
        {
            g.setColour(kt::c(theme.accent).withAlpha(0.85f));
            juce::Path wave;
            const float mid = screen.getCentreY();
            const float w = screen.getWidth();
            float live[64] {};
            const auto peaks = node.getProperty("peaks").toString();
            if (peaks.isEmpty())
                proc.copyScope(live, 64);
            const int segs = screen.getWidth() < 80.f ? 24 : 32;
            wave.startNewSubPath(screen.getX(), mid);
            for (int i = 0; i < segs; ++i)
            {
                float amp = 0.12f;
                if (peaks.isNotEmpty())
                {
                    auto parts = juce::StringArray::fromTokens(peaks, ",", {});
                    if (parts.size() > 0)
                        amp = parts[i % parts.size()].getFloatValue();
                }
                else
                    amp = std::abs(live[(i * 2) % 64]);
                wave.lineTo(screen.getX() + w * (float) i / (float) (segs - 1), mid - amp * screen.getHeight() * 0.35f);
            }
            g.strokePath(wave, juce::PathStrokeType(1.5f));
        }
    }
    else if (kind == Kind::Dial)
    {
        // Full plastic dial; JUCE slider sits transparent on top for interaction.
        kt::plastic::drawDial(g, bounds, theme, skinId, val, selected, variant);
    }
    else if (kind == Kind::Slider)
    {
        const bool horiz = node.getProperty("style").toString() == "hfader" || bounds.getWidth() > bounds.getHeight() * 1.25f;
        kt::plastic::drawFader(g, bounds, theme, skinId, val, selected, horiz);
    }
    else
    {
        kt::plastic::drawDial(g, bounds, theme, skinId, val, selected, variant);
    }

    // Resize grips only when selected/hovered (cheap)
    if (selected || hovered)
    {
        auto drawGrip = [&](float x, float y, float w, float h)
        {
            juce::Rectangle<float> grip(x, y, w, h);
            g.setColour(kt::c(theme.accent).withAlpha(selected ? 1.f : 0.7f));
            g.fillRoundedRectangle(grip, 2.f);
            g.setColour(kt::c(theme.bg).withAlpha(0.85f));
            g.fillRoundedRectangle(grip.reduced(2.f), 1.f);
        };
        drawGrip(1, 1, kHandlePx, kHandlePx);
        drawGrip(getWidth() - kHandlePx - 1, 1, kHandlePx, kHandlePx);
        drawGrip(getWidth() - kHandlePx - 1, getHeight() - kHandlePx - 1, kHandlePx, kHandlePx);
        drawGrip(1, getHeight() - kHandlePx - 1, kHandlePx, kHandlePx);
        if (getWidth() > 40 && getHeight() > 40)
        {
            const float ew = kHandlePx, eh = 3.f;
            drawGrip((float) getWidth() * 0.5f - ew * 0.5f, 1, ew, eh);
            drawGrip((float) getWidth() - kHandlePx, (float) getHeight() * 0.5f - eh * 0.5f, eh, ew);
            drawGrip((float) getWidth() * 0.5f - ew * 0.5f, (float) getHeight() - eh - 1, ew, eh);
            drawGrip(1, (float) getHeight() * 0.5f - eh * 0.5f, eh, ew);
        }
    }
}


void CanvasWidget::resized()
{
    caption.setBounds(0, 0, getWidth(), 14);
    if (kind == Kind::Slider)
    {
        const auto style = bf::sliderStyleFor(getLocalBounds());
        node.setProperty("style", style, nullptr);
        slider.setSliderStyle(style == "hfader" ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
    }
    if (kind == Kind::Dial || kind == Kind::Slider)
    {
        // Leave a frame around the control so click-drag on the plastic shell can MOVE the part.
        const int mx = juce::jmax(6, getWidth() / 7);
        const int my = juce::jmax(8, getHeight() / 6);
        slider.setBounds(mx, my, juce::jmax(8, getWidth() - 2 * mx), juce::jmax(8, getHeight() - 2 * my));
    }
    if (waveDisplay)
        waveDisplay->setBounds(4, 14, juce::jmax(8, getWidth() - 8), juce::jmax(8, getHeight() - 30));
}

// ---- Fine-grid geometry: a part owns gx/gy/gw/gh grid cells on the face plate --------------
juce::Rectangle<int> CanvasWidget::gridRect() const
{
    const auto face = faceProvider ? faceProvider() : juce::Rectangle<float>();
    if (face.getWidth() < 4.f || face.getHeight() < 4.f) return getBounds();
    const int gx = (int) node.getProperty("gx", 0);
    const int gy = (int) node.getProperty("gy", 0);
    const int gw = juce::jmax(1, (int) node.getProperty("gw", 1));
    const int gh = juce::jmax(1, (int) node.getProperty("gh", 1));
    const float cw = face.getWidth() / (float) gridCols;
    const float ch = face.getHeight() / (float) gridRows;
    return { (int) std::lround(face.getX() + gx * cw), (int) std::lround(face.getY() + gy * ch),
             juce::jmax(8, (int) std::lround(gw * cw)), juce::jmax(8, (int) std::lround(gh * ch)) };
}

void CanvasWidget::applyGrid(int gx, int gy, int gw, int gh)
{
    node.setProperty("gx", gx, nullptr);
    node.setProperty("gy", gy, nullptr);
    node.setProperty("gw", gw, nullptr);
    node.setProperty("gh", gh, nullptr);
    setBounds(gridRect());
    if (onGeometryChanged) onGeometryChanged(this);
    repaint();
}

int CanvasWidget::handleAt(juce::Point<int> pos) const
{
    const int w = getWidth(), h = getHeight();
    if (w < 18 || h < 18) return -1;
    const bool left = pos.x <= kGrabPx, right = pos.x >= w - kGrabPx;
    const bool top = pos.y <= kGrabPx, bottom = pos.y >= h - kGrabPx;
    // Corners (0-3)
    if (top && left) return 0;
    if (top && right) return 1;
    if (bottom && right) return 2;
    if (bottom && left) return 3;
    // Edges (4-7) - allow resizing by dragging sides, not just corners
    if (top) return 4;
    if (right) return 5;
    if (bottom) return 6;
    if (left) return 7;
    return -1;
}

void CanvasWidget::mouseMove(const juce::MouseEvent& e)
{
    if (! hovered) { hovered = true; repaint(); }
    const int h = handleAt(e.getPosition());
    setMouseCursor(h == 0 || h == 2 ? juce::MouseCursor::BottomRightCornerResizeCursor
                   : h == 1 || h == 3 ? juce::MouseCursor::BottomLeftCornerResizeCursor
                   : h == 4 || h == 6 ? juce::MouseCursor::UpDownResizeCursor
                   : h == 5 || h == 7 ? juce::MouseCursor::LeftRightResizeCursor
                                      : juce::MouseCursor::NormalCursor);
}

void CanvasWidget::mouseExit(const juce::MouseEvent&)
{
    if (resizing) return;
    if (hovered) { hovered = false; repaint(); }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void CanvasWidget::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (onRightClick) onRightClick(this, e);
        return;
    }
    if (onSelect) onSelect();
    resizeHandle = handleAt(e.getPosition());
    if (resizeHandle >= 0)
    {
        resizing = true;
        moving = false;
        dragStart = e.getPosition();
        startGx = (int) node.getProperty("gx", 0);
        startGy = (int) node.getProperty("gy", 0);
        startGw = juce::jmax(kMinCells, (int) node.getProperty("gw", 4));
        startGh = juce::jmax(kMinCells, (int) node.getProperty("gh", 4));
        return;
    }
    // Body drag moves the part on the free grid (screen, board, dials, etc.)
    moving = true;
    resizing = false;
    dragStart = e.getPosition();
    startGx = (int) node.getProperty("gx", 0);
    startGy = (int) node.getProperty("gy", 0);
    startGw = juce::jmax(kMinCells, (int) node.getProperty("gw", 4));
    startGh = juce::jmax(kMinCells, (int) node.getProperty("gh", 4));
    if (kind == Kind::Key)
    {
        const int note = (int) node.getProperty("note", 60);
        proc.noteOn(note, 0.9f);
        proc.triggerSample();
        repaint();
    }
    else if (kind == Kind::Button)
    {
        const int slot = (int) node.getProperty("slot", -1);
        if (slot >= 0)
        {
            const auto id = "s" + juce::String(slot + 1).paddedLeft('0', 2) + "on";
            if (auto* param = proc.apvts.getParameter(id))
            {
                const bool on = param->getValue() < 0.5f;
                param->setValueNotifyingHost(on ? 1.f : 0.f);
                node.setProperty("toggle", on, nullptr);
            }
        }
        repaint();
    }
    else if (kind == Kind::Sound)
        proc.triggerSample();
}

void CanvasWidget::mouseDrag(const juce::MouseEvent& e)
{
    if (! resizing && ! moving) return;
    const auto face = faceProvider ? faceProvider() : juce::Rectangle<float>();
    if (face.getWidth() < 4.f || face.getHeight() < 4.f) return;
    const float cw = face.getWidth() / (float) gridCols;
    const float ch = face.getHeight() / (float) gridRows;
    const int dx = (int) std::lround((e.getPosition().x - dragStart.x) / juce::jmax(1.f, cw));
    const int dy = (int) std::lround((e.getPosition().y - dragStart.y) / juce::jmax(1.f, ch));
    int gx = startGx, gy = startGy, gw = startGw, gh = startGh;
    if (moving && ! resizing)
    {
        gx = startGx + dx;
        gy = startGy + dy;
        gw = startGw;
        gh = startGh;
    }
    else
    {
        switch (resizeHandle)
        {
            case 0: gx = startGx + dx; gw = startGw - dx; gy = startGy + dy; gh = startGh - dy; break;
            case 1: gw = startGw + dx; gy = startGy + dy; gh = startGh - dy; break;
            case 2: gw = startGw + dx; gh = startGh + dy; break;
            case 3: gx = startGx + dx; gw = startGw - dx; gh = startGh + dy; break;
            case 4: gy = startGy + dy; gh = startGh - dy; break; // top edge
            case 5: gw = startGw + dx; break; // right
            case 6: gh = startGh + dy; break; // bottom
            case 7: gx = startGx + dx; gw = startGw - dx; break; // left
            default: break;
        }
    }
    gw = juce::jmax(kMinCells, gw);
    gh = juce::jmax(kMinCells, gh);
    gx = juce::jlimit(0, juce::jmax(0, gridCols - gw), gx);
    gy = juce::jlimit(0, juce::jmax(0, gridRows - gh), gy);
    if (gx + gw > gridCols) gx = juce::jmax(0, gridCols - gw);
    if (gy + gh > gridRows) gy = juce::jmax(0, gridRows - gh);
    applyGrid(gx, gy, gw, gh);
    if (onGeometryChanged) onGeometryChanged(this);
}

void CanvasWidget::mouseUp(const juce::MouseEvent&)
{
    const bool wasInteracting = resizing || moving;
    resizing = false;
    moving = false;
    resizeHandle = -1;
    if (kind == Kind::Key)
    {
        proc.noteOff((int) node.getProperty("note", 60));
        repaint();
        return;
    }
    if (wasInteracting)
        repaint();
}

FxBrowser::FxBrowser(KyotoAudioProcessor& p) : proc(p)
{
    list.setModel(this);
    list.setRowHeight(42);
    list.setOutlineThickness(0);
    addAndMakeVisible(list);
    addAndMakeVisible(search);
    addAndMakeVisible(builtInTab);
    addAndMakeVisible(customTab);
    addAndMakeVisible(familyBox);
    familyBox.addItem("All categories", 1);
    for (int i = 0; i < kt::kFxFamilyCount; ++i)
        familyBox.addItem(kt::kFxFamilyNames[i], i + 2);
    familyBox.setSelectedId(1);
    familyBox.onChange = [this] { rebuildFilter(); };
    search.setTextToShowWhenEmpty("Search effects...", juce::Colours::grey);
    search.onTextChange = [this] { rebuildFilter(); };
    builtInTab.onClick = [this] { showCustom(false); };
    customTab.onClick = [this] { showCustom(true); };
    showCustom(false);
}

void FxBrowser::setTheme(const kt::ThemePalette& t)
{
    theme = t;
    search.setColour(juce::TextEditor::backgroundColourId, kt::c(theme.bg).brighter(0.03f));
    search.setColour(juce::TextEditor::textColourId, kt::c(theme.text));
    search.setColour(juce::TextEditor::outlineColourId, kt::c(theme.border));
    search.setColour(juce::TextEditor::focusedOutlineColourId, kt::c(theme.accent).withAlpha(0.8f));
    search.setFont(kt::font(theme, 13.f));
    repaint();
    list.repaint();
}
void FxBrowser::setSelectedFx(int index) { if (index >= 0 && index < kt::kFxCount) { selected = index; showCustom(false); } }

void FxBrowser::setCustomItems(const juce::Array<CustomItem>& items)
{
    customItems = items;
    if (selectedCustom >= customItems.size()) selectedCustom = -1;
    rebuildFilter();
}

void FxBrowser::showCustom(bool custom)
{
    customMode = custom;
    builtInTab.setToggleState(!custom, juce::dontSendNotification);
    customTab.setToggleState(custom, juce::dontSendNotification);
    rebuildFilter();
    resized();
    repaint();
}

void FxBrowser::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(kt::c(theme.panel).withAlpha(0.97f));
    g.fillRoundedRectangle(r, 12.f);
    g.setColour(kt::c(theme.border));
    g.drawRoundedRectangle(r, 12.f, 1.f);
    g.setColour(kt::c(theme.accent));
    g.setFont(kt::font(theme, 13.f, true));
    g.drawText(customMode ? "CUSTOM EFFECTS" : "EFFECTS BY CATEGORY", 12, 8, getWidth()-24, 22, juce::Justification::left);
}

void FxBrowser::resized()
{
    auto a = getLocalBounds().reduced(8);
    a.removeFromTop(28);
    auto tabs = a.removeFromTop(30);
    builtInTab.setBounds(tabs.removeFromLeft((tabs.getWidth()-6)/2));
    tabs.removeFromLeft(6);
    customTab.setBounds(tabs);
    a.removeFromTop(6);
    if (! customMode)
    {
        familyBox.setBounds(a.removeFromTop(30));
        a.removeFromTop(6);
        familyBox.setVisible(true);
    }
    else
        familyBox.setVisible(false);
    search.setBounds(a.removeFromTop(30));
    a.removeFromTop(8);
    list.setBounds(a);
}

int FxBrowser::getNumRows() { return customMode ? customFiltered.size() : filtered.size(); }

void FxBrowser::rebuildFilter()
{
    filter = search.getText().toLowerCase().trim();
    filtered.clear();
    if (!customMode)
    {
        const int famSel = familyBox.getSelectedId() - 2; // -1 = all
        for (int i = 0; i < kt::kFxCount; ++i)
        {
            if (famSel >= 0 && kt::kFx[i].family != famSel) continue;
            const auto name = juce::String(kt::kFx[i].name);
            const auto cat = juce::String(kt::fxFamilyName(kt::kFx[i].family));
            if (filter.isEmpty() || name.toLowerCase().contains(filter) || cat.toLowerCase().contains(filter))
                filtered.add(i);
        }
    }
    else
    {
        customFiltered.clear();
        for (int i = 0; i < customItems.size(); ++i)
        {
            const auto& it = customItems[i];
            if (filter.isEmpty()
                || it.name.toLowerCase().contains(filter)
                || it.author.toLowerCase().contains(filter)
                || it.categories.toLowerCase().contains(filter))
                customFiltered.add(i);
        }
        // Sort custom effects by their derived categories string for stable grouping
        std::sort(customFiltered.begin(), customFiltered.end(), [this](int a, int b)
        {
            const int cmp = customItems[a].categories.compareIgnoreCase(customItems[b].categories);
            if (cmp != 0) return cmp < 0;
            return customItems[a].name.compareIgnoreCase(customItems[b].name) < 0;
        });
    }
    list.updateContent();
    list.repaint();
}

void FxBrowser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected)
{
    if (customMode)
    {
        if (row < 0 || row >= customFiltered.size()) return;
        const auto& item = customItems[customFiltered[row]];
        g.setColour(isSelected ? kt::c(theme.accent).withAlpha(0.30f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle(2.f, 2.f, (float)w-4.f, (float)h-4.f, 6.f);
        g.setColour(kt::c(theme.text));
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText(item.name, 10, 3, w-20, 18, juce::Justification::centredLeft);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.f));
        const auto meta = item.categories.isNotEmpty()
            ? item.categories
            : (item.remote ? "DreamShare  -  " + item.author : "My custom effect");
        g.drawText(meta, 10, 22, w-20, 15, juce::Justification::centredLeft);
        return;
    }
    if (row < 0 || row >= filtered.size()) return;
    const int fx = filtered[row];
    g.setColour(isSelected ? kt::c(theme.accent).withAlpha(0.30f) : juce::Colours::transparentBlack);
    g.fillRoundedRectangle(2.f, 2.f, (float)w-4.f, (float)h-4.f, 6.f);
    g.setColour(kt::c(theme.text));
    g.setFont(kt::font(theme, 13.f, true));
    g.drawText(kt::kFx[fx].name, 10, 3, w - 20, 18, juce::Justification::centredLeft);
    g.setColour(kt::c(theme.muted));
    g.setFont(kt::font(theme, 11.f));
    g.drawText(kt::fxFamilyName(kt::kFx[fx].family), 10, 22, w - 20, 15, juce::Justification::centredLeft);
}

void FxBrowser::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (customMode)
    {
        if (row >= 0 && row < customFiltered.size())
        {
            selectedCustom = customFiltered[row];
            if (onCustomSelect) onCustomSelect(customItems[selectedCustom]);
        }
        return;
    }
    if (row >= 0 && row < filtered.size())
    {
        selected = filtered[row];
        if (onSelect) onSelect(selected);
    }
}

void FxBrowser::selectedRowsChanged(int) {}

KyotoAudioProcessorEditor::KyotoAudioProcessorEditor(KyotoAudioProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    kLookAndFeel.setTheme(theme);
    socialRail.setHostTheme(theme);
    setLookAndFeel(&kLookAndFeel);
    setSize(1180, 760);
    setResizable(true, true);
    setResizeLimits(980, 620, 1900, 1150);

    for (auto* b : { &shareBtn, &chainBtn, &fxBtn, &logoutBtn, &loginBtn, &sendBtn, &feedBtn,
                     &addBtn, &saveBtn, &upBtn, &wavBtn, &chainBreakBtn, &chainMixBtn, &chainRemoveBtn, &chainUndoBtn,
                     &fxAddBtn, &fxSaveBtn, &fxUpBtn, &fxShareChatBtn, &fxShareThreadBtn, &fxRemoveBtn, &fxUndoBtn,
                     &fxBreakBtn, &fxMixBtn, &fxRandomBtn, &fxClearBtn, &pluginViewBtn, &pluginBackBtn, &newMachineBtn, &randomMachineBtn, &chatRefreshBtn, &threadsBtn, &socialBtn, &dmBtn, &adminDeleteBtn, &utilityGoBtn, &catalogModeBtn, &threadsModeBtn, &railChatBtn, &railDiscordBtn, &railOnlineBtn,
                     })
    {
        addAndMakeVisible(b);
        b->setClickingTogglesState(false);
    }

    shareBtn.onClick = [this] { showTab(0); };
    // No walkthrough any more: the Plugin Builder opens straight into the workshop on a
    // randomly generated template, and you learn the plugin by using it.
    chainBtn.onClick = [this] { showTab(1); };
    fxBtn.onClick = [this] { /* FX Builder page removed */ if (loggedIn) showTab(1); };
    pluginViewBtn.onClick = [this] { setPluginView(true); };
    pluginBackBtn.onClick = [this] { setPluginView(false); };
    newMachineBtn.onClick = [this] {
        juce::PopupMenu menu;
        menu.addItem(1, "4 : 5  -  PORTRAIT"); menu.addItem(2, "1 : 1  -  SQUARE"); menu.addItem(3, "5 : 4  -  LANDSCAPE"); menu.addItem(4, "FREEFORM");
        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(&newMachineBtn),
            [this](int choice)
            {
                if (choice >= 1 && choice <= 4)
                    startNewMachine(choice - 1);
            });
    };
    randomMachineBtn.onClick = [this] { randomizeMachine(); };
    randomTemplateBtn.onClick = [this] { randomizeTemplate(); };
    randomTemplateBtn.setTooltip("Roll modules into the template. Essential parts always land. Part count follows module size.");
    logoutBtn.onClick = [this] { logout(); };
    loginBtn.onClick = [this] { login(); };
    sendBtn.onClick = [this] { sendChat(); };
    feedBtn.onClick = [this] { if (railMode == 2) refreshDiscord(); else { refreshFeed(); refreshCatalog(); } };
    chatRefreshBtn.onClick = [this] { chatUtility("chat_list"); };
    threadsBtn.onClick = [this] { chatUtility("list_threads"); };
    socialBtn.onClick = [this] { chatUtility("social_list"); };
    dmBtn.onClick = [this] { chatUtility("dm_list"); };
    adminDeleteBtn.onClick = [this] { if (selectedCatalogId.isNotEmpty()) deleteCatalogId(selectedCatalogId); };
    utilityGoBtn.onClick = [this] {
        if (threadOpen)
        {
            postThreadComment(utilityBox.getText());
            return;
        }
    };

    addBtn.onClick = [this] { armPlacement(); };
    chainRemoveBtn.onClick = [this] { removeSelectedChainStep(); };
    chainUndoBtn.onClick = [this] { undoLast(); };
    chainBreakBtn.onClick = [this] { pendingSpecial = true; pendingSpecialType = KyotoAudioProcessor::kBreakType; pendingLabel = "CHAIN BREAK"; armedStyle = "dial"; placing = true; status.setText("Break is a knob part - click a glowing knob bay.", juce::dontSendNotification); panel.placing = true; panel.armedStyle = armedStyle; syncPanelMouse(); panel.repaint(); };
    chainMixBtn.onClick = [this] { pendingSpecial = true; pendingSpecialType = KyotoAudioProcessor::kMixType; pendingLabel = "MASTER MIX"; armedStyle = "fader"; placing = true; status.setText("Mix is a fader part - click a glowing fader bay.", juce::dontSendNotification); panel.placing = true; panel.armedStyle = armedStyle; syncPanelMouse(); panel.repaint(); };
    saveBtn.onClick = [this] { saveLocal(); if (token.isNotEmpty()) publish(); else status.setText("Saved locally - sign in to DreamShare to share it.", juce::dontSendNotification); };
    upBtn.onClick = [this] { if (token.isEmpty()) { status.setText("Sign in to DreamShare first to publish.", juce::dontSendNotification); return; } publish(); };
    wavBtn.onClick = [this] { loadWav(); };

    fxAddBtn.onClick = [this] {
        if (fxStack.getNumChildren() >= 16) { status.setText("FX Builder is full - save it as a custom effect.", juce::dontSendNotification); return; }
        captureSnapshot();
        const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
        auto step = juce::ValueTree("step");
        step.setProperty("fx", type, nullptr);
        step.setProperty("name", kt::kFx[juce::jlimit(0, kt::kFxCount - 1, type)].name, nullptr);
        step.setProperty("amount", (double) fxAmount.getValue(), nullptr);
        step.setProperty("tone", (double)fxTone.getValue(), nullptr);
        step.setProperty("motion", (double)fxMotion.getValue(), nullptr);
        step.setProperty("mix", (double)fxMix.getValue(), nullptr);
        step.setProperty("shape", (double)fxShape.getValue(), nullptr);
        fxStack.appendChild(step, nullptr);
        selectFxStep(fxStack.getNumChildren() - 1);
        stackLabel.setText("Stack " + juce::String(fxStack.getNumChildren()) + " / 16  -  custom effect lab", juce::dontSendNotification);
    };
    fxSaveBtn.onClick = [this] { saveEffect(); publishEffect(); };
    fxUpBtn.onClick = [this] { publishEffect(); };
    fxShareChatBtn.onClick = [this] { shareEffectToChat(); };
    fxShareThreadBtn.onClick = [this] { shareEffectToThread(); };
    fxRemoveBtn.onClick = [this] { removeSelectedFxStep(); };
    fxUndoBtn.onClick = [this] { undoLast(); };
    fxBreakBtn.onClick = [this] {
        if (fxStack.getNumChildren() >= 16) { status.setText("FX Builder is full - save it as a custom effect.", juce::dontSendNotification); return; }
        captureSnapshot();
        auto step = juce::ValueTree("step");
        step.setProperty("fx", KyotoAudioProcessor::kBreakType, nullptr);
        step.setProperty("name", "CHAIN BREAK", nullptr);
        step.setProperty("amount", 1.0, nullptr);
        step.setProperty("tone", 0.5, nullptr); step.setProperty("motion", 0.5, nullptr);
        step.setProperty("mix", 1.0, nullptr); step.setProperty("shape", 0.5, nullptr);
        fxStack.appendChild(step, nullptr); selectFxStep(fxStack.getNumChildren() - 1);
    };
    fxMixBtn.onClick = [this] {
        if (fxStack.getNumChildren() >= 16) { status.setText("FX Builder is full - save it as a custom effect.", juce::dontSendNotification); return; }
        captureSnapshot();
        auto step = juce::ValueTree("step");
        step.setProperty("fx", KyotoAudioProcessor::kMixType, nullptr);
        step.setProperty("name", "MASTER MIX", nullptr);
        step.setProperty("amount", 1.0, nullptr);
        step.setProperty("tone", 0.5, nullptr); step.setProperty("motion", 0.5, nullptr);
        step.setProperty("mix", 1.0, nullptr); step.setProperty("shape", 0.5, nullptr);
        fxStack.appendChild(step, nullptr); selectFxStep(fxStack.getNumChildren() - 1);
    };
    fxRandomBtn.onClick = [this] { captureSnapshot(); randomizeFxControls(); };
    fxClearBtn.onClick = [this] { if (fxStack.getNumChildren() > 0) { captureSnapshot(); fxStack.removeAllChildren(nullptr); selectedFxStep = -1; stackLabel.setText("Empty effect  -  ready for a new build", juce::dontSendNotification); } };

    addAndMakeVisible(status);
    status.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pluginViewBtn); addAndMakeVisible(pluginBackBtn); addAndMakeVisible(newMachineBtn); addAndMakeVisible(randomMachineBtn); addAndMakeVisible(randomTemplateBtn);
    pluginBackBtn.setVisible(false);
    addAndMakeVisible(whoLabel);
    whoLabel.setJustificationType(juce::Justification::centredRight);
    pluginViewBtn.toFront(false);
    addAndMakeVisible(userBox);
    addAndMakeVisible(passBox);
    addAndMakeVisible(msgBox);
    addAndMakeVisible(logBox);
    addAndMakeVisible(utilityBox);
    addAndMakeVisible(utilityActionBox); addAndMakeVisible(utilityGoBtn);
    addAndMakeVisible(chatRefreshBtn); addAndMakeVisible(threadsBtn); addAndMakeVisible(socialBtn); addAndMakeVisible(dmBtn); addAndMakeVisible(adminDeleteBtn);
    addAndMakeVisible(catalogModeBtn); addAndMakeVisible(threadsModeBtn); addAndMakeVisible(railChatBtn); addAndMakeVisible(railDiscordBtn); addAndMakeVisible(railOnlineBtn);
    addAndMakeVisible(threadBackBtn); addAndMakeVisible(threadReactBtn); addAndMakeVisible(threadShareFxBtn); addAndMakeVisible(threadSharePluginBtn);
    addAndMakeVisible(pluginsTabBtn); addAndMakeVisible(effectsTabBtn); addAndMakeVisible(myPluginsBtn); addAndMakeVisible(pendingBtn); addAndMakeVisible(sortBox);
    addAndMakeVisible(themeBox);
    addAndMakeVisible(textMinusBtn); addAndMakeVisible(textPlusBtn); addAndMakeVisible(attachChip);
    kt::loadDsScale();
    textMinusBtn.setTooltip("Smaller DreamShare text");
    textPlusBtn.setTooltip("Larger DreamShare text");
    textMinusBtn.onClick = [this] { kt::dsScale() = juce::jlimit(0.85f, 1.6f, kt::dsScale() - 0.08f); kt::saveDsScale(); applyDsScale(); status.setText("DreamShare text " + juce::String(juce::roundToInt(kt::dsScale() * 100.f)) + "%", juce::dontSendNotification); };
    textPlusBtn.onClick = [this] { kt::dsScale() = juce::jlimit(0.85f, 1.6f, kt::dsScale() + 0.08f); kt::saveDsScale(); applyDsScale(); status.setText("DreamShare text " + juce::String(juce::roundToInt(kt::dsScale() * 100.f)) + "%", juce::dontSendNotification); };
    attachChip.setTooltip("Click to remove the attached file");
    attachChip.onClick = [this] { clearAttachment(); status.setText("Attachment removed", juce::dontSendNotification); };
    addAndMakeVisible(catalogView);
    addChildComponent(threadBoard);
    wireThreadBoard();
    addAndMakeVisible(chatView);
    addAndMakeVisible(nameBox);
    addAndMakeVisible(effectNameBox);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(kindBox);
    addAndMakeVisible(effectBox);
    addAndMakeVisible(fxAmount); addAndMakeVisible(fxTone); addAndMakeVisible(fxMotion); addAndMakeVisible(fxMix); addAndMakeVisible(fxShape);
    addAndMakeVisible(fxAmountLabel); addAndMakeVisible(fxToneLabel); addAndMakeVisible(fxMotionLabel); addAndMakeVisible(fxMixLabel); addAndMakeVisible(fxShapeLabel);
    addAndMakeVisible(fxBreakBtn); addAndMakeVisible(fxMixBtn); addAndMakeVisible(fxRandomBtn); addAndMakeVisible(fxClearBtn);
    addAndMakeVisible(stackLabel); addAndMakeVisible(panel);
    addAndMakeVisible(chainLevels);
    panel.setInterceptsMouseClicks(false, true);

    // QOL: hover hints on the main controls.
    shareBtn.setTooltip("DreamShare: live chat, threads, catalog and uploads");
    chainBtn.setTooltip("Plugin Builder: lay out the chain, slots and widgets");
    fxBtn.setTooltip("FX Builder: stack up to 16 effects into one saved effect");
    loginBtn.setTooltip("Sign in (a new name creates an account)");
    logoutBtn.setTooltip("Log out of DreamShare");
    sendBtn.setTooltip("Sends to DreamShare chat. Optional: /d message for Discord-only. Kyoto Discord posts appear here with KYTO (Kyoto) or TRBN (TurboNerdos) tags via the bot bridge.");
    feedBtn.setTooltip("Refresh the feed");
    wavBtn.setTooltip("Load a WAV file onto the chain");
    saveBtn.setTooltip("Save this build locally");
    upBtn.setTooltip("Publish this build to the DreamShare catalog so others can load it");
    addBtn.setTooltip("Arm a part, then click a glowing bay. Select an effect first, PLACE a dial on it to bind Mix / Decay / Hz / Time.");
    chainUndoBtn.setTooltip("Undo the last builder step");
    fxAddBtn.setTooltip("Add the selected effect");
    fxRandomBtn.setTooltip("Randomize the effect controls");
    fxClearBtn.setTooltip("Clear all effect steps");
    pluginViewBtn.setTooltip("Show the built plugin full-screen (Esc to exit)");
    pluginBackBtn.setTooltip("Back to the editor");
    newMachineBtn.setTooltip("Pick a new machine aspect ratio");
    randomMachineBtn.setTooltip("Randomize the machine design");
    catalogModeBtn.setTooltip("Show the catalog card browser");
    threadsModeBtn.setTooltip("Show community threads");
    railChatBtn.setTooltip("Show the side chat");
    railDiscordBtn.setTooltip("Native Kyoto Discord #general (bot sees the channel; token stays on the worker)");
    railOnlineBtn.setTooltip("Show friends and who is online");

    userBox.setTextToShowWhenEmpty("Username", juce::Colours::grey);
    passBox.setTextToShowWhenEmpty("Password", juce::Colours::grey);
    passBox.setPasswordCharacter((juce::juce_wchar) 0x2022);
    nameBox.setTextToShowWhenEmpty("Chain name", juce::Colours::grey);
    effectNameBox.setTextToShowWhenEmpty("Custom effect name", juce::Colours::grey);
    msgBox.setTextToShowWhenEmpty("Message #dreamshare  -  drop files to attach", juce::Colours::grey);
    utilityBox.setTextToShowWhenEmpty("Target / ID / thread ID", juce::Colours::grey);

    // QOL: Enter sends the chat message, Enter on the login card signs in.
    msgBox.onReturnKey = [this] { sendChat(); };
    userBox.onReturnKey = [this] { if (! userBox.getText().trim().isEmpty() && ! passBox.getText().isEmpty()) login(); };
    passBox.onReturnKey = userBox.onReturnKey;
    const char* utilityItems[] = {
        "Chat list", "Threads", "Friends", "Direct messages", "Send DM",
        "Friend request", "Accept friend", "Decline friend", "Remove friend",
        "WAV request", "React: heart", "Delete chat", "Clear chat", "Presence",
        "Create thread", "Comment"
    };
    for (int i=0;i<16;++i) utilityActionBox.addItem(utilityItems[i], i+1);
    utilityActionBox.setSelectedId(1);
    logBox.setMultiLine(true); logBox.setReadOnly(true); logBox.setScrollbarsShown(true); logBox.setOpaque(true); logBox.setLineSpacing(2.0f);
    fxAmount.setRange(0.0, 1.0, 0.01); fxAmount.setValue(0.55); fxAmount.setSliderStyle(juce::Slider::LinearHorizontal); fxAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    auto setupFx = [](juce::Slider& s) { s.setRange(0.0, 1.0, 0.001); s.setValue(0.5); s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 16); };
    setupFx(fxTone); setupFx(fxMotion); setupFx(fxMix); setupFx(fxShape);
    auto setupLabel = [](juce::Label& l, const juce::String& text) { l.setText(text, juce::dontSendNotification); l.setJustificationType(juce::Justification::centred); };
    setupLabel(fxAmountLabel, "AMOUNT"); setupLabel(fxToneLabel, "TONE"); setupLabel(fxMotionLabel, "MOTION"); setupLabel(fxMixLabel, "MIX"); setupLabel(fxShapeLabel, "SHAPE");
    for (auto* s : { &fxTone, &fxMotion, &fxMix, &fxShape }) s->onValueChange = [this] { writeFxStepFromControls(); };
    fxAmount.onValueChange = [this] { writeFxStepFromControls(); };

    kindBox.addItem("Knob / Arc", 1); kindBox.addItem("Knob / Pointer", 2); kindBox.addItem("Fader", 3); kindBox.addItem("Slide", 4); kindBox.addItem("Button", 5); kindBox.addItem("Screen", 6); kindBox.addItem("Vent", 7); kindBox.addItem("Badge", 8); kindBox.addItem("Rail", 9); kindBox.setSelectedId(1);
    // Modular parts prototype: bind a placed dial/fader to any FX option, and
    // arm a modular piece (skin + special quirk) from the first batch.
    for (int i = 0; i < kt::kControlParamCount; ++i) paramBox.addItem(juce::String("Option: ") + kt::kControlParams[i].name, i + 1);
    paramBox.setSelectedId(1);
    pieceBox.addItem("Stock Part", 1);
    for (int i = 0; i < kt::kModPieceCount; ++i) pieceBox.addItem(juce::String(kt::kModPieces[i].name) + "  -  " + kt::kModPieces[i].quirk, i + 2);
    pieceBox.setSelectedId(1);
    addAndMakeVisible(paramBox); addAndMakeVisible(pieceBox);
    paramBox.setTooltip("FX option the placed control is bound to (Mix %, Tone, Motion...)");
    pieceBox.setTooltip("Modular pieces - each ships with a skin and a special quirk");
    shellLabel.setText("TEMPLATE", juce::dontSendNotification);
    playgroundThemeLabel.setText("THEME", juce::dontSendNotification);
    for (auto* l : { &shellLabel, &playgroundThemeLabel })
    {
        l->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(*l);
    }
    addAndMakeVisible(shellBox);
    for (int i = 0; i < pb::kShellCount; ++i) shellBox.addItem(pb::kShells[i].name, i + 1);
    shellBox.setSelectedId(1);
    shellBox.onChange = [this] { applyShell(shellBox.getSelectedId() - 1); };
    addAndMakeVisible(playgroundThemeBox);
    for (auto& t : kt::kThemes) playgroundThemeBox.addItem(t.name, playgroundThemeBox.getNumItems() + 1);
    playgroundThemeBox.setSelectedId(1);
    playgroundThemeBox.onChange = [this]
    {
        const int i = playgroundThemeBox.getSelectedId() - 1;
        if (i >= 0 && i < kt::kThemeCount)
            applyPlaygroundTheme(kt::kThemes[i].id);
    };
    panel.onSlot = [this](int slot) { placeInSlot(slot); };
    panel.occupied = [this](int slot) { return slotOccupied(slot); };
    panel.anchor = [this](int slot) { return slotAnchor(slot); };
    panel.theme = playgroundTheme;
    effectBox.setVisible(false);
    presetBox.onChange = [this] {
        auto display = presetBox.getText().trim();
        if (display.isEmpty()) return;
        auto name = display.fromFirstOccurrenceOf("-", false, false).trim();
        if (name.isEmpty()) name = display;
        loadCatalogId({}, name);
    };

    for (auto& t : kt::kThemes) themeBox.addItem(t.name, themeBox.getNumItems() + 1);
    themeBox.setSelectedId(1);
    themeBox.onChange = [this] { const int i = themeBox.getSelectedId()-1; if (i >= 0 && i < kt::kThemeCount) applyTheme(kt::kThemes[i].id); };

    fxBrowser = std::make_unique<FxBrowser>(proc);
    fxBrowser->onSelect = [this](int) { updateFxControls(); };
    fxBrowser->onCustomSelect = [this](const FxBrowser::CustomItem& item) { loadCatalogId(item.remote ? item.id : juce::String(), item.name); };
    addAndMakeVisible(*fxBrowser);
    updateFxControls();
    catalogView.setViewedComponent(&catalogHolder, false);
    chatView.setViewedComponent(&socialRail, false);
    chatView.setScrollBarsShown(true, false);
    catalogModeBtn.onClick = [this] { setCenterMode(0); };
    threadsModeBtn.onClick = [this] { setCenterMode(3); refreshFeed(); };
    railChatBtn.onClick = [this] { setRailMode(0); refreshFeed(); };
    railDiscordBtn.onClick = [this] { setRailMode(2); refreshDiscord(); };
    // SOCIALS takes over the centre: the Threads board is replaced by the full DreamShare directory.
    railOnlineBtn.onClick = [this] { setRailMode(1); setCenterMode(5); };
    threadBackBtn.onClick = [this] { closeThread(); };
    threadReactBtn.onClick = [this] { if (selectedThreadId.isNotEmpty()) reactTo("thread", selectedThreadId, "heart"); };
    threadShareFxBtn.onClick = [this] { const auto text = effectShareText(); if (text.isEmpty()) status.setText("Publish an effect before sharing it.", juce::dontSendNotification); else postThreadComment(text); };
    threadSharePluginBtn.onClick = [this] { const auto id = selectedCatalogId; if (id.isEmpty()) status.setText("Open a plugin from the catalog first.", juce::dontSendNotification); else postThreadComment("[KYOTRIPPAH_PLUGIN:" + id + "] " + nameBox.getText()); };
    socialRail.onBubbleMenu = [this](const SocialRail::Bubble& b, juce::Point<int> pos) { showBubbleMenu(b, pos); };
    socialRail.onPersonClick = [this](const SocialRail::Person& person) { openWavRequest(person.name); };
    socialRail.onPersonMenu = [this](const SocialRail::Person& person, juce::Point<int> pos) { showPersonMenu(person, pos); };
    socialRail.onFileClick = [this](const kt::AttachRef& ref) { saveAttachmentAs(ref); };
    applyDsScale();
    socialRail.setShowDirectory(isAdmin);
    directoryHolder.addAndMakeVisible(socialDirectory);
    socialDirectory.setHostTheme(theme);
    socialDirectory.onRowClick = [this](const SocialDirectory::Row& r) { openWavRequest(r.name); };
    socialDirectory.onRowMenu = [this](const SocialDirectory::Row& r, juce::Point<int> pos)
    {
        SocialRail::Person person;
        person.name = r.name;
        person.online = r.online;
        person.themeId = r.themeId;
        person.detail = r.detail;
        person.kind = r.isFriend ? "friend" : (r.online ? "active" : "user");
        showPersonMenu(person, pos);
    };
    pluginsTabBtn.onClick = [this] { setCenterMode(0); refreshCatalog(); };
    effectsTabBtn.onClick = [this] { /* Effects catalog removed */ setCenterMode(0); refreshCatalog(); };
    myPluginsBtn.onClick = [this] { setCenterMode(2); refreshMyModules(); };
    pendingBtn.onClick = [this] { setCenterMode(4); refreshPending(); };
    sortBox.clear(juce::dontSendNotification);
    sortBox.addItem("Sort: Newest", 1);
    sortBox.addItem("Sort: Oldest", 2);
    sortBox.addItem("Sort: Name A-Z", 3);
    sortBox.addItem("Sort: Name Z-A", 4);
    sortBox.addItem("Sort: Author", 5);
    sortBox.setSelectedId(1, juce::dontSendNotification);
    sortBox.setTooltip("Sort the plugin catalog");
    sortBox.onChange = [this] { if (loggedIn && centerMode == 0) refreshCatalog(); };
    catalogModeBtn.setClickingTogglesState(true);
    threadsModeBtn.setClickingTogglesState(true);
    railChatBtn.setClickingTogglesState(true);
    railDiscordBtn.setClickingTogglesState(true);
    railOnlineBtn.setClickingTogglesState(true);
    pluginsTabBtn.setClickingTogglesState(true);
    effectsTabBtn.setClickingTogglesState(true);
    myPluginsBtn.setClickingTogglesState(true);
    pendingBtn.setClickingTogglesState(true);

    auto session = juce::JSON::parse(sessionFile().loadFileAsString());
    if (auto* o = session.getDynamicObject())
    {
        token = o->getProperty("token").toString();
        account = o->getProperty("user").toString();
        isAdmin = (bool) o->getProperty("admin");
        if (token.isNotEmpty() && account.isNotEmpty()) setLoggedIn(true);
    }
    if (! loggedIn) setLoggedIn(false);
    if (proc.uiState.hasProperty("machineDesign")) machineDesign = MachineDesign::fromVar(juce::JSON::parse(proc.uiState.getProperty("machineDesign").toString()));
    else { machineDesign.choosePlayground((MachineDesign::PlaygroundMode) juce::jlimit(0, 3, (int)proc.uiState.getProperty("playgroundMode"))); machineDesign.theme = proc.uiState.getProperty("playgroundTheme", proc.uiState.getProperty("theme")).toString(); machineDesign.bodyDesign = proc.uiState.getProperty("bodyDesign").toString(); }
    applyTheme(proc.uiState.getProperty("theme", juce::var("trippah")).toString());
    playgroundTheme = theme;
    applyPlaygroundTheme(proc.uiState.getProperty("playgroundTheme", juce::var(theme.id)).toString());
    {
        const auto savedShell = proc.uiState.getProperty("shell").toString();
        for (int i = 0; i < pb::kShellCount; ++i)
            if (savedShell == pb::kShells[i].id) { shellIndex = i; shellBox.setSelectedId(i + 1, juce::dontSendNotification); }
    }
    refreshEffectBox();

    // ── Hidden HTML reader ──────────────────────────────────────────────
    // Scans the VST3 bundle for .html files.  If one is found the content is
    // shown in a full-screen overlay.  No button or menu ever exposes this.
    {
        auto htmlFile = ktFindHtmlInBundle();
        if (htmlFile.existsAsFile())
        {
            htmlOverlay = std::make_unique<HtmlOverlay>(theme);
            htmlOverlay->fileName = htmlFile.getFileName();
            htmlOverlay->setContent(htmlFile.loadFileAsString());
            addAndMakeVisible(*htmlOverlay);
            htmlOverlay->setBounds(getLocalBounds());
            htmlOverlay->toFront(true);
        }
    }

    // 30 Hz for smoother live screens and more reactive meters.
    startTimerHz(30);
    restoreEditorSession();
    // A brand-new instance opens on a randomly generated template, ready to work with.
    rollNewInstanceTemplate();
}

KyotoAudioProcessorEditor::~KyotoAudioProcessorEditor()
{
    // FL Studio often destroys the editor on close/minimize of the UI.
    // Stop all timers and detach views BEFORE members are destroyed.
    editorClosing = true;
    stopTimer();

    if (viewScreen != nullptr)
    {
        // Timer is a private base of PluginViewScreen — use public shutdownViewer().
        viewScreen->shutdownViewer();
        removeChildComponent(viewScreen);
        delete viewScreen;
        viewScreen = nullptr;
    }

    // Detach viewport content so Viewport does not touch dying components.
    catalogView.setViewedComponent(nullptr, false);
    chatView.setViewedComponent(nullptr, false);

    // Drop slider attachments before APVTS/processor relationships go away.
    widgets.clear();
    fxBrowser.reset();
    htmlOverlay.reset();
    feedEffectButtons.clear();

    try { persistEditorSession(); } catch (...) {}

    setLookAndFeel(nullptr);
}

void KyotoAudioProcessorEditor::setLoggedIn(bool on)
{
    loggedIn = on;
    whoLabel.setText(on ? account : "Sign in", juce::dontSendNotification);
    logoutBtn.setVisible(on);
    userBox.setVisible(! on);
    passBox.setVisible(! on);
    loginBtn.setVisible(! on);
    if (on)
    {
        userBox.clear();
        passBox.clear();
        setCenterMode(3);
        showTab(0);
        refreshFeed();
        refreshCatalog();
    }
    else
        showTab(0);
}

void KyotoAudioProcessorEditor::syncMachineDesignToUi()
{
    proc.uiState.setProperty("playgroundMode", (int) machineDesign.playgroundMode, nullptr);
    proc.uiState.setProperty("playgroundWidth", machineDesign.playgroundWidth, nullptr);
    proc.uiState.setProperty("playgroundHeight", machineDesign.playgroundHeight, nullptr);
    proc.uiState.setProperty("aspectRatio", machineDesign.aspectRatio, nullptr);
    proc.uiState.setProperty("bodyDesign", machineDesign.bodyDesign, nullptr);
    proc.uiState.setProperty("playgroundTheme", machineDesign.theme, nullptr);
    proc.uiState.setProperty("machineDesign", juce::JSON::toString(machineDesign.toVar()), nullptr);
}

void KyotoAudioProcessorEditor::startNewMachine(int mode)
{
    machineDesign.theme = playgroundTheme.id;
    machineDesign.choosePlayground((MachineDesign::PlaygroundMode) juce::jlimit(0, 3, mode));
    syncMachineDesignToUi();
    status.setText("Aspect ratio: " + machineDesign.aspectRatio + " playground", juce::dontSendNotification);
    repaint();
}

void KyotoAudioProcessorEditor::randomizeMachine()
{
    machineDesign.theme = playgroundTheme.id;
    juce::Random rng((juce::int64) juce::Time::getMillisecondCounterHiRes());
    machineDesign.randomize(rng);
    syncMachineDesignToUi();
    status.setText("Randomized " + machineDesign.bodyDesign + " - " + machineDesign.aspectRatio + " - collision checked", juce::dontSendNotification);
    repaint();
}

void KyotoAudioProcessorEditor::setPluginView(bool on)
{
    pluginView = on;
    pluginViewBtn.setVisible(!on && loggedIn);
    pluginBackBtn.setVisible(on);
    shareBtn.setVisible(!on); chainBtn.setVisible(!on); fxBtn.setVisible(false); logoutBtn.setVisible(!on); whoLabel.setVisible(!on);
    if (on)
    {
        showTab(1);
        panel.setVisible(false);
        if (fxBrowser) fxBrowser->setVisible(false);
        for (auto* w : widgets) w->setVisible(false);
    }
    else
    {
        showTab(tab);
        status.setVisible(true);
        for (auto* w : widgets) w->setVisible(true);
    }
    resized(); repaint();
}

void KyotoAudioProcessorEditor::timerCallback()
{
    if (editorClosing || ! isShowing())
        return;
    animPhase += 0.035f;
    if (animPhase > juce::MathConstants<float>::twoPi) animPhase -= juce::MathConstants<float>::twoPi;
    if (tab == 1)
        chainLevels.refresh();
    // DreamShare chat/socials are static — do NOT repaint them at 30Hz.
    for (auto* w : widgets)
        if (w != nullptr && (w->kind == CanvasWidget::Kind::Wave || w->kind == CanvasWidget::Kind::Stack || w->kind == CanvasWidget::Kind::Board))
            w->repaint();
    // Only repaint the editor itself for the theme-field hero animation on the builder tab.
    if (tab != 0)
        repaint();
}


namespace {
void drawThemeField(juce::Graphics& g, const kt::ThemePalette& t, juce::Rectangle<float> b, float phase)
{
    const juce::String id = t.id;
    auto accent = kt::c(t.accent);
    auto alt = kt::c(t.pegHot); // second accent; ThemePalette has no accent2 field
    auto ink = kt::c(t.text);
    const int seed = id.hashCode() & 0x7fffffff;
    const int kind = seed % 9;
    const int count = 6 + (seed % 5);
    g.setColour(accent.withAlpha(0.14f));
    if (id == "terminal" || id == "mono" || id == "carbon" || id == "ink")
    {
        for (int i = 0; i < 8; ++i)
        {
            const float y = b.getY() + std::fmod(i * 46.f + phase * 22.f, b.getHeight());
            g.drawLine(b.getX(), y, b.getRight(), y, 1.f);
        }
        g.setFont(kt::font(t, 9.f));
        g.drawText(id.toUpperCase() + "  " + juce::String((int)(phase * 40) % 99), b.getRight() - 120, b.getY() + 8, 108, 14, juce::Justification::right);
        return;
    }
    if (id == "arcade" || id == "neon" || id == "candy")
    {
        for (int i = 0; i < 10; ++i)
        {
            const float x = b.getX() + std::fmod(i * 90.f + phase * 36.f, b.getWidth());
            g.fillRect(x, b.getY() + 10 + (i % 3) * 18.f, 28.f, 3.f);
            g.setColour(alt.withAlpha(0.2f));
            g.drawEllipse(x, b.getBottom() - 40 - std::sin(phase + i) * 8.f, 16, 16, 1.4f);
            g.setColour(accent.withAlpha(0.16f));
        }
        return;
    }
    if (id == "sakura" || id == "rose" || id == "orchid" || id == "lilac" || id == "plum" || id == "wine")
    {
        for (int i = 0; i < count; ++i)
        {
            const float x = b.getX() + std::fmod(i * 78.f + phase * 14.f, b.getWidth());
            const float y = b.getY() + 20 + std::fmod(i * 33.f + phase * 10.f, b.getHeight() * 0.45f);
            g.fillEllipse(x, y, 7, 7);
            g.drawEllipse(x - 4, y - 4, 15, 15, 1.f);
        }
        return;
    }
    if (id == "ocean" || id == "lagoon" || id == "arctic" || id == "ice" || id == "fog" || id == "cobalt")
    {
        for (int i = 0; i < 4; ++i)
        {
            juce::Path wave;
            const float y = b.getY() + 24 + i * 16.f;
            wave.startNewSubPath(b.getX(), y);
            for (float x = 0; x < b.getWidth(); x += 12.f)
                wave.lineTo(b.getX() + x, y + std::sin(x * 0.03f + phase + i) * (6.f + i));
            g.strokePath(wave, juce::PathStrokeType(1.3f));
        }
        return;
    }
    if (id == "forest" || id == "pine" || id == "moss" || id == "mint")
    {
        for (int i = 0; i < count; ++i)
        {
            const float x = b.getX() + 20 + (i * 67 + seed % 20) % (int) juce::jmax(40.f, b.getWidth() - 30);
            const float h = 16.f + std::sin(phase + i) * 6.f;
            g.drawLine(x, b.getBottom() - 8, x, b.getBottom() - 8 - h, 1.4f);
            g.drawEllipse(x - 5, b.getBottom() - 12 - h, 10, 10, 1.f);
        }
        return;
    }
    if (id == "solar" || id == "amber" || id == "honey" || id == "sunset" || id == "ember" || id == "copper" || id == "rust")
    {
        const float cx = b.getRight() - 70, cy = b.getY() + 36;
        g.drawEllipse(cx - 12, cy - 12, 24, 24, 1.6f);
        for (int i = 0; i < 8; ++i)
        {
            const float a = phase * 0.6f + i * 0.785f;
            g.drawLine(cx, cy, cx + std::cos(a) * 28.f, cy + std::sin(a) * 28.f, 1.3f);
        }
        return;
    }
    if (id == "bloodmoon" || id == "void" || id == "abyss" || id == "midnight" || id == "ghost" || id == "ultraviolet" || id == "vapor")
    {
        const float cx = b.getX() + 48 + std::sin(phase) * 10.f, cy = b.getY() + 30;
        g.drawEllipse(cx, cy, 22, 22, 1.5f);
        g.fillEllipse(cx + 8, cy + 2, 16, 16);
        for (int i = 0; i < 5; ++i)
            g.fillEllipse(b.getRight() - 40 - i * 18.f, b.getY() + 12 + std::sin(phase + i) * 4.f, 2.5f, 2.5f);
        return;
    }
    if (id == "graphite" || id == "steel" || id == "default")
    {
        for (int i = 0; i < 5; ++i)
            g.drawRoundedRectangle(b.getX() + 16 + i * 34.f, b.getY() + 14, 22, 22, 4.f, 1.f);
        return;
    }
    // trippah / goonr and any remaining preset still get a signature orbit
    const float cx = b.getX() + 36, cy = b.getY() + 28;
    for (int i = 0; i < 3; ++i)
        g.drawEllipse(cx - 8 - i * 7, cy - 8 - i * 7 + std::sin(phase + i) * 2.f, 18 + i * 14.f, 18 + i * 14.f, 1.2f);
    juce::ignoreUnused(kind, ink);
}
}

namespace
{
// ---- DreamShare Discord-style chat layout (shared by paint, hit-testing and height) -----------
// Flat message rows: avatar circle + name/timestamp inline + body text + file chip.
constexpr float kMsgPad = 10.f;
constexpr float kMsgGap = 3.f;
constexpr float kAvatarSize = 32.f;
constexpr float kAvatarGap = 10.f;

struct BubbleLayout
{
    juce::String clean;
    kt::AttachRef ref;
    float textH = 0.f;
    float nameH = 0.f;
    float chipH = 0.f;
    float h = 0.f;
    float contentX = 0.f;
    float contentW = 0.f;
};

BubbleLayout layoutBubble(const SocialRail::Bubble& b, const kt::ThemePalette& pal, float width)
{
    BubbleLayout L;
    L.ref = kt::parseAttach(b.text, L.clean);
    const float s = kt::dsScale();
    const float avatar = kAvatarSize * s;
    L.contentX = avatar + kAvatarGap + kMsgPad;
    L.contentW = juce::jmax(40.f, width - L.contentX - kMsgPad);
    L.nameH = 15.f * s;
    if (L.clean.isNotEmpty())
    {
        juce::AttributedString as;
        as.append(L.clean, kt::dsFont(pal, 12.f), kt::c(pal.text));
        as.setWordWrap(juce::AttributedString::byWord);
        juce::TextLayout tl;
        tl.createLayout(as, L.contentW);
        L.textH = tl.getHeight() + 2.f;
    }
    L.chipH = L.ref.valid() ? 28.f * s : 0.f;
    // Extra height for image/gif link embeds in unified chat.
    float mediaH = 0.f;
    {
        const auto lower = L.clean.toLowerCase();
        if (lower.contains("http") && (lower.contains(".gif") || lower.contains(".png") || lower.contains(".jpg")
                || lower.contains(".jpeg") || lower.contains(".webp") || lower.contains("tenor.com")
                || lower.contains("giphy.com") || lower.contains("discordapp")))
            mediaH = 128.f;
    }
    const float bodyH = (L.textH > 0.f ? 2.f + L.textH : 0.f) + (L.chipH > 0.f ? 4.f + L.chipH : 0.f) + (mediaH > 0.f ? 4.f + mediaH : 0.f);
    const float minH = avatar + 4.f;
    L.h = juce::jmax(minH, L.nameH + bodyH) + 2.f * kMsgPad;
    return L;
}

juce::Rectangle<float> bubbleChipRect(float rowX, float rowY, float rowW, const BubbleLayout& L)
{
    return { rowX + L.contentX, rowY + L.h - kMsgPad - L.chipH, L.contentW, L.chipH };
}

void drawFileChip(juce::Graphics& g, const kt::ThemePalette& pal, juce::Rectangle<float> r, const kt::AttachRef& ref)
{
    const auto kind = kt::fileKindOf(ref.name);
    auto accentCol = kt::c(pal.accent);
    if (kind == "image") accentCol = juce::Colour(0xff4fa3ff);
    else if (kind == "zip") accentCol = juce::Colour(0xffe0a020);
    else if (kind == "audio") accentCol = juce::Colour(0xff40d090);
    g.setColour(accentCol.withAlpha(0.14f));
    g.fillRoundedRectangle(r, 6.f);
    g.setColour(accentCol.withAlpha(0.7f));
    g.drawRoundedRectangle(r, 6.f, 1.f);
    auto inner = r.reduced(8.f, 0.f);
    g.setColour(accentCol);
    g.setFont(kt::dsFont(pal, 9.f, true));
    g.drawText(kind.toUpperCase(), inner.removeFromLeft(38.f), juce::Justification::centredLeft);
    g.setColour(kt::c(pal.text));
    g.setFont(kt::dsFont(pal, 11.f, true));
    auto right = inner.removeFromRight(juce::jmin(120.f, inner.getWidth() * 0.45f));
    g.drawText(ref.name, inner, juce::Justification::centredLeft, true);
    g.setColour(kt::c(pal.muted));
    g.setFont(kt::dsFont(pal, 10.f));
    g.drawText(kt::humanBytes(ref.bytes) + "  -  SAVE", right, juce::Justification::centredRight, true);
}

void drawAvatar(juce::Graphics& g, const kt::ThemePalette& pal, const juce::String& name, juce::Rectangle<float> r)
{
    g.setColour(kt::c(pal.accent).withAlpha(0.25f));
    g.fillEllipse(r);
    g.setColour(kt::c(pal.accent));
    g.drawEllipse(r, 1.5f);
    g.setColour(kt::c(pal.text));
    g.setFont(kt::dsFont(pal, r.getWidth() * 0.42f, true));
    g.drawText(name.substring(0, 1).toUpperCase(), r, juce::Justification::centred, true);
}
}

int SocialRail::contentHeightFor(int width) const
{
    if (mode == 0)
    {
        float h = 6.f;
        const float w = (float) juce::jmax(220, width);
        for (const auto& m : bubbles)
            h += layoutBubble(m, kt::themeById(m.themeId.isEmpty() ? "trippah" : m.themeId), w).h + kMsgGap;
        return juce::jmax(140, (int) std::ceil(h) + 4);
    }
    int h = 12;
    auto count = [this](const juce::String& kind) {
        int n = 0; for (const auto& person : people) if (person.kind == kind) ++n; return n;
    };
    const int invites = count("invite");
    const int friends = count("friend");
    const int active = count("active");
    if (invites > 0) h += 22 + invites * 44;
    h += 22 + juce::jmax(1, friends) * 46;
    if (showDirectory) h += 22 + juce::jmax(1, active) * 40;
    return juce::jmax(180, h);
}

void SocialRail::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(host.bg).withAlpha(0.15f));
    if (mode == 0)
    {
        if (bubbles.isEmpty())
        {
            g.setColour(kt::c(host.muted));
            g.setFont(kt::dsFont(host, 12.f));
            g.drawFittedText("No messages yet.\nSay hi - or drop a file on this panel to share it.",
                             juce::Rectangle<int>(14, 18, getWidth() - 28, 60), juce::Justification::topLeft, 3);
            return;
        }
        float y = 4.f;
        const float w = (float) getWidth();
        for (const auto& m : bubbles)
        {
            const auto pal = kt::themeById(m.themeId.isEmpty() ? "trippah" : m.themeId);
            const auto L = layoutBubble(m, pal, w);
            const bool mine = selfUser.isNotEmpty() && m.user.equalsIgnoreCase(selfUser);
            const float s = kt::dsScale();
            const float avatar = kAvatarSize * s;

            // Discord-style flat row: subtle background, no rounded card.
            auto row = juce::Rectangle<float>(0.f, y, w, L.h);
            if (mine)
            {
                g.setColour(kt::c(pal.accent).withAlpha(0.06f));
                g.fillRect(row);
                g.setColour(kt::c(pal.accent).withAlpha(0.5f));
                g.fillRect(0.f, y + 2.f, 3.f, L.h - 4.f);
            }

            // Avatar circle.
            auto av = juce::Rectangle<float>(kMsgPad, y + kMsgPad, avatar, avatar);
            drawAvatar(g, pal, m.user, av);

            // Name + timestamp inline (Discord style).
            const auto nameText = mine ? m.user + " (you)" : m.user;
            const auto nameFont = kt::dsFont(pal, 11.5f, true);
            const float nameY = y + kMsgPad;
            const float nameW = (float) juce::GlyphArrangement::getStringWidthInt(nameFont, nameText);
            g.setColour(kt::c(pal.accent));
            g.setFont(nameFont);
            g.drawText(nameText, (int) L.contentX, (int) nameY, (int) nameW + 4, (int) L.nameH, juce::Justification::centredLeft, true);

            // Timestamp inline after name.
            g.setColour(kt::c(pal.muted).withAlpha(0.7f));
            g.setFont(kt::dsFont(pal, 9.5f));
            g.drawText("Today", (int) (L.contentX + nameW + 8.f), (int) nameY,
                       (int) juce::jmax(40.f, L.contentW - nameW - 8.f), (int) L.nameH, juce::Justification::centredLeft, true);

            // Community origin badge: KYTO (Kyoto) or TRBN (TurboNerdos).
            if (m.dis || m.originTag.isNotEmpty())
            {
                juce::String badge = m.originTag.toUpperCase();
                if (badge.isEmpty()) badge = "KYTO";
                if (badge != "TRBN" && badge != "KYTO") badge = "KYTO";
                juce::Rectangle<float> chip(L.contentX + nameW + 44.f, nameY + (L.nameH - 13.f) * 0.5f, 38.f, 13.f);
                if (chip.getRight() < w - 6.f)
                {
                    g.setColour(kt::c(pal.accent).withAlpha(0.18f));
                    g.fillRoundedRectangle(chip, 3.f);
                    g.setColour(kt::c(pal.accent));
                    g.setFont(kt::dsFont(pal, 8.f, true));
                    g.drawText(badge, chip, juce::Justification::centred);
                }
            }

            // Message body + media embeds (image / gif links).
            float ty = nameY + L.nameH + 2.f;
            if (L.clean.isNotEmpty())
            {
                juce::AttributedString as;
                as.append(L.clean, kt::dsFont(pal, 12.f), kt::c(pal.text));
                as.setWordWrap(juce::AttributedString::byWord);
                as.draw(g, juce::Rectangle<float>(L.contentX, ty, L.contentW, L.textH + 2.f));
                ty += L.textH + 4.f;
            }
            // Detect image/gif URLs and draw an embed card (actual pixels load via host / attachment when available).
            {
                const auto lower = L.clean.toLowerCase();
                const bool looksMedia = lower.contains("http") && (
                    lower.contains(".gif") || lower.contains(".png") || lower.contains(".jpg")
                    || lower.contains(".jpeg") || lower.contains(".webp") || lower.contains("tenor.com")
                    || lower.contains("giphy.com") || lower.contains("media.discordapp")
                    || lower.contains("cdn.discordapp"));
                if (looksMedia)
                {
                    auto media = juce::Rectangle<float>(L.contentX, ty, juce::jmin(L.contentW, 280.f), 120.f);
                    g.setColour(kt::c(pal.panel).brighter(0.08f));
                    g.fillRoundedRectangle(media, 8.f);
                    g.setColour(kt::c(pal.accent).withAlpha(0.55f));
                    g.drawRoundedRectangle(media, 8.f, 1.2f);
                    g.setColour(kt::c(pal.accent));
                    g.setFont(kt::dsFont(pal, 11.f, true));
                    g.drawText("MEDIA EMBED", media.reduced(10.f, 8.f).removeFromTop(16.f), juce::Justification::centredLeft);
                    g.setColour(kt::c(pal.muted));
                    g.setFont(kt::dsFont(pal, 10.f));
                    // Show shortened URL
                    juce::String url = L.clean;
                    for (const auto& token : juce::StringArray::fromTokens(L.clean, " 
	", {}))
                        if (token.startsWithIgnoreCase("http")) { url = token; break; }
                    g.drawFittedText(url, media.reduced(10.f, 28.f).toNearestInt(), juce::Justification::topLeft, 4);
                    ty += 128.f;
                }
            }
            if (L.chipH > 0.f) drawFileChip(g, pal, bubbleChipRect(0.f, y, w, L), L.ref);
            y += L.h + kMsgGap;
        }
        return;
    }
    // ---- Discord-style member sidebar ----
    auto drawHeader = [&](int& yy, const juce::String& title) {
        g.setColour(kt::c(host.muted));
        g.setFont(kt::dsFont(host, 10.f, true));
        g.drawText(title.toUpperCase(), 12, yy, getWidth() - 24, 14, juce::Justification::left);
        yy += 18;
    };
    auto drawPerson = [&](int& yy, const Person& person, int h) {
        const auto pal = kt::themeById(person.themeId.isEmpty() ? host.id : person.themeId);
        const float s = kt::dsScale();
        const float avSize = 28.f * s;
        auto avRect = juce::Rectangle<float>(12.f, (float) yy + 4.f, avSize, avSize);
        drawAvatar(g, pal, person.name, avRect);
        // Status dot.
        auto dot = juce::Rectangle<float>(avRect.getRight() - 8.f, avRect.getBottom() - 8.f, 9.f, 9.f);
        g.setColour(kt::c(pal.bg));
        g.fillEllipse(dot);
        g.setColour(person.online ? juce::Colour(0xff43b581) : juce::Colour(0xff747f8d));
        g.fillEllipse(dot.reduced(1.5f));
        g.setColour(person.online ? kt::c(pal.text) : kt::c(pal.muted));
        g.setFont(kt::dsFont(pal, 11.5f, true));
        g.drawText(person.name, (int) avRect.getRight() + 8, (int) yy + 4, getWidth() - (int) avRect.getRight() - 20, 16, juce::Justification::left);
        if (person.detail.isNotEmpty())
        {
            g.setColour(kt::c(pal.muted));
            g.setFont(kt::dsFont(pal, 9.f));
            g.drawText(person.detail, (int) avRect.getRight() + 8, (int) yy + 20, getWidth() - (int) avRect.getRight() - 20, 14, juce::Justification::left);
        }
        yy += h;
    };
    int yy = 8;
    bool any = false;
    drawHeader(yy, "Invites");
    for (const auto& person : people) if (person.kind == "invite") { drawPerson(yy, person, 44); any = true; }
    if (! any) { g.setColour(kt::c(host.muted)); g.setFont(kt::dsFont(host, 10.f)); g.drawText("No friend invites.", 12, yy, getWidth() - 24, 16, juce::Justification::left); yy += 22; }
    any = false;
    drawHeader(yy, "Friends");
    for (const auto& person : people) if (person.kind == "friend") { drawPerson(yy, person, 46); any = true; }
    if (! any) { g.setColour(kt::c(host.muted)); g.setFont(kt::dsFont(host, 10.f)); g.drawText("No friends yet. Right-click a name to add one.", 12, yy, getWidth() - 24, 16, juce::Justification::left); yy += 22; }
    any = false;
    drawHeader(yy, "All Active");
    for (const auto& person : people) if (person.kind == "active") { drawPerson(yy, person, 40); any = true; }
    if (! any) { g.setColour(kt::c(host.muted)); g.setFont(kt::dsFont(host, 10.f)); g.drawText("Nobody else is active.", 12, yy, getWidth() - 24, 16, juce::Justification::left); }
}

void SocialRail::mouseDown(const juce::MouseEvent& e)
{
    if (mode == 0)
    {
        float y = 4.f;
        const float w = (float) getWidth();
        for (const auto& m : bubbles)
        {
            const auto pal = kt::themeById(m.themeId.isEmpty() ? "trippah" : m.themeId);
            const auto L = layoutBubble(m, pal, w);
            auto row = juce::Rectangle<float>(0.f, y, w, L.h);
            if (row.contains(e.position))
            {
                if (e.mods.isPopupMenu()) { if (onBubbleMenu) onBubbleMenu(m, e.getScreenPosition()); }
                else if (L.chipH > 0.f && bubbleChipRect(0.f, y, w, L).contains(e.position) && onFileClick) onFileClick(L.ref);
                return;
            }
            y += L.h + kMsgGap;
        }
        return;
    }
    auto hitPerson = [this](int y, Person& out) {
        int row = 8;
        auto section = [&](const juce::String& kind, int h, bool always) {
            row += 20;
            int n = 0;
            for (const auto& person : people) if (person.kind == kind) ++n;
            if (n == 0) { row += always ? 22 : 0; return false; }
            for (const auto& person : people) if (person.kind == kind)
            {
                if (y >= row && y < row + h - 6) { out = person; return true; }
                row += h;
            }
            return false;
        };
        if (section("invite", 44, true)) return true;
        if (section("friend", 46, true)) return true;
        if (showDirectory && section("active", 40, true)) return true;
        return false;
    };
    Person person;
    if (! hitPerson(e.y, person)) return;
    if (e.mods.isPopupMenu()) { if (onPersonMenu) onPersonMenu(person, e.getScreenPosition()); }
    else if (onPersonClick) onPersonClick(person);
}

// ---- Socials directory: every DreamShare account, the ones active now pinned to the top ----
namespace
{
constexpr int kSdPad = 12;
constexpr int kSdHeaderH = 24;
constexpr int kSdLabelH = 26;
constexpr int kSdPersonH = 48;
constexpr int kSdVoiceH = 54;
}

void SocialDirectory::setData(const juce::Array<Row>& rows, int total, int active)
{
    items.clear();
    totalUsers = juce::jmax(0, total);
    activeUsers = juce::jmax(0, active);

    juce::Array<Row> onlineRows, friendRows;
    for (const auto& r : rows)
    {
        if (r.online || r.self) onlineRows.add(r);
        if (r.isFriend) friendRows.add(r);
    }

    int y = kSdPad;
    auto addHeader = [&](const juce::String& text) { Item it; it.type = ItemType::Header; it.text = text; it.y = y; it.h = kSdHeaderH; items.add(it); y += kSdHeaderH; };
    auto addLabel  = [&](const juce::String& text) { Item it; it.type = ItemType::Label;  it.text = text; it.y = y; it.h = kSdLabelH;  items.add(it); y += kSdLabelH; };
    auto addPerson = [&](const Row& r)              { Item it; it.type = ItemType::Person; it.row = r;     it.y = y; it.h = kSdPersonH; items.add(it); y += kSdPersonH; };

    addHeader("ONLINE NOW  -  " + juce::String(activeUsers) + " OF " + juce::String(totalUsers) + " ACCOUNTS");
    if (onlineRows.isEmpty()) addLabel("Nobody else is online right now.");
    else for (const auto& r : onlineRows) addPerson(r);

    addHeader("FRIENDS");
    if (friendRows.isEmpty()) addLabel("No friends yet - right-click a name to add one.");
    else for (const auto& r : friendRows) addPerson(r);

    addHeader("DREAMSHARE MEMBERS  -  " + juce::String(totalUsers));
    if (rows.isEmpty()) addLabel("No accounts yet.");
    else for (const auto& r : rows) addPerson(r);

    addHeader("VOICE");
    { Item it; it.type = ItemType::Discord; it.y = y; it.h = kSdVoiceH; items.add(it); y += kSdVoiceH; }

    totalHeight = juce::jmax(1, y + kSdPad);
    setSize(juce::jmax(1, getWidth()), totalHeight);
    repaint();
}

void SocialDirectory::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(host.bg).withAlpha(0.15f));
    for (const auto& it : items)
    {
        auto row = juce::Rectangle<int>(0, it.y, getWidth(), it.h);
        if (it.type == ItemType::Header)
        {
            g.setColour(kt::c(host.muted));
            g.setFont(kt::dsFont(host, 10.f, true));
            g.drawText(it.text.toUpperCase(), kSdPad, row.getY() + 5, getWidth() - 2 * kSdPad, 14, juce::Justification::centredLeft);
        }
        else if (it.type == ItemType::Label)
        {
            g.setColour(kt::c(host.muted));
            g.setFont(kt::dsFont(host, 10.f));
            g.drawText(it.text, kSdPad + 6, row.getY() + 3, getWidth() - 2 * kSdPad - 6, 16, juce::Justification::centredLeft, true);
        }
        else if (it.type == ItemType::Person)
        {
            const auto pal = kt::themeById(it.row.themeId.isEmpty() ? host.id : it.row.themeId);
            const float s = kt::dsScale();
            const float avSize = 30.f * s;
            auto avRect = juce::Rectangle<float>((float) kSdPad, (float) row.getY() + 6.f, avSize, avSize);
            // Avatar circle with initial.
            drawAvatar(g, pal, it.row.name, avRect);
            // Status dot (Discord-style: green=online, grey=offline).
            auto dot = juce::Rectangle<float>(avRect.getRight() - 8.f, avRect.getBottom() - 8.f, 10.f, 10.f);
            g.setColour(kt::c(pal.bg));
            g.fillEllipse(dot);
            g.setColour(it.row.online ? juce::Colour(0xff43b581) : juce::Colour(0xff747f8d));
            g.fillEllipse(dot.reduced(1.5f));
            // Self highlight: subtle accent background.
            if (it.row.self)
            {
                auto bg = row.reduced(2, 2).toFloat();
                g.setColour(kt::c(pal.accent).withAlpha(0.08f));
                g.fillRoundedRectangle(bg, 6.f);
            }
            const auto rowName = it.row.name + (it.row.self ? "  (you)" : "");
            const auto rowFont = kt::dsFont(pal, 12.f, true);
            g.setColour(it.row.online ? kt::c(pal.text) : kt::c(pal.muted));
            g.setFont(rowFont);
            g.drawText(rowName, (int) avRect.getRight() + 8, (int) row.getY() + 6, (int) (row.getWidth() - avRect.getRight() - 20), 16, juce::Justification::centredLeft, true);
            // Members online in Discord carry a DIS tag next to their name.
            if (it.row.dis)
            {
                juce::Rectangle<float> chip(avRect.getRight() + 8.f + (float) juce::GlyphArrangement::getStringWidthInt(rowFont, rowName) + 6.f, (float) row.getY() + 7.f, 28.f, 13.f);
                if (chip.getRight() < (float) row.getRight() - 6.f)
                {
                    g.setColour(kt::c(pal.accent).withAlpha(0.18f));
                    g.fillRoundedRectangle(chip, 3.f);
                    g.setColour(kt::c(pal.accent));
                    g.setFont(kt::dsFont(pal, 8.f, true));
                    g.drawText(m.originTag.isNotEmpty() ? m.originTag : juce::String("KYTO"), chip, juce::Justification::centred);
                }
            }
            g.setColour(kt::c(pal.muted));
            g.setFont(kt::dsFont(pal, 9.5f));
            g.drawText(it.row.detail, (int) avRect.getRight() + 8, (int) row.getY() + 22, (int) (row.getWidth() - avRect.getRight() - 20), 14, juce::Justification::centredLeft, true);
        }
        else if (it.type == ItemType::Discord)
        {
            auto card = row.reduced(kSdPad, 3).toFloat();
            g.setColour(kt::c(host.panel).brighter(0.04f));
            g.fillRoundedRectangle(card, 8.f);
            g.setColour(kt::c(host.border).withAlpha(0.8f));
            g.drawRoundedRectangle(card, 8.f, 1.f);
            g.setColour(kt::c(host.accent).withAlpha(0.8f));
            g.setFont(kt::dsFont(host, 11.f, true));
            g.drawText("DISCORD", (int) card.getX() + 12, (int) card.getY() + 6, (int) card.getWidth() - 24, 16, juce::Justification::centredLeft);
            g.setColour(kt::c(host.muted));
            g.setFont(kt::dsFont(host, 9.5f));
            g.drawText("Voice rooms - coming soon", (int) card.getX() + 12, (int) card.getY() + 24, (int) card.getWidth() - 24, 14, juce::Justification::centredLeft, true);
        }
    }
}

void SocialDirectory::mouseDown(const juce::MouseEvent& e)
{
    for (const auto& it : items)
    {
        if (it.type != ItemType::Person) continue;
        if (e.y < it.y || e.y >= it.y + it.h) continue;
        if (it.row.self) return; // your own card is not a WAV target
        if (e.mods.isPopupMenu()) { if (onRowMenu) onRowMenu(it.row, e.getScreenPosition()); }
        else if (onRowClick) onRowClick(it.row);
        return;
    }
}

struct BoardCard : public juce::Component
{
    juce::String title, meta, body, themeId, status, tags;
    kt::AttachRef attach;
    bool grow = false; // thread detail cards grow to fit their text; grid cards clip it
    std::function<void()> onOpen;
    std::function<void(juce::Point<int>)> onContextMenu;
    std::function<void(const kt::AttachRef&)> onFile;

    // Splits a raw message into readable text plus an optional file attachment.
    void setBodyRaw(const juce::String& raw)
    {
        juce::String clean;
        attach = kt::parseAttach(raw, clean);
        body = clean;
    }

    juce::AttributedString bodyText(const kt::ThemePalette& pal) const
    {
        juce::AttributedString as;
        as.append(body, kt::dsFont(pal, 12.f), kt::c(pal.text));
        as.setWordWrap(juce::AttributedString::byWord);
        return as;
    }

    float headerHeight() const
    {
        const float s = kt::dsScale();
        return 10.f + 20.f * s + 16.f * s + (tags.isNotEmpty() ? 15.f * s : 0.f);
    }

    int preferredHeight(int width) const
    {
        const auto pal = kt::themeById(themeId.isEmpty() ? "trippah" : themeId);
        float h = headerHeight();
        if (body.isNotEmpty())
        {
            juce::TextLayout tl;
            tl.createLayout(bodyText(pal), juce::jmax(40.f, (float) width - 28.f));
            h += 4.f + tl.getHeight();
        }
        if (attach.valid()) h += 8.f + 30.f;
        return juce::jmax(84, (int) std::ceil(h + 14.f));
    }

    juce::Rectangle<float> chipRect() const
    {
        return { 14.f, (float) getHeight() - 10.f - 30.f, (float) getWidth() - 28.f, 30.f };
    }

    void paint(juce::Graphics& g) override
    {
        const auto pal = kt::themeById(themeId.isEmpty() ? "trippah" : themeId);
        const float s = kt::dsScale();
        auto r = getLocalBounds().toFloat().reduced(1.f);
        g.setColour(kt::c(pal.panel));
        g.fillRoundedRectangle(r, 10.f);
        // Theme-aligned accent bar
        g.setColour(kt::c(pal.accent).withAlpha(0.9f));
        g.fillRoundedRectangle(r.getX(), r.getY(), 4.f, r.getHeight(), 2.f);
        g.setColour(kt::c(pal.accent).withAlpha(0.45f));
        g.drawRoundedRectangle(r, 10.f, 1.f);
        // Status badge
        if (status == "pending" || status == "denied")
        {
            const juce::String badge = status == "pending" ? "PENDING" : "DENIED";
            const juce::Colour badgeCol = status == "pending" ? juce::Colour(0xffe0a020) : juce::Colour(0xffcc3030);
            auto br = juce::Rectangle<float>(r.getRight() - 78.f * s, r.getY() + 7.f, 70.f * s, 16.f * s);
            g.setColour(badgeCol.withAlpha(0.9f));
            g.fillRoundedRectangle(br, 4.f);
            g.setColour(juce::Colours::black);
            g.setFont(juce::Font(9.f * s, juce::Font::bold));
            g.drawText(badge, br, juce::Justification::centred);
        }
        float y = 8.f;
        g.setColour(kt::c(pal.accent));
        g.setFont(kt::dsFont(pal, 13.f, true));
        g.drawText(title, 14, (int) y, getWidth() - (status.isNotEmpty() && status != "approved" ? (int) (96.f * s) : 28), (int) (20.f * s), juce::Justification::centredLeft, true);
        y += 20.f * s;
        g.setColour(kt::c(pal.muted));
        g.setFont(kt::dsFont(pal, 10.f));
        g.drawText(meta, 14, (int) y, getWidth() - 28, (int) (16.f * s), juce::Justification::centredLeft, true);
        y += 16.f * s;
        if (tags.isNotEmpty())
        {
            g.setColour(kt::c(pal.accent).withAlpha(0.8f));
            g.setFont(kt::dsFont(pal, 10.f));
            g.drawText("#" + tags.replace(",", " #"), 14, (int) y, getWidth() - 28, (int) (15.f * s), juce::Justification::centredLeft, true);
            y += 15.f * s;
        }
        if (body.isNotEmpty())
        {
            const float bottom = (float) getHeight() - (attach.valid() ? 48.f : 8.f);
            auto area = juce::Rectangle<float>(14.f, y + 4.f, (float) getWidth() - 28.f, juce::jmax(0.f, bottom - (y + 4.f)));
            g.saveState();
            g.reduceClipRegion(area.toNearestInt());
            bodyText(pal).draw(g, area.withHeight(2000.f));
            g.restoreState();
        }
        if (attach.valid()) drawFileChip(g, pal, chipRect(), attach);
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onContextMenu)
            onContextMenu(e.getScreenPosition());
        else if (attach.valid() && chipRect().contains(e.position) && onFile) onFile(attach);
        else if (onOpen && ! e.mouseWasDraggedSinceMouseDown()) onOpen();
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onContextMenu)
            onContextMenu(e.getScreenPosition());
    }
};

void KyotoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kt::c(theme.bg));
    drawThemeField(g, theme, getLocalBounds().toFloat().reduced(10.f), animPhase);
    for (int i=0;i<2;i++)
    {
        const float px = 90.f + std::fmod((float)i*280.f + animPhase*10.f, (float)juce::jmax(120,getWidth()-220));
        const float py = 168.f + std::fmod((float)i*140.f + std::sin(animPhase+i)*8.f, (float)juce::jmax(160,getHeight()-240));
        drawThemeSprite(g, theme, theme.id, px, py, 0.22f, animPhase + i);
    }

    g.setColour(kt::c(theme.panel).withAlpha(0.98f));
    g.fillRoundedRectangle(8.f, 8.f, (float)getWidth()-16.f, 42.f, 11.f);
    g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(8.5f, 8.5f, (float)getWidth()-17.f, 41.f, 11.f, 1.f);
    g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 17.f, true));
    g.drawText("DREAMSHARE", 20, 14, 160, 24, juce::Justification::left);

    if (!loggedIn)
    {
        auto card = getLocalBounds().withSizeKeepingCentre(420, 280).toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.97f)); g.fillRoundedRectangle(card, 16.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.85f)); g.fillRoundedRectangle(card.getX(), card.getY(), 4.f, card.getHeight(), 2.f);
        g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(card, 16.f, 1.f);
        g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 20.f, true)); g.drawText("ENTER THE ROOM", card.getX()+28, card.getY()+22, card.getWidth()-56, 28, juce::Justification::left);
        g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 12.f));
        g.drawFittedText("Saved presets and shared effects stay with your DreamShare account.", juce::Rectangle<int>((int)card.getX()+28, (int)card.getY()+54, (int)card.getWidth()-56, 36), juce::Justification::topLeft, 2);
        return;
    }

    if (tab == 0)
    {
        auto shell = getLocalBounds().withTrimmedTop(62).reduced(12).toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.97f));
        g.fillRoundedRectangle(shell, 15.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(shell, 15.f, 1.f);

        auto hero = shell.reduced(12.f);
        hero.setHeight(82.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.10f));
        g.fillRoundedRectangle(hero, 12.f);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 20.f, true));
        g.drawText("DREAMSHARE HOME", hero.getX()+18, hero.getY()+12, 300, 26, juce::Justification::left);
        g.setColour(kt::c(theme.text));
        g.setFont(kt::dsFont(theme, 13.f, false));
        g.drawFittedText("Chat on the left. Community plugins and threads on the right. React with emoji, post files, share plugins.", juce::Rectangle<float>(hero.getX()+18, hero.getY()+42, juce::jmax(120.f, hero.getWidth()-250.f), 36.f).toNearestInt(), juce::Justification::topLeft, 2);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::dsFont(theme, 12.f, true));
        g.drawText("LIVE  -  " + (account.isEmpty() ? juce::String("SIGNED IN") : account.toUpperCase()), hero.getRight()-220, hero.getY()+18, 200, 18, juce::Justification::right);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::dsFont(theme, 11.f));
        g.drawText("Theme: " + juce::String(theme.name) + "  -  " + juce::String(kt::kThemeCount) + " presets", hero.getRight()-220, hero.getY()+40, 200, 16, juce::Justification::right);

        g.setColour(kt::c(theme.border).withAlpha(0.55f));
        g.drawLine(shell.getX()+12.f, hero.getBottom()+10.f, shell.getRight()-12.f, hero.getBottom()+10.f, 1.f);

        auto chatCard = chatView.getBounds().toFloat().expanded(1.f);
        auto catalogCard = catalogView.getBounds().toFloat().expanded(1.f);
        g.setColour(kt::c(theme.accent).withAlpha(0.055f));
        g.fillRoundedRectangle(chatCard, 10.f);
        g.fillRoundedRectangle(catalogCard, 10.f);
        g.setColour(kt::c(theme.accent));
        g.setFont(kt::font(theme, 12.f, true));
        g.drawText(railMode == 0 ? "DREAM CHAT" : (railMode == 2 ? "DREAMSHARE CHAT" : "FRIENDS / ONLINE"), chatCard.getX()+12, chatCard.getY()+6, 220, 18, juce::Justification::left);
        juce::String centerLabel = centerMode == 0 ? "COMMUNITY PLUGINS" : centerMode == 2 ? "MY PLUGINS" : centerMode == 4 ? "PENDING APPROVAL" : centerMode == 5 ? "SOCIALS  -  DREAMSHARE DIRECTORY" : "THREADS";
        g.drawText(centerLabel, catalogCard.getX()+12, catalogCard.getY()+6, 200, 18, juce::Justification::left);
    }

    if (tab == 1)
    {
        // Toolbar background for the auto-adjusting builder bar.
        if (loggedIn)
        {
            auto tb = getLocalBounds().withTrimmedTop(52).reduced(12).removeFromTop(76);
            g.setColour(kt::c(theme.panel).withAlpha(0.55f));
            g.fillRoundedRectangle(tb.toFloat(), 10.f);
            g.setColour(kt::c(theme.border).withAlpha(0.4f));
            g.drawRoundedRectangle(tb.toFloat(), 10.f, 1.f);
        }

        // Always drive the live template preview from playground theme + selected shell.
        panel.theme = playgroundTheme;
        panel.shellIndex = shellIndex;
        panel.placing = placing;
        panel.armedStyle = armedStyle;
    }

    if (tab == 2)
    {
        // Toolbar background for the FX builder bar.
        if (loggedIn)
        {
            auto tb = getLocalBounds().withTrimmedTop(52).reduced(12).removeFromTop(76);
            g.setColour(kt::c(theme.panel).withAlpha(0.55f));
            g.fillRoundedRectangle(tb.toFloat(), 10.f);
            g.setColour(kt::c(theme.border).withAlpha(0.4f));
            g.drawRoundedRectangle(tb.toFloat(), 10.f, 1.f);
        }

        auto pb = panel.getBounds().toFloat();
        g.setColour(kt::c(theme.panel).withAlpha(0.96f)); g.fillRoundedRectangle(pb, 12.f); g.setColour(kt::c(theme.border)); g.drawRoundedRectangle(pb, 12.f, 1.f);
        g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 15.f, true)); g.drawText("CUSTOM EFFECT", pb.getX()+16, pb.getY()+12, 220, 20, juce::Justification::left);
        g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 12.f)); g.drawText("Build up to 16 stages. Categories auto-tag from the FX you stack.", pb.getX()+16, pb.getY()+34, pb.getWidth()-32, 18, juce::Justification::left);

        const int cols = juce::jmax(1, (int)(pb.getWidth() / 220.f));
        const float gap = 10.f, cw = (pb.getWidth()-gap*(cols+1))/cols, ch = 80.f;
        for (int i=0;i<fxStack.getNumChildren();++i)
        {
            auto st = fxStack.getChild(i); const int col=i%cols, row=i/cols;
            auto r = juce::Rectangle<float>(pb.getX()+gap+col*(cw+gap), pb.getY()+62.f+row*(ch+gap), cw, ch);
            const bool selected=i==selectedFxStep;
            g.setColour(selected ? kt::c(theme.accent).withAlpha(0.20f) : kt::c(theme.panel).brighter(0.06f)); g.fillRoundedRectangle(r, 9.f);
            g.setColour(kt::c(theme.border).withAlpha(selected?0.95f:0.75f)); g.drawRoundedRectangle(r, 9.f, selected?2.f:1.f);
            g.setColour(kt::c(theme.accent)); g.setFont(kt::font(theme, 12.f, true)); g.drawText(juce::String(i+1).paddedLeft('0',2), r.getX()+9, r.getY()+8, 28, 16, juce::Justification::left);
            g.setColour(kt::c(theme.text)); g.setFont(kt::font(theme, 13.f, true)); g.drawFittedText(st.getProperty("name").toString(), r.getX()+36, r.getY()+7, r.getWidth()-46, 18, juce::Justification::left, 1);
            g.setColour(kt::c(theme.muted)); g.setFont(kt::font(theme, 11.f));
            const int fxType = (int) st.getProperty("fx", 0);
            const juce::String fam = (fxType >= 0 && fxType < kt::kFxCount) ? kt::fxFamilyName(kt::kFx[fxType].family) : "FX";
            g.drawText(fam + "   A " + juce::String((double)st.getProperty("amount",0.5),2) + "   T " + juce::String((double)st.getProperty("tone",0.5),2), r.getX()+10, r.getBottom()-24, r.getWidth()-20, 16, juce::Justification::left);
        }
    }
}

bool KyotoAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (htmlOverlay && htmlOverlay->isVisible() && key == juce::KeyPress::escapeKey)
    {
        htmlOverlay->setVisible(false);
        return true;
    }
    if (pluginView && key == juce::KeyPress::escapeKey)
    {
        setPluginView(false);
        return true;
    }
    return false;
}

void KyotoAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (tab != 2) return;
    const auto pb = panel.getBounds().toFloat();
    const int cols = juce::jmax(1, (int)(pb.getWidth() / 220.f));
    const float gap = 10.f, cw = (pb.getWidth()-gap*(cols+1))/cols, ch = 80.f;
    for (int i=0;i<fxStack.getNumChildren();++i)
    {
        const int col=i%cols, row=i/cols;
        auto r=juce::Rectangle<float>(pb.getX()+gap+col*(cw+gap),pb.getY()+62.f+row*(ch+gap),cw,ch);
        if (r.contains(e.position)) { selectFxStep(i); return; }
    }
}

void KyotoAudioProcessorEditor::showTab(int next)
{
    next = juce::jlimit(0, 1, next); // FX Builder tab removed - fine-tune via right-click FX EDIT
    const bool openingChat = false;
    juce::ignoreUnused(openingChat);
    tab = loggedIn ? next : 0;
    const bool share = tab == 0;
    const bool chain = tab == 1;
    const bool fx = tab == 2;

    logBox.setVisible(false);
    utilityBox.setVisible(share && loggedIn && threadOpen);
    utilityActionBox.setVisible(false);
    utilityGoBtn.setVisible(share && loggedIn && threadOpen);
    threadBackBtn.setVisible(share && loggedIn && threadOpen);
    threadReactBtn.setVisible(share && loggedIn && threadOpen);
    threadShareFxBtn.setVisible(share && loggedIn && threadOpen);
    threadSharePluginBtn.setVisible(share && loggedIn && threadOpen);
    chatRefreshBtn.setVisible(false); threadsBtn.setVisible(false); socialBtn.setVisible(false); dmBtn.setVisible(false);
    catalogModeBtn.setVisible(false); threadsModeBtn.setVisible(share && loggedIn);
    pluginsTabBtn.setVisible(share && loggedIn); effectsTabBtn.setVisible(false); myPluginsBtn.setVisible(share && loggedIn);
    pendingBtn.setVisible(share && loggedIn && isAdmin);
    sortBox.setVisible(share && loggedIn && centerMode == 0);
    textMinusBtn.setVisible(share && loggedIn); textPlusBtn.setVisible(share && loggedIn);
    attachChip.setVisible(share && loggedIn && pendingAttach != juce::File() && ((pendingAttachTarget == 1 && (railMode == 0 || railMode == 2)) || (pendingAttachTarget == 2 && threadOpen)));
    railChatBtn.setVisible(share && loggedIn); railDiscordBtn.setVisible(share && loggedIn); railOnlineBtn.setVisible(share && loggedIn);
    adminDeleteBtn.setVisible(false); // replaced by right-click context menu
    msgBox.setVisible(share && loggedIn && (railMode == 0 || railMode == 2)); sendBtn.setVisible(share && loggedIn && (railMode == 0 || railMode == 2)); feedBtn.setVisible(share && loggedIn);
    themeBox.setVisible(share && loggedIn); // global UI theme only on DreamShare home
    // Templates, themes, and builder action buttons are now in the right-click context menu.
    shellBox.setVisible(false); playgroundThemeBox.setVisible(true);
    shellLabel.setVisible(false); playgroundThemeLabel.setVisible(true);
    catalogView.setVisible(share && loggedIn && centerMode != 3);
    threadBoard.setVisible(share && loggedIn && centerMode == 3);
    chatView.setVisible(share && loggedIn);
    for (auto* b : feedEffectButtons) b->setVisible(share && loggedIn);

    const bool builderReady = chain;
    // The live shell preview is the whole builder surface.
    panel.setVisible(chain);
    if (fxBrowser) fxBrowser->setVisible(builderReady || fx);
    chainLevels.setVisible(builderReady && ! pluginView);
    // Builder buttons moved to right-click context menu - keep only name/preset inputs.
    addBtn.setVisible(false); chainBreakBtn.setVisible(false); chainMixBtn.setVisible(false); chainRemoveBtn.setVisible(false); chainUndoBtn.setVisible(false); randomTemplateBtn.setVisible(false);
    nameBox.setVisible(builderReady); presetBox.setVisible(builderReady); saveBtn.setVisible(false); upBtn.setVisible(false); kindBox.setVisible(false); paramBox.setVisible(false); pieceBox.setVisible(false); wavBtn.setVisible(false);
    gridStyleBox.setVisible(false); effectBox.setVisible(false);
    newMachineBtn.setVisible(false); randomMachineBtn.setVisible(false);

    effectNameBox.setVisible(fx); fxAddBtn.setVisible(fx); fxSaveBtn.setVisible(fx); fxUpBtn.setVisible(false); fxShareChatBtn.setVisible(fx); fxShareThreadBtn.setVisible(fx); fxRemoveBtn.setVisible(fx); fxUndoBtn.setVisible(fx);
    fxAmount.setVisible(fx); fxTone.setVisible(fx); fxMotion.setVisible(fx); fxMix.setVisible(fx); fxShape.setVisible(fx);
    fxAmountLabel.setVisible(fx); fxToneLabel.setVisible(fx); fxMotionLabel.setVisible(fx); fxMixLabel.setVisible(fx); fxShapeLabel.setVisible(fx);
    fxBreakBtn.setVisible(fx); fxMixBtn.setVisible(fx); fxRandomBtn.setVisible(fx); fxClearBtn.setVisible(fx); stackLabel.setVisible(fx);

    shareBtn.setToggleState(share, juce::dontSendNotification); chainBtn.setToggleState(chain, juce::dontSendNotification); fxBtn.setVisible(false); fxBtn.setToggleState(false, juce::dontSendNotification);
    catalogModeBtn.setToggleState(centerMode == 0, juce::dontSendNotification); threadsModeBtn.setToggleState(centerMode == 3, juce::dontSendNotification);
    pluginsTabBtn.setToggleState(centerMode == 0, juce::dontSendNotification); effectsTabBtn.setToggleState(centerMode == 1, juce::dontSendNotification);
    myPluginsBtn.setToggleState(centerMode == 2, juce::dontSendNotification); pendingBtn.setToggleState(centerMode == 4, juce::dontSendNotification);
    railChatBtn.setToggleState(railMode == 0, juce::dontSendNotification); railDiscordBtn.setToggleState(railMode == 2, juce::dontSendNotification); railOnlineBtn.setToggleState(railMode == 1, juce::dontSendNotification);
    // Plugin View is now in the right-click menu - hide the top button.
    pluginViewBtn.setVisible(false); pluginBackBtn.setVisible(pluginView);
    if (! chain) { newMachineBtn.setVisible(false); randomMachineBtn.setVisible(false); }
    resized();
    if (openingChat) chatView.setViewPosition(0, socialRail.getHeight());
    if (chain) { ensureMotherboard(); rebuildCanvas(); }
    syncPanelMouse();
    repaint();
}

void KyotoAudioProcessorEditor::resized()
{
    const int W = getWidth();
    if (pluginView)
    {
        chainLevels.setVisible(false);
        pluginBackBtn.setBounds(24, 20, 104, 34);
        pluginViewBtn.setVisible(false); newMachineBtn.setVisible(false); randomMachineBtn.setVisible(false);
        shareBtn.setVisible(false); chainBtn.setVisible(false); fxBtn.setVisible(false); logoutBtn.setVisible(false); whoLabel.setVisible(false); status.setVisible(false);
        return;
    }
    const int navY = 8;
    const int rightPad = 18;
    const bool compactNav = W < 1100;
    const int logoutW = compactNav ? 80 : 100;
    const int whoW = compactNav ? 64 : 132;
    const int pluginW = compactNav ? 104 : 132;
    const int rightX = W - rightPad;
    logoutBtn.setBounds(rightX - logoutW, navY, logoutW, 32);
    whoLabel.setBounds(rightX - logoutW - whoW - 8, navY, whoW, 32);
    pluginViewBtn.setBounds(rightX - logoutW - whoW - pluginW - 16, navY, pluginW, 32);
    shareBtn.setBounds(190, navY, 96, 32);
    chainBtn.setBounds(292, navY, 150, 32);
    fxBtn.setVisible(false);
    fxBtn.setBounds(0, 0, 0, 0);
    const int statusX = 572;
    const int statusRight = pluginViewBtn.getX() - 12;
    status.setBounds(statusX, navY, juce::jmax(0, statusRight - statusX), 32);
    pluginViewBtn.toFront(false);
    whoLabel.toFront(false);
    logoutBtn.toFront(false);

    auto area = getLocalBounds().withTrimmedTop(52).reduced(12);
    if (! loggedIn)
    {
        auto box = getLocalBounds().withSizeKeepingCentre(360, 118).translated(0, 28);
        userBox.setBounds(box.removeFromTop(34)); box.removeFromTop(8); passBox.setBounds(box.removeFromTop(34)); box.removeFromTop(10); loginBtn.setBounds(box.removeFromTop(34));
        return;
    }

    if (tab == 0)
    {
        area.removeFromTop(88); // dashboard hero
        auto top = area.removeFromTop(36);
        textPlusBtn.setBounds(top.removeFromRight(34).reduced(0, 2)); top.removeFromRight(4);
        textMinusBtn.setBounds(top.removeFromRight(34).reduced(0, 2)); top.removeFromRight(8);
        themeBox.setBounds(top.removeFromLeft(150)); top.removeFromLeft(8);
        pluginsTabBtn.setBounds(top.removeFromLeft(84)); top.removeFromLeft(6);
        effectsTabBtn.setBounds(top.removeFromLeft(78)); top.removeFromLeft(6);
        myPluginsBtn.setBounds(top.removeFromLeft(92)); top.removeFromLeft(6);
        if (isAdmin && top.getWidth() > 84) { pendingBtn.setBounds(top.removeFromLeft(82)); top.removeFromLeft(6); }
        threadsModeBtn.setBounds(top.removeFromLeft(78)); top.removeFromLeft(6);
        feedBtn.setBounds(top.removeFromLeft(72));
        if (top.getWidth() > 116) { top.removeFromLeft(10); newMachineBtn.setBounds(top.removeFromLeft(106)); }
        if (top.getWidth() > 152) { top.removeFromLeft(8); randomMachineBtn.setBounds(top.removeFromLeft(136)); }
        // Tag search row
        if (centerMode == 0)
        {
            auto tagRow = area.removeFromTop(30);
            tagRow.removeFromTop(2);
            sortBox.setBounds(tagRow.removeFromLeft(juce::jmax(160, tagRow.getWidth() - 120)));
            tagRow.removeFromLeft(8);
        }
        // Chat on the LEFT, catalog on the RIGHT
        const int railW = juce::jlimit(260, 360, getWidth() / 4);
        auto rail = area.removeFromLeft(railW);
        area.removeFromLeft(10);
        if (threadOpen)
        {
            auto reply = area.removeFromBottom(34);
            if (attachChip.isVisible() && pendingAttachTarget == 2) attachChip.setBounds(area.removeFromBottom(30).reduced(0, 2));
            threadBackBtn.setBounds(reply.removeFromLeft(92)); reply.removeFromLeft(6);
            threadReactBtn.setBounds(reply.removeFromLeft(72)); reply.removeFromLeft(6);
            threadShareFxBtn.setBounds(reply.removeFromLeft(86)); reply.removeFromLeft(6);
            threadSharePluginBtn.setBounds(reply.removeFromLeft(110)); reply.removeFromLeft(6);
            utilityBox.setBounds(reply.removeFromLeft(juce::jmax(120, reply.getWidth() - 78)));
            reply.removeFromLeft(6);
            utilityGoBtn.setBounds(reply);
            utilityGoBtn.setButtonText("COMMENT");
        }
        catalogView.setBounds(area.reduced(0, 6));
        threadBoard.setBounds(catalogView.getBounds());
        auto railHead = rail.removeFromTop(28);
        {
            const int gap = 4;
            const int bw = juce::jmax(40, (railHead.getWidth() - gap * 2) / 3);
            railChatBtn.setBounds(railHead.removeFromLeft(bw));
            railHead.removeFromLeft(gap);
            railDiscordBtn.setBounds(railHead.removeFromLeft(bw));
            railHead.removeFromLeft(gap);
            railOnlineBtn.setBounds(railHead);
        }
        rail.removeFromTop(6);
        if (railMode == 0 || railMode == 2)
        {
            auto composer = rail.removeFromBottom(34);
            if (attachChip.isVisible() && pendingAttachTarget == 1) attachChip.setBounds(rail.removeFromBottom(30).reduced(0, 2));
            msgBox.setBounds(composer.removeFromLeft(juce::jmax(120, composer.getWidth() - 72)));
            composer.removeFromLeft(6);
            sendBtn.setBounds(composer);
            msgBox.setEnabled(true);
            sendBtn.setEnabled(true);
            msgBox.toFront(false);
            sendBtn.toFront(false);
        }
        else
        {
            // Socials: actions live on the name (click / right-click).
            msgBox.setBounds(0, 0, 0, 0);
            sendBtn.setBounds(0, 0, 0, 0);
        }
        chatView.setBounds(rail.reduced(0, 4));
        const int savedChatY = chatView.getViewPositionY();
        const int railInnerW = juce::jmax(200, chatView.getWidth() - 8);
        socialRail.setBounds(0, 0, railInnerW, socialRail.contentHeightFor(railInnerW));
        chatView.setViewedComponent(&socialRail, false);
        if (scrollChatOnRefresh && (railMode == 0 || railMode == 2))
            chatView.setViewPosition(0, socialRail.getHeight());
        else
            chatView.setViewPosition(0, savedChatY);
        layoutCenterHolder();
        for (int i=0;i<feedEffectButtons.size();++i)
        {
            const int bw = juce::jmin(240, chatView.getWidth()-16);
            feedEffectButtons[i]->setBounds(chatView.getX()+8, chatView.getY()+8+36*i, bw, 30);
        }
    }
    else if (tab == 1)
    {
        // --- Auto-adjusting toolbar: flex-distributed, fits any window width ---
        // Row 1: inline labels + dropdowns + action buttons
        {
            auto top = area.removeFromTop(34);
            flx::row(top, 5, {
                flx::Item { &shellLabel, 0.f, 58, true },
                flx::Item { &shellBox, 1.5f, 90 },
                flx::Item { &playgroundThemeLabel, 0.f, 48, true },
                flx::Item { &playgroundThemeBox, 1.5f, 90 },
                flx::Item { &nameBox, 1.2f, 70 },
                flx::Item { &kindBox, 1.3f, 80 },
                flx::Item { &paramBox, 1.3f, 80 },
                flx::Item { &pieceBox, 1.8f, 100 },
                flx::Item { &addBtn, 0.f, 54, true },
                flx::Item { &chainBreakBtn, 0.f, 54, true },
                flx::Item { &chainMixBtn, 0.f, 54, true },
                flx::Item { &chainRemoveBtn, 0.f, 60, true },
                flx::Item { &chainUndoBtn, 0.f, 50, true },
                flx::Item { &randomTemplateBtn, 0.f, 78, true },
            });
        }
        area.removeFromTop(4);
        // Row 2: presets, file ops, per-chain levels
        {
            auto actions = area.removeFromTop(34);
            flx::row(actions, 6, {
                flx::Item { &presetBox, 2.5f, 120 },
                flx::Item { &wavBtn, 0.f, 50, true },
                flx::Item { &saveBtn, 0.f, 54, true },
                flx::Item { &upBtn, 0.f, 62, true },
                flx::spacer(6),
                flx::Item { &chainLevels, 3.f, 100 },
            });
        }

        const int sideW = juce::jlimit(230, 320, area.getWidth() / 4);
        auto left = area.removeFromLeft(sideW);
        if (fxBrowser) fxBrowser->setBounds(left);
        area.removeFromLeft(8);
        panel.setBounds(area);
        reflowSeries();
    }
    else
    {
        // --- Auto-adjusting FX Builder toolbar: two flex rows ---
        // Row 1: name + build actions
        {
            auto top = area.removeFromTop(34);
            flx::row(top, 5, {
                flx::Item { &effectNameBox, 2.5f, 100 },
                flx::Item { &fxAddBtn, 0.f, 60, true },
                flx::Item { &fxBreakBtn, 0.f, 54, true },
                flx::Item { &fxMixBtn, 0.f, 78, true },
                flx::Item { &fxRemoveBtn, 0.f, 60, true },
                flx::Item { &fxUndoBtn, 0.f, 50, true },
            });
        }
        area.removeFromTop(4);
        // Row 2: save/publish/share/randomize/clear
        {
            auto top = area.removeFromTop(34);
            flx::row(top, 5, {
                flx::Item { &fxSaveBtn, 0.f, 54, true },
                flx::Item { &fxUpBtn, 0.f, 64, true },
                flx::Item { &fxShareChatBtn, 0.f, 46, true },
                flx::Item { &fxShareThreadBtn, 0.f, 54, true },
                flx::Item { &fxRandomBtn, 0.f, 60, true },
                flx::Item { &fxClearBtn, 0.f, 52, true },
                flx::spacer(4),
                flx::Item { &stackLabel, 2.f, 80 },
            });
        }
        // Proportional sidebar + inspector (shrink/grow with window)
        const int sideW = juce::jlimit(220, 320, area.getWidth() / 4);
        auto left = area.removeFromLeft(sideW);
        if (fxBrowser) fxBrowser->setBounds(left);
        area.removeFromLeft(8);
        auto inspector = area.removeFromRight(juce::jlimit(220, 300, area.getWidth() / 4));
        stackLabel.setBounds(area.removeFromTop(30));
        // Inspector: random/clear tools, then 5 knob columns (flex)
        auto tools = inspector.removeFromTop(40);
        flx::row(tools, 6, {
            flx::Item { &fxRandomBtn, 1.f, 70 },
            flx::spacer(4),
            flx::Item { &fxClearBtn, 1.f, 56 },
        });
        inspector.removeFromTop(6);
        auto labels = inspector.removeFromTop(18);
        flx::row(labels, 4, {
            flx::Item { &fxAmountLabel, 1.f, 40 },
            flx::Item { &fxToneLabel, 1.f, 40 },
            flx::Item { &fxMotionLabel, 1.f, 40 },
            flx::Item { &fxMixLabel, 1.f, 40 },
            flx::Item { &fxShapeLabel, 1.f, 40 },
        });
        auto knobs = inspector;
        flx::row(knobs, 4, {
            flx::Item { &fxAmount, 1.f, 44 },
            flx::Item { &fxTone, 1.f, 44 },
            flx::Item { &fxMotion, 1.f, 44 },
            flx::Item { &fxMix, 1.f, 44 },
            flx::Item { &fxShape, 1.f, 44 },
        });
        panel.setBounds(area);
    }

    if (htmlOverlay && htmlOverlay->isVisible())
    {
        htmlOverlay->setBounds(getLocalBounds());
        htmlOverlay->toFront(true);
    }
}

juce::Rectangle<int> KyotoAudioProcessorEditor::cellFor(int index, const juce::String& kind) const
{
    juce::Rectangle<int> r;
    if (! findAutoCell(index, kind, r)) return {};
    return r;
}

bool KyotoAudioProcessorEditor::findAutoCell(int index, const juce::String& kind, juce::Rectangle<int>& result) const
{
    int ww = 84, hh = 90;
    if (kind == "slider") { ww = 64; hh = 128; }
    else if (kind == "key") { ww = 66; hh = 66; }
    else if (kind == "wave") { ww = 190; hh = 94; }
    else if (kind == "stack") { ww = 156; hh = 86; }

    const int W = panel.getWidth(), H = panel.getHeight();
    const int margin = 14, gap = 14;
    if (W < ww + margin * 2 || H < hh + margin * 2) return false;
    const int cols = juce::jmax(1, (W - margin*2 + gap) / (ww + gap));
    const int rows = juce::jmax(1, (H - margin*2 + gap) / (hh + gap));
    const int cells = cols * rows;
    if (index >= cells && cells <= 1) return false;

    auto overlaps = [&](const juce::Rectangle<int>& candidate)
    {
        for (auto* w : widgets)
            if (candidate.expanded(gap/2, gap/2).intersects(w->getBounds())) return true;
        return false;
    };

    const int start = (index * 17 + 5) % juce::jmax(1, cells);
    for (int attempt = 0; attempt < cells; ++attempt)
    {
        const int cell = (start + attempt * 7) % cells;
        const int col = cell % cols, row = cell / cols;
        const int x = margin + col * (ww + gap) + ((cell * 13) % 7) - 3;
        const int y = margin + row * (hh + gap) + ((cell * 19) % 7) - 3;
        juce::Rectangle<int> candidate(x, y, ww, hh);
        if (candidate.getRight() <= W - margin && candidate.getBottom() <= H - margin && !overlaps(candidate))
        { result = candidate; return true; }
    }
    return false;
}

void KyotoAudioProcessorEditor::reflowSeries()
{
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    auto face = pb::faceRect(panel.getLocalBounds().toFloat());
    for (auto* w : widgets) w->setBounds({});
    for (auto* w : widgets)
    {
        const int slot = (int) w->node.getProperty("shellSlot", -1);
        if (slot < 0 || slot >= shell.slotCount) continue;
        auto fitted = bf::fittedSlot(face, shell.slots[slot], slot, shell.slotCount);
        auto r = fitted.toNearestInt();
        if (r.getWidth() < 12 || r.getHeight() < 12) continue;
        w->faceProvider = [this] { return pb::faceRect(panel.getLocalBounds().toFloat()); };
        w->gridCols = pb::kGridCols;
        w->gridRows = pb::kGridRows;
        w->onGeometryChanged = [this](CanvasWidget*) { panel.repaint(); };
        // The first layout snaps a part onto the grid; after that the stored grid cells win, so a
        // part the user resized by its corners keeps that size.
        if (! w->node.hasProperty("gx"))
        {
            const float cw = juce::jmax(1.f, face.getWidth() / (float) pb::kGridCols);
            const float ch = juce::jmax(1.f, face.getHeight() / (float) pb::kGridRows);
            const int gx = juce::jlimit(0, pb::kGridCols - 1, (int) std::lround((fitted.getX() - face.getX()) / cw));
            const int gy = juce::jlimit(0, pb::kGridRows - 1, (int) std::lround((fitted.getY() - face.getY()) / ch));
            const int gw = juce::jlimit(CanvasWidget::kMinCells, juce::jmax(CanvasWidget::kMinCells, pb::kGridCols - gx), (int) std::lround(fitted.getWidth() / cw));
            const int gh = juce::jlimit(CanvasWidget::kMinCells, juce::jmax(CanvasWidget::kMinCells, pb::kGridRows - gy), (int) std::lround(fitted.getHeight() / ch));
            w->node.setProperty("gx", gx, nullptr);
            w->node.setProperty("gy", gy, nullptr);
            w->node.setProperty("gw", gw, nullptr);
            w->node.setProperty("gh", gh, nullptr);
        }
        w->setBounds(w->gridRect());
        const int cap = bf::partCapacity(fitted);
        w->node.setProperty("partSlots", cap, nullptr);
        w->node.setProperty("x", w->getX(), nullptr);
        w->node.setProperty("y", w->getY(), nullptr);
        w->node.setProperty("w", w->getWidth(), nullptr);
        w->node.setProperty("h", w->getHeight(), nullptr);
        if (w->kind == CanvasWidget::Kind::Slider)
            w->node.setProperty("style", bf::sliderStyleFor(w->getBounds()), nullptr);
        w->setTheme(playgroundTheme);
    }
    panel.theme = playgroundTheme;
    panel.screenType = pb::boardScreenTypeOf(proc.uiState, pb::kShells[shellIndex].screenStyle);
    panel.shellIndex = shellIndex;
    panel.placing = placing;
    panel.armedStyle = armedStyle;
    panel.onRightClick = [this](int slot, juce::Point<int> pos) { showSlotMenu(slot, pos); };
    repairParents();
    panel.parentOf = [this](int bay) {
        for (auto* w : widgets)
            if ((int) w->node.getProperty("shellSlot", -1) == bay)
                return (int) w->node.getProperty("parent", -1);
        return -1;
    };
    panel.selectedSlot = (selectedChainWidget >= 0 && selectedChainWidget < widgets.size())
        ? (int) widgets[selectedChainWidget]->node.getProperty("shellSlot", -1) : -1;
    panel.onBackgroundClick = [this] {
        if (selectedChainWidget < 0) return;
        selectedChainWidget = -1;
        for (auto* item : widgets) { item->selected = false; item->repaint(); }
        panel.selectedSlot = -1;
        panel.repaint();
        status.setText("Nothing highlighted - the next part joins the end of the chain.", juce::dontSendNotification);
    };
    panel.repaint();
}

bool KyotoAudioProcessorEditor::slotOccupied(int slot) const
{
    for (auto* w : widgets)
        if ((int) w->node.getProperty("shellSlot", -1) == slot) return true;
    return false;
}

juce::Point<float> KyotoAudioProcessorEditor::slotAnchor(int slot) const
{
    for (auto* w : widgets)
        if ((int) w->node.getProperty("shellSlot", -1) == slot)
            return w->getBounds().getCentre().toFloat();
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    if (slot < 0 || slot >= shell.slotCount) return {};
    return pb::slotRect(pb::faceRect(panel.getLocalBounds().toFloat()), shell.slots[slot]).getCentre();
}

int KyotoAudioProcessorEditor::chainParentWidget() const
{
    // Highlighted part first (cosmetic parts carry no audio, so they cannot be chained into).
    if (selectedChainWidget >= 0 && selectedChainWidget < widgets.size())
    {
        auto n = widgets[selectedChainWidget]->node;
        if ((int) n.getProperty("slot", -1) >= 0) return selectedChainWidget;
    }
    int best = -1, bestDsp = -1;
    for (int i = 0; i < widgets.size(); ++i)
    {
        const int d = (int) widgets[i]->node.getProperty("slot", -1);
        if (d > bestDsp) { bestDsp = d; best = i; }
    }
    return best;
}

int KyotoAudioProcessorEditor::nearestFreeBay(const juce::String& style, juce::Point<float> localPos, int preferred) const
{
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    auto face = pb::faceRect(panel.getLocalBounds().toFloat());
    auto usable = [&](int i) {
        if (i < 0 || i >= shell.slotCount) return false;
        const auto& s = shell.slots[i];
        return s.w >= 0.02f && s.kind != pb::SlotKind::Board && ! slotOccupied(i) && pb::styleFits(style, s.kind);
    };
    if (usable(preferred)) return preferred;
    int best = -1;
    float bestDist = 1.0e9f;
    for (int i = 0; i < shell.slotCount; ++i)
    {
        if (! usable(i)) continue;
        const float d = pb::slotRect(face, shell.slots[i]).getCentre().getDistanceFrom(localPos);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

void KyotoAudioProcessorEditor::shiftDspUp(int from, int to)
{
    // Moves DSP slots [from, to-1] up by one so a new part can sit right after its parent in the chain.
    static const char* suffixes[] = { "on", "type", "amt", "tone", "mot", "mix", "shp" };
    for (int j = to - 1; j >= from; --j)
    {
        const auto src = "s" + juce::String(j + 1).paddedLeft('0', 2);
        const auto dst = "s" + juce::String(j + 2).paddedLeft('0', 2);
        for (auto* suffix : suffixes)
        {
            auto* a = proc.apvts.getParameter(src + suffix);
            auto* b = proc.apvts.getParameter(dst + suffix);
            if (a != nullptr && b != nullptr) b->setValueNotifyingHost(a->getValue());
        }
        proc.setSlotOvermax(j + 1, proc.getSlotOvermax(j));
    }
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w")) continue;
        const int s = (int) child.getProperty("slot", -1);
        if (s >= from && s < to) child.setProperty("slot", s + 1, nullptr);
    }
    proc.setSlotOvermax(from, 1.f); // the vacated slot is taken by the new part
}

void KyotoAudioProcessorEditor::repairParents()
{
    for (auto* w : widgets)
    {
        const int bay = (int) w->node.getProperty("shellSlot", -1);
        if (bay <= 0) continue;
        const int par = (int) w->node.getProperty("parent", 0);
        if (par != 0 && (par == bay || ! slotOccupied(par)))
            w->node.setProperty("parent", 0, nullptr);
    }
}

void KyotoAudioProcessorEditor::applyShell(int index)
{
    shellIndex = juce::jlimit(0, pb::kShellCount - 1, index);
    proc.uiState.setProperty("shell", pb::kShells[shellIndex].id, nullptr);
    panel.shellIndex = shellIndex;
    // A new template re-seats every part, so drop the hand-set grid cells and snap to the new bays.
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w") && child.getProperty("kind").toString() != "board")
        {
            child.removeProperty("gx", nullptr);
            child.removeProperty("gy", nullptr);
            child.removeProperty("gw", nullptr);
            child.removeProperty("gh", nullptr);
        }
    }
    ensureMotherboard();
    reflowSeries();
    repaint();
}

void KyotoAudioProcessorEditor::ensureMotherboard()
{
    const auto& shell = pb::kShells[shellIndex];
    juce::ValueTree board;
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w") && child.getProperty("kind").toString() == "board") board = child;
    }
    // The motherboard is only the start of the chain and the mandatory screen. It owns DSP bay 1, which
    // stays OFF: no hidden effect is ever seeded, so only effects the user places make sound.
    if (auto* on = proc.apvts.getParameter("s01on"))
        if (on->getValue() >= 0.5f) on->setValueNotifyingHost(0.f);
    if (! board.isValid())
    {
        auto node = juce::ValueTree("w");
        node.setProperty("slot", 0, nullptr);
        node.setProperty("shellSlot", 0, nullptr);
        node.setProperty("param", "mix", nullptr);
        node.setProperty("label", juce::String(shell.name) + " screen", nullptr);
        node.setProperty("kind", "board", nullptr);
        node.setProperty("style", "board", nullptr);
        node.setProperty("screenType", shell.screenStyle, nullptr);
        node.setProperty("series", 0, nullptr);
        proc.uiState.addChild(node, 0, nullptr);
        rebuildCanvas();
    }
    else
    {
        board.setProperty("label", juce::String(shell.name) + " screen", nullptr);
        board.setProperty("shellSlot", 0, nullptr);
        if (! board.hasProperty("screenType")) board.setProperty("screenType", shell.screenStyle, nullptr);
    }
    proc.setHardwareColour(-1, 0.f);
}

void KyotoAudioProcessorEditor::syncPanelMouse()
{
    // When armed for placement the panel must intercept clicks so bay hits reach
    // BuilderCanvas::mouseDown. When idle, let clicks pass through to widgets
    // (children) and the parent editor.
    panel.setInterceptsMouseClicks(true, true);
}

void KyotoAudioProcessorEditor::armPlacement()
{
    armedStyle = pb::styleToken(kindBox.getSelectedId());
    pendingSpecial = false;
    pendingFx = fxBrowser ? fxBrowser->getSelectedFx() : 0;
    pendingLabel = (pendingFx >= 0 && pendingFx < kt::kFxCount) ? kt::kFx[pendingFx].name : "Part";
    if (pb::styleSlot(armedStyle) == pb::SlotKind::Cosmetic)
        pendingLabel = kindBox.getText();
    const int pieceId = pieceBox.getSelectedId();
    pendingPiece = (pieceId >= 2 && pieceId - 2 < kt::kModPieceCount) ? &kt::kModPieces[pieceId - 2] : nullptr;
    placing = true;
    panel.placing = true;
    panel.armedStyle = armedStyle;
    syncPanelMouse();
    status.setText(pendingPiece != nullptr
        ? juce::String(pendingPiece->name) + " armed (" + pendingPiece->quirk + "). Click a glowing bay."
        : "Theme is " + juce::String(theme.name) + ". Click a glowing " + kindBox.getText() + " bay.", juce::dontSendNotification);
    panel.repaint();
}


int KyotoAudioProcessorEditor::findWidgetAtShellSlot(int shellSlot) const
{
    int sat = -1;
    for (int i = 0; i < widgets.size(); ++i)
    {
        if ((int) widgets[i]->node.getProperty("shellSlot", -1) != shellSlot)
            continue;
        if ((int) widgets[i]->node.getProperty("satellite", 0) != 0) { sat = i; continue; }
        return i; // prefer host effect / primary part
    }
    return sat;
}

void KyotoAudioProcessorEditor::rebuildWireGraph()
{
    juce::Array<int> order;
    kt::wire::rebuildOrder(proc.uiState, order);
    status.setText("Wire graph: " + juce::String(order.size()) + " stages (manual Put wire Into)", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::clearWireFrom(int widgetIndex)
{
    if (widgetIndex < 0 || widgetIndex >= widgets.size()) return;
    captureSnapshot();
    auto n = widgets[widgetIndex]->node;
    n.setProperty("parent", -1, nullptr);
    n.setProperty("wiredInto", -1, nullptr);
    rebuildWireGraph();
    rebuildCanvas();
    panel.repaint();
    status.setText("Cut wire from " + n.getProperty("label").toString(), juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::putWireInto(int fromWidgetIndex, int intoWidgetIndex)
{
    if (fromWidgetIndex < 0 || fromWidgetIndex >= widgets.size()) return;
    if (intoWidgetIndex < 0 || intoWidgetIndex >= widgets.size()) return;
    if (fromWidgetIndex == intoWidgetIndex)
    {
        status.setText("Cannot wire a part into itself.", juce::dontSendNotification);
        return;
    }

    captureSnapshot();
    auto from = widgets[fromWidgetIndex]->node;
    auto into = widgets[intoWidgetIndex]->node;

    // Satellites follow their host — wire the host instead.
    if ((int) from.getProperty("satellite", 0) != 0)
    {
        status.setText("Select the effect host (not a bound dial) as the source.", juce::dontSendNotification);
        return;
    }

    const int intoShell = (int) into.getProperty("shellSlot", 0);
    const int intoDsp = (int) into.getProperty("slot", 0);
    const int fromDsp = (int) from.getProperty("slot", -1);

    from.setProperty("parent", intoShell, nullptr);
    from.setProperty("wiredInto", intoDsp, nullptr);

    // Reorder DSP: source should process immediately AFTER destination in the chain.
    // We rebuild wireOrder from parent links (topological-ish by parent dsp).
    // Ensure destination has a lower/earlier slot than source when possible by swapping slot indices.
    if (fromDsp > 0 && intoDsp >= 0 && fromDsp != intoDsp)
    {
        // If source is currently before dest in numeric slot order, swap their DSP slot numbers
        // so processing order matches wire direction (into first, then from).
        if (fromDsp < intoDsp)
        {
            // Swap APVTS params between the two slots
            const auto pa = "s" + juce::String(fromDsp + 1).paddedLeft('0', 2);
            const auto pb = "s" + juce::String(intoDsp + 1).paddedLeft('0', 2);
            auto swapP = [this, &pa, &pb](const juce::String& suf)
            {
                auto* a = proc.apvts.getParameter(pa + suf);
                auto* b = proc.apvts.getParameter(pb + suf);
                if (a == nullptr || b == nullptr) return;
                const float va = a->getValue(), vb = b->getValue();
                a->setValueNotifyingHost(vb);
                b->setValueNotifyingHost(va);
            };
            for (const char* s : { "on", "type", "amt", "tone", "mot", "mix", "shp" })
                swapP(s);
            // Swap slot property on all widgets referencing these DSP ids
            for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
            {
                auto c = proc.uiState.getChild(i);
                if (! c.hasType("w")) continue;
                const int s = (int) c.getProperty("slot", -1);
                if (s == fromDsp) c.setProperty("slot", intoDsp, nullptr);
                else if (s == intoDsp) c.setProperty("slot", fromDsp, nullptr);
                const int bf = (int) c.getProperty("bindFx", -1);
                if (bf == fromDsp) c.setProperty("bindFx", intoDsp, nullptr);
                else if (bf == intoDsp) c.setProperty("bindFx", fromDsp, nullptr);
            }
        }
    }

    rebuildWireGraph();
    rebuildCanvas();
    status.setText("Wired " + from.getProperty("label").toString()
        + " → into " + into.getProperty("label").toString()
        + ". Signal flows destination then source.", juce::dontSendNotification);
}


void KyotoAudioProcessorEditor::bindDialToEffect(int shellSlot, int fxDspSlot, int fxType)
{
    captureSnapshot();
    const auto choices = kt::wire::controlsForFx(fxType);
    juce::PopupMenu menu;
    menu.addSectionHeader("Control on this effect");
    for (int i = 0; i < choices.size(); ++i)
        menu.addItem(i + 1, kt::wire::ctrlLabel(choices.getUnchecked(i)));

    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withMousePosition(),
        [safe, shellSlot, fxDspSlot, fxType, choices](int result)
        {
            if (safe == nullptr || result <= 0 || result > choices.size()) return;
            const auto ctrl = choices.getUnchecked(result - 1);
            const auto suffix = juce::String(kt::wire::ctrlId(ctrl));

            // Create a satellite dial widget bound to the effect's parameter.
            juce::ValueTree node("w");
            node.setProperty("kind", "dial", nullptr);
            node.setProperty("label", juce::String(kt::wire::ctrlLabel(ctrl)), nullptr);
            node.setProperty("shellSlot", shellSlot, nullptr);
            node.setProperty("slot", fxDspSlot, nullptr); // shares DSP identity of host effect
            node.setProperty("slotCount", 0, nullptr);    // satellite — does not insert a new DSP stage
            node.setProperty("parent", shellSlot, nullptr);
            node.setProperty("bindFx", fxDspSlot, nullptr);
            node.setProperty("bindCtrl", juce::String(kt::wire::ctrlLabel(ctrl)).replaceCharacter(' ', '_').toLowerCase(), nullptr);
            node.setProperty("bindSuffix", suffix, nullptr);
            node.setProperty("satellite", 1, nullptr);
            // Offset slightly so it sits "on top" of the host bay visually.
            node.setProperty("gx", 0, nullptr);
            node.setProperty("gy", 0, nullptr);
            node.setProperty("gw", 4, nullptr);
            node.setProperty("gh", 4, nullptr);

            // Prefer unique satellite shell slot encoding: keep same shellSlot, mark satellite.
            safe->proc.uiState.appendChild(node, nullptr);

            // Attach APVTS: dial drives host effect param.
            const auto prefix = "s" + juce::String(fxDspSlot + 1).paddedLeft('0', 2);
            // Ensure host effect stays on.
            if (auto* on = safe->proc.apvts.getParameter(prefix + "on"))
                on->setValueNotifyingHost(1.f);

            safe->placing = false;
            safe->panel.placing = false;
            safe->pendingSpecial = false;
            safe->rebuildCanvas();
            safe->rebuildWireGraph();
            safe->status.setText("Dial bound → " + juce::String(kt::wire::ctrlLabel(ctrl))
                + " on effect slot " + juce::String(fxDspSlot + 1), juce::dontSendNotification);
        });
}

void KyotoAudioProcessorEditor::placeInSlot(int slot)
{
    if (!placing) return;
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    if (slot < 0 || slot >= shell.slotCount) return;

    // Dial / fader dropped onto an occupied effect bay → bind control to that effect.
    if (slotOccupied(slot) && (armedStyle == "dial" || armedStyle == "slider" || armedStyle == "knob"))
    {
        int hostIdx = findWidgetAtShellSlot(slot);
        if (hostIdx >= 0)
        {
            auto host = widgets[hostIdx]->node;
            const int fxDsp = (int) host.getProperty("slot", -1);
            const int fxType = (int) host.getProperty("fx", (int) host.getProperty("type", 0));
            if (fxDsp > 0)
            {
                bindDialToEffect(slot, fxDsp, fxType);
                return;
            }
        }
        status.setText("That bay is full. Place a dial onto an effect part to control it.", juce::dontSendNotification);
        return;
    }

    if (!pb::styleFits(armedStyle, shell.slots[slot].kind) || slotOccupied(slot))
    {
        status.setText("That bay does not take this part.", juce::dontSendNotification);
        return;
    }
    captureSnapshot();
    const bool cosmetic = shell.slots[slot].kind == pb::SlotKind::Cosmetic;
    int dsp = -1;
    int insertedDsp = -1;
    // Manual wiring only: new parts are unconnected (parent = motherboard).
    // Use left-click select + right-click "Put wire Into" to plug A into B.
    int parentBay = -1; // -1 = unwired (do NOT default-wire to screen/motherboard)
    juce::String parentLabel = "unwired (use Put wire Into)";
    int parentDsp = 0;
    if (!cosmetic)
    {
        auto isOn = [this](int i) {
            auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on");
            return on != nullptr && on->getValue() >= 0.5f;
        };
        // Always claim the first free DSP slot — order is controlled by Put wire Into.
        int freeIdx = -1;
        for (int i = 1; i < proc.slotCount(); ++i)
            if (! isOn(i)) { freeIdx = i; break; }
        if (freeIdx < 0) { status.setText("DSP bays are full.", juce::dontSendNotification); return; }
        const int ins = freeIdx;
        dsp = ins;
        insertedDsp = ins;
        const auto prefix = "s" + juce::String(dsp + 1).paddedLeft('0', 2);
        const int type = pendingSpecial ? pendingSpecialType : pendingFx;
        if (auto* param = proc.apvts.getParameter(prefix + "type")) param->setValueNotifyingHost(param->convertTo0to1((float) type));
        if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(1.f);
        auto setValue = [this, &prefix](const juce::String& suffix, float value) { if (auto* param = proc.apvts.getParameter(prefix + suffix)) param->setValueNotifyingHost(param->convertTo0to1(value)); };
        setValue("amt", 0.45f); setValue("tone", 0.5f); setValue("mot", 0.3f); setValue("mix", 0.35f); setValue("shp", 0.5f);
    }
    auto node = juce::ValueTree("w");
    node.setProperty("slot", dsp, nullptr);
    node.setProperty("shellSlot", slot, nullptr);
    const int paramId = juce::jlimit(0, kt::kControlParamCount - 1, paramBox.getSelectedId() - 1);
    const auto paramToken = juce::String(kt::kControlParams[paramId].token);
    node.setProperty("param", paramToken, nullptr);
    node.setProperty("label", paramId == 0 ? pendingLabel : pendingLabel + " " + kt::kControlParams[paramId].name, nullptr);
    const auto slotKind = shell.slots[slot].kind;
    juce::String kindName = slotKind == pb::SlotKind::Fader ? "slider" : slotKind == pb::SlotKind::Key ? "key" : slotKind == pb::SlotKind::Screen ? "wave" : slotKind == pb::SlotKind::Cosmetic ? "cosmetic" : "dial";
    if (armedStyle == "sound" || armedStyle == "button" || armedStyle == "wave" || armedStyle == "key" || armedStyle == "dial" || armedStyle == "slider")
        kindName = armedStyle;
    node.setProperty("kind", kindName, nullptr);
    node.setProperty("style", armedStyle, nullptr);
    if (pendingSpecial || pendingFx >= 0)
        node.setProperty("fx", pendingSpecial ? pendingSpecialType : pendingFx, nullptr);
    if (pendingPiece != nullptr)
    {
        node.setProperty("skin", pendingPiece->skin, nullptr);
        node.setProperty("quirk", pendingPiece->id, nullptr);
    }
    node.setProperty("series", proc.uiState.getNumChildren(), nullptr);
    node.setProperty("parent", parentBay, nullptr);
    node.setProperty("wiredInto", -1, nullptr);
    if (insertedDsp >= 0)
    {
        // Parts that used to follow the parent now follow the new part, so the wires show the real signal order.
        for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
        {
            auto other = proc.uiState.getChild(i);
            if (! other.hasType("w")) continue;
            if (parentBay >= 0 && (int) other.getProperty("slot", -1) > insertedDsp && (int) other.getProperty("parent", -1) == parentBay
                && (int) other.getProperty("shellSlot", -1) != parentBay)
                other.setProperty("parent", slot, nullptr);
        }
    }
    proc.uiState.appendChild(node, nullptr);
    placing = false;
    pendingSpecial = false;
    panel.placing = false;
    syncPanelMouse();
    rebuildCanvas();
    ensureMotherboard();
    // Highlight the new part so the next Add continues the chain from it.
    if (! cosmetic && widgets.size() > 0)
    {
        selectedChainWidget = widgets.size() - 1;
        for (auto* item : widgets) item->selected = false;
        widgets[selectedChainWidget]->selected = true;
        panel.selectedSlot = slot;
        panel.repaint();
    }
    rebuildWireGraph();
    status.setText("Snapped " + pendingLabel + " into " + juce::String(shell.slots[slot].name) + ". Unwired on " + parentLabel + (cosmetic ? "." : ". Select it, right-click another part → Put wire Into. Drop a dial on an effect to bind Mix/Decay/Hz..."), juce::dontSendNotification);
}


void KyotoAudioProcessorEditor::rebuildCanvas()
{
    // Legacy builds defaulted parent=0 (screen). Treat as unwired unless wiredInto is set.
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w")) continue;
        if (child.getProperty("kind").toString() == "board") continue;
        const int par = (int) child.getProperty("parent", -1);
        const int wi = (int) child.getProperty("wiredInto", -1);
        if (par == 0 && wi < 0)
            child.setProperty("parent", -1, nullptr);
    }
    {
        juce::Array<int> order;
        kt::wire::rebuildOrder(proc.uiState, order);
    }
    widgets.clear();
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w")) continue;
        const int widgetIndex = widgets.size();
        auto* w = widgets.add(new CanvasWidget(proc, child));
        w->setTheme(playgroundTheme);
        w->selected = (widgetIndex == selectedChainWidget);
        w->onSelect = [this, widgetIndex] { selectedChainWidget = widgetIndex; panel.selectedSlot = (widgetIndex >= 0 && widgetIndex < widgets.size()) ? (int) widgets[widgetIndex]->node.getProperty("shellSlot", -1) : -1; for (auto* item : widgets) { item->selected = false; item->repaint(); } if (widgetIndex >= 0 && widgetIndex < widgets.size()) { widgets[widgetIndex]->selected = true; widgets[widgetIndex]->repaint(); } repaint(); };
        w->onRightClick = [this, widgetIndex](CanvasWidget*, const juce::MouseEvent& e)
        {
            selectedChainWidget = widgetIndex;
            showSlotMenu((int) widgets[widgetIndex]->node.getProperty("shellSlot", -1), e.getScreenPosition());
        };
        panel.addAndMakeVisible(w);
    }
    reflowSeries();
}

juce::String KyotoAudioProcessorEditor::serializeFxStack() const
{
    juce::Array<juce::var> arr;
    for (int i = 0; i < fxStack.getNumChildren(); ++i)
    {
        auto s = fxStack.getChild(i);
        auto* o = new juce::DynamicObject();
        o->setProperty("fx", (int)s.getProperty("fx", 0)); o->setProperty("name", s.getProperty("name").toString());
        o->setProperty("amount", (double)s.getProperty("amount", 0.5)); o->setProperty("tone", (double)s.getProperty("tone", 0.5));
        o->setProperty("motion", (double)s.getProperty("motion", 0.35)); o->setProperty("mix", (double)s.getProperty("mix", 0.4)); o->setProperty("shape", (double)s.getProperty("shape", 0.5));
        arr.add(juce::var(o));
    }
    return juce::JSON::toString(juce::var(arr));
}

void KyotoAudioProcessorEditor::restoreFxStack(const juce::String& json)
{
    fxStack.removeAllChildren(nullptr);
    auto parsed = juce::JSON::parse(json);
    if (auto* arr = parsed.getArray())
        for (auto& value : *arr)
            if (auto* o = value.getDynamicObject())
            {
                auto s = juce::ValueTree("step");
                s.setProperty("fx", (int)propertyOr(o, "fx", 0), nullptr); s.setProperty("name", o->getProperty("name").toString(), nullptr);
                s.setProperty("amount", (double)propertyOr(o, "amount", 0.5), nullptr); s.setProperty("tone", (double)propertyOr(o, "tone", 0.5), nullptr);
                s.setProperty("motion", (double)propertyOr(o, "motion", 0.35), nullptr); s.setProperty("mix", (double)propertyOr(o, "mix", 0.4), nullptr); s.setProperty("shape", (double)propertyOr(o, "shape", 0.5), nullptr);
                fxStack.appendChild(s, nullptr);
            }
    selectedFxStep = fxStack.getNumChildren() > 0 ? juce::jlimit(0, fxStack.getNumChildren()-1, selectedFxStep) : -1;
    if (selectedFxStep >= 0) selectFxStep(selectedFxStep);
    stackLabel.setText(fxStack.getNumChildren() > 0 ? "Stack " + juce::String(fxStack.getNumChildren()) + " / 16  -  custom effect lab" : "Empty effect  -  ready for a new build", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::captureSnapshot()
{
    EditorSnapshot snapshot;
    proc.getStateInformation(snapshot.processorState);
    snapshot.fxStackJson = serializeFxStack();
    snapshot.selectedFx = selectedFxStep;
    undoStack.push_back(std::move(snapshot));
    if (undoStack.size() > 24) undoStack.erase(undoStack.begin());
    lastPublishedEffectId.clear();
}

void KyotoAudioProcessorEditor::restoreSnapshot(const EditorSnapshot& snapshot)
{
    proc.setStateInformation(snapshot.processorState.getData(), (int)snapshot.processorState.getSize());
    restoreFxStack(snapshot.fxStackJson);
    selectedFxStep = snapshot.selectedFx;
    selectedChainWidget = -1;
    rebuildCanvas();
    repaint();
}


void KyotoAudioProcessorEditor::persistEditorSession()
{
    proc.uiState.setProperty("editorTab", tab, nullptr);
    proc.uiState.setProperty("theme", theme.id, nullptr);
    proc.uiState.setProperty("shell", pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)].id, nullptr);
    proc.uiState.setProperty("editorW", getWidth(), nullptr);
    proc.uiState.setProperty("editorH", getHeight(), nullptr);
    auto* o = new juce::DynamicObject();
    o->setProperty("tab", tab);
    o->setProperty("theme", theme.id);
    o->setProperty("shell", pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)].id);
    o->setProperty("w", getWidth());
    o->setProperty("h", getHeight());
    sessionFile().getSiblingFile("editor-ui.json").replaceWithText(juce::JSON::toString(juce::var(o)));
}

void KyotoAudioProcessorEditor::restoreEditorSession()
{
    auto file = sessionFile().getSiblingFile("editor-ui.json");
    auto parsed = juce::JSON::parse(file.loadFileAsString());
    auto* o = parsed.getDynamicObject();
    const int w = o != nullptr ? (int) o->getProperty("w") : (int) proc.uiState.getProperty("editorW", 980);
    const int h = o != nullptr ? (int) o->getProperty("h") : (int) proc.uiState.getProperty("editorH", 680);
    if (w >= 640 && h >= 420) setSize(w, h);
    const auto themeId = o != nullptr ? o->getProperty("theme").toString() : proc.uiState.getProperty("theme").toString();
    if (themeId.isNotEmpty()) applyTheme(themeId);
    const auto shellId = o != nullptr ? o->getProperty("shell").toString() : proc.uiState.getProperty("shell").toString();
    for (int i = 0; i < pb::kShellCount; ++i)
        if (shellId == pb::kShells[i].id) { shellIndex = i; shellBox.setSelectedId(i + 1, juce::dontSendNotification); }
    const int savedTab = o != nullptr ? (int) o->getProperty("tab") : (int) proc.uiState.getProperty("editorTab", 0);
    if (loggedIn && savedTab >= 0 && savedTab <= 2) showTab(savedTab);
    reflowSeries();
}

void KyotoAudioProcessorEditor::placeKindInSlot(const juce::String& kind, int slot, int fxIndex, const juce::String& label)
{
    armedStyle = kind == "slider" ? "slider" : kind;
    if (kind == "sound")
    {
        for (auto* w : widgets)
            if (w->kind == CanvasWidget::Kind::Sound)
            {
                status.setText("Sound is unique. Drop a sample on the existing Sound module.", juce::dontSendNotification);
                return;
            }
    }
    pendingSpecial = kind == "stack" || kind == "sound" || fxIndex >= 0;
    pendingSpecialType = juce::jmax(0, fxIndex);
    pendingFx = juce::jmax(0, fxIndex);
    pendingLabel = label;
    pendingPiece = nullptr;
    placing = true;
    placeInSlot(slot);
}

void KyotoAudioProcessorEditor::randomizeTemplate()
{
    captureSnapshot();
    juce::Random rng((juce::int64) juce::Time::getMillisecondCounterHiRes());
    // New roll -> new random layout for the hidden hardware too.
    proc.uiState.setProperty("internalsSeed", (int) rng.nextInt(1000000) + 1, nullptr);
    // Keep current shell geometry; templates are retired — only re-roll parts on the free grid.
    shellBox.setSelectedId(shellIndex + 1, juce::dontSendNotification);
    juce::Array<juce::ValueTree> keep;
    for (int i = proc.uiState.getNumChildren(); --i >= 0;)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w") && child.getProperty("kind").toString() != "board")
            proc.uiState.removeChild(child, nullptr);
    }
    for (int i = 1; i < proc.slotCount(); ++i)
        if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"))
            on->setValueNotifyingHost(0.f);
    rebuildCanvas();
    const auto& shell = pb::kShells[shellIndex];
    selectedChainWidget = -1;
    juce::StringArray essentials { "wave", "sound", "key", "dial" };
    int placed = 0;
    const int limit = juce::jmax(4, shell.slotCount - 1);
    for (int s = 0; s < shell.slotCount && placed < limit; ++s)
    {
        if (shell.slots[s].kind == pb::SlotKind::Board || shell.slots[s].w < 0.02f) continue;
        juce::String kind;
        if (essentials.size() > 0) { kind = essentials[0]; essentials.remove(0); }
        else
        {
            const char* pool[] = { "dial", "slider", "button", "wave", "key" };
            kind = pool[rng.nextInt(5)];
        }
        if (kind == "slider" && shell.slots[s].w > shell.slots[s].h) kind = "slider";
        const int fx = rng.nextInt(kt::kFxCount);
        const int bay = nearestFreeBay(kind, slotAnchor(s), s);
        if (bay < 0) continue;
        placeKindInSlot(kind, bay, kind == "dial" || kind == "slider" || kind == "button" ? fx : -1, kind == "sound" ? "Sound" : kt::kFx[fx].name);
        ++placed;
    }
    reflowSeries();
        // Move the screen/motherboard to a fresh grid cell so Randomize actually relocates it.
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (! child.hasType("w") || child.getProperty("kind").toString() != "board") continue;
        juce::Random rng2((juce::int64) juce::Time::getMillisecondCounterHiRes() + 17);
        child.setProperty("gx", 2 + rng2.nextInt(18), nullptr);
        child.setProperty("gy", 2 + rng2.nextInt(10), nullptr);
        child.setProperty("gw", juce::jmax(6, (int) child.getProperty("gw", 10)), nullptr);
        child.setProperty("gh", juce::jmax(4, (int) child.getProperty("gh", 6)), nullptr);
    }
    rebuildCanvas();
    status.setText("Random template: essentials covered, " + juce::String(placed) + " parts, limit " + juce::String(limit) + ".", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::rollNewInstanceTemplate()
{
    // A brand-new instance opens on a BLANK playground: just the mandatory motherboard, no parts.
    // A restored build is left alone. Right-click the empty playground to place a motherboard.
    bool hasBoard = false;
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w"))
        {
            if (child.getProperty("kind").toString() == "board") hasBoard = true;
            else return; // restored build with parts - leave it alone
        }
    }
    // Blank playground: ensure just the motherboard, no auto-filled parts.
    if (! hasBoard)
    {
        shellIndex = 0;
        shellBox.setSelectedId(1, juce::dontSendNotification);
        proc.uiState.setProperty("shell", pb::kShells[0].id, nullptr);
        panel.shellIndex = 0;
        ensureMotherboard();
        rebuildCanvas();
        status.setText("Blank playground - right-click to place a motherboard, then add parts.", juce::dontSendNotification);
    }
}

void KyotoAudioProcessorEditor::swapPartInBay(int bay, const juce::String& kind, int fxIndex, const juce::String& label)
{
    const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, shellIndex)];
    if (bay < 0 || bay >= shell.slotCount || kind.isEmpty()) return;

    // Find the part currently sitting in the bay; a swap keeps its place, size and chain wiring.
    juce::ValueTree old;
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w") && (int) child.getProperty("shellSlot", -1) == bay) { old = child; break; }
    }
    if (! old.isValid()) { status.setText("That bay is empty - left-click it to place a part.", juce::dontSendNotification); return; }
    if (! pb::styleFits(kind, shell.slots[bay].kind))
    {
        status.setText("The " + juce::String(shell.slots[bay].name) + " bay does not take a " + kind + ".", juce::dontSendNotification);
        return;
    }
    captureSnapshot();

    const bool cosmetic = shell.slots[bay].kind == pb::SlotKind::Cosmetic;
    const int oldSlot = (int) old.getProperty("slot", -1);
    int dsp = -1;
    if (! cosmetic)
    {
        dsp = oldSlot;
        if (dsp < 0)
        {
            for (int i = 1; i < proc.slotCount(); ++i)
            {
                auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on");
                if (on != nullptr && on->getValue() < 0.5f) { dsp = i; break; }
            }
            if (dsp < 0) { status.setText("DSP bays are full - remove a part first.", juce::dontSendNotification); return; }
        }
        const auto prefix = "s" + juce::String(dsp + 1).paddedLeft('0', 2);
        if (fxIndex >= 0)
            if (auto* param = proc.apvts.getParameter(prefix + "type")) param->setValueNotifyingHost(param->convertTo0to1((float) fxIndex));
        if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(1.f);
    }
    else if (oldSlot >= 0)
    {
        // Cosmetic parts own no DSP bay, so release the one the old part held.
        if (auto* on = proc.apvts.getParameter("s" + juce::String(oldSlot + 1).paddedLeft('0', 2) + "on")) on->setValueNotifyingHost(0.f);
    }

    auto node = juce::ValueTree("w");
    node.setProperty("slot", dsp, nullptr);
    node.setProperty("shellSlot", bay, nullptr);
    node.setProperty("series", old.getProperty("series", proc.uiState.getNumChildren()), nullptr);
    node.setProperty("parent", old.getProperty("parent", 0), nullptr);
    for (auto* key : { "gx", "gy", "gw", "gh" })
        if (old.hasProperty(key)) node.setProperty(key, old.getProperty(key), nullptr);
    node.setProperty("kind", kind, nullptr);
    node.setProperty("style", kind, nullptr);
    node.setProperty("param", old.getProperty("param", "amt"), nullptr);
    node.setProperty("label", label.isNotEmpty() ? label : kind, nullptr);

    const int index = proc.uiState.indexOf(old);
    proc.uiState.removeChild(old, nullptr);
    proc.uiState.addChild(node, index, nullptr);
    rebuildCanvas();
    ensureMotherboard();

    selectedChainWidget = -1;
    for (int i = 0; i < widgets.size(); ++i)
        if ((int) widgets[i]->node.getProperty("shellSlot", -1) == bay)
        {
            selectedChainWidget = i;
            widgets[i]->selected = true;
            widgets[i]->repaint();
        }
    panel.selectedSlot = bay;
    panel.repaint();
    status.setText("Swapped in " + (label.isNotEmpty() ? label : kind) + ".", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::showSlotMenu(int slot, juce::Point<int> screenPos)
{
    const auto local = panel.getLocalPoint(nullptr, screenPos.toFloat());
    const bool hasSelection = selectedChainWidget >= 0 && selectedChainWidget < widgets.size();
    const int selBay = hasSelection ? (int) widgets[selectedChainWidget]->node.getProperty("shellSlot", -1) : -1;

    // Check if a motherboard exists
    bool hasBoard = false;
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto child = proc.uiState.getChild(i);
        if (child.hasType("w") && child.getProperty("kind").toString() == "board") { hasBoard = true; break; }
    }

    juce::PopupMenu menu;

    // ---- Empty playground: prompt to place a motherboard first ----
    if (! hasBoard)
    {
        menu.addSectionHeader("BLANK PLAYGROUND");
        menu.addItem(900, "Place Motherboard");
        menu.addSeparator();
        // Theme submenu still available
        juce::PopupMenu themeMenu;
        for (int i = 0; i < kt::kThemeCount; ++i)
            themeMenu.addItem(5000 + i, kt::kThemes[i].name, true, playgroundTheme.id == kt::kThemes[i].id);
        menu.addSubMenu("Theme", themeMenu);
        // Aspect ratio
        juce::PopupMenu arMenu;
        arMenu.addItem(5101, "4:5 Portrait", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Portrait45);
        arMenu.addItem(5102, "1:1 Square", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Square11);
        arMenu.addItem(5103, "5:4 Landscape", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Landscape54);
        arMenu.addItem(5104, "Freeform", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Freeform);
        menu.addSubMenu("Aspect Ratio", arMenu);
        menu.addSeparator();
        menu.addItem(5200, "Randomize Machine");
        menu.addItem(5201, "Load WAV");
        menu.addItem(5202, "Save Build");
        menu.addItem(5203, "Publish");
        menu.addItem(5204, "Plugin View");
        menu.addItem(5205, "Undo");
        menu.addItem(5206, "Randomize Parts");
        menu.addSeparator();
        menu.addItem(5999, "Guide");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ screenPos.x, screenPos.y, 1, 1 }),
            [this, safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this)](int result)
            {
                if (safeEd == nullptr || safeEd->editorClosing) return;
                if (result == 0) return;
                if (result == 5999) { safeEd->showBuilderGuide(); return; }
                if (result == 900) { ensureMotherboard(); rebuildCanvas(); status.setText("Motherboard placed. Right-click to add parts.", juce::dontSendNotification); return; }
                if (result >= 5000 && result < 5000 + kt::kThemeCount) { const auto& tid = kt::kThemes[result - 5000].id; applyPlaygroundTheme(tid); applyTheme(tid); return; }
                if (result == 5101) { startNewMachine(0); return; }
                if (result == 5102) { startNewMachine(1); return; }
                if (result == 5103) { startNewMachine(2); return; }
                if (result == 5104) { startNewMachine(3); return; }
                if (result == 5200) { randomizeMachine(); return; }
                if (result == 5201) { loadWav(); return; }
                if (result == 5202) { saveLocal(); return; }
                if (result == 5203) { if (token.isEmpty()) status.setText("Sign in to DreamShare first to publish.", juce::dontSendNotification); else publish(); return; }
                if (result == 5204) { setPluginView(true); return; }
                if (result == 5205) { undoLast(); return; }
                if (result == 5206) { randomizeTemplate(); return; }
            });
        return;
    }

    // ---- Normal right-click menu with all features ----
    {
        const auto src = (selectedChainWidget >= 0 && selectedChainWidget < widgets.size())
            ? widgets[selectedChainWidget]->node.getProperty("label").toString()
            : juce::String("(none selected)");
        menu.addSectionHeader("Selected: " + src + "  —  use Put wire Into to connect");
    }

    // Change Screen (on motherboard)
    if (slot == 0 || (slot < 0 && !hasSelection))
    {
        juce::PopupMenu scr;
        const int cur = pb::boardScreenTypeOf(proc.uiState, pb::kShells[shellIndex].screenStyle);
        for (int i = 0; i < pb::kScreenTypeCount; ++i) scr.addItem(2000 + i, pb::kScreenTypes[i], true, i == cur);
        menu.addSubMenu("Change Screen", scr);
        menu.addSeparator();
    }

    // Add parts submenu
    juce::PopupMenu add, parts;
    parts.addItem(1, "Dial %");
    parts.addItem(2, "Slider (follows module ratio)");
    parts.addItem(3, "Button toggle");
    parts.addItem(5, "Key (MIDI)");
    parts.addItem(6, "Sound (one sample)");
    add.addSubMenu("Part", parts);

    // Modular pieces submenu (Timer, Randomizer, etc.)
    juce::PopupMenu modParts;
    for (int i = 0; i < kt::kModPieceCount; ++i)
        modParts.addItem(6000 + i, juce::String(kt::kModPieces[i].name) + "  -  " + kt::kModPieces[i].quirk);
    add.addSubMenu("Modular Parts", modParts);

    // FX by family
    for (int fam = 0; fam < kt::kFxFamilyCount; ++fam)
    {
        juce::PopupMenu famMenu;
        for (int i = 0; i < kt::kFxCount; ++i)
            if (kt::kFx[i].family == fam)
                famMenu.addItem(1000 + i, kt::kFx[i].name);
        add.addSubMenu(kt::kFxFamilyNames[fam], famMenu);
    }
    // Chain break and master mix
    add.addSeparator();
    add.addItem(1100, "Chain Break");
    add.addItem(1101, "Master Mix");
    menu.addSubMenu("Add", add);

    // Manual wire: left-click select source, right-click target → Put wire Into
    if (hasSelection && selectedChainWidget >= 0)
    {
        const int clickHost = findWidgetAtShellSlot(slot);
        if (clickHost >= 0 && clickHost != selectedChainWidget)
        {
            const auto srcName = widgets[selectedChainWidget]->node.getProperty("label").toString();
            const auto dstName = widgets[clickHost]->node.getProperty("label").toString();
            menu.addSectionHeader("Wire");
            menu.addItem(6001, "Put wire Into: " + dstName + "  ←  " + srcName);
            menu.addItem(6002, "Cut wire from " + srcName);
            menu.addSeparator();
        }
        else if (selectedChainWidget >= 0)
        {
            menu.addItem(6002, "Cut wire from selected");
            menu.addSeparator();
        }
    }

    // Swap part (if selection)
    if (hasSelection)
    {
        juce::PopupMenu swap, swapParts;
        swapParts.addItem(3001, "Dial %");
        swapParts.addItem(3002, "Slider (follows module ratio)");
        swapParts.addItem(3003, "Button toggle");
        swapParts.addItem(3005, "Key (MIDI)");
        swapParts.addItem(3006, "Sound (one sample)");
        swap.addSubMenu("Part", swapParts);
        for (int fam = 0; fam < kt::kFxFamilyCount; ++fam)
        {
            juce::PopupMenu famMenu;
            for (int i = 0; i < kt::kFxCount; ++i)
                if (kt::kFx[i].family == fam)
                    famMenu.addItem(4000 + i, kt::kFx[i].name);
            swap.addSubMenu(kt::kFxFamilyNames[fam], famMenu);
        }
        menu.addSubMenu("Swap Part", swap);
        menu.addItem(7, "FX EDIT");
        menu.addItem(8, "Remove");
    }

    menu.addSeparator();

    // Theme submenu
    juce::PopupMenu themeMenu;
    for (int i = 0; i < kt::kThemeCount; ++i)
        themeMenu.addItem(5000 + i, kt::kThemes[i].name, true, playgroundTheme.id == kt::kThemes[i].id);
    menu.addSubMenu("Theme", themeMenu);

    // Aspect ratio submenu
    juce::PopupMenu arMenu;
    arMenu.addItem(5101, "4:5 Portrait", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Portrait45);
    arMenu.addItem(5102, "1:1 Square", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Square11);
    arMenu.addItem(5103, "5:4 Landscape", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Landscape54);
    arMenu.addItem(5104, "Freeform", true, machineDesign.playgroundMode == MachineDesign::PlaygroundMode::Freeform);
    menu.addSubMenu("Aspect Ratio", arMenu);

    // Templates removed — free playground only.

    menu.addSeparator();

    // Machine operations
    menu.addItem(5200, "Randomize Machine");
    menu.addItem(5206, "Randomize Parts");
    menu.addItem(5205, "Undo");
    menu.addSeparator();
    menu.addItem(5201, "Load WAV");
    menu.addItem(5202, "Save Build");
    menu.addItem(5203, "Publish");
    menu.addSeparator();
    menu.addItem(5204, "Plugin View");
    menu.addSeparator();
    menu.addItem(5999, "Guide");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ screenPos.x, screenPos.y, 1, 1 }),
        [this, safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this), slot, local, hasSelection, selBay](int result)
        {
            if (safeEd == nullptr || safeEd->editorClosing) return;
            if (result == 0) return;
            if (result == 5999) { safeEd->showBuilderGuide(); return; }
            // Screen type change
            if (result >= 2000 && result < 2000 + pb::kScreenTypeCount)
            {
                for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
                {
                    auto child = proc.uiState.getChild(i);
                    if (child.hasType("w") && child.getProperty("kind").toString() == "board")
                        child.setProperty("screenType", result - 2000, nullptr);
                }
                panel.screenType = result - 2000;
                for (auto* w : widgets) w->repaint();
                panel.repaint();
                status.setText(juce::String("Screen changed to ") + pb::kScreenTypes[result - 2000], juce::dontSendNotification);
                return;
            }
            // Theme change
            if (result >= 5000 && result < 5000 + kt::kThemeCount) { const auto& tid = kt::kThemes[result - 5000].id; applyPlaygroundTheme(tid); applyTheme(tid); return; }
            // Aspect ratio
            if (result == 5101) { startNewMachine(0); return; }
            if (result == 5102) { startNewMachine(1); return; }
            if (result == 5103) { startNewMachine(2); return; }
            if (result == 5104) { startNewMachine(3); return; }
            // Machine ops
            if (result == 5200) { randomizeMachine(); return; }
            if (result == 5201) { loadWav(); return; }
            if (result == 5202) { saveLocal(); return; }
            if (result == 5203) { if (token.isEmpty()) status.setText("Sign in to DreamShare first to publish.", juce::dontSendNotification); else publish(); return; }
            if (result == 5204) { setPluginView(true); return; }
            if (result == 5205) { undoLast(); return; }
            if (result == 5206) { randomizeTemplate(); return; }
            // Chain break / master mix
            if (result == 1100) { pendingSpecial = true; pendingSpecialType = KyotoAudioProcessor::kBreakType; pendingLabel = "CHAIN BREAK"; armedStyle = "dial"; placing = true; status.setText("Break is a knob part - click a glowing knob bay.", juce::dontSendNotification); panel.placing = true; panel.armedStyle = armedStyle; syncPanelMouse(); panel.repaint(); return; }
            if (result == 1101) { pendingSpecial = true; pendingSpecialType = KyotoAudioProcessor::kMixType; pendingLabel = "MASTER MIX"; armedStyle = "fader"; placing = true; status.setText("Mix is a fader part - click a glowing fader bay.", juce::dontSendNotification); panel.placing = true; panel.armedStyle = armedStyle; syncPanelMouse(); panel.repaint(); return; }
            // Modular pieces
            if (result >= 6000 && result < 6000 + kt::kModPieceCount)
            {
                const int pieceIdx = result - 6000;
                const auto& piece = kt::kModPieces[pieceIdx];
                pendingPiece = &piece;
                {
                    const juce::String pid (piece.id);
                    armedStyle = (pid == "flip" || pid == "lfo" || pid == "envelope") ? "fader" : "dial";
                }
                pendingFx = fxBrowser ? fxBrowser->getSelectedFx() : 0;
                pendingLabel = piece.name;
                pendingSpecial = false;
                placing = true;
                panel.placing = true;
                panel.armedStyle = armedStyle;
                syncPanelMouse();
                status.setText(juce::String(piece.name) + " armed (" + piece.quirk + "). Click a glowing bay.", juce::dontSendNotification);
                panel.repaint();
                return;
            }
            if (result == 7) { editEffectPopup(selectedChainWidget); return; }
            if (result == 8) { removeSelectedChainStep(); return; }
            if (result == 6001)
            {
                const int clickHost = findWidgetAtShellSlot(slot);
                if (clickHost >= 0 && selectedChainWidget >= 0)
                    putWireInto(selectedChainWidget, clickHost);
                return;
            }
            if (result == 6002)
            {
                if (selectedChainWidget >= 0) clearWireFrom(selectedChainWidget);
                return;
            }
            const char* kinds[] = { "", "dial", "slider", "button", "wave", "key", "sound" };
            // Swap: replace the part already in the selected bay, keeping its place and grid size.
            if (hasSelection && selBay >= 0)
            {
                if (result >= 3000 && result <= 3006)
                {
                    const juce::String kind = kinds[result - 3000];
                    const bool control = kind == "dial" || kind == "slider" || kind == "button";
                    swapPartInBay(selBay, kind, control ? (fxBrowser ? fxBrowser->getSelectedFx() : 0) : -1, kind);
                    return;
                }
                if (result >= 4000 && result < 4000 + kt::kFxCount)
                {
                    const int fx = result - 4000;
                    swapPartInBay(selBay, "dial", fx, kt::kFx[fx].name);
                    return;
                }
            }
            // Right-clicking anywhere works: if the clicked bay is taken or does not fit, the nearest free fitting bay is used.
            if (result >= 1 && result <= 6)
            {
                const juce::String kind = kinds[result];
                const int bay = nearestFreeBay(kind, local, slot);
                if (bay < 0) { status.setText("No free bay takes a " + kind + ".", juce::dontSendNotification); return; }
                placeKindInSlot(kind, bay, 0, kind);
            }
            else if (result >= 1000 && result < 1000 + kt::kFxCount)
            {
                const int fx = result - 1000;
                juce::String kind = "dial";
                int bay = nearestFreeBay("dial", local, slot);
                if (bay < 0) { kind = "slider"; bay = nearestFreeBay("slider", local, slot); }
                if (bay < 0) { status.setText("No free knob or fader bay left - remove a part or pick a bigger template.", juce::dontSendNotification); return; }
                placeKindInSlot(kind, bay, fx, kt::kFx[fx].name);
            }
        });
}

void KyotoAudioProcessorEditor::editEffectPopup(int widgetIndex)
{
    if (widgetIndex < 0 || widgetIndex >= widgets.size()) return;
    auto node = widgets[widgetIndex]->node;
    const int dsp = (int) node.getProperty("slot", -1);
    if (dsp < 0 || dsp >= proc.slotCount()) { status.setText("This part has no effect slot to edit.", juce::dontSendNotification); return; }
    const auto prefix = "s" + juce::String(dsp + 1).paddedLeft('0', 2);
    const int type = (int) proc.apvts.getRawParameterValue(prefix + "type")->load();
    const int fam = (type >= 0 && type < kt::kFxCount) ? kt::kFx[type].family : 0;
    const juce::String fxName = (type >= 0 && type < kt::kFxCount) ? kt::kFx[type].name : juce::String("Effect");

    // Family-specific knob labels (mirrors the former FX Builder knobs).
    juce::String amountLabel = "Amount", toneLabel = "Tone", motionLabel = "Motion", mixLabel = "Mix", shapeLabel = "Shape";
    if (fam == 0) { amountLabel = "Time"; motionLabel = "Feedback"; shapeLabel = "Spread"; }
    else if (fam == 1) { amountLabel = "Size"; motionLabel = "Decay"; shapeLabel = "Diffusion"; }
    else if (fam == 2 || fam == 3) { amountLabel = "Depth"; toneLabel = "Color"; motionLabel = "Rate"; shapeLabel = "Width"; }
    else if (fam == 4) { amountLabel = "Cutoff"; toneLabel = "Resonance"; motionLabel = "Sweep"; shapeLabel = "Slope"; }
    else if (fam == 5) { amountLabel = "Drive"; motionLabel = "Bias"; }
    else if (fam == 6) { amountLabel = "Thresh"; toneLabel = "Ratio"; motionLabel = "Attack"; shapeLabel = "Release"; }

    auto readParam = [&](const char* suffix) { return (double) proc.apvts.getRawParameterValue(prefix + suffix)->load(); };
    const double curAmount = readParam("amt");
    const double curTone = readParam("tone");
    const double curMotion = readParam("mot");
    const double curMix = readParam("mix");
    const double curShape = readParam("shp");
    const double curOver = proc.getSlotOvermax(dsp);

    auto* win = new juce::AlertWindow("FX EDIT  -  " + fxName,
        "Fine-tune this effect. Values are %. Overmax is allowed (0-200).", juce::AlertWindow::NoIcon);
    win->addTextEditor("amount", juce::String(juce::roundToInt(curAmount * 100.0)), amountLabel + " %");
    win->addTextEditor("tone", juce::String(juce::roundToInt(curTone * 100.0)), toneLabel + " %");
    win->addTextEditor("motion", juce::String(juce::roundToInt(curMotion * 100.0)), motionLabel + " %");
    win->addTextEditor("mix", juce::String(juce::roundToInt(curMix * 100.0)), mixLabel + " %");
    win->addTextEditor("shape", juce::String(juce::roundToInt(curShape * 100.0)), shapeLabel + " %");
    win->addTextEditor("overmax", juce::String(juce::roundToInt(curOver * 100.0)), "Overmax % (drive)");
    win->addButton("Apply", 1);
    win->addButton("Close", 0);
    win->enterModalState(true, juce::ModalCallbackFunction::create([this, safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this), win, dsp, prefix, node](int code)
    {
        if (safeEd == nullptr || safeEd->editorClosing) { if (win != nullptr) delete win; return; }
        if (code == 1 && dsp >= 0)
        {
            auto read = [&](const char* id, double fallback) { const auto t = win->getTextEditorContents(id); return t.getDoubleValue() > 0.0 || t == "0" ? t.getDoubleValue() : fallback; };
            const double amount = juce::jlimit(0.0, 100.0, read("amount", 50.0)) / 100.0;
            const double tone = juce::jlimit(0.0, 100.0, read("tone", 50.0)) / 100.0;
            const double motion = juce::jlimit(0.0, 100.0, read("motion", 35.0)) / 100.0;
            const double mix = juce::jlimit(0.0, 100.0, read("mix", 80.0)) / 100.0;
            const double shape = juce::jlimit(0.0, 100.0, read("shape", 50.0)) / 100.0;
            const double over = juce::jlimit(0.25, 4.0, read("overmax", 100.0) / 100.0);
            auto setOver = [&](const char* id, double value)
            {
                if (auto* param = proc.apvts.getParameter(prefix + id))
                    param->setValueNotifyingHost(param->convertTo0to1((float) juce::jlimit(0.0, 1.0, value)));
            };
            setOver("amt", amount);
            setOver("tone", tone);
            setOver("mot", motion);
            setOver("mix", mix);
            setOver("shp", shape);
            proc.setSlotOvermax(dsp, (float) over);
            auto nodeCopy = node;
            nodeCopy.setProperty("overmax", over, nullptr);
            status.setText("FX edited  -  " + juce::String(juce::roundToInt(amount * 100.0)) + "/"
                           + juce::String(juce::roundToInt(tone * 100.0)) + "/" + juce::String(juce::roundToInt(motion * 100.0))
                           + "/" + juce::String(juce::roundToInt(mix * 100.0)) + "/" + juce::String(juce::roundToInt(shape * 100.0))
                           + "  overmax " + juce::String(juce::roundToInt(over * 100.0)) + "%", juce::dontSendNotification);
            repaint();
        }
        delete win;
    }), true);
}

bool KyotoAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (tab == 0 && loggedIn) return files.size() > 0; // DreamShare: any file can be attached to chat / a thread
    for (auto& f : files)
    {
        const auto ext = f.fromLastOccurrenceOf(".", false, false).toLowerCase();
        if (ext == "wav" || ext == "aiff" || ext == "aif" || ext == "flac" || ext == "mp3" || ext == "ogg") return true;
    }
    return false;
}

void KyotoAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    dragTarget = 0;
    repaint();
    if (tab == 0 && loggedIn)
    {
        const int target = dropTargetAt(x, y);
        if (target == 0)
        {
            status.setText("Drop files on the thread board (images, .zip, audio) or on the chat on the left.", juce::dontSendNotification);
            return;
        }
        if (files.size() > 0) stageAttachment(juce::File(files[0]), target);
        if (files.size() > 1) status.setText("One file per message - attached " + juce::File(files[0]).getFileName(), juce::dontSendNotification);
        return;
    }
    for (auto& f : files)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(juce::File(f)));
        if (reader == nullptr) continue;
        const int n = (int) juce::jmin<juce::int64>(reader->lengthInSamples, 48000 * 30);
        juce::AudioBuffer<float> buf(1, n);
        reader->read(&buf, 0, n, 0, true, true);
        proc.loadSample(buf, reader->sampleRate);
        bool have = false;
        for (auto* w : widgets) if (w->kind == CanvasWidget::Kind::Sound) have = true;
        if (! have)
        {
            const auto& shell = pb::kShells[shellIndex];
            for (int s = 0; s < shell.slotCount; ++s)
                if (! slotOccupied(s) && shell.slots[s].kind != pb::SlotKind::Board)
                {
                    placeKindInSlot("sound", s, -1, "Sound");
                    break;
                }
        }
        status.setText("Sound sample loaded: " + juce::File(f).getFileName(), juce::dontSendNotification);
        return;
    }
}

int KyotoAudioProcessorEditor::dropTargetAt(int x, int y) const
{
    if (tab != 0 || ! loggedIn) return 0;
    const int railRight = catalogView.getX() - 4;
    if (x < railRight && y >= railChatBtn.getY()) return 1;      // the whole chat column
    if (x >= railRight && threadOpen && y >= catalogView.getY()) return 2; // an open thread
    return 0;
}

void KyotoAudioProcessorEditor::fileDragEnter(const juce::StringArray&, int x, int y)
{
    const int t = dropTargetAt(x, y);
    if (t != dragTarget) { dragTarget = t; repaint(); }
}

void KyotoAudioProcessorEditor::fileDragMove(const juce::StringArray&, int x, int y)
{
    const int t = dropTargetAt(x, y);
    if (t != dragTarget) { dragTarget = t; repaint(); }
}

void KyotoAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
    if (dragTarget != 0) { dragTarget = 0; repaint(); }
}

void KyotoAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    if (dragTarget == 0 || tab != 0) return;
    juce::Rectangle<int> area = dragTarget == 1
        ? juce::Rectangle<int>(chatView.getX(), railChatBtn.getY(), chatView.getWidth(), juce::jmax(40, msgBox.getBottom() - railChatBtn.getY()))
        : catalogView.getBounds();
    auto r = area.toFloat().reduced(2.f);
    g.setColour(kt::c(theme.bg).withAlpha(0.72f));
    g.fillRoundedRectangle(r, 12.f);
    g.setColour(kt::c(theme.accent).withAlpha(0.20f));
    g.fillRoundedRectangle(r, 12.f);
    g.setColour(kt::c(theme.accent));
    g.drawRoundedRectangle(r.reduced(3.f), 10.f, 2.5f);
    g.setFont(kt::dsFont(theme, 15.f, true));
    g.drawFittedText(dragTarget == 1 ? "DROP TO SEND IN LIVE CHAT" : "DROP TO ATTACH TO YOUR COMMENT",
                     area.reduced(18), juce::Justification::centred, 3);
}

void KyotoAudioProcessorEditor::stageAttachment(const juce::File& file, int target)
{
    if (! file.existsAsFile()) { status.setText("That is not a file I can attach", juce::dontSendNotification); return; }
    if (file.getSize() > kt::kAttachMaxBytes)
    {
        status.setText("Too big to share: " + kt::humanBytes(file.getSize()) + " (limit " + kt::humanBytes(kt::kAttachMaxBytes) + ")", juce::dontSendNotification);
        return;
    }
    pendingAttach = file;
    pendingAttachTarget = target;
    if (target == 1 && railMode != 0) setRailMode(0);
    attachChip.setButtonText("FILE  " + kt::cleanAttachName(file.getFileName()) + "  (" + kt::humanBytes(file.getSize()) + ")   x");
    showTab(tab);
    resized();
    status.setText(target == 1 ? "File ready - write a message (optional) and press SEND" : "File ready - write a comment (optional) and press COMMENT", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::clearAttachment()
{
    pendingAttach = juce::File();
    pendingAttachTarget = 0;
    attachChip.setVisible(false);
    resized();
}

void KyotoAudioProcessorEditor::sendWithAttachment(int target, const juce::String& text)
{
    const auto file = pendingAttach;
    if (! file.existsAsFile() || token.isEmpty()) { clearAttachment(); return; }
    const auto tokenCopy = token, threadId = selectedThreadId;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    status.setText("Uploading " + file.getFileName() + " (" + kt::humanBytes(file.getSize()) + ")...", juce::dontSendNotification);
    attachChip.setEnabled(false);
    sendBtn.setEnabled(false); utilityGoBtn.setEnabled(false);
    std::thread([safe, tokenCopy, threadId, file, target, text] {
        kt::AttachRef ref;
        juce::String err;
        kt::DreamResult r;
        if (kt::uploadAttachment(tokenCopy, file, ref, err))
        {
            const auto typed = text.trim().substring(0, target == 1 ? 300 : 380);
            const auto msg = (typed + " " + kt::attachToken(ref)).trim();
            if (target == 1) r = kt::sendChat(tokenCopy, msg);
            else
            {
                auto* o = new juce::DynamicObject();
                o->setProperty("threadId", threadId);
                o->setProperty("text", msg);
                r = kt::postAction("comment", juce::var(o), tokenCopy);
            }
        }
        else r.error = err;
        juce::MessageManager::callAsync([safe, r, target] {
            if (safe == nullptr) return;
            safe->attachChip.setEnabled(true);
            safe->sendBtn.setEnabled(true); safe->utilityGoBtn.setEnabled(true);
            if (! r.ok)
            {
                safe->status.setText("Attach failed: " + (r.error.isEmpty() ? juce::String("unknown error") : r.error), juce::dontSendNotification);
                return; // keep the file staged so it can be retried
            }
            safe->clearAttachment();
            if (target == 1) { safe->msgBox.clear(); safe->scrollChatOnRefresh = true; }
            else { safe->utilityBox.clear(); safe->scrollChatOnRefresh = false; }
            safe->refreshFeed();
            safe->status.setText("Sent with file", juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::saveAttachmentAs(const kt::AttachRef& ref)
{
    if (! ref.valid() || token.isEmpty()) return;
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    auto chooser = std::make_shared<juce::FileChooser>("Save " + ref.name,
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(ref.name), "*");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe, chooser, ref, tokenCopy](const juce::FileChooser& fc) {
            const auto dest = fc.getResult();
            if (dest == juce::File() || safe == nullptr) return;
            safe->status.setText("Downloading " + ref.name + "...", juce::dontSendNotification);
            std::thread([safe, ref, tokenCopy, dest] {
                juce::String err;
                const bool ok = kt::downloadAttachment(tokenCopy, ref, dest, err);
                juce::MessageManager::callAsync([safe, ok, err, dest] {
                    if (safe == nullptr) return;
                    safe->status.setText(ok ? "Saved " + dest.getFileName() : "Download failed: " + err, juce::dontSendNotification);
                });
            }).detach();
        });
}

void KyotoAudioProcessorEditor::undoLast()
{
    if (undoStack.empty()) { status.setText("Nothing to undo.", juce::dontSendNotification); return; }
    auto snapshot = std::move(undoStack.back());
    undoStack.pop_back();
    restoreSnapshot(snapshot);
    status.setText("Undid last builder change.", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::removeSelectedChainStep()
{
    if (selectedChainWidget < 0 || selectedChainWidget >= widgets.size()) { status.setText("Select a control first.", juce::dontSendNotification); return; }
    auto node = widgets[selectedChainWidget]->node;
    if (node.getProperty("kind").toString() == "board") { status.setText("The motherboard stays. It is the start of the chain.", juce::dontSendNotification); return; }
    captureSnapshot();
    {
        // Children of the removed part plug into its parent, so the chain stays joined.
        const int removedBay = (int) node.getProperty("shellSlot", -1);
        const int newParent = (int) node.getProperty("parent", 0);
        for (auto* w : widgets)
            if (w->node != node && (int) w->node.getProperty("parent", 0) == removedBay)
                w->node.setProperty("parent", newParent, nullptr);
    }
    const int firstSlot = (int)node.getProperty("slot", -1);
    const int count = juce::jmax(1, (int)node.getProperty("slotCount", 1));
    if (firstSlot >= 0)
        for (int i=firstSlot; i<juce::jmin(proc.slotCount(), firstSlot+count); ++i)
            if (auto* on=proc.apvts.getParameter("s"+juce::String(i+1).paddedLeft('0',2)+"on")) on->setValueNotifyingHost(0.f);
    proc.uiState.removeChild(node, nullptr);
    selectedChainWidget = -1;
    rebuildCanvas();
    reflowSeries();
    status.setText("Removed chain control. Layout refit. UNDO is available.", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::removeSelectedFxStep()
{
    if (selectedFxStep < 0 || selectedFxStep >= fxStack.getNumChildren()) { status.setText("Select an FX step first.", juce::dontSendNotification); return; }
    captureSnapshot();
    fxStack.removeChild(selectedFxStep, nullptr);
    selectedFxStep = fxStack.getNumChildren() > 0 ? juce::jmin(selectedFxStep, fxStack.getNumChildren()-1) : -1;
    if (selectedFxStep >= 0) selectFxStep(selectedFxStep);
    stackLabel.setText(fxStack.getNumChildren() > 0 ? "Stack " + juce::String(fxStack.getNumChildren()) + " / 16  -  custom effect lab" : "Empty effect  -  ready for a new build", juce::dontSendNotification);
    repaint();
}

void KyotoAudioProcessorEditor::addSeriesStep()
{
    const int kindId = kindBox.getSelectedId();
    juce::String kind = kindId == 2 ? "slider" : kindId == 3 ? "key" : kindId == 4 ? "wave" : "dial";
    if (kindId == 5)
    {
        // A custom catalog selection is loaded into FX Builder when clicked; the preset shelf
        // also exposes saved custom effects for placing them as one chain stage.
        auto name = presetBox.getText().trim().fromFirstOccurrenceOf("-", false, false).trim();
        auto file = effectDir().getChildFile(name + ".json");
        if (!file.existsAsFile())
        {
            status.setText("Choose a saved custom effect from the preset shelf first.", juce::dontSendNotification);
            return;
        }
        auto parsed = juce::JSON::parse(file.loadFileAsString());
        auto* obj = parsed.getDynamicObject();
        auto* steps = obj != nullptr ? obj->getProperty("steps").getArray() : nullptr;
        if (steps == nullptr || steps->isEmpty()) { status.setText("Custom effect has no steps.", juce::dontSendNotification); return; }
        juce::Rectangle<int> room;
        if (! findAutoCell(widgets.size(), "stack", room)) { status.setText("No room left. Build a custom FX to keep the chain compact.", juce::dontSendNotification); return; }
        captureSnapshot();
        int first = -1;
        for (auto& value : *steps)
        {
            auto* src = value.getDynamicObject();
            if (!src) continue;
            int slot = -1;
            for (int i = 0; i < proc.slotCount(); ++i)
                if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on && on->getValue() < 0.5f) { slot = i; break; }
            if (slot < 0) { status.setText("Not enough DSP slots for this custom effect.", juce::dontSendNotification); undoLast(); return; }
            if (first < 0) first = slot;
            const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
            auto setP = [&](const juce::String& key, float v) { if (auto* p = proc.apvts.getParameter(prefix + key)) p->setValueNotifyingHost(p->convertTo0to1(v)); };
            setP("type", (float)(int)propertyOr(src, "fx", 0)); setP("amt", (float)propertyOr(src, "amount", 0.5)); setP("tone", (float)propertyOr(src, "tone", 0.5)); setP("mot", (float)propertyOr(src, "motion", 0.35)); setP("mix", (float)propertyOr(src, "mix", 0.4)); setP("shp", (float)propertyOr(src, "shape", 0.5));
            if (auto* p = proc.apvts.getParameter(prefix + "on")) p->setValueNotifyingHost(1.f);
        }
        auto node = juce::ValueTree("w"); node.setProperty("slot", first, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", name, nullptr); node.setProperty("kind", "stack", nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", (int)steps->size(), nullptr); proc.uiState.appendChild(node, nullptr);
        rebuildCanvas(); status.setText("Added custom effect: " + name, juce::dontSendNotification); return;
    }

    juce::Rectangle<int> room;
    if (! findAutoCell(widgets.size(), kind, room))
    {
        status.setText("No safe room left. Use FX Builder to build a custom effect.", juce::dontSendNotification);
        return;
    }
    if (kind == "key")
    {
        captureSnapshot();
        auto node = juce::ValueTree("w"); node.setProperty("slot", -1, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", "C" + juce::String(widgets.size() % 8 + 3), nullptr); node.setProperty("kind", "key", nullptr); node.setProperty("note", 60 + widgets.size(), nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", 1, nullptr); proc.uiState.appendChild(node, nullptr);
    }
    else if (kind == "wave")
    {
        captureSnapshot();
        auto node = juce::ValueTree("w"); node.setProperty("slot", -1, nullptr); node.setProperty("param", "amt", nullptr); node.setProperty("label", "WAV", nullptr); node.setProperty("kind", "wave", nullptr); node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", 1, nullptr); proc.uiState.appendChild(node, nullptr);
    }
    else
    {
        const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
        int slot = -1;
        for (int i = 0; i < proc.slotCount(); ++i)
            if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on && on->getValue() < 0.5f) { slot = i; break; }
        if (slot < 0) { status.setText("DSP slots are full - build a custom FX instead.", juce::dontSendNotification); return; }
        captureSnapshot();
        const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
        if (auto* t = proc.apvts.getParameter(prefix + "type")) t->setValueNotifyingHost(t->convertTo0to1((float)type));
        if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(1.f);
        auto setValue = [this, &prefix](const juce::String& suffix, float value) { if (auto* p = proc.apvts.getParameter(prefix + suffix)) p->setValueNotifyingHost(p->convertTo0to1(value)); };
        setValue("amt", (float)fxAmount.getValue()); setValue("tone", (float)fxTone.getValue()); setValue("mot", (float)fxMotion.getValue()); setValue("mix", (float)fxMix.getValue()); setValue("shp", (float)fxShape.getValue());
        auto node = juce::ValueTree("w"); node.setProperty("slot", slot, nullptr);
        const int paramId = juce::jlimit(0, kt::kControlParamCount - 1, paramBox.getSelectedId() - 1);
        node.setProperty("param", kt::kControlParams[paramId].token, nullptr);
        node.setProperty("label", paramId == 0 ? juce::String(kt::kFx[type].name) : juce::String(kt::kFx[type].name) + " " + kt::kControlParams[paramId].name, nullptr);
        node.setProperty("kind", kind, nullptr);
        const int pieceId = pieceBox.getSelectedId();
        if (pieceId >= 2 && pieceId - 2 < kt::kModPieceCount)
        {
            node.setProperty("skin", kt::kModPieces[pieceId - 2].skin, nullptr);
            node.setProperty("quirk", kt::kModPieces[pieceId - 2].id, nullptr);
        }
        node.setProperty("series", proc.uiState.getNumChildren(), nullptr); node.setProperty("slotCount", 1, nullptr); proc.uiState.appendChild(node, nullptr);
    }
    rebuildCanvas();
    status.setText("Added safely - " + juce::String(widgets.size()) + " control" + (widgets.size() == 1 ? "" : "s"), juce::dontSendNotification);
}

juce::File KyotoAudioProcessorEditor::sessionFile() const
{
    auto f = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("KYOTRIPPAH");
    f.createDirectory();
    return f.getChildFile("session.json");
}
juce::File KyotoAudioProcessorEditor::moduleDir() const
{
    auto dir = sessionFile().getParentDirectory().getChildFile(proc.isFx() ? "fx" : "kyoto");
    dir.createDirectory();
    return dir;
}
juce::File KyotoAudioProcessorEditor::effectDir() const
{
    auto dir = sessionFile().getParentDirectory().getChildFile("effects");
    dir.createDirectory();
    return dir;
}

void KyotoAudioProcessorEditor::login()
{
    const auto user = userBox.getText().trim();
    const auto pass = passBox.getText();
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, user, pass] {
        auto r = kt::login(user, pass);
        juce::MessageManager::callAsync([safe, r, user] {
            if (safe == nullptr) return;
            if (!r.ok || r.token.isEmpty()) { safe->status.setText(r.error.isEmpty() ? "Login failed" : r.error, juce::dontSendNotification); return; }
            safe->token = r.token; safe->account = r.user.isNotEmpty() ? r.user : user;
            safe->isAdmin = (r.role == "super" || safe->account.equalsIgnoreCase("Trippah") || safe->account.equalsIgnoreCase("Goonr"));
            auto* o = new juce::DynamicObject(); o->setProperty("user", safe->account); o->setProperty("token", safe->token); o->setProperty("admin", safe->isAdmin);
            safe->sessionFile().replaceWithText(juce::JSON::toString(juce::var(o)));
            safe->setLoggedIn(true); safe->status.setText("Signed in as " + safe->account, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::logout()
{
    sessionFile().deleteFile();
    token.clear();
    account.clear();
    setLoggedIn(false);
    status.setText("Logged out", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::sendChat()
{
    scrollChatOnRefresh = true;
    auto text = msgBox.getText().trim();
    const auto tokenCopy = token;
    if (tokenCopy.isEmpty()) return;
    if (pendingAttach.existsAsFile() && pendingAttachTarget == 1) { sendWithAttachment(1, text); return; }
    if (text.isEmpty()) return;

    // Default: always post into DreamShare live chat.
    // Optional: "/d ..." still posts only to Discord for power users.
    // The Discord bridge bot (not a VST poll loop) mirrors Discord guild traffic into chat with KYTO/TRBN origin tags.
    bool forceDiscord = false;
    if (text.startsWithIgnoreCase("/d ") || text.startsWithIgnoreCase("/discord "))
    {
        forceDiscord = true;
        text = text.fromFirstOccurrenceOf(" ", false, false).trim();
        if (text.isEmpty()) return;
    }

    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    const auto accountCopy = account;

    std::thread([safe, text, tokenCopy, accountCopy, forceDiscord] {
        kt::DreamResult r;
        if (forceDiscord)
            r = kt::sendDiscordMessage(tokenCopy, accountCopy, text);
        else
            r = kt::sendChat(tokenCopy, text); // DreamShare chat is the default path
        juce::MessageManager::callAsync([safe, r, forceDiscord] {
            if (safe == nullptr) return;
            if (!r.ok) safe->status.setText(r.error, juce::dontSendNotification);
            else
            {
                safe->msgBox.clear();
                // Refresh DreamShare feed only — no constant Discord polling from the VST.
                safe->refreshFeed();
                if (forceDiscord)
                    safe->status.setText("Sent to Discord", juce::dontSendNotification);
            }
        });
    }).detach();
}

void KyotoAudioProcessorEditor::chatUtility(const juce::String& selectedAction)
{
    if (token.isEmpty()) return;
    const auto action = selectedAction.trim(), target = utilityBox.getText().trim(), message = msgBox.getText().trim(), tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, action, target, message, tokenCopy] {
        kt::DreamResult r;
        if (action == "dm_list") r = kt::getDM(tokenCopy, target);
        else if (action == "dm_send") r = kt::sendDM(tokenCopy, target, message);
        else if (action == "social_list") r = kt::getSocial(tokenCopy);
        else if (action == "friend_request" || action == "friend_accept" || action == "friend_decline" || action == "friend_remove" || action == "wav_request") r = kt::friendRequest(tokenCopy, action, target);
        else if (action == "react_heart") r = kt::react(tokenCopy, "chat", target, "heart");
        else if (action == "chat_delete") { auto* o=new juce::DynamicObject(); o->setProperty("id",target); r=kt::postAction("chat_delete",juce::var(o),tokenCopy); }
        else if (action == "chat_clear") r=kt::postAction("chat_clear",juce::var(new juce::DynamicObject()),tokenCopy);
        else if (action == "create_thread") { auto* o=new juce::DynamicObject(); o->setProperty("text",message); r=kt::postAction("create_thread",juce::var(o),tokenCopy); }
        else if (action == "comment") { auto* o=new juce::DynamicObject(); o->setProperty("threadId", target.isEmpty()? message : target); o->setProperty("text", message.isEmpty()? target : message); r=kt::postAction("comment",juce::var(o),tokenCopy); }
        else r=kt::postAction(action,juce::var(new juce::DynamicObject()),tokenCopy);
        juce::MessageManager::callAsync([safe, r, action] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->logBox.setText(r.error.isEmpty() ? r.raw : r.error); safe->status.setText("Utility failed: " + r.error, juce::dontSendNotification); return; }
            juce::String out;
            if (action == "list_threads")
            {
                if (auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("threads").getArray() : nullptr)
                    for (int i=0;i<arr->size();++i) if (auto* t=arr->getReference(i).getDynamicObject()) out << "THREAD " << juce::String(i+1) << "  " << t->getProperty("title").toString() << "\n" << t->getProperty("text").toString() << "\nby " << t->getProperty("user").toString() << "\n\n";
            }
            else out = r.raw.isEmpty() ? r.body : r.raw;
            safe->logBox.setText(out.isEmpty() ? r.body : out); safe->renderEffectLinks(out.isEmpty() ? r.body : out);
            if (action == "social_list" || action == "dm_list")
            {
                juce::Array<SocialRail::Person> people;
                auto* obj = r.parsed.getDynamicObject();
                if (obj != nullptr)
                {
                    for (auto key : { "friendsDetailed", "friends", "online", "users", "messages" })
                        if (auto* arr = obj->getProperty(key).getArray())
                            for (auto& item : *arr)
                            {
                                SocialRail::Person person;
                                if (auto* o = item.getDynamicObject())
                                {
                                    person.name = o->getProperty("name").toString();
                                    if (person.name.isEmpty()) person.name = o->getProperty("user").toString();
                                    if (person.name.isEmpty()) person.name = o->getProperty("from").toString();
                                    person.themeId = o->getProperty("theme").toString();
                                    person.detail = o->getProperty("text").toString();
                                }
                                else person.name = item.toString();
                                person.online = juce::String(key) == "online";
                                if (person.detail.isEmpty()) person.detail = key;
                                if (person.name.isNotEmpty()) people.add(person);
                            }
                }
                if (! people.isEmpty()) safe->socialRail.setPeople(people);
            }
            if (action == "create_thread" || action == "comment") { safe->setCenterMode(1); safe->refreshFeed(); }
            safe->status.setText("DreamShare utility complete", juce::dontSendNotification);
            if (action == "chat_list") safe->refreshFeed();
        });
    }).detach();
}

void KyotoAudioProcessorEditor::deleteCatalogId(const juce::String& id)
{
    if (id.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tokenCopy] { auto r=kt::deleteModule(tokenCopy,id); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"CATALOG ITEM REMOVED":"REMOVE FAILED: "+r.error,juce::dontSendNotification); if(r.ok){safe->selectedCatalogId.clear();safe->refreshCurrentCenter();} }); }).detach();
}

void KyotoAudioProcessorEditor::clearEffectLinks()
{
    feedEffectButtons.clear();
}

void KyotoAudioProcessorEditor::renderEffectLinks(const juce::String& text)
{
    clearEffectLinks();
    const juce::String marker = "[KYOTRIPPAH_EFFECT:";
    int pos = 0;
    int count = 0;
    while (count < 8)
    {
        const int start = text.indexOf(pos, marker);
        if (start < 0) break;
        const int idStart = start + marker.length();
        const int end = text.indexOfChar(idStart, ']');
        if (end <= idStart) { pos = idStart; continue; }
        const auto id = text.substring(idStart, end).trim();
        if (id.isEmpty()) { pos = end + 1; continue; }
        auto tail = text.substring(end + 1).upToFirstOccurrenceOf("\n", false, false).trim();
        if (tail.isEmpty()) tail = "Shared custom effect";
        auto* button = feedEffectButtons.add(new juce::TextButton("LOAD FX  -  " + tail));
        button->onClick = [this, id] { loadCatalogId(id, {}); };
        addAndMakeVisible(button);
        ++count;
        pos = end + 1;
    }
    resized();
}


void KyotoAudioProcessorEditor::setCenterMode(int mode)
{
    if (threadOpen)
    {
        // Switching tabs leaves the open thread (otherwise the thread view and the board fight over the viewport).
        threadOpen = false;
        selectedThreadId.clear();
        if (pendingAttachTarget == 2) clearAttachment();
    }
    centerMode = juce::jlimit(0, 5, mode);
    if (centerMode == 1) centerMode = 0; // Effects catalog removed
    if (centerMode == 5) rebuildDirectory();
    catalogModeBtn.setToggleState(centerMode == 0, juce::dontSendNotification);
    threadsModeBtn.setToggleState(centerMode == 3, juce::dontSendNotification);
    pluginsTabBtn.setToggleState(centerMode == 0, juce::dontSendNotification);
    effectsTabBtn.setToggleState(centerMode == 1, juce::dontSendNotification);
    myPluginsBtn.setToggleState(centerMode == 2, juce::dontSendNotification);
    pendingBtn.setToggleState(centerMode == 4, juce::dontSendNotification);
    const bool useThreads = centerMode == 3;
    auto* viewed = useThreads ? static_cast<juce::Component*>(&threadHolder)
                              : (centerMode == 5 ? static_cast<juce::Component*>(&directoryHolder)
                                                 : static_cast<juce::Component*>(&catalogHolder));
    catalogView.setViewedComponent(viewed, false);
    if (useThreads) rebuildThreadBoard();
    showTab(tab);
}

void KyotoAudioProcessorEditor::setRailMode(int mode)
{
    railMode = juce::jlimit(0, 2, mode);
    railChatBtn.setToggleState(railMode == 0, juce::dontSendNotification);
    railDiscordBtn.setToggleState(railMode == 2, juce::dontSendNotification);
    railOnlineBtn.setToggleState(railMode == 1, juce::dontSendNotification);
    // SocialRail modes: 0 = chat bubbles, 1 = online/friends. Discord reuses chat bubbles (mode 0).
    socialRail.setMode(railMode == 1 ? 1 : 0);
    socialRail.setShowDirectory(isAdmin);
    if (railMode == 2)
        msgBox.setTextToShowWhenEmpty("Message Discord #general  -  right-click a name to @mention", juce::Colours::grey);
    else
        msgBox.setTextToShowWhenEmpty("Message #dreamshare  -  drop files to attach", juce::Colours::grey);
    showTab(tab);
    resized();
}

void KyotoAudioProcessorEditor::rebuildCenter()
{
    layoutCenterHolder();
}

void KyotoAudioProcessorEditor::layoutCenterHolder()
{
    const int catalogW = juce::jmax(220, catalogView.getWidth() - 18);
    const int gap = 8;
    // Socials: one full-width scrolling column of accounts + DMs + the Discord slot.
    if (centerMode == 5)
    {
        socialDirectory.setBounds(gap, gap, catalogW - gap * 2, juce::jmax(420, socialDirectory.contentHeight()));
        directoryHolder.setSize(catalogW, juce::jmax(catalogView.getHeight(), socialDirectory.contentHeight() + gap * 2));
        return;
    }
    const bool detail = threadOpen && centerMode == 3;
    auto* holder = (centerMode == 3 && ! threadOpen) ? &threadHolder : &catalogHolder;
    if (detail)
    {
        // Open thread: one readable column, every post as tall as its text needs.
        const int w = catalogW - gap * 2;
        int y = gap;
        for (int i = 0; i < holder->getNumChildComponents(); ++i)
        {
            auto* child = holder->getChildComponent(i);
            int h = 96;
            if (auto* card = dynamic_cast<BoardCard*>(child)) h = card->preferredHeight(w);
            child->setBounds(gap, y, w, h);
            y += h + gap;
        }
        holder->setSize(catalogW, juce::jmax(catalogView.getHeight(), y));
        return;
    }
    const int cols = catalogW >= 720 ? 3 : (catalogW >= 440 ? 2 : 1);
    const int cardW = juce::jmax(160, (catalogW - gap * (cols + 1)) / cols);
    const int cardH = (int) std::ceil(104.f + 14.f * kt::dsScale());
    const int n = holder->getNumChildComponents();
    holder->setSize(catalogW, juce::jmax(catalogView.getHeight(), ((n + cols - 1) / juce::jmax(1, cols)) * (cardH + gap) + gap));
    for (int i = 0; i < n; ++i)
    {
        const int col = i % cols, row = i / cols;
        holder->getChildComponent(i)->setBounds(gap + col * (cardW + gap), gap + row * (cardH + gap), cardW, cardH);
    }
}

void KyotoAudioProcessorEditor::applyDsScale()
{
    const float s = kt::dsScale();
    const auto f = juce::Font(14.f * s);
    msgBox.setFont(f);
    utilityBox.setFont(f);
    // sortBox is a ComboBox (sort bar); theme applies via LookAndFeel
    socialRail.setSize(socialRail.getWidth(), socialRail.contentHeight());
    if (threadOpen) rebuildThreadDetail(); else if (centerMode == 3) rebuildThreadBoard();
    resized();
    repaint();
}

void KyotoAudioProcessorEditor::rebuildThreadBoard()
{
    juce::Array<kt::BoardThread> out;
    for (const auto& t : threads)
    {
        kt::BoardThread bt;
        bt.id = t.id; bt.user = t.user; bt.title = t.title; bt.text = t.text; bt.themeId = t.themeId;
        bt.at = t.at; bt.score = t.score;
        for (const auto& cm : t.commentList)
        {
            kt::BoardComment bc;
            bc.id = cm.id; bc.user = cm.user; bc.text = cm.text; bc.themeId = cm.themeId;
            bt.comments.add(bc);
        }
        out.add(bt);
    }
    threadBoard.setTheme(theme);
    threadBoard.setThreads(out);
}


void KyotoAudioProcessorEditor::refreshSocial()
{
    if (token.isEmpty()) return;
    socialRail.setShowDirectory(isAdmin);
    const auto tokenCopy = token;
    const bool admin = isAdmin;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy, admin] {
        auto social = kt::getSocial(tokenCopy);
        auto presence = kt::postAction("presence", juce::var(new juce::DynamicObject()), tokenCopy);
        juce::MessageManager::callAsync([safe, social, presence, admin] {
            if (safe == nullptr) return;
            juce::Array<SocialRail::Person> people;
            auto* root = social.parsed.getDynamicObject();
            auto addArr = [&](const char* key, const juce::String& kind, const juce::String& detail, bool online) {
                if (root == nullptr) return;
                if (auto* arr = root->getProperty(key).getArray())
                    for (auto& item : *arr)
                    {
                        SocialRail::Person person;
                        person.kind = kind;
                        person.detail = detail;
                        person.online = online;
                        if (auto* o = item.getDynamicObject())
                        {
                            person.name = o->getProperty("name").toString();
                            if (person.name.isEmpty()) person.name = o->getProperty("from").toString();
                            person.themeId = o->getProperty("theme").toString();
                            person.requestId = o->getProperty("id").toString();
                            if (o->hasProperty("status")) person.detail = o->getProperty("status").toString();
                        }
                        else person.name = item.toString();
                        if (person.name.isNotEmpty()) people.add(person);
                    }
            };
            addArr("incoming", "invite", "friend invite", false);
            addArr("friendsDetailed", "friend", "friend  -  click for WAV", true);
            if (admin)
            {
                auto* online = presence.parsed.getDynamicObject() ? presence.parsed.getDynamicObject()->getProperty("onlineUsers").getArray() : nullptr;
                if (online != nullptr)
                    for (auto& item : *online)
                    {
                        SocialRail::Person person;
                        person.kind = "active";
                        person.online = true;
                        person.detail = "active now";
                        if (auto* o = item.getDynamicObject()) { person.name = o->getProperty("name").toString(); person.themeId = o->getProperty("theme").toString(); }
                        else person.name = item.toString();
                        if (person.name.isNotEmpty() && person.name != safe->account) people.add(person);
                    }
            }
            safe->setDirectoryData(social.parsed, presence.parsed);
            safe->socialRail.setPeople(people);
            safe->status.setText("Socials updated", juce::dontSendNotification);
            safe->resized();
        });
    }).detach();
}

void KyotoAudioProcessorEditor::rebuildDirectory()
{
    // Entering SOCIALS (or re-laying the column out) pulls a fresh account list + presence.
    if (token.isEmpty())
    {
        juce::Array<SocialDirectory::Row> empty;
        socialDirectory.setData(empty, 0, 0);
        status.setText("Sign in to DreamShare to browse the directory.", juce::dontSendNotification);
        return;
    }
    refreshSocial();
}

void KyotoAudioProcessorEditor::setDirectoryData(const juce::var& socialParsed, const juce::var& presenceParsed)
{
    auto* root = socialParsed.getDynamicObject();

    juce::StringArray friendNames;
    if (root != nullptr)
        if (auto* arr = root->getProperty("friendsDetailed").getArray())
            for (auto& item : *arr)
            {
                if (auto* o = item.getDynamicObject()) friendNames.add(o->getProperty("name").toString());
                else friendNames.add(item.toString());
            }

    juce::StringArray onlineNames, discordNames;
    juce::HashMap<juce::String, juce::String> onlineThemes;
    if (auto* presence = presenceParsed.getDynamicObject())
        if (auto* arr = presence->getProperty("onlineUsers").getArray())
            for (auto& item : *arr)
            {
                juce::String name, themeId;
                bool dis = false;
                if (auto* o = item.getDynamicObject()) { name = o->getProperty("name").toString(); themeId = o->getProperty("theme").toString(); dis = (bool) o->getProperty("dis"); }
                else name = item.toString();
                if (name.isNotEmpty())
                {
                    onlineNames.add(name);
                    if (dis) discordNames.add(name);
                    onlineThemes.set(name.toLowerCase(), themeId);
                }
            }

    juce::Array<SocialDirectory::Row> rows;
    auto already = [&rows](const juce::String& name) {
        for (const auto& r : rows) if (r.name.equalsIgnoreCase(name)) return true;
        return false;
    };
    auto addRow = [&](const juce::String& name, bool onlineHint) {
        if (name.isEmpty() || already(name)) return;
        SocialDirectory::Row r;
        r.name = name;
        r.self = name.equalsIgnoreCase(account);
        r.isFriend = friendNames.contains(name, true);
        r.online = onlineHint || r.self || onlineNames.contains(name, true);
        r.dis = discordNames.contains(name, true);
        const auto themeId = onlineThemes[name.toLowerCase()];
        r.themeId = themeId.isNotEmpty() ? themeId : (r.self ? theme.id : juce::String());
        r.detail = r.self ? "you - signed in"
                 : (r.online && r.isFriend) ? "friend - active now"
                 : r.online ? "active now"
                 : r.isFriend ? "friend"
                 : "member";
        rows.add(r);
    };

    int total = 0;
    if (root != nullptr)
        if (auto* dir = root->getProperty("directory").getArray())
            for (auto& item : *dir)
            {
                const auto name = item.toString();
                if (name.isEmpty()) continue;
                ++total;
                addRow(name, false);
            }
    // Accounts that only appear through live presence still belong in the list.
    for (const auto& name : onlineNames) addRow(name, true);

    socialDirectory.setData(rows, juce::jmax(total, rows.size()), onlineNames.size());
}

void KyotoAudioProcessorEditor::openWavRequest(const juce::String& name)
{
    if (name.isEmpty() || token.isEmpty()) return;
    auto* win = new juce::AlertWindow("WAV request", "Ask " + name + " for a WAV.", juce::AlertWindow::NoIcon);
    win->addTextEditor("note", "", "Note");
    win->addButton("SEND", 1);
    win->addButton("CANCEL", 0);
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    win->enterModalState(true, juce::ModalCallbackFunction::create([safe, tokenCopy, name, win](int result) {
        if (result != 1 || safe == nullptr) return;
        auto* o = new juce::DynamicObject();
        o->setProperty("target", name);
        o->setProperty("note", win->getTextEditorContents("note"));
        std::thread([safe, tokenCopy, body = juce::var(o)] {
            auto r = kt::postAction("wav_request", body, tokenCopy);
            juce::MessageManager::callAsync([safe, r] {
                if (safe == nullptr) return;
                safe->status.setText(r.ok ? "WAV request sent" : "WAV request failed: " + r.error, juce::dontSendNotification);
            });
        }).detach();
    }), true);
}

void KyotoAudioProcessorEditor::openDirectMessage(const juce::String& name)
{
    if (name.isEmpty() || token.isEmpty()) return;
    auto* win = new juce::AlertWindow("DM " + name, "Private message", juce::AlertWindow::NoIcon);
    win->addTextEditor("text", "", "Message");
    win->addButton("SEND", 1);
    win->addButton("CANCEL", 0);
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    win->enterModalState(true, juce::ModalCallbackFunction::create([safe, tokenCopy, name, win](int result) {
        if (result != 1 || safe == nullptr) return;
        const auto text = win->getTextEditorContents("text");
        std::thread([safe, tokenCopy, name, text] {
            auto r = kt::sendDM(tokenCopy, name, text);
            juce::MessageManager::callAsync([safe, r, name] {
                if (safe == nullptr) return;
                safe->status.setText(r.ok ? "DM sent to " + name : "DM failed: " + r.error, juce::dontSendNotification);
            });
        }).detach();
    }), true);
}

void KyotoAudioProcessorEditor::showPersonMenu(const SocialRail::Person& person, juce::Point<int> screenPos)
{
    juce::PopupMenu menu;
    menu.addItem(1, "DM " + person.name);
    if (person.kind == "invite") { menu.addItem(2, "Accept invite"); menu.addItem(3, "Decline invite"); }
    else if (person.kind != "friend") menu.addItem(4, "Add friend");
    if (person.kind == "friend") menu.addItem(5, "Remove friend");
    if (isAdmin) { menu.addSeparator(); menu.addItem(6, "Promote mod"); menu.addItem(7, "Demote mod"); }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
        [this, safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this), person](int choice) {
            if (safeEd == nullptr || safeEd->editorClosing) return;
            if (choice == 1) openDirectMessage(person.name);
            else if (choice == 2) { auto* o = new juce::DynamicObject(); o->setProperty("id", person.requestId); auto rbody = juce::var(o); const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this); std::thread([safe, tokenCopy, rbody]{ auto r = kt::postAction("friend_accept", rbody, tokenCopy); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Invite accepted":r.error, juce::dontSendNotification); safe->refreshSocial(); }); }).detach(); }
            else if (choice == 3) { auto* o = new juce::DynamicObject(); o->setProperty("id", person.requestId); auto rbody = juce::var(o); const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this); std::thread([safe, tokenCopy, rbody]{ auto r = kt::postAction("friend_decline", rbody, tokenCopy); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Invite declined":r.error, juce::dontSendNotification); safe->refreshSocial(); }); }).detach(); }
            else if (choice == 4) { const auto tokenCopy = token; const auto name = person.name; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this); std::thread([safe, tokenCopy, name]{ auto r = kt::friendRequest(tokenCopy, "friend_request", name); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Friend request sent":r.error, juce::dontSendNotification); }); }).detach(); }
            else if (choice == 5) { const auto tokenCopy = token; const auto name = person.name; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this); std::thread([safe, tokenCopy, name]{ auto r = kt::friendRequest(tokenCopy, "friend_remove", name); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Friend removed":r.error, juce::dontSendNotification); safe->refreshSocial(); }); }).detach(); }
            else if (choice == 6 || choice == 7) { auto* o = new juce::DynamicObject(); o->setProperty("target", person.name); auto rbody = juce::var(o); const auto tokenCopy = token; const auto action = choice == 6 ? juce::String("promote_mod") : juce::String("demote_mod"); juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this); std::thread([safe, tokenCopy, rbody, action]{ auto r = kt::postAction(action, rbody, tokenCopy); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Role updated":r.error, juce::dontSendNotification); }); }).detach(); }
        });
}

void KyotoAudioProcessorEditor::showBubbleMenu(const SocialRail::Bubble& bubble, juce::Point<int> screenPos)
{
    juce::PopupMenu menu;
    // Discord bubbles: mention uses the real Discord user id when the worker provides it.
    if (railMode == 2 || bubble.dis)
    {
        menu.addItem(30, "Mention @" + bubble.user);
        menu.addSeparator();
    }
    menu.addItem(1, "Add friend");
    juce::PopupMenu react;
    // ASCII labels only — Windows VST hosts often fail to render emoji glyphs in PopupMenu.
    react.addItem(10, "Heart");
    react.addItem(11, "Fire");
    react.addItem(12, "Laugh");
    react.addItem(13, "Moon");
    react.addItem(14, "100");
    react.addItem(15, "Up");
    react.addItem(16, "Skull");
    react.addItem(17, "Eyes");
    react.addItem(18, "Sparkles");
    react.addItem(19, "Wave");
    menu.addSubMenu("React with emoji", react);
    menu.addSeparator();
    menu.addItem(2, "Reply");
    if (isAdmin) { menu.addSeparator(); menu.addItem(20, "Delete message"); }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
        [this, safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this), bubble](int choice) {
            if (safeEd == nullptr || safeEd->editorClosing) return;
            if (choice == 30)
            {
                // Prefer Discord snowflake mention when available; else plain @name.
                juce::String insert;
                if (bubble.discordId.isNotEmpty())
                    insert = "<@" + bubble.discordId + "> ";
                else
                    insert = "@" + bubble.user + " ";
                msgBox.moveCaretToEnd();
                msgBox.insertTextAtCaret(insert);
                msgBox.grabKeyboardFocus();
                if (railMode != 2)
                    setRailMode(2);
            }
            else if (choice == 1)
            {
                const auto tokenCopy = token; const auto name = bubble.user;
                juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
                std::thread([safe, tokenCopy, name]{ auto r = kt::friendRequest(tokenCopy, "friend_request", name); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Friend request sent":r.error, juce::dontSendNotification); }); }).detach();
            }
            else if (choice >= 10 && choice <= 19)
            {
                const char* emoji[] = { "heart", "fire", "laugh", "moon", "100", "up", "skull", "eyes", "sparkles", "wave" };
                reactTo("chat", bubble.id, emoji[choice - 10]);
            }
            else if (choice == 2)
            {
                msgBox.setText(">>" + bubble.id + " ", false);
                msgBox.grabKeyboardFocus();
            }
            else if (choice == 20)
            {
                auto* o = new juce::DynamicObject(); o->setProperty("id", bubble.id);
                const auto tokenCopy = token; auto body = juce::var(o);
                juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
                std::thread([safe, tokenCopy, body]{ auto r = kt::postAction("chat_delete", body, tokenCopy); juce::MessageManager::callAsync([safe, r]{ if (safe==nullptr) return; safe->status.setText(r.ok?"Message removed":r.error, juce::dontSendNotification); if (r.ok) safe->refreshFeed(); }); }).detach();
            }
        });
}


void KyotoAudioProcessorEditor::openThread(const juce::String& id)
{
    if (centerMode != 3) setCenterMode(3);
    threadBoard.openThreadById(id);
    status.setText("Thread open. Drop files on it to attach them to your reply.", juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::closeThread()
{
    threadBoard.closeThread();
}

void KyotoAudioProcessorEditor::postThreadComment(const juce::String& text)
{
    if (selectedThreadId.isEmpty() || token.isEmpty()) return;
    if (pendingAttach.existsAsFile() && pendingAttachTarget == 2) { sendWithAttachment(2, text); return; }
    if (text.trim().isEmpty()) return;
    auto* o = new juce::DynamicObject();
    o->setProperty("threadId", selectedThreadId);
    o->setProperty("text", text.trim());
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy, body = juce::var(o)] {
        auto r = kt::postAction("comment", body, tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->status.setText("Comment failed: " + r.error, juce::dontSendNotification); return; }
            safe->utilityBox.clear();
            safe->scrollChatOnRefresh = false;
            safe->refreshFeed();
            safe->status.setText("Comment posted", juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::reactTo(const juce::String& kind, const juce::String& id, const juce::String& emoji)
{
    if (id.isEmpty() || token.isEmpty()) return;
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy, kind, id, emoji] {
        auto r = kt::react(tokenCopy, kind, id, emoji);
        juce::MessageManager::callAsync([safe, r, emoji] {
            if (safe == nullptr) return;
            safe->status.setText(r.ok ? "Reacted " + emoji : "React failed: " + r.error, juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::rebuildThreadDetail()
{
    catalogHolder.removeAllChildren();
    const ThreadItem* found = nullptr;
    for (const auto& th : threads) if (th.id == selectedThreadId) { found = &th; break; }
    if (found == nullptr)
    {
        auto* card = new BoardCard();
        card->title = "Thread";
        card->body = "This thread is not in the current feed yet.";
        card->themeId = theme.id;
        catalogHolder.addAndMakeVisible(card);
        return;
    }
    auto* head = new BoardCard();
    head->title = found->title.isEmpty() ? "Thread" : found->title;
    head->meta = found->user + "  -  " + found->themeId;
    head->setBodyRaw(found->text);
    head->grow = true;
    head->onFile = [this](const kt::AttachRef& r) { saveAttachmentAs(r); };
    head->themeId = found->themeId.isEmpty() ? theme.id : found->themeId;
    catalogHolder.addAndMakeVisible(head);
    for (const auto& c : found->commentList)
    {
        auto* card = new BoardCard();
        card->title = c.user;
        card->meta = "comment";
        card->setBodyRaw(c.text);
        card->grow = true;
        card->onFile = [this](const kt::AttachRef& r) { saveAttachmentAs(r); };
        card->themeId = c.themeId.isEmpty() ? theme.id : c.themeId;
        const auto cid = c.id;
        card->onContextMenu = [safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this), cid](juce::Point<int> pos) {
            if (safeEd == nullptr || safeEd->editorClosing) return;
            juce::PopupMenu menu;
            menu.addItem(1, "heart"); menu.addItem(2, "fire"); menu.addItem(3, "laugh"); menu.addItem(4, "moon");
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(pos.x, pos.y, 1, 1)),
                [safeEd, cid](int choice) {
                    if (safeEd == nullptr || safeEd->editorClosing) return;
                    const char* emoji[] = { "heart", "fire", "laugh", "moon" };
                    if (choice >= 1 && choice <= 4) safeEd->reactTo("comment", cid, emoji[choice - 1]);
                });
        };
        catalogHolder.addAndMakeVisible(card);
    }
    resized();
}


void KyotoAudioProcessorEditor::refreshDiscord()
{
    if (token.isEmpty()) return;
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    status.setText("Loading Discord #general...", juce::dontSendNotification);
    std::thread([safe, tokenCopy] {
        auto r = kt::getDiscordMessages(tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            if (!r.ok)
            {
                safe->status.setText(r.error.isEmpty() ? "Discord #general unavailable (set DISCORD_BOT_TOKEN on the worker)" : r.error, juce::dontSendNotification);
                return;
            }
            juce::Array<SocialRail::Bubble> bubbles;
            juce::String log;
            auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("messages").getArray() : nullptr;
            if (arr != nullptr)
            {
                for (auto& item : *arr)
                {
                    if (auto* m = item.getDynamicObject())
                    {
                        SocialRail::Bubble b;
                        b.id = m->getProperty("id").toString();
                        b.user = m->getProperty("user").toString();
                        b.text = m->getProperty("text").toString();
                        b.themeId = "trippah";
                        b.dis = true;
                        b.originTag = m->getProperty("tag").toString();
                        if (b.originTag.isEmpty()) b.originTag = "KYTO";
                        b.discordId = m->getProperty("discordId").toString();
                        if (b.discordId.isEmpty()) b.discordId = m->getProperty("authorId").toString();
                        bubbles.add(b);
                        log << "[DIS] " << b.user << ": " << b.text << "\n";
                    }
                }
            }
            const int viewY = safe->chatView.getViewPositionY();
            const bool nearBottom = safe->socialRail.getHeight() > safe->chatView.getViewHeight()
                && viewY + safe->chatView.getViewHeight() >= safe->socialRail.getHeight() - 36;
            safe->socialRail.setSelfUser(safe->account);
            safe->socialRail.setBubbles(bubbles);
            if (safe->railMode == 2 && (safe->scrollChatOnRefresh || nearBottom))
                safe->chatView.setViewPosition(0, safe->socialRail.getHeight());
            else if (safe->railMode == 2)
                safe->chatView.setViewPosition(0, viewY);
            if (safe->railMode == 2) safe->scrollChatOnRefresh = false;
            safe->status.setText("Discord #general · " + juce::String(bubbles.size()) + " messages", juce::dontSendNotification);
            safe->logBox.setText(log.isEmpty() ? r.body : log);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshFeed()
{
    if (token.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy] {
        auto r = kt::getFeed(tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->clearEffectLinks(); safe->status.setText(r.error.isEmpty()?"DreamShare feed failed":r.error, juce::dontSendNotification); safe->logBox.setText(r.raw); return; }
            juce::String log;
            juce::Array<SocialRail::Bubble> bubbles;
            if (auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("chat").getArray() : nullptr)
                for (auto& item : *arr) if (auto* m=item.getDynamicObject())
                {
                    SocialRail::Bubble b;
                    b.id = m->getProperty("id").toString();
                    b.user = m->getProperty("user").toString();
                    b.text = m->getProperty("text").toString();
                    b.themeId = m->getProperty("theme").toString();
                    if (b.themeId.isEmpty()) b.themeId = "trippah";
                    b.dis = (bool) m->getProperty("dis");
                    b.originTag = m->getProperty("tag").toString();
                    if (b.originTag.isEmpty() && b.dis) b.originTag = "KYTO";
                    bubbles.add(b);
                    log << b.user << ": " << b.text << "\n";
                }
            // Keep the newest messages in view on open, but preserve a reader's
            // position if they have deliberately scrolled into the history.
            const int viewY = safe->chatView.getViewPositionY();
            const bool nearBottom = safe->socialRail.getHeight() > safe->chatView.getViewHeight()
                && viewY + safe->chatView.getViewHeight() >= safe->socialRail.getHeight() - 36;
            safe->socialRail.setSelfUser(safe->account);
            safe->socialRail.setBubbles(bubbles);
            if (safe->railMode == 0 && (safe->scrollChatOnRefresh || nearBottom))
                safe->chatView.setViewPosition(0, safe->socialRail.getHeight());
            else if (safe->railMode == 0)
                safe->chatView.setViewPosition(0, viewY);
            if (safe->railMode == 0) safe->scrollChatOnRefresh = false;
            safe->threads.clear();
            if (auto* arr = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("threads").getArray() : nullptr)
                for (auto& item : *arr) if (auto* t=item.getDynamicObject())
                {
                    KyotoAudioProcessorEditor::ThreadItem th;
                    th.id = t->getProperty("id").toString();
                    th.user = t->getProperty("user").toString();
                    th.title = t->getProperty("title").toString();
                    th.text = t->getProperty("text").toString();
                    th.themeId = t->getProperty("theme").toString();
                    th.at = (juce::int64) t->getProperty("at");
                    if (auto* rx = t->getProperty("reactions").getDynamicObject())
                        for (auto& nv : rx->getProperties()) th.score += (int) nv.value;
                    if (auto* comments = t->getProperty("comments").getArray())
                    {
                        th.comments = comments->size();
                        for (auto& citem : *comments) if (auto* c = citem.getDynamicObject())
                        {
                            ThreadComment tc;
                            tc.id = c->getProperty("id").toString();
                            tc.user = c->getProperty("user").toString();
                            tc.text = c->getProperty("text").toString();
                            tc.themeId = c->getProperty("theme").toString();
                            th.commentList.add(tc);
                        }
                    }
                    safe->threads.add(th);
                }
            if (safe->railMode == 1) safe->refreshSocial();
            if (safe->threadOpen) safe->rebuildThreadDetail();
            safe->rebuildThreadBoard();
            safe->logBox.setText(log.isEmpty()?r.body:log); safe->renderEffectLinks(log);
        });
    }).detach();
}


namespace
{
juce::String tagsToText(const juce::var& tags)
{
    if (auto* arr = tags.getArray())
    {
        juce::StringArray parts;
        for (const auto& item : *arr)
        {
            const auto part = item.toString().trim();
            if (part.isNotEmpty()) parts.add(part);
        }
        return parts.joinIntoString(", ");
    }
    return tags.toString();
}
}

void KyotoAudioProcessorEditor::refreshCatalog()
{
    if (token.isEmpty()) return;
    const int sortMode = sortBox.getSelectedId(); // 1 Newest 2 Oldest 3 Name A-Z 4 Name Z-A 5 Author
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    const int seq = ++catalogSeq;
    status.setText("Loading catalog...", juce::dontSendNotification);
    std::thread([safe, tokenCopy, sortMode, seq] {
        auto r = kt::getCatalog(tokenCopy);
        juce::MessageManager::callAsync([safe, r, sortMode, seq] {
            if (safe == nullptr) return;
            if (seq != safe->catalogSeq) return; // a newer request is already on its way
            // Called from the builders too (after publishing): then only refresh the data, never the cards on screen.
            const bool showCards = (safe->centerMode == 0);
            safe->catalog.clear();
            if (showCards) safe->catalogHolder.removeAllChildren();
            auto* mods = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("modules").getArray() : nullptr;
            if (mods == nullptr) { safe->status.setText(r.ok?"Catalog empty":(r.error.isEmpty() ? juce::String("Could not load the catalog") : "Catalog: " + r.error), juce::dontSendNotification); safe->refreshEffectBox(); return; }

            struct Row { CatalogItem c; juce::int64 at = 0; bool hidden = false; };
            juce::Array<Row> rows;
            for (auto& item:*mods)
            {
                auto* m=item.getDynamicObject(); if(!m) continue;
                const auto face = m->getProperty("face").toString().toLowerCase();
                // Effects catalog removed — only plugin faces in the public catalog.
                const bool isEffect = face == "fx" || face == "effect";
                if (isEffect) continue;
                const auto itemStatus = m->getProperty("status").toString().toLowerCase();
                const bool hiddenFromPublic = (itemStatus == "pending" || itemStatus == "denied");
                CatalogItem c;
                c.id = m->getProperty("id").toString();
                c.name = m->getProperty("name").toString();
                c.face = face;
                c.author = m->getProperty("author").toString();
                c.status = m->getProperty("status").toString();
                c.tags = tagsToText(m->getProperty("tags"));
                Row row; row.c = c; row.hidden = hiddenFromPublic;
                row.at = (juce::int64) m->getProperty("at");
                if (row.at == 0) row.at = (juce::int64) m->getProperty("updated");
                rows.add(row);
            }

            std::stable_sort(rows.begin(), rows.end(), [sortMode](const Row& a, const Row& b) {
                if (sortMode == 2) return a.at < b.at; // Oldest
                if (sortMode == 3) return a.c.name.compareIgnoreCase(b.c.name) < 0;
                if (sortMode == 4) return a.c.name.compareIgnoreCase(b.c.name) > 0;
                if (sortMode == 5) return a.c.author.compareIgnoreCase(b.c.author) < 0;
                return a.at > b.at; // Newest (default)
            });

            int i = 0;
            for (auto& row : rows)
            {
                safe->catalog.add(row.c);
                if (row.hidden) continue;
                const auto& c = row.c;
                auto* card=new BoardCard();
                card->title=c.name;
                card->meta=c.face+"  -  "+c.author;
                card->body="Open in the builder";
                card->themeId=safe->theme.id;
                card->status=c.status;
                card->tags=c.tags;
                const auto id=c.id, name=c.name, author=c.author;
                card->onOpen=[safe,id,name]{ if(safe!=nullptr){safe->selectedCatalogId=id;safe->loadCatalogId(id,name);} };
                card->onContextMenu=[safe,id,name,author](juce::Point<int> pos){ if(safe!=nullptr) safe->showCatalogCardMenu(id,name,author,pos); };
                if (showCards) safe->catalogHolder.addAndMakeVisible(card); else delete card;
                ++i;
            }
            if (! showCards) { safe->refreshEffectBox(); return; }
            if (i == 0)
            {
                auto* empty = new BoardCard();
                empty->title = "No plugins yet";
                empty->body = "Approved items show up here. Check MY PLUGINS for your own uploads and their approval status.";
                empty->themeId = safe->theme.id;
                safe->catalogHolder.addAndMakeVisible(empty);
            }
            safe->catalogView.setViewedComponent(&safe->catalogHolder,false); safe->resized(); safe->refreshEffectBox();
            safe->status.setText(juce::String(i) + (i == 1 ? " plugin" : " plugins"), juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshCurrentCenter()
{
    switch (centerMode)
    {
        case 2: refreshMyModules(); break;
        case 4: refreshPending(); break;
        case 3: refreshFeed(); break;
        default: refreshCatalog(); break;
    }
}

void KyotoAudioProcessorEditor::refreshMyModules()
{
    if (token.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy] {
        auto r = kt::getMyModules(tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            safe->catalog.clear(); safe->catalogHolder.removeAllChildren();
            auto* mods = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("modules").getArray() : nullptr;
            if (mods == nullptr) { safe->status.setText("No saved modules", juce::dontSendNotification); safe->refreshEffectBox(); return; }
            int i=0;
            for (auto& item:*mods)
            {
                auto* m=item.getDynamicObject(); if(!m) continue;
                CatalogItem c;
                c.id=m->getProperty("id").toString(); c.name=m->getProperty("name").toString();
                c.face=m->getProperty("face").toString(); c.author=m->getProperty("author").toString();
                c.status=m->getProperty("status").toString(); c.tags=tagsToText(m->getProperty("tags"));
                safe->catalog.add(c);
                auto* card=new BoardCard(); card->title=c.name; card->meta=c.face+"  -  "+c.author;
                card->body="Open in the builder"; card->themeId=safe->theme.id; card->status=c.status; card->tags=c.tags;
                const auto id=c.id, name=c.name, author=c.author;
                card->onOpen=[safe,id,name]{ if(safe!=nullptr){safe->selectedCatalogId=id;safe->loadCatalogId(id,name);} };
                card->onContextMenu=[safe,id,name,author](juce::Point<int> pos){ if(safe!=nullptr) safe->showCatalogCardMenu(id,name,author,pos); };
                safe->catalogHolder.addAndMakeVisible(card); ++i;
            }
            safe->catalogHolder.setSize(juce::jmax(220, safe->catalogView.getWidth()-18), juce::jmax(220, ((i+1)/2)*92));
            safe->catalogView.setViewedComponent(&safe->catalogHolder,false); safe->resized(); safe->refreshEffectBox();
            safe->status.setText("My plugins: " + juce::String(i), juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::refreshPending()
{
    if (token.isEmpty() || !isAdmin) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, tokenCopy] {
        auto r = kt::getPendingModules(tokenCopy);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            safe->catalog.clear(); safe->catalogHolder.removeAllChildren();
            auto* mods = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("modules").getArray() : nullptr;
            if (mods == nullptr) { safe->status.setText("No pending items", juce::dontSendNotification); safe->refreshEffectBox(); return; }
            int i=0;
            for (auto& item:*mods)
            {
                auto* m=item.getDynamicObject(); if(!m) continue;
                CatalogItem c;
                c.id=m->getProperty("id").toString(); c.name=m->getProperty("name").toString();
                c.face=m->getProperty("face").toString(); c.author=m->getProperty("author").toString();
                c.status="pending"; c.tags=tagsToText(m->getProperty("tags"));
                safe->catalog.add(c);
                auto* card=new BoardCard(); card->title=c.name; card->meta=c.face+"  -  "+c.author;
                card->body="Right-click to approve or deny"; card->themeId=safe->theme.id; card->status="pending"; card->tags=c.tags;
                const auto id=c.id, name=c.name, author=c.author;
                card->onOpen=[safe,id,name]{ if(safe!=nullptr){safe->selectedCatalogId=id;safe->loadCatalogId(id,name);} };
                card->onContextMenu=[safe,id,name,author](juce::Point<int> pos){ if(safe!=nullptr) safe->showCatalogCardMenu(id,name,author,pos); };
                safe->catalogHolder.addAndMakeVisible(card); ++i;
            }
            safe->catalogHolder.setSize(juce::jmax(220, safe->catalogView.getWidth()-18), juce::jmax(220, ((i+1)/2)*92));
            safe->catalogView.setViewedComponent(&safe->catalogHolder,false); safe->resized(); safe->refreshEffectBox();
            safe->status.setText("Pending: " + juce::String(i), juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::showCatalogCardMenu(const juce::String& id, const juce::String& name, const juce::String& author, juce::Point<int> screenPos)
{
    juce::PopupMenu menu;
    menu.addItem("Open: " + name, [this, id, name] { selectedCatalogId = id; loadCatalogId(id, name); });
    menu.addSeparator();
    if (isAdmin)
    {
        menu.addItem("Approve", [this, id] { approveCatalogId(id); });
        menu.addItem("Deny", [this, id] { denyCatalogId(id); });
        menu.addSeparator();
        menu.addItem("Add Tags...", [this, id] {
            auto* win = new juce::AlertWindow("Add Tags", "Enter comma-separated tags for this module.", juce::AlertWindow::NoIcon);
            win->addTextEditor("tags", "", "Tags");
            win->addButton("OK", 1); win->addButton("Cancel", 0);
            win->enterModalState(true, juce::ModalCallbackFunction::create([this, win, id](int code) {
                if (code == 1) tagCatalogId(id, win->getTextEditorContents("tags"));
                delete win;
            }), true);
        });
        menu.addItem("Remove", [this, id] { deleteCatalogId(id); });
    }
    else
    {
        // Non-admin: can only delete their own
        if (author.equalsIgnoreCase(account))
            menu.addItem("Remove My Upload", [this, id] { deleteCatalogId(id); });
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ screenPos.x, screenPos.y, 1, 1 }));
}

void KyotoAudioProcessorEditor::approveCatalogId(const juce::String& id)
{
    if (!isAdmin || id.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tokenCopy] { auto r=kt::approveModule(tokenCopy,id); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"APPROVED - it is now in the public catalog":"Approve failed: "+r.error,juce::dontSendNotification); if(r.ok) safe->refreshCurrentCenter(); }); }).detach();
}

void KyotoAudioProcessorEditor::denyCatalogId(const juce::String& id)
{
    if (!isAdmin || id.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tokenCopy] { auto r=kt::denyModule(tokenCopy,id); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"DENIED":"Deny failed: "+r.error,juce::dontSendNotification); if(r.ok) safe->refreshCurrentCenter(); }); }).detach();
}

void KyotoAudioProcessorEditor::tagCatalogId(const juce::String& id, const juce::String& tags)
{
    if (!isAdmin || id.isEmpty()) return;
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tags, tokenCopy] { auto r=kt::tagModule(tokenCopy,id,tags); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"TAGS ADDED":"Tag failed: "+r.error,juce::dontSendNotification); if(r.ok) safe->refreshCurrentCenter(); }); }).detach();
}

float KyotoAudioProcessorEditor::scaledFont(float baseSize) const
{
    const float scale = juce::jlimit(0.75f, 1.6f, (float)getWidth() / 900.0f);
    return baseSize * scale;
}

void KyotoAudioProcessorEditor::loadCatalogId(const juce::String& id, const juce::String& name)
{
    if (id.isEmpty())
    {
        auto file = moduleDir().getChildFile(name + ".json");
        if (! file.existsAsFile()) file = effectDir().getChildFile(name + ".json");
        if (! file.existsAsFile()) return;
        auto parsed = juce::JSON::parse(file.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
        {
            if (obj->getProperty("face").toString() == "effect" || obj->getProperty("format").toString() == "kyoteppah-effect-1")
            {
                fxStack.removeAllChildren(nullptr);
                if (auto* steps=obj->getProperty("steps").getArray()) for(auto& v:*steps) if(auto* so=v.getDynamicObject()){auto st=juce::ValueTree("step");st.setProperty("fx",(int)so->getProperty("fx"),nullptr);st.setProperty("name",so->getProperty("name").toString(),nullptr);st.setProperty("amount",(double)(so->hasProperty("amount") ? so->getProperty("amount") : juce::var(0.5)),nullptr);st.setProperty("tone",(double)(so->hasProperty("tone") ? so->getProperty("tone") : juce::var(0.5)),nullptr);st.setProperty("motion",(double)(so->hasProperty("motion") ? so->getProperty("motion") : juce::var(0.35)),nullptr);st.setProperty("mix",(double)(so->hasProperty("mix") ? so->getProperty("mix") : juce::var(0.4)),nullptr);st.setProperty("shape",(double)(so->hasProperty("shape") ? so->getProperty("shape") : juce::var(0.5)),nullptr);fxStack.appendChild(st,nullptr);}
                effectNameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification); if (fxStack.getNumChildren() > 0) selectFxStep(0); showTab(2); return;
            }
            nameBox.setText(obj->getProperty("name").toString(), juce::dontSendNotification);
            proc.restoreChainLevels(obj->getProperty("chainLevels"));
            if (auto* slots = obj->getProperty("slots").getArray())
            {
                for (int i = 0; i < proc.slotCount(); ++i)
                {
                    const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
                    if (auto* on = proc.apvts.getParameter(prefix + "on")) on->setValueNotifyingHost(0.f);
                }
                for (int i = 0; i < juce::jmin(proc.slotCount(), slots->size()); ++i)
                {
                    auto* src = slots->getReference(i).getDynamicObject(); if (!src) continue;
                    const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
                    if (auto* p = proc.apvts.getParameter(prefix + "on")) p->setValueNotifyingHost((src->hasProperty("on") ? src->getProperty("on") : juce::var(true)) ? 1.f : 0.f);
                    if (auto* p = proc.apvts.getParameter(prefix + "type")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("fx") ? src->getProperty("fx") : juce::var(0))));
                    if (auto* p = proc.apvts.getParameter(prefix + "amt")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("amount") ? src->getProperty("amount") : juce::var(0.5))));
                    if (auto* p = proc.apvts.getParameter(prefix + "tone")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("tone") ? src->getProperty("tone") : juce::var(0.5))));
                    if (auto* p = proc.apvts.getParameter(prefix + "mot")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("motion") ? src->getProperty("motion") : juce::var(0.35))));
                    if (auto* p = proc.apvts.getParameter(prefix + "mix")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("mix") ? src->getProperty("mix") : juce::var(0.4))));
                    if (auto* p = proc.apvts.getParameter(prefix + "shp")) p->setValueNotifyingHost(p->convertTo0to1((float)(src->hasProperty("shape") ? src->getProperty("shape") : juce::var(0.5))));
                }
            }
            if (auto* md = obj->getProperty("machineDesign").getDynamicObject())
            {
                machineDesign = MachineDesign::fromVar(juce::var(md));
                syncMachineDesignToUi();
            }
            if (auto* warr = obj->getProperty("widgets").getArray())
            {
                proc.uiState.removeAllChildren(nullptr);
                syncMachineDesignToUi();
                for (auto& item : *warr)
                {
                    auto* wsrc = item.getDynamicObject();
                    if (wsrc == nullptr) continue;
                    auto w = juce::ValueTree("w");
                    w.setProperty("slot", (int) wsrc->getProperty("slot"), nullptr);
                    w.setProperty("param", nativeKey(wsrc->getProperty("param").toString()), nullptr);
                    w.setProperty("label", wsrc->getProperty("label").toString(), nullptr);
                    w.setProperty("kind", wsrc->getProperty("kind").toString(), nullptr);
                    w.setProperty("slotCount", (int)propertyOr(wsrc, "slotCount", 1), nullptr);
                    w.setProperty("peaks", wsrc->getProperty("peaks").toString(), nullptr);
                    if (wsrc->hasProperty("style")) w.setProperty("style", wsrc->getProperty("style").toString(), nullptr);
                    if (wsrc->hasProperty("skin")) w.setProperty("skin", wsrc->getProperty("skin").toString(), nullptr);
                    if (wsrc->hasProperty("screenType")) w.setProperty("screenType", (int) wsrc->getProperty("screenType"), nullptr);
                    proc.uiState.appendChild(w, nullptr);
                }
            }
            {   // Theme law: a loaded plugin wears ITS OWN theme, never the signed-in user's.
                auto pt = obj->getProperty("playgroundTheme").toString();
                if (pt.isEmpty()) pt = obj->getProperty("theme").toString();
                if (pt.isNotEmpty()) applyPlaygroundTheme(pt);
            }
            showTab(1);
        }
        return;
    }
    const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, id, tokenCopy] {
        auto r = kt::getModule(tokenCopy, id);
        juce::MessageManager::callAsync([safe, r] {
            if (safe == nullptr) return;
            if (!r.ok) { safe->status.setText(r.error, juce::dontSendNotification); return; }
            auto* root = r.parsed.getDynamicObject(); if (!root) return;
            auto mod = root->getProperty("module"); auto* mo = mod.getDynamicObject(); if (!mo) return;
            const auto modName = mo->getProperty("name").toString();
            if (mo->getProperty("face").toString() == "effect" || mo->getProperty("format").toString() == "kyoteppah-effect-1")
            {
                safe->fxStack.removeAllChildren(nullptr);
                if (auto* steps=mo->getProperty("steps").getArray()) for(auto& v:*steps) if(auto* so=v.getDynamicObject())
                {
                    auto st=juce::ValueTree("step"); st.setProperty("fx",(int)so->getProperty("fx"),nullptr); st.setProperty("name",so->getProperty("name").toString(),nullptr);
                    st.setProperty("amount",(double)propertyOr(so, "amount", 0.5),nullptr); st.setProperty("tone",(double)propertyOr(so, "tone", 0.5),nullptr); st.setProperty("motion",(double)propertyOr(so, "motion", 0.35),nullptr); st.setProperty("mix",(double)propertyOr(so, "mix", 0.4),nullptr); st.setProperty("shape",(double)propertyOr(so, "shape", 0.5),nullptr); safe->fxStack.appendChild(st,nullptr);
                }
                auto downloaded = safe->effectDir().getChildFile(modName + ".json"); downloaded.replaceWithText(juce::JSON::toString(mod)); safe->refreshEffectBox();
                safe->lastPublishedEffectId = mo->getProperty("id").toString(); safe->effectNameBox.setText(modName, juce::dontSendNotification); safe->selectedFxStep = safe->fxStack.getNumChildren()>0?0:-1; if(safe->selectedFxStep>=0)safe->selectFxStep(0); safe->showTab(2); safe->status.setText("Loaded effect: "+modName,juce::dontSendNotification);
            }
            else
            {
                auto file=safe->moduleDir().getChildFile(modName+".json"); file.replaceWithText(juce::JSON::toString(mod));
                if (mo->hasProperty("machineDesign")) { safe->machineDesign = MachineDesign::fromVar(mo->getProperty("machineDesign")); safe->syncMachineDesignToUi(); }
                safe->loadCatalogId({},modName); safe->status.setText("Loaded catalog module",juce::dontSendNotification);
            }
        });
    }).detach();
}


void KyotoAudioProcessorEditor::showBuilderGuide()
{
    juce::String html;
    // Prefer Resources/BuilderGuide.html next to the project / binary when present.
    const juce::File candidates[] = {
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory().getParentDirectory().getParentDirectory()
            .getChildFile("Resources").getChildFile("BuilderGuide.html"),
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory().getChildFile("Resources").getChildFile("BuilderGuide.html"),
        juce::File::getCurrentWorkingDirectory().getChildFile("Resources").getChildFile("BuilderGuide.html"),
    };
    for (const auto& f : candidates)
        if (f.existsAsFile())
        {
            html = f.loadFileAsString();
            break;
        }
    if (html.isEmpty())
        html = kt::builderGuideHtml();

    if (htmlOverlay == nullptr)
    {
        htmlOverlay = std::make_unique<HtmlOverlay>(theme);
        addAndMakeVisible(*htmlOverlay);
    }
    htmlOverlay->setTheme(theme);
    htmlOverlay->fileName = "Builder Guide";
    htmlOverlay->setContent(html);
    htmlOverlay->setBounds(getLocalBounds());
    htmlOverlay->setVisible(true);
    htmlOverlay->toFront(true);
}

void KyotoAudioProcessorEditor::applyTheme(const juce::String& id)
{
    theme = kt::themeById(id);
    proc.uiState.setProperty("theme", theme.id, nullptr);
    for (int i = 0; i < kt::kThemeCount; ++i)
        if (juce::String(kt::kThemes[i].id).equalsIgnoreCase(theme.id))
        {
            themeBox.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    // The signed-in (VST) theme only styles the DreamShare UI. The plugin keeps its own theme law.
    panel.theme = playgroundTheme;
    for (auto* w : widgets) w->setTheme(playgroundTheme);
    panel.repaint();
    if (fxBrowser) fxBrowser->setTheme(theme);
    for (auto* e : { &logBox, &msgBox, &utilityBox, &userBox, &passBox, &nameBox, &effectNameBox })
    {
        e->setColour(juce::TextEditor::backgroundColourId, kt::c(theme.bg).brighter(0.03f));
        e->setColour(juce::TextEditor::textColourId, kt::c(theme.text));
        e->setColour(juce::TextEditor::outlineColourId, kt::c(theme.border));
        e->setColour(juce::TextEditor::focusedOutlineColourId, kt::c(theme.accent).withAlpha(0.85f));
        e->setColour(juce::TextEditor::highlightColourId, kt::c(theme.accent).withAlpha(0.30f));
        e->setColour(juce::TextEditor::highlightedTextColourId, kt::c(theme.text));
        e->setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
        e->setFont(kt::font(theme, 13.f));
        e->applyFontToAllText(kt::font(theme, 13.f));
        e->repaint();
    }
    // Combo boxes (theme, sort, shell, utility) — DreamShare chrome uniqueness
    for (auto* cb : { &themeBox, &sortBox, &shellBox, &playgroundThemeBox, &kindBox, &paramBox, &pieceBox, &utilityActionBox })
    {
        cb->setColour(juce::ComboBox::backgroundColourId, kt::c(theme.panel));
        cb->setColour(juce::ComboBox::textColourId, kt::c(theme.text));
        cb->setColour(juce::ComboBox::outlineColourId, kt::c(theme.border));
        cb->setColour(juce::ComboBox::arrowColourId, kt::c(theme.accent));
        cb->setColour(juce::ComboBox::focusedOutlineColourId, kt::c(theme.accent));
        cb->repaint();
    }
    catalogView.setColour(juce::ScrollBar::thumbColourId, kt::c(theme.accent).withAlpha(0.55f));
    chatView.setColour(juce::ScrollBar::thumbColourId, kt::c(theme.accent).withAlpha(0.55f));
    socialRail.setHostTheme(theme);
    socialRail.repaint();
    socialDirectory.setHostTheme(theme);
    socialDirectory.repaint();
    threadBoard.setTheme(theme);
    threadBoard.repaint();
    for (auto* l : { &shellLabel, &playgroundThemeLabel })
    {
        l->setFont(kt::font(theme, 10.f, true));
        l->setColour(juce::Label::textColourId, kt::c(theme.muted));
    }
    for (auto* l : { &status, &whoLabel, &fxAmountLabel, &fxToneLabel, &fxMotionLabel, &fxMixLabel, &fxShapeLabel, &stackLabel })
    {
        l->setColour(juce::Label::textColourId, kt::c(theme.text));
        l->setFont(kt::font(theme, 12.f, true));
    }
    for (auto* w : widgets) w->setTheme(playgroundTheme);
    kLookAndFeel.setTheme(theme);
    socialRail.setHostTheme(theme);
    socialDirectory.setHostTheme(theme);
    kLookAndFeel.setColour(juce::PopupMenu::backgroundColourId, kt::c(theme.panel));
    kLookAndFeel.setColour(juce::PopupMenu::textColourId, kt::c(theme.text));
    kLookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, kt::c(theme.accent).withAlpha(0.32f));
    kLookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, kt::c(theme.text));
    for (auto* b : { &shareBtn, &chainBtn, &fxBtn, &logoutBtn, &feedBtn, &chatRefreshBtn, &threadsBtn, &socialBtn, &dmBtn, &adminDeleteBtn, &sendBtn, &utilityGoBtn, &addBtn, &chainBreakBtn, &chainMixBtn, &chainRemoveBtn, &chainUndoBtn, &randomTemplateBtn, &saveBtn, &upBtn, &wavBtn, &fxAddBtn, &fxBreakBtn, &fxMixBtn, &fxRandomBtn, &fxClearBtn, &fxSaveBtn, &fxUpBtn, &fxShareChatBtn, &fxShareThreadBtn, &fxRemoveBtn, &fxUndoBtn, &pluginViewBtn, &pluginBackBtn, &newMachineBtn, &randomMachineBtn, &catalogModeBtn, &threadsModeBtn, &railChatBtn, &railDiscordBtn, &railOnlineBtn })
    {
        b->setColour(juce::TextButton::buttonColourId, kt::c(theme.panel).brighter(0.08f));
        b->setColour(juce::TextButton::buttonOnColourId, kt::c(theme.accent).withAlpha(0.30f));
        b->setColour(juce::TextButton::textColourOffId, kt::c(theme.text));
        b->setColour(juce::TextButton::textColourOnId, kt::c(theme.text));
    }
    if (htmlOverlay) htmlOverlay->setTheme(theme);
    syncMachineDesignToUi();
    repaint();
}

void KyotoAudioProcessorEditor::applyPlaygroundTheme(const juce::String& id)
{
    playgroundTheme = kt::themeById(id);
    proc.uiState.setProperty("playgroundTheme", playgroundTheme.id, nullptr);
    machineDesign.theme = playgroundTheme.id;
    machineDesign.normalizeThemeIds();
    // Builder shell, widgets, and Plugin View follow playground theme (unique per theme).
    panel.theme = playgroundTheme;
    for (auto* w : widgets) w->setTheme(playgroundTheme);
    panel.repaint();
    for (int i = 0; i < kt::kThemeCount; ++i)
        if (juce::String(kt::kThemes[i].id).equalsIgnoreCase(playgroundTheme.id))
        {
            playgroundThemeBox.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    // Shell / template chrome inherits accent for knobs already painted via panel.theme.
    if (viewScreen != nullptr) viewScreen->repaint();
    syncMachineDesignToUi();
    repaint();
}

juce::String KyotoAudioProcessorEditor::deriveCategoriesFromStack() const
{
    bool used[8] = {};
    for (int i = 0; i < fxStack.getNumChildren(); ++i)
    {
        const int fx = (int) fxStack.getChild(i).getProperty("fx", 0);
        if (fx >= 0 && fx < kt::kFxCount)
        {
            const int fam = kt::kFx[fx].family;
            if (fam >= 0 && fam < 8) used[fam] = true;
        }
    }
    juce::StringArray cats;
    for (int f = 0; f < 8; ++f)
        if (used[f]) cats.add(kt::kFxFamilyNames[f]);
    return cats.joinIntoString(" - ");
}

void KyotoAudioProcessorEditor::loadWav()
{
    auto chooser = std::make_shared<juce::FileChooser>("Load WAV", juce::File(), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (! file.existsAsFile()) return;
            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
            if (reader == nullptr) { status.setText("Could not read WAV", juce::dontSendNotification); return; }
            juce::AudioBuffer<float> buf((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read(&buf, 0, (int) reader->lengthInSamples, 0, true, true);
            proc.loadSample(buf, reader->sampleRate);
            juce::String peaks;
            const int bins = 48;
            const int hop = juce::jmax(1, buf.getNumSamples() / bins);
            for (int b = 0; b < bins; ++b)
            {
                float m = 0.f;
                for (int i = 0; i < hop && b * hop + i < buf.getNumSamples(); ++i)
                    m = juce::jmax(m, std::abs(buf.getSample(0, b * hop + i)));
                peaks << juce::String(m, 3) << (b == bins - 1 ? "" : ",");
            }
            auto node = juce::ValueTree("w");
            node.setProperty("slot", -1, nullptr);
            node.setProperty("param", "amt", nullptr);
            node.setProperty("label", file.getFileNameWithoutExtension(), nullptr);
            node.setProperty("kind", "wave", nullptr);
            node.setProperty("peaks", peaks, nullptr);
            proc.uiState.appendChild(node, nullptr);
            showTab(1);
            rebuildCanvas();
            status.setText("WAV viewer loaded " + file.getFileName(), juce::dontSendNotification);
        });
}

void KyotoAudioProcessorEditor::saveEffect()
{
    if (fxStack.getNumChildren() == 0) { status.setText("Add at least one FX stage before saving.", juce::dontSendNotification); return; }
    if (fxStack.getNumChildren() > 16) { status.setText("FX Builder limit is 16 stages.", juce::dontSendNotification); return; }
    auto* obj = new juce::DynamicObject();
    obj->setProperty("format", "kyoteppah-effect-1");
    obj->setProperty("face", "effect");
    const auto name = effectNameBox.getText().trim().isEmpty() ? "untitled-fx" : effectNameBox.getText().trim();
    obj->setProperty("name", name);
    juce::Array<juce::var> steps;
    for (int i = 0; i < fxStack.getNumChildren(); ++i)
    {
        auto s = fxStack.getChild(i);
        auto* o = new juce::DynamicObject();
        o->setProperty("fx", (int) s.getProperty("fx"));
        o->setProperty("name", s.getProperty("name").toString());
        o->setProperty("amount", (double) s.getProperty("amount", 0.5));
        o->setProperty("tone", (double) s.getProperty("tone", 0.5));
        o->setProperty("motion", (double) s.getProperty("motion", 0.35));
        o->setProperty("mix", (double) s.getProperty("mix", 0.4));
        o->setProperty("shape", (double) s.getProperty("shape", 0.5));
        steps.add(juce::var(o));
    }
    obj->setProperty("steps", steps);
    const auto cats = deriveCategoriesFromStack();
    obj->setProperty("categories", cats);
    effectDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    refreshEffectBox();
    status.setText(cats.isNotEmpty() ? ("Saved effect " + name + "  -  " + cats) : ("Saved effect " + name), juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publishEffect()
{
    if (fxStack.getNumChildren() == 0) { status.setText("Add at least one FX stage before publishing.", juce::dontSendNotification); return; }
    saveEffect();
    const auto name = effectNameBox.getText().trim().isEmpty() ? "untitled-fx" : effectNameBox.getText().trim();
    const auto body = effectDir().getChildFile(name + ".json").loadFileAsString(); const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe,name,body,tokenCopy]{ auto r=kt::publishModule(tokenCopy,name,body); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; if(r.ok){ if(auto* o=r.parsed.getDynamicObject()) safe->lastPublishedEffectId=o->getProperty("id").toString(); safe->status.setText(safe->lastPublishedEffectId.isNotEmpty()?"Auto-uploaded FX - "+safe->lastPublishedEffectId+" (pending approval)":"Effect auto-uploaded (pending approval)",juce::dontSendNotification); safe->refreshCatalog(); } else safe->status.setText(r.error,juce::dontSendNotification); }); }).detach();
}

juce::String KyotoAudioProcessorEditor::effectShareText() const
{
    if (lastPublishedEffectId.isEmpty()) return {};
    const auto name = effectNameBox.getText().trim().isEmpty() ? "Custom FX" : effectNameBox.getText().trim();
    return effectTokenFor(lastPublishedEffectId) + " " + name + "  -  click to load into FX Builder";
}

void KyotoAudioProcessorEditor::shareEffectToChat()
{
    const auto text=effectShareText(), tokenCopy=token; if(tokenCopy.isEmpty()||text.isEmpty()){status.setText("Publish this effect first, then share it.",juce::dontSendNotification);return;} juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe,text,tokenCopy]{auto r=kt::sendChat(tokenCopy,text);juce::MessageManager::callAsync([safe,r]{if(safe==nullptr)return;safe->status.setText(r.ok?"FX shared to chat":r.error,juce::dontSendNotification);if(r.ok)safe->refreshFeed();});}).detach();
}

void KyotoAudioProcessorEditor::shareEffectToThread()
{
    const auto text=effectShareText(), tokenCopy=token; if(tokenCopy.isEmpty()||text.isEmpty()){status.setText("Publish this effect first, then share it.",juce::dontSendNotification);return;} const auto title=effectNameBox.getText().trim().isEmpty()?"Custom KYOTRIPPAH FX":effectNameBox.getText().trim(); juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe,text,title,tokenCopy]{auto* o=new juce::DynamicObject();o->setProperty("title",title);o->setProperty("text",text);auto r=kt::postAction("create_thread",juce::var(o),tokenCopy);juce::MessageManager::callAsync([safe,r]{if(safe==nullptr)return;safe->status.setText(r.ok?"FX shared to Threads":r.error,juce::dontSendNotification);});}).detach();
}

void KyotoAudioProcessorEditor::updateFxControls()
{
    const int type = fxBrowser ? fxBrowser->getSelectedFx() : 0;
    const int fam = (type >= 0 && type < kt::kFxCount) ? kt::kFx[type].family : 0;
    if (fam == 0) { fxAmountLabel.setText("TIME", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("FEEDBACK", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SPREAD", juce::dontSendNotification); }
    else if (fam == 1) { fxAmountLabel.setText("SIZE", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("DECAY", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("DIFFUSION", juce::dontSendNotification); }
    else if (fam == 2 || fam == 3) { fxAmountLabel.setText("DEPTH", juce::dontSendNotification); fxToneLabel.setText("COLOR", juce::dontSendNotification); fxMotionLabel.setText("RATE", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("WIDTH", juce::dontSendNotification); }
    else if (fam == 4) { fxAmountLabel.setText("CUTOFF", juce::dontSendNotification); fxToneLabel.setText("RESONANCE", juce::dontSendNotification); fxMotionLabel.setText("SWEEP", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SLOPE", juce::dontSendNotification); }
    else if (fam == 5) { fxAmountLabel.setText("DRIVE", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("BIAS", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SHAPE", juce::dontSendNotification); }
    else if (fam == 6) { fxAmountLabel.setText("THRESH", juce::dontSendNotification); fxToneLabel.setText("RATIO", juce::dontSendNotification); fxMotionLabel.setText("ATTACK", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("RELEASE", juce::dontSendNotification); }
    else { fxAmountLabel.setText("AMOUNT", juce::dontSendNotification); fxToneLabel.setText("TONE", juce::dontSendNotification); fxMotionLabel.setText("MOTION", juce::dontSendNotification); fxMixLabel.setText("MIX", juce::dontSendNotification); fxShapeLabel.setText("SHAPE", juce::dontSendNotification); }
}

void KyotoAudioProcessorEditor::selectFxStep(int index)
{
    if (index < 0 || index >= fxStack.getNumChildren()) return;
    selectedFxStep = index;
    auto s = fxStack.getChild(index);
    const int type = (int)s.getProperty("fx", 0);
    if (fxBrowser && type >= 0 && type < kt::kFxCount) fxBrowser->setSelectedFx(type);
    fxAmount.setValue((double)s.getProperty("amount", 0.5), juce::dontSendNotification);
    fxTone.setValue((double)s.getProperty("tone", 0.5), juce::dontSendNotification);
    fxMotion.setValue((double)s.getProperty("motion", 0.35), juce::dontSendNotification);
    fxMix.setValue((double)s.getProperty("mix", 0.4), juce::dontSendNotification);
    fxShape.setValue((double)s.getProperty("shape", 0.5), juce::dontSendNotification);
    updateFxControls();
    repaint();
}

void KyotoAudioProcessorEditor::writeFxStepFromControls()
{
    if (selectedFxStep < 0 || selectedFxStep >= fxStack.getNumChildren()) return;
    auto s = fxStack.getChild(selectedFxStep);
    s.setProperty("amount", fxAmount.getValue(), nullptr);
    s.setProperty("tone", fxTone.getValue(), nullptr);
    s.setProperty("motion", fxMotion.getValue(), nullptr);
    s.setProperty("mix", fxMix.getValue(), nullptr);
    s.setProperty("shape", fxShape.getValue(), nullptr);
    lastPublishedEffectId.clear();
    repaint();
}

void KyotoAudioProcessorEditor::randomizeFxControls()
{
    juce::Random rng;
    fxAmount.setValue(rng.nextFloat(), juce::sendNotificationSync);
    fxTone.setValue(rng.nextFloat(), juce::sendNotificationSync);
    fxMotion.setValue(rng.nextFloat(), juce::sendNotificationSync);
    fxMix.setValue(0.25f + rng.nextFloat() * 0.7f, juce::sendNotificationSync);
    fxShape.setValue(rng.nextFloat(), juce::sendNotificationSync);
}

void KyotoAudioProcessorEditor::addSpecialChainStep(int type, const juce::String& name)
{
    juce::Rectangle<int> room;
    if (!findAutoCell(widgets.size(), "stack", room)) { status.setText("No safe room left. Use FX Builder to build a custom effect.", juce::dontSendNotification); return; }
    int slot = -1;
    for (int i = 0; i < proc.slotCount(); ++i)
        if (auto* on = proc.apvts.getParameter("s" + juce::String(i + 1).paddedLeft('0', 2) + "on"); on != nullptr && on->getValue() < 0.5f) { slot = i; break; }
    if (slot < 0) { status.setText("DSP slots are full - build a custom FX instead.", juce::dontSendNotification); return; }
    captureSnapshot();
    const auto prefix = "s" + juce::String(slot + 1).paddedLeft('0', 2);
    auto setFloat = [this, &prefix](const juce::String& suffix, float value) { if (auto* p = proc.apvts.getParameter(prefix + suffix)) p->setValueNotifyingHost(p->convertTo0to1(value)); };
    if (auto* p = proc.apvts.getParameter(prefix + "type")) p->setValueNotifyingHost(p->convertTo0to1((float)type));
    if (auto* p = proc.apvts.getParameter(prefix + "on")) p->setValueNotifyingHost(1.f);
    setFloat("amt", type == KyotoAudioProcessor::kMixType ? 1.f : 0.5f);
    setFloat("mix", 1.f);
    auto node = juce::ValueTree("w");
    node.setProperty("slot", slot, nullptr);
    node.setProperty("param", "amt", nullptr);
    node.setProperty("label", name, nullptr);
    node.setProperty("kind", "stack", nullptr);
    node.setProperty("slotCount", 1, nullptr);
    node.setProperty("series", proc.uiState.getNumChildren(), nullptr);
    proc.uiState.appendChild(node, nullptr);
    rebuildCanvas();
}

void KyotoAudioProcessorEditor::refreshEffectBox()
{
    auto writeEffectPreset = [this](const juce::String& name, std::initializer_list<std::array<double, 6>> rows)
    {
        const auto file = effectDir().getChildFile(name + ".json");
        if (file.existsAsFile()) return;
        auto* obj = new juce::DynamicObject(); obj->setProperty("format", "kyoteppah-effect-1"); obj->setProperty("face", "effect"); obj->setProperty("name", name);
        juce::Array<juce::var> steps;
        for (const auto& row : rows)
        {
            auto* step = new juce::DynamicObject(); step->setProperty("fx", (int)row[0]); step->setProperty("name", row[0] >= 0 && row[0] < kt::kFxCount ? kt::kFx[(int)row[0]].name : "Utility");
            step->setProperty("amount", row[1]); step->setProperty("tone", row[2]); step->setProperty("motion", row[3]); step->setProperty("mix", row[4]); step->setProperty("shape", row[5]); steps.add(juce::var(step));
        }
        obj->setProperty("steps", steps); file.replaceWithText(juce::JSON::toString(juce::var(obj)));
    };
    writeEffectPreset("Velvet Glue", {{20,0.48,0.62,0.25,0.55,0.40},{21,0.34,0.70,0.18,0.42,0.35},{30,0.28,0.78,0.12,0.30,0.55}});
    writeEffectPreset("Ghost Corridor", {{2,0.42,0.52,0.58,0.48,0.40},{3,0.55,0.64,0.72,0.42,0.62},{59,0.38,0.70,0.35,0.34,0.72}});
    writeEffectPreset("Neon Damage", {{0,0.64,0.42,0.22,0.58,0.72},{1,0.46,0.70,0.38,0.46,0.68},{11,0.55,0.38,0.66,0.52,0.82}});

    auto writeChainPreset = [this](const juce::String& name, std::initializer_list<std::array<double, 6>> rows)
    {
        const auto dir = sessionFile().getParentDirectory().getChildFile("kyoto"); dir.createDirectory();
        const auto file = dir.getChildFile(name + ".json"); if (file.existsAsFile()) return;
        auto* obj = new juce::DynamicObject(); obj->setProperty("format", "kyoteppah-module-1"); obj->setProperty("face", "chain"); obj->setProperty("name", name); obj->setProperty("grid", 0); obj->setProperty("theme", playgroundTheme.id); obj->setProperty("playgroundTheme", playgroundTheme.id);
        juce::Array<juce::var> slots;
        for (const auto& row : rows) { auto* step = new juce::DynamicObject(); step->setProperty("on", true); step->setProperty("fx", (int)row[0]); step->setProperty("amount", row[1]); step->setProperty("tone", row[2]); step->setProperty("motion", row[3]); step->setProperty("mix", row[4]); step->setProperty("shape", row[5]); slots.add(juce::var(step)); }
        obj->setProperty("slots", slots); obj->setProperty("widgets", juce::var(juce::Array<juce::var>())); file.replaceWithText(juce::JSON::toString(juce::var(obj)));
    };
    writeChainPreset("Midnight Bloom", {{3,0.52,0.60,0.62,0.42,0.58},{1,0.42,0.72,0.38,0.40,0.62},{30,0.30,0.78,0.18,0.28,0.55},{20,0.38,0.60,0.22,0.44,0.40}});
    writeChainPreset("Tape Prayer", {{15,0.40,0.62,0.52,0.46,0.50},{2,0.36,0.54,0.45,0.38,0.46},{21,0.32,0.72,0.20,0.40,0.38},{200,1.0,0.5,0.5,1.0,0.5}});
    writeChainPreset("Wide Dream", {{4,0.55,0.50,0.32,0.50,0.78},{1,0.35,0.70,0.28,0.40,0.72},{59,0.32,0.68,0.40,0.34,0.70},{200,0.82,0.5,0.5,1.0,0.5}});

    const auto isStarter = [](const juce::String& n)
    {
        return n == "Midnight Bloom" || n == "Tape Prayer" || n == "Wide Dream" || n == "Velvet Glue" || n == "Ghost Corridor" || n == "Neon Damage";
    };
    presetBox.clear();
    int id = 1;
    for (auto f : moduleDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        if (!isStarter(f.getFileNameWithoutExtension())) presetBox.addItem("CHAIN  -  " + f.getFileNameWithoutExtension(), id++);
    for (auto f : effectDir().findChildFiles(juce::File::findFiles, false, "*.json"))
        if (!isStarter(f.getFileNameWithoutExtension())) presetBox.addItem("FX  -  " + f.getFileNameWithoutExtension(), id++);

    juce::Array<FxBrowser::CustomItem> custom;
    for (auto f : effectDir().findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        if (isStarter(f.getFileNameWithoutExtension())) continue;
        FxBrowser::CustomItem item;
        item.name = f.getFileNameWithoutExtension();
        item.author = account;
        item.remote = false;
        auto parsed = juce::JSON::parse(f.loadFileAsString());
        if (auto* o = parsed.getDynamicObject())
        {
            item.categories = o->getProperty("categories").toString();
            if (item.categories.isEmpty() && o->getProperty("steps").isArray())
            {
                bool used[8] = {};
                for (auto& v : *o->getProperty("steps").getArray())
                    if (auto* so = v.getDynamicObject())
                    {
                        const int fx = (int) so->getProperty("fx");
                        if (fx >= 0 && fx < kt::kFxCount)
                        {
                            const int fam = kt::kFx[fx].family;
                            if (fam >= 0 && fam < 8) used[fam] = true;
                        }
                    }
                juce::StringArray cats;
                for (int fam = 0; fam < 8; ++fam)
                    if (used[fam]) cats.add(kt::kFxFamilyNames[fam]);
                item.categories = cats.joinIntoString(" - ");
            }
        }
        custom.add(item);
    }
    for (const auto& c : catalog)
        if (c.face.equalsIgnoreCase("effect")) custom.add({c.id, c.name, c.author, {}, true});
    if (fxBrowser) fxBrowser->setCustomItems(custom);
}

void KyotoAudioProcessorEditor::saveLocal()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("format", "kyoteppah-module-1");
    obj->setProperty("face", proc.isFx() ? "fx" : "chain");
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    obj->setProperty("name", name);
    obj->setProperty("grid", 0);
    obj->setProperty("theme", playgroundTheme.id); obj->setProperty("playgroundTheme", playgroundTheme.id);
    obj->setProperty("machineDesign", machineDesign.toVar());
    juce::Array<juce::var> widgetsArr, slots, steps;
    for (int i = 0; i < proc.slotCount(); ++i)
    {
        auto* s = new juce::DynamicObject();
        const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
        s->setProperty("on", proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f);
        s->setProperty("fx", (int) proc.apvts.getRawParameterValue(prefix + "type")->load());
        s->setProperty("amount", proc.apvts.getRawParameterValue(prefix + "amt")->load());
        s->setProperty("tone", proc.apvts.getRawParameterValue(prefix + "tone")->load());
        s->setProperty("motion", proc.apvts.getRawParameterValue(prefix + "mot")->load());
        s->setProperty("mix", proc.apvts.getRawParameterValue(prefix + "mix")->load());
        s->setProperty("shape", proc.apvts.getRawParameterValue(prefix + "shp")->load());
        slots.add(juce::var(s));
        if (proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f)
        {
            auto* step = new juce::DynamicObject();
            const int fx = (int) proc.apvts.getRawParameterValue(prefix + "type")->load();
            step->setProperty("fx", fx);
            step->setProperty("name", fx == KyotoAudioProcessor::kMixType ? "MASTER MIX" : (fx == KyotoAudioProcessor::kBreakType ? "CHAIN BREAK" : (fx >= 0 && fx < kt::kFxCount ? kt::kFx[fx].name : "FX")));
            step->setProperty("amount", proc.apvts.getRawParameterValue(prefix + "amt")->load());
            step->setProperty("tone", proc.apvts.getRawParameterValue(prefix + "tone")->load());
            step->setProperty("motion", proc.apvts.getRawParameterValue(prefix + "mot")->load());
            step->setProperty("mix", proc.apvts.getRawParameterValue(prefix + "mix")->load());
            step->setProperty("shape", proc.apvts.getRawParameterValue(prefix + "shp")->load());
            steps.add(juce::var(step));
        }
    }
    for (int i = 0; i < proc.uiState.getNumChildren(); ++i)
    {
        auto w = proc.uiState.getChild(i);
        if (! w.hasType("w")) continue;
        auto* o = new juce::DynamicObject();
        o->setProperty("slot", (int) w.getProperty("slot"));
        o->setProperty("param", w.getProperty("param").toString());
        o->setProperty("label", w.getProperty("label").toString());
        o->setProperty("kind", w.getProperty("kind").toString());
        o->setProperty("slotCount", (int)w.getProperty("slotCount", 1));
        o->setProperty("peaks", w.getProperty("peaks").toString());
        o->setProperty("x", (int) w.getProperty("x"));
        o->setProperty("y", (int) w.getProperty("y"));
        o->setProperty("shellSlot", (int) w.getProperty("shellSlot", -1));
        o->setProperty("parent", (int) w.getProperty("parent", 0));
        o->setProperty("style", w.getProperty("style").toString());
        o->setProperty("skin", w.getProperty("skin").toString());
        if (w.hasProperty("screenType")) o->setProperty("screenType", (int) w.getProperty("screenType"));
        widgetsArr.add(juce::var(o));
    }
    obj->setProperty("chainLevels", proc.exportChainLevels());
    obj->setProperty("slots", slots);
    obj->setProperty("steps", steps);
    obj->setProperty("widgets", widgetsArr);
    moduleDir().getChildFile(name + ".json").replaceWithText(juce::JSON::toString(juce::var(obj)));
    refreshEffectBox();
    status.setText("Saved chain " + name, juce::dontSendNotification);
}

void KyotoAudioProcessorEditor::publish()
{
    saveLocal();
    const auto name = nameBox.getText().trim().isEmpty() ? "untitled" : nameBox.getText().trim();
    const auto body = moduleDir().getChildFile(name + ".json").loadFileAsString(); const auto tokenCopy = token; juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    std::thread([safe, name, body, tokenCopy] { auto r=kt::publishModule(tokenCopy,name,body); juce::MessageManager::callAsync([safe,r]{ if(safe==nullptr)return; safe->status.setText(r.ok?"Auto-uploaded. Pending admin approval.":r.error,juce::dontSendNotification); if(r.ok)safe->refreshCatalog(); }); }).detach();
}
// ============================================================================
//  DreamShare thread board glue (Reddit / 4chan style board)
// ============================================================================
void KyotoAudioProcessorEditor::wireThreadBoard()
{
    threadBoard.onStatus = [this](const juce::String& m) { status.setText(m, juce::dontSendNotification); };
    threadBoard.onRefresh = [this] { refreshFeed(); };
    threadBoard.onUpvote = [this](const juce::String& id) { reactTo("thread", id, "heart"); };
    threadBoard.onOpenPlugin = [this](const juce::String& id, const juce::String& name) { selectedCatalogId = id; loadCatalogId(id, name); };
    threadBoard.onFileClick = [this](const kt::AttachRef& ref, juce::Point<int> pos) { showBoardFileMenu(ref, pos); };

    threadBoard.fetchImage = [this](const kt::AttachRef& ref, std::function<void(juce::Image)> done)
    {
        const auto tokenCopy = token;
        juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
        std::thread([safe, tokenCopy, ref, done] {
            const auto ext = ref.name.fromLastOccurrenceOf(".", true, false);
            auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("ds_" + ref.upload.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") + ext);
            juce::String err;
            juce::Image img;
            if (kt::downloadAttachment(tokenCopy, ref, tmp, err))
                img = juce::ImageFileFormat::loadFrom(tmp);
            tmp.deleteFile();
            juce::MessageManager::callAsync([safe, done, img] {
                if (safe == nullptr) return;
                done(img);
            });
        }).detach();
    };

    threadBoard.onPickPlugin = [this](std::function<void(juce::String, juce::String)> picked)
    {
        if (token.isEmpty()) return;
        const auto tokenCopy = token;
        juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
        status.setText("Loading your plugins...", juce::dontSendNotification);
        std::thread([safe, tokenCopy, picked] {
            auto r = kt::getMyModules(tokenCopy);
            juce::MessageManager::callAsync([safe, r, picked] {
                if (safe == nullptr) return;
                auto* mods = r.parsed.getDynamicObject() ? r.parsed.getDynamicObject()->getProperty("modules").getArray() : nullptr;
                if (mods == nullptr || mods->isEmpty())
                {
                    safe->status.setText("You have no saved plugins yet - build and save one first.", juce::dontSendNotification);
                    return;
                }
                auto ids = std::make_shared<juce::StringArray>();
                auto names = std::make_shared<juce::StringArray>();
                juce::PopupMenu menu;
                menu.addSectionHeader("Attach one of your plugins");
                for (auto& item : *mods)
                {
                    auto* m = item.getDynamicObject();
                    if (m == nullptr) continue;
                    ids->add(m->getProperty("id").toString());
                    names->add(m->getProperty("name").toString());
                    menu.addItem(ids->size(), (*names)[names->size() - 1] + "  (" + m->getProperty("face").toString() + ")");
                }
                safe->status.setText({}, juce::dontSendNotification);
                menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&safe->threadBoard),
                    [ids, names, picked](int choice) {
                        if (choice >= 1 && choice <= ids->size()) picked((*ids)[choice - 1], (*names)[choice - 1]);
                    });
            });
        }).detach();
    };

    threadBoard.onPost = [this](const juce::String& title, const juce::String& body, const juce::Array<juce::File>& files, const juce::String& mod)
    {
        submitBoardPost({}, title, body, files, mod);
    };
    threadBoard.onReply = [this](const juce::String& threadId, const juce::String& body, const juce::Array<juce::File>& files, const juce::String& mod)
    {
        submitBoardPost(threadId, {}, body, files, mod);
    };
}

// Uploads every staged file (sequentially, 1.5 MB chunks), then creates the thread or reply with the tokens appended.
void KyotoAudioProcessorEditor::submitBoardPost(const juce::String& threadId, const juce::String& title, const juce::String& body,
                                                const juce::Array<juce::File>& files, const juce::String& modToken)
{
    if (token.isEmpty()) return;
    const bool newThread = threadId.isEmpty();
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    threadBoard.setBusy(true, files.isEmpty() ? juce::String("Posting...") : "Uploading " + juce::String(files.size()) + " file(s)...");
    std::thread([safe, tokenCopy, threadId, title, body, files, modToken, newThread] {
        juce::String err, tokens;
        bool ok = true;
        for (const auto& f : files)
        {
            kt::AttachRef ref;
            if (! kt::uploadAttachment(tokenCopy, f, ref, err)) { ok = false; break; }
            tokens += " " + kt::attachToken(ref);
        }
        kt::DreamResult r;
        if (ok)
        {
            tokens += (modToken.isNotEmpty() ? " " + modToken : juce::String());
            const int limit = newThread ? 1000 : 500;
            auto text = body.substring(0, juce::jmax(0, limit - tokens.length())) + tokens;
            text = text.trim();
            auto* o = new juce::DynamicObject();
            if (newThread)
            {
                auto t = title.isNotEmpty() ? title : (body.isNotEmpty() ? body.substring(0, 60) : juce::String("Shared files"));
                o->setProperty("title", t.substring(0, 120));
                o->setProperty("text", text);
                r = kt::postAction("create_thread", juce::var(o), tokenCopy);
            }
            else
            {
                o->setProperty("threadId", threadId);
                o->setProperty("text", text);
                r = kt::postAction("comment", juce::var(o), tokenCopy);
            }
        }
        else r.error = err;
        juce::MessageManager::callAsync([safe, r, newThread] {
            if (safe == nullptr) return;
            safe->threadBoard.setBusy(false);
            if (! r.ok)
            {
                safe->status.setText("Post failed: " + (r.error.isEmpty() ? juce::String("unknown error") : r.error) + " (your files stay staged - press again to retry)", juce::dontSendNotification);
                return;
            }
            safe->threadBoard.clearComposer();
            if (! newThread) { /* stay in the thread */ }
            safe->refreshFeed();
            safe->status.setText(newThread ? "Thread posted" : "Reply posted", juce::dontSendNotification);
        });
    }).detach();
}

void KyotoAudioProcessorEditor::showBoardFileMenu(const kt::AttachRef& ref, juce::Point<int> pos)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Save " + ref.name + " as...");
    const bool audio = kt::fileKindOf(ref.name) == "audio";
    if (audio) menu.addItem(2, "Use as my Sound sample (loads into this plugin)");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(pos.x, pos.y, 1, 1)),
        [this, safeEd = juce::Component::SafePointer<KyotoAudioProcessorEditor>(this), ref](int choice) {
            if (safeEd == nullptr || safeEd->editorClosing) return;
            if (choice == 1) saveAttachmentAs(ref);
            else if (choice == 2) useAttachmentAsSample(ref);
        });
}

void KyotoAudioProcessorEditor::useAttachmentAsSample(const kt::AttachRef& ref)
{
    const auto tokenCopy = token;
    juce::Component::SafePointer<KyotoAudioProcessorEditor> safe(this);
    status.setText("Downloading " + ref.name + "...", juce::dontSendNotification);
    std::thread([safe, tokenCopy, ref] {
        auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("ds_sample_" + juce::String(juce::Time::currentTimeMillis()) + ref.name.fromLastOccurrenceOf(".", true, false));
        juce::String err;
        juce::AudioBuffer<float> buf;
        double sr = 44100.0;
        bool ok = kt::downloadAttachment(tokenCopy, ref, tmp, err);
        if (ok)
        {
            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(tmp));
            if (reader != nullptr)
            {
                const int n = (int) juce::jmin<juce::int64>(reader->lengthInSamples, 48000 * 30);
                buf.setSize(1, n);
                reader->read(&buf, 0, n, 0, true, true);
                sr = reader->sampleRate;
            }
            else { ok = false; err = "format not readable here (use WAV / AIFF / FLAC / MP3 / OGG)"; }
        }
        tmp.deleteFile();
        juce::MessageManager::callAsync([safe, ok, err, buf, sr, name = ref.name]() mutable {
            if (safe == nullptr) return;
            if (! ok) { safe->status.setText("Sample failed: " + err, juce::dontSendNotification); return; }
            safe->proc.loadSample(buf, sr);
            safe->status.setText("Sound sample loaded from thread: " + name, juce::dontSendNotification);
        });
    }).detach();
}

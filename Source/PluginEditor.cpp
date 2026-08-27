#include "PluginEditor.h"
#include <VoidwormAssets.h>
#include "PresetDefinitions.h"
#include "UI/EqGraphInteraction.h"
#include "UI/EditorLayout.h"
#include "UI/FooterAnimation.h"
#include "UI/ReactorWorkspaceState.h"
#include "UI/ThemePalette.h"

namespace
{
const juce::Colour defaultWarmWhite (0xffd8d0c6);
constexpr float designWidth = 708.0f;
constexpr float designHeight = 633.0f;

const auto& editorLayout = voidworm::ui::editorLayout();

constexpr std::array<float, 4> meterFloorsDb { -48.0f, -60.0f, -72.0f, -96.0f };
constexpr std::array<int, 4> meterHoldTicks { 0, 4, 8, 18 };
constexpr std::array<float, 3> uiScaleFactors { 0.90f, 1.0f, 1.25f };

namespace uiStyle
{
constexpr float xs = 4.0f;
constexpr float s = 8.0f;
constexpr float m = 12.0f;
constexpr float l = 16.0f;
constexpr float overlayRadius = 7.0f;
constexpr float overlayHeader = 32.0f;
constexpr float overlayPadding = 12.0f;
constexpr float titleFont = 10.0f;
constexpr float sectionFont = 7.2f;
constexpr float normalFont = 8.5f;
constexpr float secondaryFont = 7.0f;
constexpr float searchFont = 11.5f;
const juce::Colour utilitySurface (0xff0b0b0b);
const juce::Colour utilityRaised (0xff121110);
const juce::Colour utilityHover (0xff191614);
const juce::Colour utilityEdge (0xff37322f);
}

float frequencyToProportion (float frequency) noexcept
{
    return voidworm::ui::frequencyToProportion (frequency);
}

float proportionToFrequency (float proportion) noexcept
{
    return voidworm::ui::proportionToFrequency (proportion);
}

juce::String formatFrequency (float frequency)
{
    if (frequency >= 1000.0f)
        return juce::String (frequency / 1000.0f, frequency < 10000.0f ? 1 : 0) + " kHz";
    return juce::String (juce::roundToInt (frequency)) + " Hz";
}

int getOversamplingFactor (juce::AudioProcessorValueTreeState& state) noexcept
{
    constexpr std::array<int, 4> factors { 1, 2, 4, 8 };
    const auto index = juce::jlimit (0, 3,
        juce::roundToInt (state.getRawParameterValue ("oversample")->load()));
    return factors[static_cast<size_t> (index)];
}

void setParameterActual (juce::AudioProcessorValueTreeState& state, const char* id, float value)
{
    if (auto* parameter = state.getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

struct LowerWorkspaceLayout
{
    juce::Rectangle<float> masterTab;
    juce::Rectangle<float> reactorTab;
    juce::Rectangle<float> masterPlot;
    juce::Rectangle<float> masterControls;
    juce::Rectangle<float> weldSlider;
    juce::Rectangle<float> limiterThresholdSlider;
    juce::Rectangle<float> limiterCeilingSlider;
    juce::Rectangle<float> limiterEnabled;
    juce::Rectangle<float> gainReduction;
    std::array<juce::Rectangle<float>, 4> reactorTabs;
    juce::Rectangle<float> context;
    juce::Rectangle<float> amountSlider;
    juce::Rectangle<float> characterASlider;
    juce::Rectangle<float> characterBSlider;
    juce::Rectangle<float> solo;
    juce::Rectangle<float> enabled;
    juce::Rectangle<float> preEqLabel;
    juce::Rectangle<float> reactorPlot;
    juce::Rectangle<float> valueRow;
};

LowerWorkspaceLayout makeLowerWorkspaceLayout (juce::Rectangle<float> area) noexcept
{
    LowerWorkspaceLayout layout;
    auto inner = area.reduced (5.0f);
    auto topTabs = inner.removeFromTop (27.0f);
    const auto topWidth = (topTabs.getWidth() - 4.0f) * 0.5f;
    layout.masterTab = topTabs.removeFromLeft (topWidth);
    topTabs.removeFromLeft (4.0f);
    layout.reactorTab = topTabs;
    inner.removeFromTop (4.0f);
    auto reactorInner = inner;
    layout.masterControls = inner.removeFromTop (editorLayout.masterControls.getHeight());
    inner.removeFromTop (8.0f);
    layout.masterPlot = inner;
    layout.weldSlider = editorLayout.weld;
    layout.limiterThresholdSlider = editorLayout.limiterThreshold;
    layout.limiterCeilingSlider = editorLayout.limiterCeiling;
    layout.limiterEnabled = editorLayout.limiter;
    layout.gainReduction = { layout.masterControls.getRight() - 157.0f, layout.masterControls.getY() + 16.0f, 150.0f, 42.0f };

    auto reactorTabs = reactorInner.removeFromTop (31.0f);
    const auto reactorWidth = reactorTabs.getWidth() / 4.0f;
    for (size_t index = 0; index < layout.reactorTabs.size(); ++index)
    {
        layout.reactorTabs[index] = juce::Rectangle<float> (
            reactorTabs.getX() + reactorWidth * static_cast<float> (index), reactorTabs.getY(),
            reactorWidth, reactorTabs.getHeight()).reduced (2.0f, 1.0f);
    }
    reactorInner.removeFromTop (4.0f);
    layout.context = reactorInner.removeFromTop (49.0f);
    layout.amountSlider = { layout.context.getX() + 113.0f, layout.context.getY() + 10.0f, 42.0f, 39.0f };
    layout.characterASlider = { layout.context.getX() + 205.0f, layout.context.getY() + 10.0f, 42.0f, 39.0f };
    layout.characterBSlider = { layout.context.getX() + 297.0f, layout.context.getY() + 10.0f, 42.0f, 39.0f };
    layout.enabled = layout.context.removeFromRight (70.0f).reduced (2.0f, 9.0f);
    layout.solo = layout.context.removeFromRight (76.0f).reduced (2.0f, 9.0f);
    layout.preEqLabel = reactorInner.removeFromTop (14.0f);
    layout.valueRow = reactorInner.removeFromBottom (18.0f);
    layout.reactorPlot = reactorInner;
    return layout;
}

void drawDistressedFrame (juce::Graphics& g, juce::Rectangle<float> area, float cornerRadius,
                           int seed, int damageCount, juce::Colour accent)
{
    g.setColour (accent.withAlpha (0.62f));
    g.drawRoundedRectangle (area, cornerRadius, 0.9f);
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawRoundedRectangle (area.reduced (1.5f), juce::jmax (0.5f, cornerRadius - 0.8f), 0.8f);
    g.setColour (juce::Colour (0xff30231e).withAlpha (0.88f));
    g.drawRoundedRectangle (area.reduced (3.6f), juce::jmax (0.5f, cornerRadius - 1.5f), 0.55f);

    juce::Random random (static_cast<juce::int64> (seed));
    for (int damage = 0; damage < damageCount; ++damage)
    {
        const auto horizontal = random.nextBool();
        const auto farEdge = random.nextBool();
        const auto length = 1.2f + std::pow (random.nextFloat(), 1.7f) * 10.0f;
        const auto inset = 0.35f + random.nextFloat() * 1.2f;
        juce::Line<float> nick;
        if (horizontal)
        {
            const auto x = area.getX() + 5.0f + random.nextFloat() * juce::jmax (1.0f, area.getWidth() - 10.0f - length);
            const auto y = farEdge ? area.getBottom() - inset : area.getY() + inset;
            nick = { x, y, x + length, y + (random.nextFloat() - 0.5f) * 0.65f };
        }
        else
        {
            const auto y = area.getY() + 5.0f + random.nextFloat() * juce::jmax (1.0f, area.getHeight() - 10.0f - length);
            const auto x = farEdge ? area.getRight() - inset : area.getX() + inset;
            nick = { x, y, x + (random.nextFloat() - 0.5f) * 0.65f, y + length };
        }

        g.setColour (juce::Colours::black.withAlpha (0.62f + random.nextFloat() * 0.24f));
        g.drawLine (nick, 0.8f + random.nextFloat() * 0.65f);
        if (damage % 3 == 0)
        {
            const auto offset = juce::Point<float> (horizontal ? 0.0f : -0.55f,
                                                     horizontal ? -0.55f : 0.0f);
            const juce::Line<float> highlight (nick.getStart() + offset, nick.getEnd() + offset);
            g.setColour (accent.brighter (0.55f).withAlpha (0.17f + random.nextFloat() * 0.11f));
            g.drawLine (highlight, 0.45f);
        }
    }
}
}

VoidwormAudioProcessorEditor::AnimatedToggleButton::AnimatedToggleButton (
    VoidwormAudioProcessorEditor& editor, const juce::String& name)
    : juce::ToggleButton (name), owner (editor)
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void VoidwormAudioProcessorEditor::AnimatedToggleButton::snapToState() noexcept
{
    visualPosition = getToggleState() ? 1.0f : 0.0f;
    initialised = true;
    repaint();
}

bool VoidwormAudioProcessorEditor::AnimatedToggleButton::updateAnimation (float step) noexcept
{
    const auto target = getToggleState() ? 1.0f : 0.0f;
    if (! initialised) { visualPosition = target; initialised = true; return false; }
    if (std::abs (target - visualPosition) < 0.012f) { visualPosition = target; return false; }
    visualPosition += (target - visualPosition) * juce::jlimit (0.05f, 1.0f, step);
    repaint();
    return true;
}

void VoidwormAudioProcessorEditor::AnimatedToggleButton::paintButton (
    juce::Graphics& g, bool highlighted, bool down)
{
    if (! initialised) snapToState();
    const auto lowAccentColour = owner.lowAccent();
    const auto accent = owner.primaryAccent();
    const auto highlight = owner.secondaryAccent();
    auto area = getLocalBounds().toFloat().reduced (1.0f, 2.0f);
    const auto rawAmount = juce::jlimit (0.0f, 1.0f, visualPosition);
    const auto onAmount = rawAmount * rawAmount * (3.0f - 2.0f * rawAmount);
    const auto housingHeight = juce::jmin (36.0f, area.getHeight());
    const auto housingWidth = juce::jmin (102.0f, area.getWidth());
    const auto housing = juce::Rectangle<float> (housingWidth, housingHeight).withCentre (area.getCentre());
    const auto housingRadius = 8.0f;

    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.fillRoundedRectangle (housing.translated (0.0f, 2.0f).expanded (1.2f, 0.7f), housingRadius + 1.0f);
    juce::ColourGradient steel (juce::Colour (0xff302c29), housing.getX(), housing.getY(),
                                juce::Colour (0xff080808), housing.getX(), housing.getBottom(), false);
    steel.addColour (0.38, juce::Colour (0xff1d1a18));
    g.setGradientFill (steel); g.fillRoundedRectangle (housing, housingRadius);
    g.setColour (juce::Colour (0xff5c5550).withAlpha (highlighted ? 0.48f : 0.28f));
    g.drawRoundedRectangle (housing.reduced (0.5f), housingRadius, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.drawHorizontalLine (juce::roundToInt (housing.getY() + 1.0f), housing.getX() + 8.0f, housing.getRight() - 8.0f);

    const auto trench = housing.reduced (4.0f, 3.0f);
    juce::ColourGradient recess (juce::Colour (0xff050505).interpolatedWith (lowAccentColour, onAmount * 0.20f),
                                 trench.getX(), trench.getCentreY(),
                                 juce::Colour (0xff090909).interpolatedWith (highlight, onAmount * 0.56f),
                                 trench.getRight(), trench.getCentreY(), false);
    recess.addColour (0.50, juce::Colour (0xff070707).interpolatedWith (accent, onAmount * 0.42f));
    g.setGradientFill (recess);
    g.fillRoundedRectangle (trench, trench.getHeight() * 0.5f);
    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.drawRoundedRectangle (trench, trench.getHeight() * 0.5f, 1.2f);

    const auto diameter = juce::jmin (32.0f, housingHeight - 4.0f);
    const auto left = housing.getX() + 2.0f;
    const auto right = housing.getRight() - diameter - 2.0f;
    const auto thumb = juce::Rectangle<float> (juce::jmap (onAmount, left, right), housing.getCentreY() - diameter * 0.5f,
                                                diameter, diameter);
    g.setColour (juce::Colours::black.withAlpha (0.84f));
    g.fillEllipse (thumb.translated (0.0f, 1.7f).expanded (1.0f));
    juce::ColourGradient metal (juce::Colour (0xff3d3936),
                                thumb.getX(), thumb.getY() + (down ? 1.0f : 0.0f),
                                juce::Colour (0xff0a0909), thumb.getRight(), thumb.getBottom(), false);
    metal.addColour (0.45, juce::Colour (0xff24211f).interpolatedWith (lowAccentColour, onAmount * 0.05f));
    g.setGradientFill (metal); g.fillEllipse (thumb);
    g.setColour (juce::Colour (0xff68615c).withAlpha (down ? 0.52f : 0.34f));
    g.drawEllipse (thumb.reduced (0.5f), 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.drawEllipse (thumb.reduced (3.5f), 1.0f);
    const auto core = thumb.reduced (diameter * 0.36f);
    juce::ColourGradient coreGlow (highlight.withAlpha (0.08f + onAmount * 0.82f), core.getCentre(),
                                   lowAccentColour.withAlpha (0.04f + onAmount * 0.32f),
                                   juce::Point<float> (core.getRight(), core.getCentreY()), true);
    coreGlow.addColour (0.55, accent.withAlpha (0.06f + onAmount * 0.56f));
    g.setGradientFill (coreGlow); g.fillEllipse (core);
    g.setColour ((onAmount > 0.5f ? highlight : lowAccentColour).withAlpha (0.28f + onAmount * 0.42f));
    g.drawEllipse (core.reduced (0.35f), 0.75f);
}

VoidwormAudioProcessorEditor::CircularPushButton::CircularPushButton (
    VoidwormAudioProcessorEditor& editor, const juce::String& name)
    : juce::Button (name), owner (editor)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void VoidwormAudioProcessorEditor::CircularPushButton::setActive (bool shouldBeActive) noexcept
{
    if (active == shouldBeActive) return;
    active = shouldBeActive;
    repaint();
}

void VoidwormAudioProcessorEditor::CircularPushButton::paintButton (
    juce::Graphics& g, bool highlighted, bool down)
{
    const auto lowAccentColour = owner.lowAccent();
    const auto accent = owner.primaryAccent();
    const auto secondary = owner.secondaryAccent();
    auto housing = getLocalBounds().toFloat().reduced (1.0f);
    const auto diameter = juce::jmin (housing.getWidth(), housing.getHeight());
    housing = juce::Rectangle<float> (diameter, diameter).withCentre (housing.getCentre());
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillEllipse (housing.translated (0.0f, 1.4f).expanded (0.8f));
    juce::ColourGradient rim (juce::Colour (0xff766e68), housing.getX(), housing.getY(),
                              juce::Colour (0xff181615), housing.getX(), housing.getBottom(), false);
    g.setGradientFill (rim); g.fillEllipse (housing);
    g.setColour (juce::Colour (0xffa19892).withAlpha (highlighted ? 0.66f : 0.43f));
    g.drawEllipse (housing.reduced (0.45f), 0.85f);
    const auto well = housing.reduced (3.0f);
    g.setColour (juce::Colour (0xff070707)); g.fillEllipse (well);
    g.setColour (juce::Colours::black.withAlpha (0.84f)); g.drawEllipse (well, 0.9f);
    auto cap = well.reduced (down ? 3.6f : 2.7f);
    juce::ColourGradient capMetal (juce::Colour (0xff5e5753).interpolatedWith (secondary, active ? 0.10f : 0.0f),
                                   cap.getX(), cap.getY(), juce::Colour (0xff181615),
                                   cap.getRight(), cap.getBottom(), false);
    g.setGradientFill (capMetal); g.fillEllipse (cap);
    g.setColour (juce::Colour (0xff9a9089).withAlpha (0.42f)); g.drawEllipse (cap, 0.7f);
    const auto lamp = cap.reduced (cap.getWidth() * 0.32f);
    if (active)
    {
        juce::ColourGradient lampGlow (secondary.withAlpha (0.92f), lamp.getCentre(),
                                       lowAccentColour.withAlpha (0.42f),
                                       juce::Point<float> (lamp.getRight(), lamp.getCentreY()), true);
        lampGlow.addColour (0.58, accent.withAlpha (0.82f));
        g.setGradientFill (lampGlow); g.fillEllipse (lamp);
    }
    else { g.setColour (juce::Colour (0xff211b19)); g.fillEllipse (lamp); }
    g.setColour (active ? secondary.withAlpha (0.58f) : juce::Colours::black.withAlpha (0.74f));
    g.drawEllipse (lamp, 0.65f);
}

void VoidwormAudioProcessorEditor::StandaloneToast::paint (juce::Graphics& g)
{
    const auto accent = owner.primaryAccent();
    const auto hot = owner.secondaryAccent();
    const auto area = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillRoundedRectangle (area.translated (0.0f, 2.0f), 5.0f);
    juce::ColourGradient surface (juce::Colour (0xff1c1917), area.getX(), area.getY(),
                                  juce::Colour (0xff070707), area.getX(), area.getBottom(), false);
    g.setGradientFill (surface);
    g.fillRoundedRectangle (area, 5.0f);
    g.setColour (accent.interpolatedWith (hot, 0.45f).withAlpha (0.78f));
    g.drawRoundedRectangle (area, 5.0f, 0.9f);
    g.setColour (owner.themeText().withAlpha (0.92f));
    g.setFont (juce::FontOptions (juce::jmax (9.0f, getHeight() * 0.29f))
                   .withName ("Bahnschrift").withStyle ("SemiBold"));
    g.drawFittedText (message, getLocalBounds().reduced (12, 0), juce::Justification::centred, 1);
}


VoidwormAudioProcessorEditor::OverlayPanel::OverlayPanel (VoidwormAudioProcessorEditor& editor)
    : owner (editor)
{
    setVisible (false);
    setWantsKeyboardFocus (true);
    addKeyListener (this);
    searchEditor.setMultiLine (false);
    searchEditor.setReturnKeyStartsNewLine (false);
    searchEditor.setJustification (juce::Justification::centredLeft);
    searchEditor.setTextToShowWhenEmpty ("SEARCH PRESETS...", juce::Colour (0xff77706b));
    searchEditor.onTextChange = [this]
    {
        owner.headerPanelState.setSearchQuery (searchEditor.getText());
        owner.headerPanelState.searchScrollRows = 0;
        repaint();
    };
    searchEditor.addKeyListener (this);
    addAndMakeVisible (searchEditor);
    clearSearchButton.onClick = [this]
    {
        searchEditor.clear();
        searchEditor.grabKeyboardFocus();
    };
    clearSearchButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (clearSearchButton);
    presetNameEditor.setMultiLine (false);
    presetNameEditor.setReturnKeyStartsNewLine (false);
    presetNameEditor.setInputRestrictions (voidworm::UserPresetStore::maximumNameLength);
    presetNameEditor.addKeyListener (this);
    presetNameEditor.onReturnKey = [this] { performDialogAction(); };
    addChildComponent (presetNameEditor);
    refreshTheme();
}

VoidwormAudioProcessorEditor::OverlayPanel::~OverlayPanel()
{
    searchEditor.removeKeyListener (this);
    presetNameEditor.removeKeyListener (this);
    removeKeyListener (this);
}

juce::Rectangle<float> VoidwormAudioProcessorEditor::OverlayPanel::panelBounds() const noexcept
{
    using P = voidworm::ui::HeaderPanel;
    const auto panelTop = editorLayout.mainControls.getY();
    switch (owner.headerPanelState.panel)
    {
        case P::gate: return { 449.0f, panelTop, 246.0f, 112.0f };
        case P::settings: return { 326.0f, panelTop, 369.0f, settingsThemesExpanded ? 490.0f : 354.0f };
        case P::presetBrowser:
        case P::presetMatrix:
        case P::presetSearch: return { 190.0f, panelTop, 505.0f, 400.0f };
        default: break;
    }
    return {};
}

juce::Rectangle<float> VoidwormAudioProcessorEditor::OverlayPanel::bodyBounds() const noexcept
{
    auto body = panelBounds().reduced (uiStyle::overlayPadding);
    body.removeFromTop (uiStyle::overlayHeader - uiStyle::overlayPadding);
    return body;
}

juce::Rectangle<float> VoidwormAudioProcessorEditor::OverlayPanel::settingsControlBounds() const noexcept
{
    auto body = bodyBounds();
    return body;
}

juce::Rectangle<float> VoidwormAudioProcessorEditor::OverlayPanel::settingsThemeBounds() const noexcept
{
    auto body = bodyBounds();
    if (! settingsThemesExpanded)
        return {};
    return { body.getX(), body.getY() + 180.0f, body.getWidth(), 139.0f };
}

juce::Rectangle<float> VoidwormAudioProcessorEditor::OverlayPanel::gateThresholdBounds() const noexcept
{
    auto body = bodyBounds();
    body.removeFromTop (17.0f);
    return body.removeFromTop (31.0f);
}

void VoidwormAudioProcessorEditor::OverlayPanel::showMode (voidworm::ui::HeaderPanel newMode)
{
    refreshUserPresets();
    closeDialog();
    owner.headerPanelState.open (newMode);
    if (! owner.headerPanelState.isOpen())
    {
        hide();
        return;
    }
    setVisible (true);
    toFront (false);
    if (newMode == voidworm::ui::HeaderPanel::presetBrowser)
    {
        const auto& presets = voidworm::factoryPresets();
        auto selectedRow = 0;
        juce::String previousFamily;
        for (int index = 0; index <= juce::jlimit (0, static_cast<int> (presets.size()) - 1, owner.presetIndex); ++index)
        {
            const juce::String family (presets[static_cast<size_t> (index)].family);
            if (family != previousFamily) { ++selectedRow; previousFamily = family; }
            if (index < owner.presetIndex) ++selectedRow;
        }
        constexpr auto visibleRows = 18;
        owner.headerPanelState.browserScrollRows = juce::jlimit (0, 42, selectedRow - visibleRows / 2);
    }
    else if (newMode == voidworm::ui::HeaderPanel::settings)
    {
        constexpr auto visibleRows = 3;
        owner.headerPanelState.themeScrollRows = juce::jlimit (0, voidworm::ui::themeCount - visibleRows,
            owner.getThemeIndex() - visibleRows / 2);
    }
    const auto searchVisible = owner.headerPanelState.panel == voidworm::ui::HeaderPanel::presetSearch;
    searchEditor.setVisible (searchVisible);
    clearSearchButton.setVisible (searchVisible);
    resized();
    refreshTheme();
    if (searchVisible)
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<OverlayPanel> (this)]
        {
            if (safe != nullptr && safe->isVisible())
                safe->searchEditor.grabKeyboardFocus();
        });
    else
        grabKeyboardFocus();
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::hide()
{
    searchEditor.giveAwayKeyboardFocus();
    searchEditor.setVisible (false);
    clearSearchButton.setVisible (false);
    presetNameEditor.giveAwayKeyboardFocus();
    presetNameEditor.setVisible (false);
    owner.headerPanelState.close();
    setVisible (false);
    owner.grabKeyboardFocus();
}

void VoidwormAudioProcessorEditor::OverlayPanel::refreshTheme()
{
    const auto accent = owner.primaryAccent();
    searchEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchEditor.setColour (juce::TextEditor::textColourId, owner.themeText());
    searchEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    searchEditor.setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.30f));
    searchEditor.setColour (juce::CaretComponent::caretColourId, accent);
    presetNameEditor.setColour (juce::TextEditor::backgroundColourId, uiStyle::utilitySurface);
    presetNameEditor.setColour (juce::TextEditor::textColourId, owner.themeText());
    presetNameEditor.setColour (juce::TextEditor::outlineColourId, uiStyle::utilityEdge);
    presetNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, accent.withAlpha (0.82f));
    presetNameEditor.setColour (juce::CaretComponent::caretColourId, accent);
}

void VoidwormAudioProcessorEditor::OverlayPanel::resized()
{
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    auto field = bodyBounds().removeFromTop (36.0f);
    auto actual = juce::Rectangle<int> (juce::roundToInt (field.getX() * sx), juce::roundToInt (field.getY() * sy),
                                        juce::roundToInt (field.getWidth() * sx), juce::roundToInt (field.getHeight() * sy));
    const auto fullField = actual;
    clearSearchButton.setBounds (actual.removeFromRight (32));
    searchEditor.setBounds (fullField.withTrimmedRight (32));
    searchEditor.setIndents (juce::roundToInt (uiStyle::m * sx), 0);
    searchEditor.setFont (juce::FontOptions (uiStyle::searchFont * sy).withName ("Bahnschrift"));
    searchEditor.setJustification (juce::Justification::centredLeft);
    clearSearchButton.toFront (false);
    const auto dialogField = juce::Rectangle<float> (panelBounds().getCentreX() - 174.0f,
        panelBounds().getCentreY() - 12.0f, 348.0f, 34.0f);
    presetNameEditor.setBounds (juce::Rectangle<int> (juce::roundToInt (dialogField.getX() * sx),
        juce::roundToInt (dialogField.getY() * sy), juce::roundToInt (dialogField.getWidth() * sx),
        juce::roundToInt (dialogField.getHeight() * sy)));
    presetNameEditor.setIndents (juce::roundToInt (uiStyle::m * sx), 0);
    presetNameEditor.setFont (juce::FontOptions (uiStyle::searchFont * sy).withName ("Bahnschrift"));
}

void VoidwormAudioProcessorEditor::OverlayPanel::refreshUserPresets()
{
    owner.userPresets.refresh();
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::paint (juce::Graphics& g)
{
    using P = voidworm::ui::HeaderPanel;
    const auto accent = owner.primaryAccent();
    const auto secondary = owner.secondaryAccent();
    const auto text = owner.themeText();
    const auto dim = owner.themeDimText();
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    g.addTransform (juce::AffineTransform::scale (sx, sy));
    const auto panel = panelBounds();
    if (panel.isEmpty()) return;
    g.setColour (juce::Colours::black.withAlpha (0.52f));
    g.fillRoundedRectangle (panel.translated (0.0f, 5.0f), uiStyle::overlayRadius);
    juce::ColourGradient background (juce::Colour (0xff211d1a), panel.getX(), panel.getY(),
                                     juce::Colour (0xff070707), panel.getX(), panel.getBottom(), false);
    g.setGradientFill (background);
    g.fillRoundedRectangle (panel, uiStyle::overlayRadius);
    owner.drawSteelTexture (g, panel.reduced (1.0f), 82);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.92f));
    g.drawRoundedRectangle (panel, uiStyle::overlayRadius, 0.8f);
    g.setColour (juce::Colours::white.withAlpha (0.075f));
    g.drawHorizontalLine (juce::roundToInt (panel.getY() + 1.0f), panel.getX() + 8.0f, panel.getRight() - 8.0f);
    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.drawHorizontalLine (juce::roundToInt (panel.getBottom() - 1.0f), panel.getX() + 8.0f, panel.getRight() - 8.0f);
    g.setColour (accent.withAlpha (0.50f));
    g.fillRoundedRectangle (panel.getX() + uiStyle::m, panel.getY(), 42.0f, 1.2f, 0.6f);
    const auto mode = owner.headerPanelState.panel;
    const auto title = mode == P::presetBrowser ? "PRESET BROWSER"
                     : mode == P::presetMatrix ? "PRESET MATRIX"
                     : mode == P::presetSearch ? "SEARCH PRESETS"
                     : mode == P::gate ? "INPUT GATE" : "PREFERENCES";
    g.setColour (text);
    g.setFont (juce::FontOptions (uiStyle::titleFont).withName ("Bahnschrift").withStyle ("SemiBold"));
    g.drawText (title, juce::Rectangle<float> (panel.getX() + 13.0f, panel.getY() + 7.0f,
                panel.getWidth() - 55.0f, 20.0f).toNearestInt(), juce::Justification::centredLeft);
    const auto closeArea = juce::Rectangle<float> (panel.getRight() - 38.0f, panel.getY(), 38.0f, 34.0f);
    g.setColour (closeArea.contains (hoverPoint) ? accent : dim.withAlpha (0.72f));
    g.setFont (juce::FontOptions (14.0f).withName ("Bahnschrift").withStyle ("Light"));
    g.drawText ("x", juce::roundToInt (panel.getRight() - 31.0f), juce::roundToInt (panel.getY() + 5.0f),
                22, 22, juce::Justification::centred);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.52f));
    g.drawHorizontalLine (juce::roundToInt (panel.getY() + uiStyle::overlayHeader),
                          panel.getX() + uiStyle::m, panel.getRight() - uiStyle::m);

    const auto recessedBody = bodyBounds().expanded (4.0f, 3.0f);
    juce::ColourGradient bodyWell (juce::Colour (0xff060606), recessedBody.getX(), recessedBody.getY(),
                                   juce::Colour (0xff100e0d), recessedBody.getX(), recessedBody.getBottom(), false);
    g.setGradientFill (bodyWell);
    g.fillRoundedRectangle (recessedBody, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawRoundedRectangle (recessedBody, 4.0f, 1.0f);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.40f));
    g.drawRoundedRectangle (recessedBody.reduced (1.0f), 3.0f, 0.55f);

    if (mode == P::presetSearch)
    {
        const auto searchField = bodyBounds().removeFromTop (36.0f);
        auto textZone = searchField;
        const auto clearZone = textZone.removeFromRight (32.0f);
        const auto searchHover = searchField.contains (hoverPoint);
        const auto focused = searchEditor.hasKeyboardFocus (true);
        juce::ColourGradient searchWell (juce::Colour (0xff050505), searchField.getX(), searchField.getY(),
                                         juce::Colour (0xff11100f), searchField.getX(), searchField.getBottom(), false);
        g.setGradientFill (searchWell); g.fillRoundedRectangle (searchField, 4.0f);
        g.setColour (focused ? accent.withAlpha (0.86f)
                             : searchHover ? uiStyle::utilityEdge.brighter (0.20f) : uiStyle::utilityEdge);
        g.drawRoundedRectangle (searchField, 4.0f, focused ? 1.0f : 0.7f);
        g.setColour (uiStyle::utilityEdge.withAlpha (0.62f));
        g.drawVerticalLine (juce::roundToInt (clearZone.getX()), clearZone.getY() + 7.0f,
                            clearZone.getBottom() - 7.0f);
        g.setColour (clearSearchButton.isMouseOver() ? accent : dim);
        g.setFont (juce::FontOptions (11.0f).withName ("Bahnschrift").withStyle ("Light"));
        g.drawText ("x", clearZone.toNearestInt(), juce::Justification::centred);
    }

    const auto body = bodyBounds();
    const auto& presets = voidworm::factoryPresets();
    g.saveState();
    g.reduceClipRegion (body.toNearestInt());
    if (mode == P::presetBrowser)
    {
        constexpr auto rowHeight = 19.0f;
        auto rowIndex = -owner.headerPanelState.browserScrollRows;
        juce::String previousFamily;
        for (size_t index = 0; index < presets.size(); ++index)
        {
            const juce::String family (presets[index].family);
            if (family != previousFamily)
            {
                const auto y = body.getY() + rowIndex++ * rowHeight;
                if (y + rowHeight >= body.getY() && y < body.getBottom())
                {
                    g.setColour (accent.withAlpha (0.58f));
                    g.setFont (juce::FontOptions (uiStyle::sectionFont).withName ("Bahnschrift").withStyle ("SemiBold"));
                    g.drawText (family, juce::Rectangle<float> (body.getX() + 4.0f, y,
                        body.getWidth() - 8.0f, rowHeight).toNearestInt(), juce::Justification::centredLeft);
                }
                previousFamily = family;
            }
            const auto y = body.getY() + rowIndex++ * rowHeight;
            if (y + rowHeight < body.getY() || y >= body.getBottom()) continue;
            const auto row = juce::Rectangle<float> (body.getX() + 3.0f, y, body.getWidth() - 6.0f, rowHeight - 1.0f);
            const auto active = owner.presetAssociation == PresetAssociation::factory
                             && static_cast<int> (index) == owner.presetIndex;
            const auto hovered = row.contains (hoverPoint);
            if (active || hovered)
            {
                g.setColour (active ? accent.withAlpha (0.14f) : uiStyle::utilityHover.withAlpha (0.78f));
                g.fillRoundedRectangle (row, 2.0f);
            }
            if (active) { g.setColour (accent.withAlpha (0.86f)); g.fillRect (row.getX(), row.getY() + 3.0f, 1.2f, row.getHeight() - 6.0f); }
            g.setColour (active ? secondary.brighter (0.18f) : text.withAlpha (0.86f));
            g.setFont (juce::FontOptions (uiStyle::normalFont).withName ("Bahnschrift"));
            g.drawText (presets[index].name, row.reduced (13.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft);
        }
        auto y = body.getY() + rowIndex++ * rowHeight;
        if (y + rowHeight >= body.getY() && y < body.getBottom())
        {
            g.setColour (uiStyle::utilityEdge.withAlpha (0.62f)); g.drawHorizontalLine (juce::roundToInt (y + 4.0f), body.getX(), body.getRight());
            g.setColour (accent.withAlpha (0.62f)); g.setFont (juce::FontOptions (uiStyle::sectionFont).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText ("USER PRESETS", juce::Rectangle<float> (body.getX() + 4.0f, y + 5.0f,
                body.getWidth() - 8.0f, rowHeight).toNearestInt(), juce::Justification::centredLeft);
        }
        ++rowIndex;
        if (owner.userPresets.presets().empty())
        {
            y = body.getY() + rowIndex++ * rowHeight;
            if (y + rowHeight >= body.getY() && y < body.getBottom())
            {
                g.setColour (dim.withAlpha (0.68f));
                g.setFont (juce::FontOptions (uiStyle::secondaryFont).withName ("Bahnschrift"));
                g.drawText ("NO USER PRESETS YET", juce::Rectangle<float> (body.getX() + 13.0f, y,
                    body.getWidth() - 26.0f, rowHeight).toNearestInt(), juce::Justification::centredLeft);
            }
        }
        for (size_t index = 0; index < owner.userPresets.presets().size(); ++index)
        {
            y = body.getY() + rowIndex++ * rowHeight;
            if (y + rowHeight < body.getY() || y >= body.getBottom()) continue;
            const auto row = juce::Rectangle<float> (body.getX() + 3.0f, y, body.getWidth() - 6.0f, rowHeight - 1.0f);
            const auto active = owner.presetAssociation == PresetAssociation::user
                && owner.currentUserPresetFile == owner.userPresets.presets()[index].file;
            const auto hovered = row.contains (hoverPoint);
            if (active || hovered) { g.setColour (active ? accent.withAlpha (0.14f) : uiStyle::utilityHover); g.fillRoundedRectangle (row, 2.0f); }
            if (active) { g.setColour (accent.withAlpha (0.86f)); g.fillRect (row.getX(), row.getY() + 3.0f, 1.2f, row.getHeight() - 6.0f); }
            g.setColour (active ? secondary : text); g.setFont (juce::FontOptions (uiStyle::normalFont).withName ("Bahnschrift"));
            g.drawText (owner.userPresets.presets()[index].name, row.reduced (13.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft);
        }
        const auto actions = juce::Rectangle<float> (body.getX(), body.getBottom() - 28.0f, body.getWidth(), 26.0f);
        g.setColour (uiStyle::utilitySurface); g.fillRoundedRectangle (actions, 3.0f);
        g.setColour (uiStyle::utilityEdge); g.drawRoundedRectangle (actions, 3.0f, 0.7f);
        const auto userSelected = owner.presetAssociation == PresetAssociation::user;
        const auto count = userSelected ? 4 : 1;
        const auto width = actions.getWidth() / static_cast<float> (count);
        const std::array<const char*, 4> labels { "SAVE CURRENT", "RENAME", "OVERWRITE", "DELETE" };
        for (int index = 0; index < count; ++index)
        {
            g.setColour (index == 3 ? secondary.withAlpha (0.82f) : text.withAlpha (0.82f));
            g.setFont (juce::FontOptions (7.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText (labels[static_cast<size_t> (index)], juce::Rectangle<float> (actions.getX() + index * width,
                actions.getY(), width, actions.getHeight()).toNearestInt(), juce::Justification::centred);
            if (index > 0) { g.setColour (uiStyle::utilityEdge.withAlpha (0.72f)); g.drawVerticalLine (juce::roundToInt (actions.getX() + index * width), actions.getY() + 5.0f, actions.getBottom() - 5.0f); }
        }
    }
    else if (mode == P::presetMatrix)
    {
        auto families = voidworm::ui::presetFamilies();
        families.push_back ("USER");
        families.insert (families.begin(), "ALL");
        const auto chipWidth = body.getWidth() / 7.0f;
        const auto segmentRows = static_cast<int> ((families.size() + 6) / 7);
        for (int row = 0; row < segmentRows; ++row)
        {
            const auto segment = juce::Rectangle<float> (body.getX(), body.getY() + row * 20.0f,
                                                          body.getWidth(), 18.0f);
            g.setColour (uiStyle::utilitySurface); g.fillRoundedRectangle (segment, 3.0f);
            g.setColour (uiStyle::utilityEdge.withAlpha (0.72f)); g.drawRoundedRectangle (segment, 3.0f, 0.7f);
        }
        for (size_t index = 0; index < families.size(); ++index)
        {
            const auto chip = juce::Rectangle<float> (body.getX() + static_cast<float> (index % 7) * chipWidth,
                body.getY() + static_cast<float> (index / 7) * 20.0f, chipWidth, 18.0f);
            const auto active = owner.headerPanelState.matrixFamily.equalsIgnoreCase (families[index]);
            const auto hovered = chip.contains (hoverPoint);
            if (active || hovered) { g.setColour (active ? accent.withAlpha (0.075f) : uiStyle::utilityHover); g.fillRect (chip.reduced (1.0f, 1.0f)); }
            if (index % 7 != 0) { g.setColour (uiStyle::utilityEdge.withAlpha (0.52f)); g.drawVerticalLine (juce::roundToInt (chip.getX()), chip.getY() + 4.0f, chip.getBottom() - 4.0f); }
            if (active) { g.setColour (accent.withAlpha (0.72f)); g.fillRect (chip.getX() + 8.0f, chip.getBottom() - 1.2f, chip.getWidth() - 16.0f, 1.0f); }
            g.setColour (active ? text : dim);
            g.setFont (juce::FontOptions (6.7f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawFittedText (families[index], chip.toNearestInt().reduced (3, 0), juce::Justification::centred, 1);
        }
        const auto results = voidworm::ui::filterPresetItems (owner.userPresets.presets(), owner.headerPanelState.matrixFamily);
        constexpr auto columns = 3;
        constexpr auto rowHeight = 40.0f;
        const auto cardsTop = body.getY() + 42.0f;
        const auto cardWidth = (body.getWidth() - 8.0f) / static_cast<float> (columns);
        for (size_t result = 0; result < results.size(); ++result)
        {
            const auto rowIndex = static_cast<int> (result / columns) - owner.headerPanelState.matrixScrollRows;
            const auto column = static_cast<int> (result % columns);
            const auto card = juce::Rectangle<float> (body.getX() + column * cardWidth,
                cardsTop + rowIndex * rowHeight, cardWidth - 4.0f, 35.0f);
            if (card.getBottom() < cardsTop || card.getY() >= body.getBottom()) continue;
            const auto& item = results[result];
            const auto active = item.source == voidworm::ui::PresetSource::factory
                ? owner.presetAssociation == PresetAssociation::factory && static_cast<int> (item.index) == owner.presetIndex
                : owner.presetAssociation == PresetAssociation::user && owner.currentUserPresetFile == owner.userPresets.presets()[item.index].file;
            const auto hovered = card.contains (hoverPoint);
            juce::ColourGradient cardSurface (active ? uiStyle::utilityRaised.interpolatedWith (accent, 0.055f) : hovered ? uiStyle::utilityHover : uiStyle::utilityRaised,
                                               card.getX(), card.getY(), uiStyle::utilitySurface,
                                               card.getX(), card.getBottom(), false);
            g.setGradientFill (cardSurface); g.fillRoundedRectangle (card, 3.0f);
            g.setColour (active ? accent.withAlpha (0.68f) : hovered ? uiStyle::utilityEdge.brighter (0.18f) : uiStyle::utilityEdge.withAlpha (0.66f));
            g.drawRoundedRectangle (card, 3.0f, active ? 0.9f : 0.65f);
            g.setColour (active ? text : text.withAlpha (0.78f));
            g.setFont (juce::FontOptions (7.7f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawFittedText (item.name, card.withTrimmedBottom (12.0f).toNearestInt().reduced (5, 0),
                              juce::Justification::centred, 1);
            g.setColour (active ? secondary : dim.withAlpha (0.72f));
            g.setFont (juce::FontOptions (6.1f).withName ("Bahnschrift"));
            g.drawText (item.family, card.withTrimmedTop (21.0f).toNearestInt(), juce::Justification::centred);
        }
    }
    else if (mode == P::presetSearch)
    {
        const auto results = voidworm::ui::searchPresetItems (owner.userPresets.presets(), owner.headerPanelState.searchQuery);
        const auto resultsTop = body.getY() + 39.0f;
        constexpr auto rowHeight = 27.0f;
        if (results.empty())
        {
            g.setColour (dim);
            g.setFont (juce::FontOptions (9.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText ("NO PRESETS FOUND", juce::Rectangle<float> (body.getX(), resultsTop + 6.0f,
                body.getWidth(), 28.0f).toNearestInt(), juce::Justification::centred);
        }
        for (size_t result = 0; result < results.size(); ++result)
        {
            const auto visualRow = static_cast<int> (result) - owner.headerPanelState.searchScrollRows;
            const auto row = juce::Rectangle<float> (body.getX() + 2.0f, resultsTop + visualRow * rowHeight,
                                                      body.getWidth() - 4.0f, rowHeight - 2.0f);
            if (row.getBottom() < resultsTop || row.getY() >= body.getBottom()) continue;
            const auto& item = results[result];
            const auto keyboard = static_cast<int> (result) == owner.headerPanelState.highlightedSearchResult;
            const auto active = item.source == voidworm::ui::PresetSource::factory
                ? owner.presetAssociation == PresetAssociation::factory && static_cast<int> (item.index) == owner.presetIndex
                : owner.presetAssociation == PresetAssociation::user && owner.currentUserPresetFile == owner.userPresets.presets()[item.index].file;
            const auto hovered = row.contains (hoverPoint);
            if (keyboard || active || hovered) { g.setColour (active ? accent.withAlpha (0.14f) : hovered ? uiStyle::utilityHover : uiStyle::utilityRaised); g.fillRoundedRectangle (row, 2.0f); }
            if (keyboard || active) { g.setColour (active ? secondary : accent); g.fillRect (row.getX(), row.getY() + 4.0f, 1.2f, row.getHeight() - 8.0f); }
            auto nameArea = row.reduced (8.0f, 0.0f);
            auto familyArea = nameArea.removeFromRight (92.0f);
            g.setColour (text); g.setFont (juce::FontOptions (8.5f).withName ("Bahnschrift"));
            g.drawText (item.name, nameArea.toNearestInt(), juce::Justification::centredLeft);
            g.setColour (dim); g.setFont (juce::FontOptions (7.0f).withName ("Bahnschrift"));
            g.drawText (item.family, familyArea.toNearestInt(), juce::Justification::centredRight);
        }
    }
    else if (mode == P::gate)
    {
        auto thresholdRow = gateThresholdBounds();
        const auto threshold = owner.processor.parameters.getRawParameterValue ("gateThreshold")->load();
        const auto enabled = owner.processor.parameters.getRawParameterValue ("gateEnabled")->load() >= 0.5f;
        const auto muting = enabled && owner.processor.isInputGateMuting();
        g.setColour (muting ? secondary : enabled ? accent : dim);
        g.setFont (juce::FontOptions (6.8f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText (enabled ? (muting ? "ON  /  MUTING" : "ON  /  OPEN") : "OFF  /  CLICK GATE TO ENABLE",
                    body.withHeight (14.0f).toNearestInt(), juce::Justification::centredLeft);
        const auto trackLeft = thresholdRow.getX() + 62.0f;
        const auto trackRight = thresholdRow.getRight() - 64.0f;
        const auto track = juce::Rectangle<float> (trackLeft, thresholdRow.getCentreY() - 1.0f,
                                                    trackRight - trackLeft, 2.0f);
        const auto proportion = juce::jlimit (0.0f, 1.0f, (threshold + 80.0f) / 60.0f);
        g.setColour (text.withAlpha (0.84f));
        g.setFont (juce::FontOptions (7.4f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("THRESHOLD", thresholdRow.withWidth (58.0f).toNearestInt(), juce::Justification::centredLeft);
        g.setColour (uiStyle::utilityEdge.withAlpha (0.82f));
        g.fillRoundedRectangle (track, 1.0f);
        g.setColour (accent.withAlpha (thresholdRow.contains (hoverPoint) ? 0.92f : 0.70f));
        g.fillRoundedRectangle (track.withWidth (track.getWidth() * proportion), 1.0f);
        g.fillEllipse (track.getX() + track.getWidth() * proportion - 3.0f,
                       track.getCentreY() - 3.0f, 6.0f, 6.0f);
        g.setColour (text.withAlpha (0.80f));
        g.setFont (juce::FontOptions (7.4f).withName ("Bahnschrift"));
        g.drawText (juce::String (threshold, 1) + " dB", thresholdRow.removeFromRight (58.0f).toNearestInt(),
                    juce::Justification::centredRight);
    }
    else if (mode == P::settings)
    {
        const auto controls = settingsControlBounds();
        const auto themes = settingsThemeBounds();
        constexpr std::array<const char*, 3> scales { "90%", "100%", "125%" };
        constexpr std::array<const char*, 4> floors { "-48 dB", "-60 dB", "-72 dB", "-96 dB" };
        constexpr std::array<const char*, 4> holds { "OFF", "SHORT", "MEDIUM", "LONG" };
        const std::array<std::pair<juce::String, juce::String>, 3> rows {{
            { "UI SCALE", scales[static_cast<size_t> (owner.uiScaleIndex)] },
            { "METER FLOOR", floors[static_cast<size_t> (owner.meterFloorIndex)] },
            { "METER HOLD", holds[static_cast<size_t> (owner.meterHoldIndex)] }
        }};
        const auto drawSettingRow = [&] (juce::Rectangle<float> row, const juce::String& label,
                                         const juce::String& value)
        {
            const auto hovered = row.contains (hoverPoint);
            juce::ColourGradient surface (hovered ? uiStyle::utilityHover : uiStyle::utilityRaised,
                                           row.getX(), row.getY(), uiStyle::utilitySurface,
                                           row.getX(), row.getBottom(), false);
            g.setGradientFill (surface); g.fillRoundedRectangle (row, 2.0f);
            g.setColour (uiStyle::utilityEdge.withAlpha (hovered ? 0.72f : 0.42f));
            g.drawRoundedRectangle (row, 2.0f, 0.6f);
            auto content = row.reduced (8.0f, 0.0f);
            const auto valueArea = content.removeFromRight (78.0f);
            g.setColour (text.withAlpha (0.88f));
            g.setFont (juce::FontOptions (uiStyle::normalFont).withName ("Bahnschrift"));
            g.drawText (label, content.toNearestInt(), juce::Justification::centredLeft);
            g.setColour (hovered ? accent : text.withAlpha (0.72f));
            g.drawFittedText (value, valueArea.toNearestInt(), juce::Justification::centredRight, 1);
        };

        g.setColour (accent.withAlpha (0.58f));
        g.setFont (juce::FontOptions (uiStyle::sectionFont).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("INTERFACE", controls.withHeight (16.0f).toNearestInt(), juce::Justification::centredLeft);
        for (int index = 0; index < 3; ++index)
        {
            const auto row = juce::Rectangle<float> (controls.getX(), controls.getY() + 17.0f + index * 31.0f,
                                                      controls.getWidth(), 27.0f);
            drawSettingRow (row, rows[static_cast<size_t> (index)].first, rows[static_cast<size_t> (index)].second);
        }

        const auto appearanceY = controls.getY() + 115.0f;
        g.setColour (accent.withAlpha (0.58f));
        g.setFont (juce::FontOptions (uiStyle::sectionFont).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("APPEARANCE", juce::Rectangle<float> (controls.getX(), appearanceY,
                    controls.getWidth(), 16.0f).toNearestInt(), juce::Justification::centredLeft);
        const auto folder = juce::Rectangle<float> (controls.getX(), appearanceY + 17.0f, controls.getWidth(), 40.0f);
        const auto folderHovered = folder.contains (hoverPoint);
        g.setColour (folderHovered ? uiStyle::utilityHover : uiStyle::utilityRaised);
        g.fillRoundedRectangle (folder, 3.0f);
        g.setColour (settingsThemesExpanded ? accent.withAlpha (0.70f)
                                             : uiStyle::utilityEdge.withAlpha (folderHovered ? 0.78f : 0.52f));
        g.drawRoundedRectangle (folder, 3.0f, settingsThemesExpanded ? 0.9f : 0.65f);
        juce::Path folderGlyph;
        folderGlyph.startNewSubPath (folder.getX() + 9.0f, folder.getY() + 12.0f);
        folderGlyph.lineTo (folder.getX() + 15.0f, folder.getY() + 12.0f);
        folderGlyph.lineTo (folder.getX() + 18.0f, folder.getY() + 15.0f);
        folderGlyph.lineTo (folder.getX() + 29.0f, folder.getY() + 15.0f);
        folderGlyph.lineTo (folder.getX() + 29.0f, folder.getY() + 28.0f);
        folderGlyph.lineTo (folder.getX() + 9.0f, folder.getY() + 28.0f);
        folderGlyph.closeSubPath();
        g.setColour (folderHovered || settingsThemesExpanded ? accent.withAlpha (0.86f) : dim.withAlpha (0.72f));
        g.strokePath (folderGlyph, juce::PathStrokeType (0.9f));
        g.setColour (text.withAlpha (0.90f));
        g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("FACEPLATE THEMES", juce::Rectangle<float> (folder.getX() + 36.0f, folder.getY() + 4.0f,
                    folder.getWidth() - 62.0f, 16.0f).toNearestInt(), juce::Justification::centredLeft);
        g.setColour (dim.withAlpha (0.78f));
        g.setFont (juce::FontOptions (6.6f).withName ("Bahnschrift"));
        g.drawFittedText (voidworm::ui::themePalette (owner.getThemeIndex()).name,
                          juce::Rectangle<float> (folder.getX() + 36.0f, folder.getY() + 19.0f,
                          folder.getWidth() - 62.0f, 14.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        juce::Path caret;
        if (settingsThemesExpanded)
            caret.addTriangle (folder.getRight() - 18.0f, folder.getCentreY() + 3.0f,
                               folder.getRight() - 8.0f, folder.getCentreY() + 3.0f,
                               folder.getRight() - 13.0f, folder.getCentreY() - 3.0f);
        else
            caret.addTriangle (folder.getRight() - 16.0f, folder.getCentreY() - 5.0f,
                               folder.getRight() - 16.0f, folder.getCentreY() + 5.0f,
                               folder.getRight() - 10.0f, folder.getCentreY());
        g.setColour (folderHovered ? accent : dim.withAlpha (0.70f));
        g.fillPath (caret);

        const auto actionsY = controls.getY() + (settingsThemesExpanded ? 329.0f : 181.0f);
        g.setColour (accent.withAlpha (0.58f));
        g.setFont (juce::FontOptions (uiStyle::sectionFont).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("ACTIONS", juce::Rectangle<float> (controls.getX(), actionsY, controls.getWidth(), 16.0f).toNearestInt(),
                    juce::Justification::centredLeft);
        const auto reset = juce::Rectangle<float> (controls.getX(), actionsY + 17.0f, controls.getWidth(), 32.0f);
        const auto resetHovered = reset.contains (hoverPoint);
        g.setColour (resetHovered ? uiStyle::utilityHover : uiStyle::utilitySurface); g.fillRoundedRectangle (reset, 3.0f);
        g.setColour (resetHovered ? accent.withAlpha (0.76f) : uiStyle::utilityEdge); g.drawRoundedRectangle (reset, 3.0f, 0.8f);
        g.setColour (resetHovered ? text : dim); g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift"));
        g.drawText ("RESET SOUND TO DEFAULT INIT", reset.toNearestInt(), juce::Justification::centred);
        if (owner.isStandaloneMode())
        {
            const auto standaloneY = actionsY + 58.0f;
            g.setColour (accent.withAlpha (0.58f));
            g.setFont (juce::FontOptions (uiStyle::sectionFont).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText ("STANDALONE", juce::Rectangle<float> (controls.getX(), standaloneY,
                        controls.getWidth(), 16.0f).toNearestInt(), juce::Justification::centredLeft);
            const auto device = juce::Rectangle<float> (controls.getX(), standaloneY + 17.0f, controls.getWidth(), 32.0f);
            const auto hovered = device.contains (hoverPoint);
            g.setColour (hovered ? uiStyle::utilityHover : uiStyle::utilityRaised); g.fillRoundedRectangle (device, 3.0f);
            g.setColour (hovered ? accent.withAlpha (0.76f) : uiStyle::utilityEdge); g.drawRoundedRectangle (device, 3.0f, 0.8f);
            g.setColour (hovered ? text : dim); g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift"));
            g.drawText ("AUDIO / MIDI DEVICE...", device.toNearestInt(), juce::Justification::centred);
        }

        if (settingsThemesExpanded)
        {
            g.setColour (uiStyle::utilitySurface.withAlpha (0.88f));
            g.fillRoundedRectangle (themes.expanded (4.0f), 4.0f);
            g.setColour (accent.withAlpha (0.46f));
            g.drawRoundedRectangle (themes.expanded (4.0f), 4.0f, 0.75f);
            const auto list = themes;
            constexpr auto rowHeight = 43.0f;
            constexpr auto rowGap = 5.0f;
            constexpr auto visibleRows = 3;
            for (int visibleIndex = 0; visibleIndex < visibleRows; ++visibleIndex)
            {
                const auto index = owner.headerPanelState.themeScrollRows + visibleIndex;
                if (index >= voidworm::ui::themeCount) break;
                const auto& paletteData = voidworm::ui::themePalette (index);
                const auto paletteLow = juce::Colour (paletteData.accentLow);
                const auto paletteMid = juce::Colour (paletteData.accentMid);
                const auto paletteHot = juce::Colour (paletteData.accentHot);
                const auto tile = juce::Rectangle<float> (list.getX(), list.getY() + visibleIndex * (rowHeight + rowGap),
                                                           list.getWidth(), rowHeight);
                const auto active = index == owner.getThemeIndex();
                const auto hovered = tile.contains (hoverPoint);
                g.setColour (active ? uiStyle::utilityRaised.interpolatedWith (paletteLow, 0.10f)
                                    : hovered ? uiStyle::utilityHover : uiStyle::utilityRaised);
                g.fillRoundedRectangle (tile, 4.0f);
                g.setColour (active ? paletteMid.withAlpha (0.70f) : uiStyle::utilityEdge.withAlpha (hovered ? 0.90f : 0.72f));
                g.drawRoundedRectangle (tile, 4.0f, active ? 1.0f : 0.7f);
                if (active) { g.setColour (paletteMid.withAlpha (0.88f)); g.fillRoundedRectangle (tile.getX(), tile.getY() + 7.0f, 2.0f, tile.getHeight() - 14.0f, 1.0f); }
                g.setColour (active || hovered ? text : dim);
                g.setFont (juce::FontOptions (8.3f).withName ("Bahnschrift").withStyle ("SemiBold"));
                g.drawFittedText (paletteData.name, juce::Rectangle<float> (tile.getX() + 12.0f, tile.getY() + 4.0f,
                                  136.0f, 17.0f).toNearestInt(), juce::Justification::centredLeft, 1);
                g.setColour (dim.withAlpha (0.74f));
                g.setFont (juce::FontOptions (6.2f).withName ("Bahnschrift"));
                g.drawFittedText (paletteData.descriptor, juce::Rectangle<float> (tile.getX() + 12.0f, tile.getY() + 21.0f,
                                  136.0f, 14.0f).toNearestInt(), juce::Justification::centredLeft, 1);
                const auto palette = juce::Rectangle<float> (tile.getRight() - 128.0f, tile.getCentreY() - 4.0f, 102.0f, 8.0f);
                juce::ColourGradient preview (paletteLow, palette.getX(), palette.getCentreY(), paletteHot,
                                              palette.getRight(), palette.getCentreY(), false);
                preview.addColour (0.50, paletteMid);
                g.setGradientFill (preview); g.fillRoundedRectangle (palette, 4.0f);
                const auto indicator = juce::Rectangle<float> (tile.getRight() - 17.0f, tile.getCentreY() - 4.0f, 8.0f, 8.0f);
                g.setColour (active ? paletteHot.withAlpha (0.86f) : juce::Colour (0xff211f1d)); g.fillEllipse (indicator);
                g.setColour (active ? paletteMid.withAlpha (0.68f) : uiStyle::utilityEdge); g.drawEllipse (indicator, 0.7f);
            }
        }
    }
    g.restoreState();

    if (dialog != Dialog::none)
    {
        g.setColour (juce::Colours::black.withAlpha (0.70f)); g.fillRoundedRectangle (panel, uiStyle::overlayRadius);
        const auto box = juce::Rectangle<float> (panel.getCentreX() - 190.0f, panel.getCentreY() - 82.0f, 380.0f, 164.0f);
        g.setColour (uiStyle::utilityRaised); g.fillRoundedRectangle (box, 5.0f);
        g.setColour (uiStyle::utilityEdge); g.drawRoundedRectangle (box, 5.0f, 0.8f);
        g.setColour (accent.withAlpha (0.62f)); g.fillRect (box.getX() + uiStyle::m, box.getY(), 38.0f, 1.2f);
        const auto titleText = dialog == Dialog::save ? "SAVE USER PRESET" : dialog == Dialog::overwrite ? "PRESET EXISTS"
            : dialog == Dialog::rename ? "RENAME USER PRESET" : "DELETE USER PRESET?";
        g.setColour (text); g.setFont (juce::FontOptions (10.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText (titleText, box.withHeight (35.0f).reduced (14.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft);
        if (dialog == Dialog::deleteConfirm || dialog == Dialog::overwrite)
        {
            g.setColour (dim); g.setFont (juce::FontOptions (9.0f).withName ("Bahnschrift"));
            const auto prompt = dialog == Dialog::deleteConfirm ? "DELETE \"" + owner.presetName + "\"?"
                                                                 : "OVERWRITE \"" + presetNameEditor.getText() + "\"?";
            g.drawFittedText (prompt, box.reduced (16.0f).withTrimmedTop (37.0f).withTrimmedBottom (46.0f).toNearestInt(), juce::Justification::centred, 1);
        }
        else
        {
            g.setColour (dim.withAlpha (0.78f));
            g.setFont (juce::FontOptions (6.8f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText ("NAME", juce::Rectangle<float> (box.getX() + 17.0f, box.getY() + 40.0f,
                box.getWidth() - 34.0f, 12.0f).toNearestInt(), juce::Justification::centredLeft);
        }
        auto buttonsArea = box.reduced (14.0f);
        auto buttons = buttonsArea.removeFromBottom (31.0f);
        auto cancel = buttons.removeFromLeft ((buttons.getWidth() - 8.0f) * 0.5f); buttons.removeFromLeft (8.0f);
        for (const auto& button : { cancel, buttons }) { g.setColour (uiStyle::utilitySurface); g.fillRoundedRectangle (button, 3.0f); g.setColour (uiStyle::utilityEdge); g.drawRoundedRectangle (button, 3.0f, 0.7f); }
        g.setColour (dim); g.drawText ("CANCEL", cancel.toNearestInt(), juce::Justification::centred);
        g.setColour (dialog == Dialog::deleteConfirm ? secondary : accent);
        g.drawText (dialog == Dialog::deleteConfirm ? "DELETE" : dialog == Dialog::overwrite ? "OVERWRITE" : dialog == Dialog::rename ? "RENAME" : "SAVE",
                    buttons.toNearestInt(), juce::Justification::centred);
    }
    if (statusMessage.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() < statusMessageUntil)
    {
        const auto message = juce::Rectangle<float> (panel.getX() + 14.0f, panel.getBottom() - 34.0f, panel.getWidth() - 28.0f, 24.0f);
        g.setColour (uiStyle::utilityRaised); g.fillRoundedRectangle (message, 3.0f);
        g.setColour (uiStyle::utilityEdge); g.drawRoundedRectangle (message, 3.0f, 0.7f);
        g.setColour (text); g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift"));
        g.drawFittedText (statusMessage, message.toNearestInt().reduced (7, 0), juce::Justification::centred, 1);
    }
}

void VoidwormAudioProcessorEditor::OverlayPanel::showMessage (const juce::String& message)
{
    statusMessage = message;
    statusMessageUntil = juce::Time::getMillisecondCounterHiRes() + 2600.0;
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::showDialog (Dialog requested, const juce::String& initialText)
{
    dialog = requested;
    presetNameEditor.setText (initialText, juce::dontSendNotification);
    presetNameEditor.setVisible (requested != Dialog::deleteConfirm && requested != Dialog::overwrite);
    resized(); presetNameEditor.toFront (false);
    if (presetNameEditor.isVisible())
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<OverlayPanel> (this)]
        { if (safe != nullptr) { safe->presetNameEditor.grabKeyboardFocus(); safe->presetNameEditor.selectAll(); } });
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::closeDialog()
{
    dialog = Dialog::none; presetNameEditor.setVisible (false); repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::performDialogAction()
{
    voidworm::UserPresetResult result;
    if (dialog == Dialog::save)
    {
        result = owner.userPresets.save (presetNameEditor.getText(), owner.processor.captureParameterSnapshot(), false);
        if (! result.ok && result.message == "PRESET ALREADY EXISTS") { showDialog (Dialog::overwrite, presetNameEditor.getText()); return; }
    }
    else if (dialog == Dialog::overwrite)
        result = owner.userPresets.save (presetNameEditor.getText(), owner.processor.captureParameterSnapshot(), true);
    else if (dialog == Dialog::rename)
        result = owner.userPresets.rename (owner.currentUserPresetFile, presetNameEditor.getText());
    else if (dialog == Dialog::deleteConfirm)
        result = owner.userPresets.remove (owner.currentUserPresetFile);
    else return;

    if (! result.ok) { showMessage (result.message); return; }
    if (dialog == Dialog::deleteConfirm)
    {
        owner.presetAssociation = PresetAssociation::custom;
        owner.currentUserPresetFile = {};
        owner.presetName = "Custom";
        owner.presetIsDirty = true;
    }
    else if (result.preset)
    {
        owner.presetAssociation = PresetAssociation::user;
        owner.currentUserPresetFile = result.preset->file;
        owner.presetName = result.preset->name;
        owner.cleanPresetSound = result.preset->parameters;
        owner.presetIsDirty = false;
    }
    const auto completedDialog = dialog;
    closeDialog(); refreshUserPresets(); owner.repaint();
    showMessage (completedDialog == Dialog::rename ? "PRESET RENAMED" : "USER PRESETS UPDATED");
}

void VoidwormAudioProcessorEditor::OverlayPanel::mouseDown (const juce::MouseEvent& event)
{
    finishGateThresholdDrag();
    using P = voidworm::ui::HeaderPanel;
    const auto point = juce::Point<float> (event.position.x * designWidth / static_cast<float> (getWidth()),
                                            event.position.y * designHeight / static_cast<float> (getHeight()));
    const auto header = owner.headerGeometry();
    if (header.previousPreset.contains (point))
    { owner.applyPreset (owner.presetIndex - 1); return; }
    if (header.nextPreset.contains (point))
    { owner.applyPreset (owner.presetIndex + 1); return; }
    if (header.presetMenu.contains (point))
    { showMode (P::presetBrowser); return; }
    if (header.gate.contains (point))
    {
        if (! event.mods.isPopupMenu())
            owner.toggleParameter ("gateEnabled");
        repaint();
        return;
    }
    if (header.grid.contains (point))
    { showMode (P::presetMatrix); return; }
    if (header.search.contains (point))
    { showMode (P::presetSearch); return; }
    if (header.settings.contains (point))
    { showMode (P::settings); return; }
    if (header.minimise.contains (point))
    { owner.minimiseStandaloneWindow(); return; }
    if (header.close.contains (point))
    { owner.closeStandaloneWindow(); return; }
    const auto panel = panelBounds();
    if (! panel.contains (point)) { hide(); return; }
    if (dialog != Dialog::none)
    {
        const auto box = juce::Rectangle<float> (panel.getCentreX() - 190.0f, panel.getCentreY() - 82.0f, 380.0f, 164.0f);
        auto buttons = box.reduced (14.0f).removeFromBottom (31.0f);
        const auto cancel = buttons.removeFromLeft ((buttons.getWidth() - 8.0f) * 0.5f); buttons.removeFromLeft (8.0f);
        if (cancel.contains (point)) { closeDialog(); return; }
        if (buttons.contains (point)) { performDialogAction(); return; }
        return;
    }
    if (juce::Rectangle<float> (panel.getRight() - 38.0f, panel.getY(), 38.0f, 34.0f).contains (point))
    { hide(); return; }
    const auto body = bodyBounds();
    const auto& presets = voidworm::factoryPresets();
    const auto mode = owner.headerPanelState.panel;
    if (mode == P::presetBrowser)
    {
        const auto actions = juce::Rectangle<float> (body.getX(), body.getBottom() - 28.0f, body.getWidth(), 26.0f);
        if (actions.contains (point))
        {
            const auto userSelected = owner.presetAssociation == PresetAssociation::user;
            const auto count = userSelected ? 4 : 1;
            const auto action = juce::jlimit (0, count - 1,
                static_cast<int> ((point.x - actions.getX()) / (actions.getWidth() / count)));
            if (action == 0) showDialog (Dialog::save, owner.presetName == "Custom" ? juce::String() : owner.presetName);
            else if (action == 1) showDialog (Dialog::rename, owner.presetName);
            else if (action == 2) showDialog (Dialog::overwrite, owner.presetName);
            else showDialog (Dialog::deleteConfirm);
            return;
        }
        constexpr auto rowHeight = 19.0f;
        auto rowIndex = -owner.headerPanelState.browserScrollRows;
        juce::String previousFamily;
        for (size_t index = 0; index < presets.size(); ++index)
        {
            const juce::String family (presets[index].family);
            if (family != previousFamily) { ++rowIndex; previousFamily = family; }
            const auto row = juce::Rectangle<float> (body.getX(), body.getY() + rowIndex++ * rowHeight,
                                                      body.getWidth(), rowHeight);
            if (row.contains (point)) { owner.applyPreset (static_cast<int> (index)); hide(); return; }
        }
        rowIndex += 2;
        for (size_t index = 0; index < owner.userPresets.presets().size(); ++index)
        {
            const auto row = juce::Rectangle<float> (body.getX(), body.getY() + rowIndex++ * rowHeight,
                                                      body.getWidth(), rowHeight);
            if (row.contains (point)) { owner.applyUserPreset (index); repaint(); return; }
        }
    }
    else if (mode == P::presetMatrix)
    {
        auto families = voidworm::ui::presetFamilies();
        families.push_back ("USER");
        families.insert (families.begin(), "ALL");
        const auto chipWidth = body.getWidth() / 7.0f;
        for (size_t index = 0; index < families.size(); ++index)
        {
            const auto chip = juce::Rectangle<float> (body.getX() + static_cast<float> (index % 7) * chipWidth,
                body.getY() + static_cast<float> (index / 7) * 20.0f, chipWidth, 18.0f);
            if (chip.contains (point))
            {
                owner.headerPanelState.setMatrixFamily (families[index]);
                owner.headerPanelState.matrixScrollRows = 0;
                repaint(); return;
            }
        }
        const auto results = voidworm::ui::filterPresetItems (owner.userPresets.presets(), owner.headerPanelState.matrixFamily);
        const auto cardWidth = (body.getWidth() - 8.0f) / 3.0f;
        for (size_t result = 0; result < results.size(); ++result)
        {
            const auto row = static_cast<int> (result / 3) - owner.headerPanelState.matrixScrollRows;
            const auto card = juce::Rectangle<float> (body.getX() + static_cast<float> (result % 3) * cardWidth,
                body.getY() + 42.0f + row * 40.0f, cardWidth - 4.0f, 35.0f);
            if (card.contains (point))
            {
                const auto& item = results[result];
                if (item.source == voidworm::ui::PresetSource::factory) owner.applyPreset (static_cast<int> (item.index));
                else owner.applyUserPreset (item.index);
                repaint(); return;
            }
        }
    }
    else if (mode == P::presetSearch)
    {
        const auto results = voidworm::ui::searchPresetItems (owner.userPresets.presets(), owner.headerPanelState.searchQuery);
        constexpr auto rowHeight = 27.0f;
        for (size_t result = 0; result < results.size(); ++result)
        {
            const auto row = juce::Rectangle<float> (body.getX(), body.getY() + 39.0f
                + (static_cast<int> (result) - owner.headerPanelState.searchScrollRows) * rowHeight,
                body.getWidth(), rowHeight);
            if (row.contains (point))
            {
                const auto& item = results[result];
                if (item.source == voidworm::ui::PresetSource::factory) owner.applyPreset (static_cast<int> (item.index));
                else owner.applyUserPreset (item.index);
                hide(); return;
            }
        }
    }
    else if (mode == P::gate)
    {
        if (gateThresholdBounds().contains (point))
        {
            draggedGateThreshold = owner.processor.parameters.getParameter ("gateThreshold");
            if (draggedGateThreshold != nullptr)
                draggedGateThreshold->beginChangeGesture();
            setGateThresholdFromPoint (point.x);
            repaint();
        }
    }
    else if (mode == P::settings)
    {
        const auto controls = settingsControlBounds();
        const auto themes = settingsThemeBounds();
        for (int index = 0; index < 3; ++index)
        {
            const auto y = controls.getY() + 17.0f + index * 31.0f;
            if (juce::Rectangle<float> (controls.getX(), y, controls.getWidth(), 27.0f).contains (point))
            {
                if (index == 0) owner.cycleUiScale();
                else if (index == 1) owner.cycleMeterFloor();
                else owner.cycleMeterHold();
                repaint(); return;
            }
        }
        const auto themeFolder = juce::Rectangle<float> (controls.getX(), controls.getY() + 132.0f,
                                                          controls.getWidth(), 40.0f);
        if (themeFolder.contains (point))
        {
            settingsThemesExpanded = ! settingsThemesExpanded;
            if (settingsThemesExpanded)
            {
                constexpr auto visibleRows = 3;
                owner.headerPanelState.themeScrollRows = juce::jlimit (0, voidworm::ui::themeCount - visibleRows,
                    owner.getThemeIndex() - visibleRows / 2);
            }
            repaint(); return;
        }
        const auto actionsY = controls.getY() + (settingsThemesExpanded ? 329.0f : 181.0f);
        if (juce::Rectangle<float> (controls.getX(), actionsY + 17.0f,
                                    controls.getWidth(), 32.0f).contains (point))
        { owner.applyPreset (0); hide(); return; }
        if (owner.isStandaloneMode()
            && juce::Rectangle<float> (controls.getX(), actionsY + 75.0f,
                                        controls.getWidth(), 32.0f).contains (point))
        {
            hide();
            if (owner.showStandaloneAudioMidiSettings)
                owner.showStandaloneAudioMidiSettings();
            return;
        }

        constexpr auto rowHeight = 43.0f;
        constexpr auto rowGap = 5.0f;
        constexpr auto visibleRows = 3;
        const auto list = themes;
        for (int visibleIndex = 0; settingsThemesExpanded && visibleIndex < visibleRows; ++visibleIndex)
        {
            const auto index = owner.headerPanelState.themeScrollRows + visibleIndex;
            if (index >= voidworm::ui::themeCount) break;
            if (juce::Rectangle<float> (list.getX(), list.getY() + visibleIndex * (rowHeight + rowGap),
                                         list.getWidth(), rowHeight).contains (point))
            { owner.setTheme (index); repaint(); return; }
        }
    }
}

void VoidwormAudioProcessorEditor::OverlayPanel::setGateThresholdFromPoint (float pointX)
{
    if (draggedGateThreshold == nullptr)
        return;

    const auto thresholdRow = gateThresholdBounds();
    const auto controlLeft = thresholdRow.getX() + 62.0f;
    const auto controlRight = thresholdRow.getRight() - 64.0f;
    const auto proportion = juce::jlimit (0.0f, 1.0f,
        (pointX - controlLeft) / juce::jmax (1.0f, controlRight - controlLeft));
    const auto threshold = std::round ((-80.0f + proportion * 60.0f) * 10.0f) * 0.1f;
    draggedGateThreshold->setValueNotifyingHost (draggedGateThreshold->convertTo0to1 (threshold));
}

void VoidwormAudioProcessorEditor::OverlayPanel::finishGateThresholdDrag()
{
    if (draggedGateThreshold != nullptr)
    {
        draggedGateThreshold->endChangeGesture();
        draggedGateThreshold = nullptr;
    }
}

void VoidwormAudioProcessorEditor::OverlayPanel::mouseDrag (const juce::MouseEvent& event)
{
    if (draggedGateThreshold == nullptr)
        return;

    hoverPoint = { event.position.x * designWidth / static_cast<float> (getWidth()),
                   event.position.y * designHeight / static_cast<float> (getHeight()) };
    setGateThresholdFromPoint (hoverPoint.x);
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::mouseUp (const juce::MouseEvent&)
{
    finishGateThresholdDrag();
}

void VoidwormAudioProcessorEditor::OverlayPanel::mouseMove (const juce::MouseEvent& event)
{
    hoverPoint = { event.position.x * designWidth / static_cast<float> (getWidth()),
                   event.position.y * designHeight / static_cast<float> (getHeight()) };
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::mouseExit (const juce::MouseEvent&)
{
    hoverPoint = { -1000.0f, -1000.0f };
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::mouseWheelMove (const juce::MouseEvent& event,
                                                                  const juce::MouseWheelDetails& wheel)
{
    const auto delta = wheel.deltaY > 0.0f ? -2 : wheel.deltaY < 0.0f ? 2 : 0;
    using P = voidworm::ui::HeaderPanel;
    if (owner.headerPanelState.panel == P::gate)
    {
        const auto point = juce::Point<float> (event.position.x * designWidth / static_cast<float> (getWidth()),
                                                event.position.y * designHeight / static_cast<float> (getHeight()));
        const auto thresholdRow = gateThresholdBounds();
        if (thresholdRow.contains (point) && wheel.deltaY != 0.0f)
        {
            const auto current = owner.processor.parameters.getRawParameterValue ("gateThreshold")->load();
            const auto next = juce::jlimit (-80.0f, -20.0f, current + (wheel.deltaY > 0.0f ? 1.0f : -1.0f));
            setParameterActual (owner.processor.parameters, "gateThreshold", next);
            repaint();
            return;
        }
    }
    if (owner.headerPanelState.panel == P::settings && settingsThemesExpanded)
    {
        const auto point = juce::Point<float> (event.position.x * designWidth / static_cast<float> (getWidth()),
                                                event.position.y * designHeight / static_cast<float> (getHeight()));
        if (settingsThemeBounds().contains (point))
        {
            constexpr auto visibleRows = 3;
            owner.headerPanelState.themeScrollRows = juce::jlimit (0, voidworm::ui::themeCount - visibleRows,
                                                                   owner.headerPanelState.themeScrollRows + delta);
            repaint();
            return;
        }
    }
    if (owner.headerPanelState.panel == P::presetBrowser)
    {
        const auto totalRows = 49 + 11 + 2 + static_cast<int> (owner.userPresets.presets().size());
        owner.headerPanelState.browserScrollRows = juce::jlimit (0, juce::jmax (0, totalRows - 18),
                                                                 owner.headerPanelState.browserScrollRows + delta);
    }
    else if (owner.headerPanelState.panel == P::presetMatrix)
    {
        const auto rows = static_cast<int> ((voidworm::ui::filterPresetItems (owner.userPresets.presets(),
            owner.headerPanelState.matrixFamily).size() + 2) / 3);
        owner.headerPanelState.matrixScrollRows = juce::jlimit (0, juce::jmax (0, rows - 6),
                                                                owner.headerPanelState.matrixScrollRows + delta);
    }
    else if (owner.headerPanelState.panel == P::presetSearch)
    {
        const auto rows = static_cast<int> (voidworm::ui::searchPresetItems (owner.userPresets.presets(),
            owner.headerPanelState.searchQuery).size());
        owner.headerPanelState.searchScrollRows = juce::jlimit (0, juce::jmax (0, rows - 10),
                                                                owner.headerPanelState.searchScrollRows + delta);
    }
    repaint();
}

void VoidwormAudioProcessorEditor::OverlayPanel::loadSearchSelection()
{
    const auto results = voidworm::ui::searchPresetItems (owner.userPresets.presets(), owner.headerPanelState.searchQuery);
    if (! results.empty())
    {
        const auto index = static_cast<size_t> (juce::jlimit (0, static_cast<int> (results.size()) - 1,
                                                             owner.headerPanelState.highlightedSearchResult));
        if (results[index].source == voidworm::ui::PresetSource::factory)
            owner.applyPreset (static_cast<int> (results[index].index));
        else owner.applyUserPreset (results[index].index);
    }
    hide();
}

bool VoidwormAudioProcessorEditor::OverlayPanel::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (dialog != Dialog::none) closeDialog(); else hide();
        return true;
    }
    if (dialog != Dialog::none) return false;
    if (owner.headerPanelState.panel != voidworm::ui::HeaderPanel::presetSearch)
        return false;
    const auto resultCount = static_cast<int> (voidworm::ui::searchPresetItems (
        owner.userPresets.presets(), owner.headerPanelState.searchQuery).size());
    const auto move = [&] (int delta)
    {
        if (resultCount <= 0) { owner.headerPanelState.highlightedSearchResult = 0; return; }
        owner.headerPanelState.highlightedSearchResult = (owner.headerPanelState.highlightedSearchResult + delta) % resultCount;
        if (owner.headerPanelState.highlightedSearchResult < 0) owner.headerPanelState.highlightedSearchResult += resultCount;
    };
    if (key == juce::KeyPress::downKey) move (1);
    else if (key == juce::KeyPress::upKey) move (-1);
    else if (key == juce::KeyPress::returnKey) { loadSearchSelection(); return true; }
    else return false;
    const auto selected = owner.headerPanelState.highlightedSearchResult;
    if (selected < owner.headerPanelState.searchScrollRows)
        owner.headerPanelState.searchScrollRows = selected;
    else if (selected > owner.headerPanelState.searchScrollRows + 8)
        owner.headerPanelState.searchScrollRows = selected - 8;
    repaint();
    return true;
}

VoidwormAudioProcessorEditor::VoidwormAudioProcessorEditor (VoidwormAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), surgeSwitch (*this, "SURGE"),
      limiterSwitch (*this, "LIMITER"), hqButton (*this, "HQ MODE"), overlay (*this)
{
    processor.setReactorSoloTarget (0);
    lookAndFeel.setMetalTexture (juce::ImageFileFormat::loadFrom (VoidwormAssets::industrial_metal_v2_png,
                                                                  VoidwormAssets::industrial_metal_v2_pngSize));
    lookAndFeel.setSteelPanelTexture (juce::ImageFileFormat::loadFrom (VoidwormAssets::extracted_reference_faceplate_v1_png,
                                                                       VoidwormAssets::extracted_reference_faceplate_v1_pngSize));
    lookAndFeel.setNeonPanelTexture (juce::ImageFileFormat::loadFrom (VoidwormAssets::hellforge_neon_faceplate_v1_png,
                                                                      VoidwormAssets::hellforge_neon_faceplate_v1_pngSize));
    lookAndFeel.setThemeStyle (currentThemeStyle());
    setLookAndFeel (&lookAndFeel);
    setSize (920, 823);
    setResizable (true, true);
    setResizeLimits (820, 733, 1200, 1073);
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (designWidth / designHeight);
    setWantsKeyboardFocus (true);

    configureSlider (breach, "BREACH", breachLabel);
    configureSlider (tear, "TEAR", tearLabel);
    configureSlider (overload, "OVERLOAD", overloadLabel);
    configureSlider (rot, "ROT", rotLabel);
    configureSlider (drive, "DRIVE", driveLabel);
    drive.setMouseDragSensitivity (125);
    drive.setTooltip ("DRIVE - input gain into the wet reactor");
    configureSlider (gateThreshold, "GATE THRESHOLD", gateThresholdLabel);
    gateThreshold.setMouseDragSensitivity (140);
    gateThreshold.setTooltip ("Input gate threshold - drag vertically");
    gateThresholdLabel.setText ("THR", juce::dontSendNotification);
    configureSlider (mix, "MIX", mixLabel);
    surgeLabel.setText ("SURGE", juce::dontSendNotification);
    surgeLabel.setJustificationType (juce::Justification::centred);
    surgeLabel.setColour (juce::Label::textColourId, defaultWarmWhite);
    surgeSwitch.setClickingTogglesState (true);
    surgeSwitch.setTooltip ("Smoothly escalates reactor thresholds, sag, density and compression");
    addAndMakeVisible (surgeLabel);
    addAndMakeVisible (surgeSwitch);

    for (auto* button : { &previousPresetButton, &nextPresetButton, &presetMenuButton, &gateButton, &gridButton,
                          &searchButton, &settingsButton, &minimiseButton, &closeButton, &oversampleButton })
    {
        button->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (*button);
    }
    previousPresetButton.onClick = [this] { applyPreset (presetIndex - 1); };
    nextPresetButton.onClick = [this] { applyPreset (presetIndex + 1); };
    presetMenuButton.onClick = [this] { overlay.showMode (voidworm::ui::HeaderPanel::presetBrowser); };
    gateButton.onClick = [this] { toggleParameter ("gateEnabled"); };
    gateButton.setTooltip ("Input gate - click to enable/disable");
    gridButton.onClick = [this] { overlay.showMode (voidworm::ui::HeaderPanel::presetMatrix); };
    searchButton.onClick = [this] { overlay.showMode (voidworm::ui::HeaderPanel::presetSearch); };
    settingsButton.onClick = [this] { overlay.showMode (voidworm::ui::HeaderPanel::settings); };
    minimiseButton.onClick = [this] { minimiseStandaloneWindow(); };
    closeButton.onClick = [this] { closeStandaloneWindow(); };
    minimiseButton.setTooltip ("Minimise");
    closeButton.setTooltip ("Close");
    minimiseButton.setVisible (isStandaloneMode());
    closeButton.setVisible (isStandaloneMode());
    oversampleButton.onClick = [this] { cycleOversampling(); };
    hqButton.onClick = [this] { toggleParameter ("hqMode"); };
    addAndMakeVisible (hqButton);
    addChildComponent (standaloneToast);
    configureSlider (low, "LOW", lowLabel);
    configureSlider (mid, "MID", midLabel);
    configureSlider (high, "HIGH", highLabel);
    configureSlider (range, "RANGE", rangeLabel);
    configureSlider (output, "OUTPUT", outputLabel);
    configureSlider (weld, "WELD", weldLabel);
    configureSlider (limiterThreshold, "LIMIT", limiterThresholdLabel);
    configureSlider (limiterCeiling, "CEILING", limiterCeilingLabel);
    limiterSwitch.setClickingTogglesState (true);
    limiterSwitch.setTooltip ("Enable the dedicated final sample-peak limiter");
    limiterSwitchLabel.setText ("LIMITER", juce::dontSendNotification);
    limiterSwitchLabel.setJustificationType (juce::Justification::centred);
    limiterSwitchLabel.setColour (juce::Label::textColourId, defaultWarmWhite);
    addAndMakeVisible (limiterSwitch);
    addAndMakeVisible (limiterSwitchLabel);
    reactorAmount.setName ("AMOUNT");
    reactorAmount.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    reactorAmount.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                       juce::MathConstants<float>::pi * 2.80f, true);
    reactorAmount.setMouseDragSensitivity (180);
    reactorAmount.setTextBoxStyle (juce::Slider::NoTextBox, false, 60, 19);
    reactorAmount.setTooltip ("Selected reactor amount - drag vertically");
    reactorAmount.onValueChange = [this] { repaint(); };
    const auto configureContextSlider = [this] (TouchSlider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                    juce::MathConstants<float>::pi * 2.80f, true);
        slider.setMouseDragSensitivity (180);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 60, 19);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
        slider.setVisible (false);
    };
    configureContextSlider (reactorAmount);
    configureContextSlider (reactorCharacterA);
    configureContextSlider (reactorCharacterB);

    overloadLabel.setFont (juce::FontOptions (14.0f).withName ("Bahnschrift").withStyle ("Light"));
    overload.setMouseDragSensitivity (260);
    range.setTooltip ("Global high-pass / low-pass span");

    breachAttachment = std::make_unique<Attachment> (processor.parameters, "breach", breach);
    tearAttachment = std::make_unique<Attachment> (processor.parameters, "tear", tear);
    overloadAttachment = std::make_unique<Attachment> (processor.parameters, "overload", overload);
    rotAttachment = std::make_unique<Attachment> (processor.parameters, "rot", rot);
    driveAttachment = std::make_unique<Attachment> (processor.parameters, "drive", drive);
    gateThresholdAttachment = std::make_unique<Attachment> (processor.parameters, "gateThreshold", gateThreshold);
    mixAttachment = std::make_unique<Attachment> (processor.parameters, "mix", mix);
    surgeAttachment = std::make_unique<ButtonAttachment> (processor.parameters, "surge", surgeSwitch);
    lowAttachment = std::make_unique<Attachment> (processor.parameters, "low", low);
    midAttachment = std::make_unique<Attachment> (processor.parameters, "mid", mid);
    highAttachment = std::make_unique<Attachment> (processor.parameters, "high", high);
    rangeAttachment = std::make_unique<Attachment> (processor.parameters, "range", range);
    outputAttachment = std::make_unique<Attachment> (processor.parameters, "output", output);
    weldAttachment = std::make_unique<Attachment> (processor.parameters, "weld", weld);
    limiterThresholdAttachment = std::make_unique<Attachment> (processor.parameters, "limiterThreshold", limiterThreshold);
    limiterCeilingAttachment = std::make_unique<Attachment> (processor.parameters, "limiterCeiling", limiterCeiling);
    limiterSwitchAttachment = std::make_unique<ButtonAttachment> (processor.parameters, "limiterEnabled", limiterSwitch);
    hqButton.setActive (processor.parameters.getRawParameterValue ("hqMode")->load() >= 0.5f);
    surgeSwitch.snapToState(); limiterSwitch.snapToState();
    // The host may restore a session state before constructing the editor. Treat
    // that actual musical state as clean until the user changes a sound control.
    cleanPresetSound = processor.captureParameterSnapshot();
    addChildComponent (overlay);
    setTheme (getThemeIndex());
    configureUiPreferences();
    applyUiScale();
    resized();
    startTimerHz (30);
}

VoidwormAudioProcessorEditor::~VoidwormAudioProcessorEditor()
{
    processor.setReactorSoloTarget (0);
    saveUiPreferences();
    setLookAndFeel (nullptr);
}

void VoidwormAudioProcessorEditor::configureSlider (TouchSlider& slider, const juce::String& name,
                                                     juce::Label& label)
{
    slider.setName (name);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                juce::MathConstants<float>::pi * 2.80f, true);
    slider.setMouseDragSensitivity (180);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 60, 19);
    slider.setTooltip (name + " - drag vertically");
    slider.onTouch = [this] (TouchSlider& touched, bool down)
    {
        activeSlider = down ? &touched : nullptr;
        if (&touched == &rot)
        {
            drive.setVisible (! down);
            driveLabel.setVisible (! down);
        }
        repaint();
    };
    slider.onValueChange = [this, &slider]
    {
        if (activeSlider == &slider)
            repaint();
    };
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (9.0f).withName ("Bahnschrift").withStyle ("Regular"));
    label.setColour (juce::Label::textColourId, defaultWarmWhite);
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void VoidwormAudioProcessorEditor::drawSteelTexture (juce::Graphics& g, juce::Rectangle<float> area, int density)
{
    const auto opacity = juce::jlimit (0.44f, 0.68f, 0.42f + static_cast<float> (density) / 1230.0f);
    lookAndFeel.drawPanelTexture (g, area, opacity);
}

int VoidwormAudioProcessorEditor::getThemeIndex() const
{
    if (auto* value = processor.parameters.getRawParameterValue ("theme"))
        return juce::jlimit (0, voidworm::ui::themeCount - 1, juce::roundToInt (value->load()));
    return 0;
}

bool VoidwormAudioProcessorEditor::isNeonTheme() const { return getThemeIndex() == 0; }

VoidLookAndFeel::ThemeStyle VoidwormAudioProcessorEditor::currentThemeStyle() const noexcept
{
    return static_cast<VoidLookAndFeel::ThemeStyle> (getThemeIndex());
}

juce::Colour VoidwormAudioProcessorEditor::primaryAccent() const
{
    return juce::Colour (voidworm::ui::themePalette (getThemeIndex()).accentMid);
}

bool VoidwormAudioProcessorEditor::isStandaloneMode() const noexcept
{
    return processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
}

VoidwormAudioProcessorEditor::HeaderGeometry VoidwormAudioProcessorEditor::headerGeometry() const noexcept
{
    HeaderGeometry geometry;
    geometry.preset = editorLayout.preset;
    geometry.previousPreset = editorLayout.previousPreset;
    geometry.presetMenu = editorLayout.presetMenu;
    geometry.nextPreset = editorLayout.nextPreset;
    geometry.gate = editorLayout.gateButton;
    geometry.gateThreshold = editorLayout.gateThreshold;
    geometry.grid = editorLayout.gridButton;
    geometry.search = editorLayout.searchButton;
    geometry.settings = editorLayout.settingsButton;
    if (isStandaloneMode())
    {
        geometry.preset = { 247.0f, 16.0f, 198.0f, 28.0f };
        geometry.previousPreset = { 247.0f, 16.0f, 28.0f, 28.0f };
        geometry.presetMenu = { 275.0f, 16.0f, 142.0f, 28.0f };
        geometry.nextPreset = { 417.0f, 16.0f, 28.0f, 28.0f };
        geometry.grid = { 478.0f, 13.0f, 36.0f, 34.0f };
        geometry.search = { 518.0f, 13.0f, 36.0f, 34.0f };
        geometry.settings = { 558.0f, 13.0f, 38.0f, 34.0f };
        geometry.minimise = { 626.0f, 15.0f, 27.0f, 30.0f };
        geometry.close = { 660.0f, 15.0f, 27.0f, 30.0f };
    }
    return geometry;
}

void VoidwormAudioProcessorEditor::configureStandaloneShell (std::function<void()> showAudioMidiSettings)
{
    if (! isStandaloneMode())
        return;
    showStandaloneAudioMidiSettings = std::move (showAudioMidiSettings);
    minimiseButton.setVisible (true);
    closeButton.setVisible (true);
    resized();
    repaint();
}

void VoidwormAudioProcessorEditor::showStandaloneToast (const juce::String& message, int durationMs)
{
    if (! isStandaloneMode())
        return;
    standaloneToast.message = message;
    standaloneToast.expiresAt = juce::Time::getMillisecondCounterHiRes() + durationMs;
    standaloneToast.setVisible (true);
    standaloneToast.toFront (false);
    standaloneToast.repaint();
}

void VoidwormAudioProcessorEditor::beginStandaloneWindowDrag (const juce::MouseEvent& event)
{
    if (auto* topLevel = getTopLevelComponent())
    {
        standaloneWindowDragger.startDraggingComponent (topLevel, event.getEventRelativeTo (topLevel));
        draggingStandaloneWindow = true;
    }
}

void VoidwormAudioProcessorEditor::minimiseStandaloneWindow()
{
    if (isStandaloneMode())
        if (auto* window = dynamic_cast<juce::ResizableWindow*> (getTopLevelComponent()))
            window->setMinimised (true);
}

void VoidwormAudioProcessorEditor::closeStandaloneWindow()
{
    if (isStandaloneMode())
        if (auto* window = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent()))
            window->closeButtonPressed();
}

juce::Colour VoidwormAudioProcessorEditor::secondaryAccent() const
{
    return juce::Colour (voidworm::ui::themePalette (getThemeIndex()).accentHot);
}

juce::Colour VoidwormAudioProcessorEditor::lowAccent() const
{
    return juce::Colour (voidworm::ui::themePalette (getThemeIndex()).accentLow);
}

juce::Colour VoidwormAudioProcessorEditor::themeText() const
{
    return juce::Colour (voidworm::ui::themePalette (getThemeIndex()).text);
}

juce::Colour VoidwormAudioProcessorEditor::themeDimText() const
{
    return juce::Colour (voidworm::ui::themePalette (getThemeIndex()).dimText);
}

void VoidwormAudioProcessorEditor::setTheme (int index)
{
    setParameterActual (processor.parameters, "theme", static_cast<float> (juce::jlimit (0, voidworm::ui::themeCount - 1, index)));
    lookAndFeel.setThemeStyle (currentThemeStyle());
    const auto text = themeText();
    for (auto* label : { &breachLabel, &tearLabel, &overloadLabel, &rotLabel, &driveLabel, &mixLabel, &surgeLabel,
                         &gateThresholdLabel,
                         &lowLabel, &midLabel, &highLabel, &rangeLabel, &outputLabel })
        label->setColour (juce::Label::textColourId, text);
    for (auto* slider : { &breach, &tear, &overload, &rot, &drive, &mix, &gateThreshold, &low, &mid, &high, &range, &output,
                          &reactorAmount, &reactorCharacterA, &reactorCharacterB })
        slider->repaint();
    surgeSwitch.repaint();
    repaint();
    overlay.refreshTheme();
    overlay.repaint();
}

void VoidwormAudioProcessorEditor::paint (juce::Graphics& g)
{
    lookAndFeel.setThemeStyle (currentThemeStyle());
    const auto lowAccentColour = lowAccent();
    const auto crimson = primaryAccent();
    const auto ember = secondaryAccent();
    const auto warmWhite = themeText();
    const auto dimText = themeDimText();
    g.fillAll (juce::Colour (0xff050505));
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    g.addTransform (juce::AffineTransform::scale (sx, sy));
    g.setColour (juce::Colour (0xff696969));
    g.drawRect (juce::Rectangle<int> (0, 0, static_cast<int> (designWidth),
                                     static_cast<int> (designHeight)), 1);

    const auto presetPanel = editorLayout.header;
    juce::ColourGradient panel (juce::Colour (0xff201c1a), presetPanel.getX(), presetPanel.getY(),
                                juce::Colour (0xff090909), presetPanel.getX(), presetPanel.getBottom(), false);
    g.setGradientFill (panel);
    g.fillRoundedRectangle (presetPanel, 3.0f);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.82f));
    g.drawRoundedRectangle (presetPanel, 3.0f, 0.8f);
    drawSteelTexture (g, presetPanel, 100);

    g.setFont (juce::FontOptions (11.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
    juce::ColourGradient brandGradient (crimson, 23.0f, 22.0f, ember, 193.0f, 22.0f, false);
    g.setGradientFill (brandGradient);
    g.drawText ("V O I D W O R M", 23, 15, 170, 15, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (7.2f).withName ("Bahnschrift").withStyle ("Medium"));
    juce::ColourGradient brandSubGradient (lowAccentColour.interpolatedWith (warmWhite, 0.16f).withAlpha (0.90f), 23.0f, 37.0f,
                                           crimson.interpolatedWith (warmWhite, 0.12f).withAlpha (0.94f), 218.0f, 37.0f, false);
    g.setGradientFill (brandSubGradient);
    g.drawText ("Source-reactive industrial distortion.", 23, 31, 210, 12, juce::Justification::centredLeft);

    const auto header = headerGeometry();
    const auto preset = header.preset;
    g.setColour (juce::Colour (0xff080808));
    g.fillRoundedRectangle (preset, 4.0f);
    const auto presetOpen = headerPanelState.panel == voidworm::ui::HeaderPanel::presetBrowser;
    if (presetOpen)
    {
        juce::ColourGradient presetEdge (lowAccentColour.withAlpha (0.56f), preset.getX(), preset.getBottom(),
                                         ember.withAlpha (0.72f), preset.getRight(), preset.getBottom(), false);
        presetEdge.addColour (0.50, crimson.withAlpha (0.68f));
        g.setGradientFill (presetEdge); g.drawRoundedRectangle (preset, 4.0f, 0.8f);
    }
    else { g.setColour (uiStyle::utilityEdge.withAlpha (0.82f)); g.drawRoundedRectangle (preset, 4.0f, 0.8f); }
    g.setColour (presetOpen ? warmWhite.withAlpha (0.94f) : warmWhite.withAlpha (0.88f));
    g.setFont (juce::FontOptions (10.0f).withName ("Bahnschrift"));
    g.drawFittedText (presetName + (presetIsDirty ? " *" : ""), preset.toNearestInt().reduced (31, 0),
                      juce::Justification::centred, 1);
    g.setFont (juce::FontOptions (22.0f).withName ("Bahnschrift").withStyle ("Light"));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x80\xb9"), header.previousPreset.toNearestInt(), juce::Justification::centred);
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x80\xba"), header.nextPreset.toNearestInt(), juce::Justification::centred);

    // Preset grid, search, and settings remain in the header's utility zone.
    const auto gridCentreX = header.grid.getCentreX();
    const auto searchCentreX = header.search.getCentreX();
    const auto settingsCentreX = header.settings.getCentreX();
    const auto iconCentreY = header.grid.getCentreY();
    const auto utilityState = [&] (const HotspotButton& button, voidworm::ui::HeaderPanel panelType, float centreX)
    {
        const auto active = headerPanelState.panel == panelType;
        const auto over = button.isMouseOver();
        const auto area = juce::Rectangle<float> (centreX - 11.0f, iconCentreY - 11.0f, 22.0f, 22.0f);
        if (active || over)
        {
            g.setColour (active ? crimson.withAlpha (0.045f) : juce::Colours::white.withAlpha (0.025f));
            g.fillRoundedRectangle (area, 5.0f);
            g.setColour (active ? crimson.withAlpha (0.15f) : uiStyle::utilityEdge.withAlpha (0.28f));
            g.drawRoundedRectangle (area.reduced (0.5f), 5.0f, 0.5f);
        }
        return active ? crimson.interpolatedWith (ember, 0.42f).withAlpha (0.90f)
                      : over ? crimson.interpolatedWith (ember, 0.22f).withAlpha (0.78f)
                             : crimson.withAlpha (0.62f);
    };
    g.setColour (utilityState (gridButton, voidworm::ui::HeaderPanel::presetMatrix, gridCentreX));
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column)
            g.fillRoundedRectangle (gridCentreX - 6.1f + column * 4.8f,
                                    iconCentreY - 6.1f + row * 4.8f, 2.6f, 2.6f, 0.55f);
    const auto searchLensCentre = juce::Point<float> (searchCentreX - 1.25f, iconCentreY - 1.25f);
    g.setColour (utilityState (searchButton, voidworm::ui::HeaderPanel::presetSearch, searchCentreX));
    g.drawEllipse (searchLensCentre.x - 5.0f, searchLensCentre.y - 5.0f, 10.0f, 10.0f, 1.5f);
    g.drawLine (searchLensCentre.x + 3.4f, searchLensCentre.y + 3.4f,
                searchLensCentre.x + 7.0f, searchLensCentre.y + 7.0f, 1.5f);
    g.setColour (utilityState (settingsButton, voidworm::ui::HeaderPanel::settings, settingsCentreX));
    g.drawEllipse (settingsCentreX - 5.5f, iconCentreY - 5.5f, 11.0f, 11.0f, 1.3f);
    for (int tooth = 0; tooth < 8; ++tooth)
    {
        const auto a = static_cast<float> (tooth) * juce::MathConstants<float>::pi * 0.25f;
        g.drawLine (settingsCentreX + std::cos (a) * 6.0f, iconCentreY + std::sin (a) * 6.0f,
                    settingsCentreX + std::cos (a) * 8.0f, iconCentreY + std::sin (a) * 8.0f, 1.0f);
    }
    g.fillEllipse (settingsCentreX - 2.1f, iconCentreY - 2.1f, 4.2f, 4.2f);

    if (isStandaloneMode())
    {
        const auto separatorX = (header.settings.getRight() + header.minimise.getX()) * 0.5f;
        g.setColour (uiStyle::utilityEdge.withAlpha (0.46f));
        g.drawVerticalLine (juce::roundToInt (separatorX), 18.0f, 42.0f);

        const auto drawWindowControl = [&] (HotspotButton& button, juce::Rectangle<float> bounds, bool close)
        {
            const auto hovered = button.isMouseOver();
            const auto down = button.isDown();
            if (hovered || down)
            {
                const auto surface = juce::Rectangle<float> (18.0f, 18.0f)
                    .withCentre ({ bounds.getCentreX(), iconCentreY });
                juce::ColourGradient housing (down ? juce::Colour (0xff050505) : juce::Colour (0xff201d1c),
                                               surface.getX(), surface.getY(), juce::Colour (0xff090909),
                                               surface.getX(), surface.getBottom(), false);
                g.setGradientFill (housing);
                g.fillRoundedRectangle (surface, 4.0f);
                g.setColour (crimson.withAlpha (down ? 0.50f : 0.32f));
                g.drawRoundedRectangle (surface.reduced (0.5f), 4.0f, 0.65f);
            }
            const auto mark = hovered ? (close ? crimson.interpolatedWith (ember, 0.46f)
                                               : warmWhite.withAlpha (0.86f))
                                      : warmWhite.withAlpha (0.38f);
            g.setColour (mark.withAlpha (down ? 0.72f : hovered ? 0.94f : 0.76f));
            if (close)
            {
                g.drawLine (bounds.getCentreX() - 3.4f, iconCentreY - 3.4f,
                            bounds.getCentreX() + 3.4f, iconCentreY + 3.4f, 1.1f);
                g.drawLine (bounds.getCentreX() + 3.4f, iconCentreY - 3.4f,
                            bounds.getCentreX() - 3.4f, iconCentreY + 3.4f, 1.1f);
            }
            else
                g.drawLine (bounds.getCentreX() - 4.5f, iconCentreY + 2.0f,
                            bounds.getCentreX() + 4.5f, iconCentreY + 2.0f, 1.1f);
        };
        drawWindowControl (minimiseButton, header.minimise, false);
        drawWindowControl (closeButton, header.close, true);
    }

    const auto workPanel = editorLayout.mainControls;
    juce::ColourGradient work (juce::Colour (0xff171514), workPanel.getX(), workPanel.getY(),
                               juce::Colour (0xff080808), workPanel.getRight(), workPanel.getBottom(), false);
    g.setGradientFill (work);
    g.fillRoundedRectangle (workPanel, 3.0f);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.76f));
    g.drawRoundedRectangle (workPanel, 3.0f, 0.8f);
    drawSteelTexture (g, workPanel, 320);

    const std::array<juce::Rectangle<float>, 2> controlContainers {{
        editorLayout.primaryControls,
        editorLayout.secondaryControls
    }};
    for (const auto container : controlContainers)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (container, 4.0f);
        g.setColour (uiStyle::utilityEdge.withAlpha (0.42f));
        g.drawRoundedRectangle (container, 4.0f, 0.55f);
    }

    drawMeter (g, { 13.0f, 63.0f, 75.0f, 278.0f }, true);
    drawMeter (g, { 621.0f, 63.0f, 75.0f, 278.0f }, false);

    // The gate belongs to the signal path, so it lives in a compact module
    // inside the INPUT meter chassis rather than among global header tools.
    const auto gateModule = juce::Rectangle<float> (16.0f, 82.0f, 65.0f, 35.0f);
    g.setColour (juce::Colour (0xff100f0e).withAlpha (0.62f));
    g.fillRoundedRectangle (gateModule, 3.0f);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.34f));
    g.drawRoundedRectangle (gateModule, 3.0f, 0.55f);

    const auto gateEnabled = processor.parameters.getRawParameterValue ("gateEnabled")->load() >= 0.5f;
    const auto gateMuting = gateEnabled && processor.isInputGateMuting();
    const auto gateOver = gateButton.isMouseOver();
    const auto gateCentreX = header.gate.getCentreX();
    const auto gateCentreY = 103.5f;
    g.setColour (gateOver ? warmWhite.withAlpha (0.82f) : dimText.withAlpha (0.78f));
    g.setFont (juce::FontOptions (5.8f).withName ("Bahnschrift").withStyle ("SemiBold"));
    g.drawText ("GATE", 17, 84, 29, 8, juce::Justification::centred);
    if (gateOver)
    {
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        g.fillRoundedRectangle (header.gate.reduced (2.0f), 4.0f);
    }
    const auto gateColour = gateMuting ? ember.withAlpha (0.98f)
                          : gateEnabled ? crimson.withAlpha (0.82f)
                                        : dimText.withAlpha (gateOver ? 0.72f : 0.48f);
    if (gateMuting)
    {
        g.setColour (ember.withAlpha (0.13f));
        g.fillEllipse (gateCentreX - 8.0f, gateCentreY - 8.0f, 16.0f, 16.0f);
    }
    g.setColour (gateColour);
    g.drawLine (gateCentreX - 6.2f, gateCentreY, gateCentreX - 3.6f, gateCentreY, 1.1f);
    g.drawLine (gateCentreX + 3.6f, gateCentreY, gateCentreX + 6.2f, gateCentreY, 1.1f);
    juce::Path gateGlyph;
    gateGlyph.startNewSubPath (gateCentreX - 3.6f, gateCentreY - 4.3f);
    gateGlyph.lineTo (gateCentreX - 1.0f, gateCentreY);
    gateGlyph.lineTo (gateCentreX - 3.6f, gateCentreY + 4.3f);
    gateGlyph.startNewSubPath (gateCentreX + 3.6f, gateCentreY - 4.3f);
    gateGlyph.lineTo (gateCentreX + 1.0f, gateCentreY);
    gateGlyph.lineTo (gateCentreX + 3.6f, gateCentreY + 4.3f);
    g.strokePath (gateGlyph, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    const auto gateLamp = juce::Rectangle<float> (gateCentreX + 4.0f, gateCentreY - 7.0f, 3.6f, 3.6f);
    if (gateMuting)
    {
        g.setColour (ember.withAlpha (0.22f));
        g.fillEllipse (gateLamp.expanded (2.5f));
    }
    g.setColour (gateMuting ? secondaryAccent() : gateEnabled ? crimson.withAlpha (0.52f)
                                                               : juce::Colour (0xff171513));
    g.fillEllipse (gateLamp);
    g.setColour (gateMuting ? warmWhite.withAlpha (0.82f) : uiStyle::utilityEdge.withAlpha (0.72f));
    g.drawEllipse (gateLamp, 0.65f);

    drawEqGraph (g, editorLayout.lowerWorkspace);

    const auto footer = editorLayout.footer;
    juce::ColourGradient footerGradient (juce::Colour (0xff171412), footer.getX(), footer.getY(),
                                         juce::Colour (0xff080808), footer.getX(), footer.getBottom(), false);
    g.setGradientFill (footerGradient);
    g.fillRoundedRectangle (footer, 3.0f);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.76f));
    g.drawRoundedRectangle (footer, 3.0f, 0.8f);
    drawSteelTexture (g, footer, 75);

    const auto oversampleFactor = getOversamplingFactor (processor.parameters);
    const auto oversampleOn = oversampleFactor > 1;
    g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift"));
    g.setColour (dimText);
    g.drawText ("OVERSAMPLE", 22, juce::roundToInt (footer.getY() + 12.0f), 61, 13,
                juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff090909));
    g.fillRoundedRectangle (91.0f, footer.getY() + 8.0f, 41.0f, 19.0f, 7.0f);
    g.setColour (oversampleOn ? crimson : dimText.darker (0.52f));
    g.drawRoundedRectangle (91.0f, footer.getY() + 8.0f, 41.0f, 19.0f, 7.0f, 1.0f);
    g.setFont (juce::FontOptions (10.5f).withName ("Bahnschrift").withStyle ("SemiBold"));
    g.drawText (juce::String (oversampleFactor) + "X", 91, juce::roundToInt (footer.getY() + 8.0f),
                41, 19, juce::Justification::centred);

    // Restrained sequential reactor-bed illumination. The timer drives only this
    // visual phase; it has no connection to the audio processor or DSP state.
    g.setColour (juce::Colours::black.withAlpha (0.68f));
    juce::Path grille;
    grille.startNewSubPath (157.0f, footer.getBottom() - 4.0f);
    grille.lineTo (190.0f, footer.getY() + 4.0f);
    grille.lineTo (519.0f, footer.getY() + 4.0f);
    grille.lineTo (552.0f, footer.getBottom() - 4.0f);
    grille.closeSubPath();
    g.fillPath (grille);
    g.setColour (lowAccentColour.withAlpha (0.54f));
    g.strokePath (grille, juce::PathStrokeType (1.0f));

    constexpr int ventCount = 16;
    for (int vent = 0; vent < ventCount; ++vent)
    {
        const auto x = 184.0f + vent * 21.35f;
        const auto illumination = voidworm::ui::footerStripeIllumination (vent, animationPhase, ventCount);
        juce::Path slot;
        slot.startNewSubPath (x, footer.getBottom() - 9.0f);
        slot.lineTo (x + 10.5f, footer.getY() + 9.0f);
        slot.lineTo (x + 17.0f, footer.getY() + 9.0f);
        slot.lineTo (x + 6.5f, footer.getBottom() - 9.0f);
        slot.closeSubPath();
        const auto spatial = static_cast<float> (vent) / static_cast<float> (ventCount - 1);
        const auto ventColour = spatial < 0.5f
            ? lowAccentColour.interpolatedWith (crimson, spatial * 2.0f)
            : crimson.interpolatedWith (ember, (spatial - 0.5f) * 2.0f);
        if (illumination > 0.04f)
        {
            g.setColour (ventColour.interpolatedWith (ember, 0.28f).withAlpha (0.09f * illumination));
            g.fillPath (slot, juce::AffineTransform::scale (1.12f, 1.17f, x + 8.0f,
                                                           footer.getCentreY()));
        }
        g.setColour (ventColour.withAlpha (0.31f + 0.62f * illumination));
        g.fillPath (slot);
    }

    g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift"));
    g.setColour (dimText);
    g.drawText ("HQ MODE", 598, juce::roundToInt (footer.getY() + 12.0f), 57, 13,
                juce::Justification::centredRight);

    g.setColour (juce::Colour (0xff0a0909));
    g.fillRect (editorLayout.footerBase);
    drawSteelTexture (g, editorLayout.footerBase, 70);
    g.setColour (juce::Colour (0xff302a27));
    g.drawHorizontalLine (juce::roundToInt (editorLayout.footerBase.getY()), 4.0f, 704.0f);
    g.setFont (juce::FontOptions (10.0f).withName ("Bahnschrift").withStyle ("Light"));
    g.setColour (warmWhite.withAlpha (0.78f));
    drawTransientValue (g);
}

void VoidwormAudioProcessorEditor::drawMeter (juce::Graphics& g, juce::Rectangle<float> area, bool inputMeter)
{
    const auto lowAccentColour = lowAccent();
    const auto crimson = primaryAccent();
    const auto ember = secondaryAccent();
    const auto warmWhite = themeText();
    const auto dimText = themeDimText();
    g.setColour (juce::Colour (0xff090909).withAlpha (0.82f));
    g.fillRoundedRectangle (area, 3.0f);
    drawSteelTexture (g, area.reduced (1.0f), 35);
    drawDistressedFrame (g, area, 3.0f, inputMeter ? 19031 : 60193, isNeonTheme() ? 3 : 29,
                          crimson.darker (0.62f));
    g.setColour (warmWhite);
    g.setFont (juce::FontOptions (9.0f).withName ("Bahnschrift"));
    g.drawText (inputMeter ? "INPUT" : "OUTPUT",
                area.withHeight (inputMeter ? 19.0f : 27.0f).toNearestInt(), juce::Justification::centred);

    const auto meterTop = area.getY() + (inputMeter ? 60.0f : 37.0f);
    const auto meterBottom = area.getBottom() - 38.0f;
    const auto& display = inputMeter ? inputMeterDisplay : outputMeterDisplay;
    const auto leftLevel = display[0];
    const auto rightLevel = display[1];
    const std::array<float, 2> levels { leftLevel, rightLevel };
    const std::array<float, 2> xs { area.getCentreX() - 11.0f, area.getCentreX() + 5.0f };
    const auto meterFloor = meterFloorsDb[static_cast<size_t> (juce::jlimit (0, 3, meterFloorIndex))];
    constexpr int segments = 32;
    for (int channel = 0; channel < 2; ++channel)
    {
        const auto db = juce::Decibels::gainToDecibels (levels[static_cast<size_t> (channel)], meterFloor);
        const auto litSegments = juce::roundToInt (juce::jmap (juce::jlimit (meterFloor, 0.0f, db), meterFloor, 0.0f,
                                                               0.0f, static_cast<float> (segments)));
        for (int segment = 0; segment < segments; ++segment)
        {
            const auto y = meterBottom - (static_cast<float> (segment + 1) / static_cast<float> (segments)) * (meterBottom - meterTop);
            const auto lit = segment < litSegments;
            const auto energy = static_cast<float> (segment) / static_cast<float> (segments - 1);
            const auto energyColour = energy < 0.5f
                ? lowAccentColour.interpolatedWith (crimson, energy * 2.0f)
                : crimson.interpolatedWith (ember, (energy - 0.5f) * 2.0f);
            g.setColour (lit ? energyColour : lowAccentColour.withAlpha (0.20f));
            g.fillRect (xs[static_cast<size_t> (channel)], y, 6.0f, 3.2f);
        }
    }

    g.setFont (juce::FontOptions (6.8f).withName ("Bahnschrift"));
    for (int mark = 0; mark < 8; ++mark)
    {
        const auto db = juce::roundToInt (meterFloor * static_cast<float> (mark) / 7.0f);
        const auto y = juce::jmap (static_cast<float> (db), 0.0f, meterFloor, meterTop, meterBottom);
        g.setColour (dimText.withAlpha (0.75f));
        g.drawText (juce::String (db), juce::roundToInt (area.getX() + 4.0f), juce::roundToInt (y - 5.0f), 18, 10,
                    juce::Justification::centredRight);
        g.drawText (juce::String (db), juce::roundToInt (area.getRight() - 22.0f), juce::roundToInt (y - 5.0f), 18, 10,
                    juce::Justification::centredLeft);
    }

    const auto maxLevel = juce::jmax (leftLevel, rightLevel);
    const auto db = juce::Decibels::gainToDecibels (maxLevel, meterFloor);
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (area.getX() + 11.0f, area.getBottom() - 30.0f, area.getWidth() - 22.0f, 21.0f, 3.0f);
    g.setColour (juce::Colour (0xff4d1b14));
    g.drawRoundedRectangle (area.getX() + 11.0f, area.getBottom() - 30.0f, area.getWidth() - 22.0f, 21.0f, 3.0f, 0.8f);
    g.setColour (crimson);
    g.setFont (juce::FontOptions (10.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
    g.drawText (juce::String (db, 1),
                juce::Rectangle<float> (area.getX() + 12.0f, area.getBottom() - 30.0f,
                                        area.getWidth() - 24.0f, 13.0f).toNearestInt(),
                juce::Justification::centred);
    g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift"));
    g.drawText ("dB", juce::Rectangle<float> (area.getX() + 12.0f, area.getBottom() - 17.0f,
                                               area.getWidth() - 24.0f, 8.0f).toNearestInt(),
                juce::Justification::centred);
}

void VoidwormAudioProcessorEditor::drawEqGraph (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto lowAccentColour = lowAccent();
    const auto crimson = primaryAccent();
    const auto ember = secondaryAccent();
    const auto dimText = themeDimText();
    const auto text = themeText();
    g.setColour (juce::Colour (0xff070808));
    g.fillRect (area);
    g.setColour (uiStyle::utilityEdge.withAlpha (0.78f));
    g.drawRect (area, 0.8f);

    const std::array<const char*, 4> reactorNames { "MASS", "FURNACE", "ARC", "FEEDBACK" };
    const std::array<const char*, 4> enabledIds {
        "massEnabled", "furnaceEnabled", "arcEnabled", "feedbackEnabled"
    };
    const std::array<const char*, 4> amountIds {
        "massAmount", "furnaceAmount", "arcAmount", "feedbackAmount"
    };
    const auto layout = makeLowerWorkspaceLayout (area);
    const auto reactorPage = reactorWorkspace.isReactorPage();
    const auto selectedIndex = reactorWorkspace.selectedReactor;
    const auto soloTarget = processor.getReactorSoloTarget();

    const auto drawSelectorButton = [&] (juce::Rectangle<float> button, const juce::String& text,
                                         bool active, bool strong, juce::Colour activeColour)
    {
        juce::ColourGradient selector (active ? uiStyle::utilityRaised.interpolatedWith (activeColour, strong ? 0.055f : 0.035f)
                                              : uiStyle::utilityRaised,
                                       button.getX(), button.getY(), uiStyle::utilitySurface,
                                       button.getX(), button.getBottom(), false);
        g.setGradientFill (selector); g.fillRoundedRectangle (button, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawHorizontalLine (juce::roundToInt (button.getY() + 1.0f), button.getX() + 3.0f, button.getRight() - 3.0f);
        g.setColour (active ? activeColour.withAlpha (0.34f) : uiStyle::utilityEdge.withAlpha (0.58f));
        g.drawRoundedRectangle (button, 2.0f, active ? 0.9f : 0.55f);
        g.setFont (juce::FontOptions (strong ? 9.4f : 8.5f).withName ("Bahnschrift")
                   .withStyle (strong ? "SemiBold" : "Regular"));
        g.setColour (active ? themeText() : dimText.withAlpha (0.58f));
        g.drawFittedText (text, button.toNearestInt().reduced (2, 0), juce::Justification::centred, 1);
        if (strong)
        {
            const auto underline = juce::Rectangle<float> (button.getX() + uiStyle::m, button.getBottom() - 1.6f,
                juce::jmax (1.0f, button.getWidth() - uiStyle::m * 2.0f), 1.2f);
            juce::ColourGradient underlineGradient (lowAccentColour, underline.getX(), underline.getCentreY(),
                secondaryAccent(), underline.getRight(), underline.getCentreY(), false);
            underlineGradient.addColour (0.50, activeColour);
            g.setGradientFill (underlineGradient); g.fillRect (underline);
        }
    };

    drawSelectorButton (layout.masterTab, "MASTER", ! reactorPage, ! reactorPage, crimson);
    drawSelectorButton (layout.reactorTab, "REACTOR", reactorPage, reactorPage, crimson);

    if (reactorPage)
    {
        for (size_t index = 0; index < layout.reactorTabs.size(); ++index)
        {
            const auto enabled = processor.parameters.getRawParameterValue (enabledIds[index])->load() >= 0.5f;
            const auto amount = juce::jlimit (0.0f, 1.0f,
                processor.parameters.getRawParameterValue (amountIds[index])->load());
            const auto selected = selectedIndex == index;
            const auto soloed = soloTarget == static_cast<int> (index) + 1;
            auto status = juce::String (reactorNames[index]);
            if (soloed)
                status << "  S";
            else if (! enabled)
                status << "  OFF";
            else
                status << "  " << juce::roundToInt (amount * 100.0f) << "%";
            drawSelectorButton (layout.reactorTabs[index], status, selected || enabled,
                                selected, soloed ? ember : crimson);
        }

        const auto enabled = processor.parameters.getRawParameterValue (enabledIds[selectedIndex])->load() >= 0.5f;
        const auto amount = juce::jlimit (0.0f, 1.0f,
            processor.parameters.getRawParameterValue (amountIds[selectedIndex])->load());
        const auto characterBinding = getSelectedCharacterBinding();
        g.setColour (dimText.withAlpha (0.58f));
        g.setFont (juce::FontOptions (7.2f).withName ("Bahnschrift"));
        g.drawText ("SELECTED REACTOR", juce::Rectangle<float> (
                        layout.context.getX() + 8.0f, layout.context.getY() + 4.0f, 108.0f, 11.0f).toNearestInt(),
                    juce::Justification::centredLeft);
        g.setColour (text);
        g.setFont (juce::FontOptions (12.5f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText (reactorNames[selectedIndex], juce::Rectangle<float> (
                        layout.context.getX() + 8.0f, layout.context.getY() + 16.0f, 108.0f, 24.0f).toNearestInt(),
                    juce::Justification::centredLeft);
        juce::ignoreUnused (amount);
        g.setColour (dimText);
        g.setFont (juce::FontOptions (7.3f).withName ("Bahnschrift").withStyle ("SemiBold"));
        const std::array<juce::String, 3> characterLabels {
            "AMOUNT", characterBinding.labelA, characterBinding.labelB
        };
        const std::array<juce::Rectangle<float>, 3> characterSliders {
            layout.amountSlider, layout.characterASlider, layout.characterBSlider
        };
        for (size_t index = 0; index < characterLabels.size(); ++index)
            g.drawFittedText (characterLabels[index],
                              characterSliders[index].withY (layout.context.getY()).withHeight (11.0f)
                                  .expanded (17.0f, 0.0f).toNearestInt(),
                              juce::Justification::centred, 1);
        drawSelectorButton (layout.solo, "SOLO", soloTarget != 0, soloTarget != 0, ember);
        drawSelectorButton (layout.enabled, enabled ? "ON" : "OFF", enabled, true, crimson);
        g.setColour (dimText.withAlpha (0.76f));
        g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("PRE EQ", layout.preEqLabel.toNearestInt().reduced (3, 0),
                    juce::Justification::centredLeft);
    }
    else
    {
        juce::ColourGradient masterBed (juce::Colour (0xff151210), layout.masterControls.getX(), layout.masterControls.getY(),
                                        juce::Colour (0xff060606), layout.masterControls.getX(), layout.masterControls.getBottom(), false);
        g.setGradientFill (masterBed); g.fillRect (layout.masterControls);
        const auto postGroup = layout.masterControls.withWidth (178.0f).reduced (3.0f);
        const auto finalGroup = layout.masterControls.withTrimmedLeft (184.0f).reduced (3.0f);
        for (const auto group : { postGroup, finalGroup })
        {
            g.setColour (juce::Colours::black.withAlpha (0.24f)); g.fillRoundedRectangle (group, 3.0f);
            g.setColour (uiStyle::utilityEdge.withAlpha (0.34f)); g.drawRoundedRectangle (group, 3.0f, 0.55f);
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.drawHorizontalLine (juce::roundToInt (group.getY() + 1.0f), group.getX() + 5.0f, group.getRight() - 5.0f);
        }
        g.setColour (crimson.darker (0.70f).withAlpha (0.60f));
        g.drawHorizontalLine (juce::roundToInt (layout.masterControls.getBottom()),
                              layout.masterControls.getX(), layout.masterControls.getRight());
        constexpr auto groupLabelInset = 8.0f;
        const auto postReactorLabelArea = juce::Rectangle<float> (
            postGroup.getX() + groupLabelInset, editorLayout.masterLabelY,
            editorLayout.postReactorLabel.getWidth(), editorLayout.masterLabelHeight);
        const auto finalLabelArea = juce::Rectangle<float> (
            finalGroup.getX() + groupLabelInset, editorLayout.masterLabelY,
            editorLayout.finalLabel.getWidth(), editorLayout.masterLabelHeight);
        g.setColour (dimText.withAlpha (0.78f));
        g.setFont (juce::FontOptions (7.6f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText ("POST REACTOR", postReactorLabelArea.toNearestInt(),
                    juce::Justification::centredLeft);
        g.drawText ("FINAL STAGE", finalLabelArea.toNearestInt(),
                    juce::Justification::centredLeft);
        auto reductionArea = layout.gainReduction;
        const auto drawGainReduction = [&] (juce::Rectangle<float> row, const juce::String& label, float reduction)
        {
            auto telemetryArea = row.removeFromRight (122.0f).reduced (4.0f, 0.0f);
            auto valueArea = telemetryArea.removeFromRight (46.0f);
            telemetryArea.removeFromRight (6.0f);
            g.setColour (dimText.withAlpha (0.72f));
            g.setFont (juce::FontOptions (7.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText (label, telemetryArea.toNearestInt(), juce::Justification::centredRight);

            const auto emphasis = juce::jlimit (0.0f, 1.0f, reduction / 6.0f);
            g.setColour (crimson.interpolatedWith (ember, emphasis * 0.35f)
                                 .withAlpha (0.58f + emphasis * 0.34f));
            g.setFont (juce::FontOptions (7.5f).withName ("Bahnschrift").withStyle ("SemiBold"));
            g.drawText ("-" + juce::String (reduction, 1) + " dB", valueArea.toNearestInt(),
                        juce::Justification::centredRight);
        };
        drawGainReduction (reductionArea.removeFromTop (20.0f), "WELD GR", weldGainReductionDisplay);
        drawGainReduction (reductionArea, "LIMIT GR", limiterGainReductionDisplay);
    }

    const auto plot = reactorPage ? layout.reactorPlot : layout.masterPlot;
    const auto valueRow = reactorPage ? layout.valueRow : juce::Rectangle<float>();

    const std::array<float, 10> gridFrequencies {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };
    for (size_t line = 0; line < gridFrequencies.size(); ++line)
    {
        const auto x = plot.getX() + frequencyToProportion (gridFrequencies[line]) * plot.getWidth();
        g.setColour (crimson.darker (0.88f).withAlpha (line % 2 == 0 ? 0.40f : 0.20f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int line = 1; line < 4; ++line)
    {
        const auto y = plot.getY() + plot.getHeight() * static_cast<float> (line) / 4.0f;
        g.setColour (crimson.darker (0.88f).withAlpha (0.32f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    voidworm::ReactorEqSettings reactorSettings;
    if (reactorPage)
    {
        const auto ids = getSelectedEqParameterIds();
        reactorSettings = {
            processor.parameters.getRawParameterValue (ids.hp)->load(),
            processor.parameters.getRawParameterValue (ids.focus1)->load(),
            processor.parameters.getRawParameterValue (ids.gain1)->load(),
            processor.parameters.getRawParameterValue (ids.lp)->load(),
            processor.parameters.getRawParameterValue (ids.focus2)->load(),
            processor.parameters.getRawParameterValue (ids.gain2)->load()
        };
    }

    const auto hostRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    const auto responseSampleRate = reactorPage ? hostRate * getOversamplingFactor (processor.parameters) : hostRate;
    std::optional<voidworm::ui::ReactorEqGraphModel> reactorGraph;
    if (reactorPage)
        reactorGraph.emplace (responseSampleRate, reactorSettings, plot);
    juce::Path response;
    juce::Path fill;
    constexpr int points = 160;
    std::vector<float> curvePositions;
    curvePositions.reserve (points + 4);
    for (int point = 0; point < points; ++point)
        curvePositions.push_back (static_cast<float> (point) / static_cast<float> (points - 1));
    if (reactorGraph.has_value())
        for (const auto frequency : reactorGraph->nodeFrequencies())
            curvePositions.push_back (frequencyToProportion (frequency));
    std::sort (curvePositions.begin(), curvePositions.end());
    curvePositions.erase (std::unique (curvePositions.begin(), curvePositions.end()), curvePositions.end());
    for (size_t point = 0; point < curvePositions.size(); ++point)
    {
        const auto t = curvePositions[point];
        const auto frequency = proportionToFrequency (t);
        juce::Point<float> graphPoint;
        if (reactorGraph.has_value())
            graphPoint = reactorGraph->responsePoint (frequency);
        else
        {
            const auto magnitude = voidworm::OutputTone::getResponseMagnitude (
                responseSampleRate, frequency, static_cast<float> (range.getValue()),
                static_cast<float> (low.getValue()), static_cast<float> (mid.getValue()),
                static_cast<float> (high.getValue()));
            const auto totalDb = juce::Decibels::gainToDecibels (juce::jmax (0.0001f, magnitude));
            graphPoint = { plot.getX() + t * plot.getWidth(),
                juce::jlimit (plot.getY() + 1.0f, plot.getBottom() - 1.0f,
                              plot.getCentreY() - totalDb * plot.getHeight() * 0.36f / 12.0f) };
        }
        const auto x = graphPoint.x;
        const auto y = graphPoint.y;
        if (point == 0)
        {
            response.startNewSubPath (x, y);
            fill.startNewSubPath (x, plot.getBottom());
            fill.lineTo (x, y);
        }
        else
        {
            response.lineTo (x, y);
            fill.lineTo (x, y);
        }
    }
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.closeSubPath();
    juce::ColourGradient responseFill (crimson.withAlpha (0.19f), plot.getCentreX(), plot.getY(),
                                       lowAccentColour.withAlpha (0.0f), plot.getCentreX(), plot.getBottom(), false);
    responseFill.addColour (0.28, crimson.withAlpha (0.12f));
    g.setGradientFill (responseFill);
    g.fillPath (fill);
    g.setColour (ember.withAlpha (0.14f));
    g.strokePath (response, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
    juce::ColourGradient traceGradient (lowAccentColour.interpolatedWith (crimson, 0.45f), plot.getX(), plot.getCentreY(),
                                        ember, plot.getRight(), plot.getCentreY(), false);
    traceGradient.addColour (0.52, crimson);
    g.setGradientFill (traceGradient);
    g.strokePath (response, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved));

    g.setColour (dimText.withAlpha (0.70f));
    g.setFont (juce::FontOptions (7.0f).withName ("Bahnschrift"));
    const std::array<const char*, 5> labels { "20", "100", "1k", "10k", "20k" };
    const std::array<float, 5> frequencies { 20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f };
    for (size_t index = 0; index < labels.size(); ++index)
    {
        const auto x = plot.getX() + frequencyToProportion (frequencies[index]) * plot.getWidth();
        g.drawText (labels[index], juce::roundToInt (x - 11.0f), juce::roundToInt (plot.getBottom() - 9.0f), 22, 8,
                    juce::Justification::centred);
    }

    if (reactorPage)
    {
        const auto& sanitised = reactorGraph->settings;
        for (const auto node : reactorGraph->nodePositions())
        {
            g.setColour (juce::Colour (0xff080808));
            g.fillEllipse (node.x - 4.0f, node.y - 4.0f, 8.0f, 8.0f);
            g.setColour (crimson);
            g.drawEllipse (node.x - 4.0f, node.y - 4.0f, 8.0f, 8.0f, 1.1f);
        }

        g.setColour (dimText.withAlpha (0.62f));
        g.setFont (juce::FontOptions (7.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
        g.drawText (juce::String (reactorNames[selectedIndex]) + " // PRE",
                    plot.toNearestInt().reduced (5, 1), juce::Justification::topRight);

        const auto formatGain = [] (float gain)
        {
            return juce::String (gain > 0.0f ? "+" : "") + juce::String (gain, 1) + " dB";
        };
        const std::array<juce::String, 6> values {
            juce::String ("HP ") + formatFrequency (sanitised.hp),
            juce::String ("F1 ") + formatFrequency (sanitised.focusFrequency),
            juce::String ("G1 ") + formatGain (sanitised.focusGainDb),
            juce::String ("F2 ") + formatFrequency (sanitised.focus2Frequency),
            juce::String ("G2 ") + formatGain (sanitised.focus2GainDb),
            juce::String ("LP ") + formatFrequency (sanitised.lp)
        };
        g.setColour (juce::Colour (0xff0a0909));
        g.fillRect (valueRow);
        g.setFont (juce::FontOptions (7.4f).withName ("Bahnschrift"));
        for (size_t index = 0; index < values.size(); ++index)
        {
            const auto valueArea = juce::Rectangle<float> (
                valueRow.getX() + valueRow.getWidth() * static_cast<float> (index) / 6.0f,
                valueRow.getY(), valueRow.getWidth() / 6.0f, valueRow.getHeight());
            g.setColour (index == 2 || index == 4 ? crimson : dimText);
            g.drawText (values[index], valueArea.toNearestInt(), juce::Justification::centred);
        }
    }
}

void VoidwormAudioProcessorEditor::drawTransientValue (juce::Graphics& g)
{
    const auto crimson = primaryAccent();
    if (activeSlider == nullptr)
        return;

    const auto name = activeSlider->getName();
    float centreX = 356.0f;
    float y = 182.0f;
    if (name == "BREACH") centreX = 146.0f;
    else if (name == "TEAR") centreX = 231.0f;
    else if (name == "OVERLOAD") { centreX = 356.0f; y = 220.0f; }
    else if (name == "ROT") centreX = 475.0f;
    else if (name == "DRIVE") { centreX = 475.0f; y = 226.0f; }
    else if (name == "GATE THRESHOLD") { centreX = headerGeometry().gateThreshold.getCentreX(); y = 117.0f; }
    else if (name == "MIX") centreX = 559.0f;
    else if (name == "LOW") { centreX = 236.0f; y = 328.0f; }
    else if (name == "MID") { centreX = 319.0f; y = 328.0f; }
    else if (name == "HIGH") { centreX = 402.0f; y = 328.0f; }
    else if (name == "RANGE") { centreX = 485.0f; y = 328.0f; }
    else if (name == "OUTPUT") { centreX = 568.0f; y = 328.0f; }
    const auto isDb = name == "DRIVE" || name == "GATE THRESHOLD" || name == "LOW" || name == "MID"
                   || name == "HIGH" || name == "OUTPUT";
    const auto parameterSpan = juce::jmax (0.000001, activeSlider->getMaximum() - activeSlider->getMinimum());
    const auto percentage = (activeSlider->getValue() - activeSlider->getMinimum()) / parameterSpan;
    const auto text = isDb ? juce::String (activeSlider->getValue(), 1) + " dB"
                           : juce::String (juce::roundToInt (percentage * 100.0)) + "%";

    const auto valueWidth = name == "DRIVE" ? 48.0f : 56.0f;
    const auto valueArea = juce::Rectangle<float> (centreX - valueWidth * 0.5f, y, valueWidth, 15.0f);
    g.setColour (juce::Colour (0xff070707));
    g.fillRoundedRectangle (valueArea, 4.0f);
    g.setColour (crimson.withAlpha (0.68f));
    g.drawRoundedRectangle (valueArea, 4.0f, 0.8f);
    g.setColour (crimson.brighter (0.42f));
    g.setFont (juce::FontOptions (8.0f).withName ("Bahnschrift").withStyle ("SemiBold"));
    g.drawText (text, valueArea.toNearestInt(), juce::Justification::centred);
}

void VoidwormAudioProcessorEditor::resized()
{
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    const auto R = [sx, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * sx), juce::roundToInt (y * sy),
                                     juce::roundToInt (w * sx), juce::roundToInt (h * sy));
    };

    const auto scaled = [&R] (juce::Rectangle<float> area)
    {
        return R (area.getX(), area.getY(), area.getWidth(), area.getHeight());
    };

    breach.setBounds (scaled (editorLayout.breach));
    tear.setBounds (scaled (editorLayout.tear));
    overload.setBounds (scaled (editorLayout.overload));
    rot.setBounds (scaled (editorLayout.rot));
    drive.setBounds (scaled (editorLayout.drive));
    mix.setBounds (scaled (editorLayout.mix));

    surgeSwitch.setBounds (scaled (editorLayout.surge));
    low.setBounds (scaled (editorLayout.secondaryKnobs[0]));
    mid.setBounds (scaled (editorLayout.secondaryKnobs[1]));
    high.setBounds (scaled (editorLayout.secondaryKnobs[2]));
    range.setBounds (scaled (editorLayout.secondaryKnobs[3]));
    output.setBounds (scaled (editorLayout.secondaryKnobs[4]));
    const auto workspaceLayout = makeLowerWorkspaceLayout (editorLayout.lowerWorkspace);
    weld.setBounds (R (workspaceLayout.weldSlider.getX(), workspaceLayout.weldSlider.getY(),
                       workspaceLayout.weldSlider.getWidth(), workspaceLayout.weldSlider.getHeight()));
    limiterThreshold.setBounds (R (workspaceLayout.limiterThresholdSlider.getX(), workspaceLayout.limiterThresholdSlider.getY(),
                                   workspaceLayout.limiterThresholdSlider.getWidth(), workspaceLayout.limiterThresholdSlider.getHeight()));
    limiterCeiling.setBounds (R (workspaceLayout.limiterCeilingSlider.getX(), workspaceLayout.limiterCeilingSlider.getY(),
                                 workspaceLayout.limiterCeilingSlider.getWidth(), workspaceLayout.limiterCeilingSlider.getHeight()));
    limiterSwitch.setBounds (R (workspaceLayout.limiterEnabled.getX(), workspaceLayout.limiterEnabled.getY(),
                                workspaceLayout.limiterEnabled.getWidth(), workspaceLayout.limiterEnabled.getHeight()));
    reactorAmount.setBounds (R (workspaceLayout.amountSlider.getX(), workspaceLayout.amountSlider.getY(),
                                workspaceLayout.amountSlider.getWidth(), workspaceLayout.amountSlider.getHeight()));
    reactorCharacterA.setBounds (R (workspaceLayout.characterASlider.getX(), workspaceLayout.characterASlider.getY(),
                                    workspaceLayout.characterASlider.getWidth(), workspaceLayout.characterASlider.getHeight()));
    reactorCharacterB.setBounds (R (workspaceLayout.characterBSlider.getX(), workspaceLayout.characterBSlider.getY(),
                                    workspaceLayout.characterBSlider.getWidth(), workspaceLayout.characterBSlider.getHeight()));
    for (auto* slider : { &reactorAmount, &reactorCharacterA, &reactorCharacterB })
        slider->setVisible (reactorWorkspace.isReactorPage());
    const auto masterVisible = ! reactorWorkspace.isReactorPage();
    const std::array<juce::Component*, 8> masterComponents {
        &weld, &limiterThreshold, &limiterCeiling, &limiterSwitch,
        &weldLabel, &limiterThresholdLabel, &limiterCeilingLabel, &limiterSwitchLabel
    };
    for (auto* component : masterComponents)
        component->setVisible (masterVisible);

    const auto setLabelAt = [&R] (juce::Label& label, float centreX, float y, float width, float height = 14.0f)
    {
        label.setBounds (R (centreX - width * 0.5f, y, width, height));
    };
    setLabelAt (breachLabel, editorLayout.breach.getCentreX(), editorLayout.primaryLabelY, 78.0f);
    setLabelAt (tearLabel, editorLayout.tear.getCentreX(), editorLayout.primaryLabelY, 78.0f);
    setLabelAt (overloadLabel, editorLayout.overload.getCentreX(), editorLayout.overloadLabelY, 118.0f, 16.0f);
    setLabelAt (rotLabel, editorLayout.rot.getCentreX(), editorLayout.primaryLabelY, 78.0f);
    setLabelAt (driveLabel, editorLayout.drive.getCentreX(), 185.0f, 48.0f, 10.0f);
    setLabelAt (mixLabel, editorLayout.mix.getCentreX(), editorLayout.primaryLabelY, 78.0f);
    setLabelAt (surgeLabel, editorLayout.surge.getCentreX(), editorLayout.surgeLabelY, 78.0f);
    setLabelAt (lowLabel, editorLayout.secondaryKnobs[0].getCentreX(), editorLayout.secondaryLabelY, 78.0f);
    setLabelAt (midLabel, editorLayout.secondaryKnobs[1].getCentreX(), editorLayout.secondaryLabelY, 78.0f);
    setLabelAt (highLabel, editorLayout.secondaryKnobs[2].getCentreX(), editorLayout.secondaryLabelY, 78.0f);
    setLabelAt (rangeLabel, editorLayout.secondaryKnobs[3].getCentreX(), editorLayout.secondaryLabelY, 94.0f);
    setLabelAt (outputLabel, editorLayout.secondaryKnobs[4].getCentreX(), editorLayout.secondaryLabelY, 78.0f);
    const auto setWorkspaceLabel = [&R] (juce::Label& label, juce::Rectangle<float> sliderArea, float width)
    {
        label.setBounds (R (sliderArea.getCentreX() - width * 0.5f, editorLayout.masterLabelY,
                            width, editorLayout.masterLabelHeight));
    };
    setWorkspaceLabel (weldLabel, workspaceLayout.weldSlider, 58.0f);
    setWorkspaceLabel (limiterThresholdLabel, workspaceLayout.limiterThresholdSlider, 58.0f);
    setWorkspaceLabel (limiterCeilingLabel, workspaceLayout.limiterCeilingSlider, 68.0f);
    limiterSwitchLabel.setBounds (R (workspaceLayout.limiterEnabled.getX(), editorLayout.masterLabelY,
                                     workspaceLayout.limiterEnabled.getWidth(), editorLayout.masterLabelHeight));

    const auto header = headerGeometry();
    previousPresetButton.setBounds (scaled (header.previousPreset));
    presetMenuButton.setBounds (scaled (header.presetMenu));
    nextPresetButton.setBounds (scaled (header.nextPreset));
    gateButton.setBounds (scaled (header.gate));
    gateThreshold.setBounds (scaled (header.gateThreshold));
    setLabelAt (gateThresholdLabel, header.gateThreshold.getCentreX(), 83.0f, 29.0f, 7.0f);
    gridButton.setBounds (scaled (header.grid));
    searchButton.setBounds (scaled (header.search));
    settingsButton.setBounds (scaled (header.settings));
    minimiseButton.setBounds (scaled (header.minimise));
    closeButton.setBounds (scaled (header.close));
    oversampleButton.setBounds (scaled (editorLayout.oversampleHit));
    hqButton.setBounds (scaled (editorLayout.hqButton));
    overlay.setBounds (getLocalBounds());
    const auto toastWidth = juce::jmin (juce::roundToInt (330.0f * sx), getWidth() - 24);
    standaloneToast.setBounds ((getWidth() - toastWidth) / 2, juce::roundToInt (60.0f * sy),
                               toastWidth, juce::roundToInt (34.0f * sy));

    const auto smallFont = 9.0f * sy;
    for (auto* label : { &breachLabel, &tearLabel, &rotLabel, &mixLabel, &surgeLabel,
                         &lowLabel, &midLabel, &highLabel, &rangeLabel, &outputLabel,
                         &weldLabel, &limiterThresholdLabel, &limiterCeilingLabel, &limiterSwitchLabel })
        label->setFont (juce::FontOptions (smallFont).withName ("Bahnschrift").withStyle ("Regular"));
    for (auto* label : { &weldLabel, &limiterThresholdLabel, &limiterCeilingLabel, &limiterSwitchLabel })
        label->setFont (juce::FontOptions (8.0f * sy).withName ("Bahnschrift").withStyle ("SemiBold"));
    driveLabel.setFont (juce::FontOptions (7.0f * sy).withName ("Bahnschrift").withStyle ("Regular"));
    gateThresholdLabel.setFont (juce::FontOptions (5.8f * sy).withName ("Bahnschrift").withStyle ("SemiBold"));
    overloadLabel.setFont (juce::FontOptions (14.0f * sy).withName ("Bahnschrift").withStyle ("Light"));
}

void VoidwormAudioProcessorEditor::toggleParameter (const char* parameterID)
{
    if (auto* parameter = processor.parameters.getParameter (parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->getValue() >= 0.5f ? 0.0f : 1.0f);
        parameter->endChangeGesture();
        repaint();
    }
}

void VoidwormAudioProcessorEditor::cycleOversampling()
{
    if (auto* parameter = processor.parameters.getParameter ("oversample"))
    {
        const auto currentIndex = juce::jlimit (0, 3,
            juce::roundToInt (processor.parameters.getRawParameterValue ("oversample")->load()));
        const auto nextIndex = (currentIndex + 1) % 4;
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (nextIndex)));
        parameter->endChangeGesture();
        repaint();
        overlay.repaint();
    }
}

void VoidwormAudioProcessorEditor::configureUiPreferences()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "VOIDWORM";
    options.filenameSuffix = ".settings";
    options.folderName = "LWNX DSP/VOIDWORM";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    applicationProperties.setStorageParameters (options);

    if (auto* settings = applicationProperties.getUserSettings())
    {
        uiScaleIndex = juce::jlimit (0, 2, settings->getIntValue ("uiScaleIndex", 1));
        meterFloorIndex = juce::jlimit (0, 3, settings->getIntValue ("meterFloorIndex", 1));
        meterHoldIndex = juce::jlimit (0, 3, settings->getIntValue ("meterHoldIndex", 2));
    }
}

void VoidwormAudioProcessorEditor::saveUiPreferences()
{
    if (auto* settings = applicationProperties.getUserSettings())
    {
        settings->setValue ("uiScaleIndex", uiScaleIndex);
        settings->setValue ("meterFloorIndex", meterFloorIndex);
        settings->setValue ("meterHoldIndex", meterHoldIndex);
        settings->saveIfNeeded();
    }
}

void VoidwormAudioProcessorEditor::applyUiScale()
{
    const auto scale = uiScaleFactors[static_cast<size_t> (juce::jlimit (0, 2, uiScaleIndex))];
    setSize (juce::roundToInt (920.0f * scale), juce::roundToInt (823.0f * scale));
}

void VoidwormAudioProcessorEditor::cycleUiScale()
{
    uiScaleIndex = (uiScaleIndex + 1) % static_cast<int> (uiScaleFactors.size());
    saveUiPreferences();
    applyUiScale();
    overlay.repaint();
}

void VoidwormAudioProcessorEditor::cycleMeterFloor()
{
    meterFloorIndex = (meterFloorIndex + 1) % static_cast<int> (meterFloorsDb.size());
    saveUiPreferences();
    repaint();
    overlay.repaint();
}

void VoidwormAudioProcessorEditor::cycleMeterHold()
{
    meterHoldIndex = (meterHoldIndex + 1) % static_cast<int> (meterHoldTicks.size());
    inputMeterHold.fill (0);
    outputMeterHold.fill (0);
    saveUiPreferences();
    overlay.repaint();
}

void VoidwormAudioProcessorEditor::applyPreset (int index)
{
    const auto& presets = voidworm::factoryPresets();
    presetIndex = (index % static_cast<int> (presets.size()) + static_cast<int> (presets.size()))
                % static_cast<int> (presets.size());
    const auto snapshot = voidworm::makeFactoryPresetSnapshot (
        presetIndex, processor.captureParameterSnapshot());
    applySoundSnapshot (snapshot.parameters, presets[static_cast<size_t> (presetIndex)].name, true);
}

void VoidwormAudioProcessorEditor::applyUserPreset (size_t index)
{
    userPresets.refresh();
    if (index >= userPresets.presets().size()) return;
    const auto& selected = userPresets.presets()[index];
    const auto loaded = userPresets.load (selected.file);
    if (! loaded.ok || ! loaded.preset) return;
    auto sound = loaded.preset->parameters;
    const auto current = processor.captureParameterSnapshot();
    sound.oversampleFactor = current.oversampleFactor;
    sound.hqMode = current.hqMode;
    sound.reactorSolo = current.reactorSolo;
    applySoundSnapshot (sound, loaded.preset->name, false, loaded.preset->file);
}

void VoidwormAudioProcessorEditor::applySoundSnapshot (voidworm::Parameters p, const juce::String& name,
                                                        bool factory, const juce::File& userFile)
{
    presetName = name;
    presetAssociation = factory ? PresetAssociation::factory : PresetAssociation::user;
    currentUserPresetFile = factory ? juce::File() : userFile;
    presetIsDirty = false;
    cleanPresetSound = p;
    processor.beginPresetChange();
    const std::array<const char*, 11> ids {
        "breach", "tear", "rot", "drive", "overload", "mix",
        "low", "mid", "high", "range", "output"
    };
    const std::array<float, 11> values {
        p.breach, p.tear, p.rot, p.driveDb, p.overload, p.mix,
        p.lowDb, p.midDb, p.highDb, p.range, p.outputDb
    };
    for (size_t value = 0; value < ids.size(); ++value)
        setParameterActual (processor.parameters, ids[value], values[value]);
    setParameterActual (processor.parameters, "surge", p.surge ? 1.0f : 0.0f);
    setParameterActual (processor.parameters, "weld", p.weld);
    setParameterActual (processor.parameters, "limiterEnabled", p.limiterEnabled ? 1.0f : 0.0f);
    setParameterActual (processor.parameters, "limiterThreshold", p.limiterThresholdDb);
    setParameterActual (processor.parameters, "limiterCeiling", p.limiterCeilingDb);
    setParameterActual (processor.parameters, "gateEnabled", p.gateEnabled ? 1.0f : 0.0f);
    setParameterActual (processor.parameters, "gateThreshold", p.gateThresholdDb);

    const std::array<const char*, 4> amountIds {
        "massAmount", "furnaceAmount", "arcAmount", "feedbackAmount"
    };
    for (size_t value = 0; value < amountIds.size(); ++value)
        setParameterActual (processor.parameters, amountIds[value], p.reactorAmounts[value]);
    const std::array<const char*, 4> enabledIds {
        "massEnabled", "furnaceEnabled", "arcEnabled", "feedbackEnabled"
    };
    for (size_t value = 0; value < enabledIds.size(); ++value)
        setParameterActual (processor.parameters, enabledIds[value], p.reactorEnabled[value] ? 1.0f : 0.0f);

    constexpr std::array<const char*, 8> characterIds {
        "massSaturation", "massHarmonics", "furnaceStarve", "furnaceFold",
        "arcXmod", "arcFold", "feedbackReturn", "feedbackDamp"
    };
    const std::array<float, 8> characterValues {
        p.reactorCharacter.massSaturation, p.reactorCharacter.massHarmonics,
        p.reactorCharacter.furnaceStarve, p.reactorCharacter.furnaceFold,
        p.reactorCharacter.arcXmod, p.reactorCharacter.arcFold,
        p.reactorCharacter.feedbackReturn, p.reactorCharacter.feedbackDamp
    };
    for (size_t characterIndex = 0; characterIndex < characterIds.size(); ++characterIndex)
        setParameterActual (processor.parameters, characterIds[characterIndex], characterValues[characterIndex]);

    const std::array<const char*, 24> eqIds {
        "massHp", "massFocusFreq", "massFocusGain", "massFocus2Freq", "massFocus2Gain", "massLp",
        "furnaceHp", "furnaceFocusFreq", "furnaceFocusGain", "furnaceFocus2Freq", "furnaceFocus2Gain", "furnaceLp",
        "arcHp", "arcFocusFreq", "arcFocusGain", "arcFocus2Freq", "arcFocus2Gain", "arcLp",
        "feedbackHp", "feedbackFocusFreq", "feedbackFocusGain", "feedbackFocus2Freq", "feedbackFocus2Gain", "feedbackLp"
    };
    const auto& massPreset = p.massEq;
    const auto& furnacePreset = p.furnaceEq;
    const auto& arcPreset = p.arcEq;
    const auto& feedbackPreset = p.feedbackEq;
    const std::array<float, 24> eqValues {
        massPreset.hp, massPreset.focusFrequency, massPreset.focusGainDb, massPreset.focus2Frequency, massPreset.focus2GainDb, massPreset.lp,
        furnacePreset.hp, furnacePreset.focusFrequency, furnacePreset.focusGainDb, furnacePreset.focus2Frequency, furnacePreset.focus2GainDb, furnacePreset.lp,
        arcPreset.hp, arcPreset.focusFrequency, arcPreset.focusGainDb, arcPreset.focus2Frequency, arcPreset.focus2GainDb, arcPreset.lp,
        feedbackPreset.hp, feedbackPreset.focusFrequency, feedbackPreset.focusGainDb, feedbackPreset.focus2Frequency, feedbackPreset.focus2GainDb, feedbackPreset.lp
    };
    for (size_t value = 0; value < eqIds.size(); ++value)
        setParameterActual (processor.parameters, eqIds[value], eqValues[value]);
    processor.commitPresetChange ({ p });
    repaint();
    overlay.repaint();
}

VoidwormAudioProcessorEditor::EqParameterIds
VoidwormAudioProcessorEditor::getSelectedEqParameterIds() const noexcept
{
    switch (reactorWorkspace.selectedReactor)
    {
        case 1: return { "furnaceHp", "furnaceFocusFreq", "furnaceFocusGain", "furnaceFocus2Freq", "furnaceFocus2Gain", "furnaceLp" };
        case 2: return { "arcHp", "arcFocusFreq", "arcFocusGain", "arcFocus2Freq", "arcFocus2Gain", "arcLp" };
        case 3: return { "feedbackHp", "feedbackFocusFreq", "feedbackFocusGain", "feedbackFocus2Freq", "feedbackFocus2Gain", "feedbackLp" };
        default: break;
    }
    return { "massHp", "massFocusFreq", "massFocusGain", "massFocus2Freq", "massFocus2Gain", "massLp" };
}

const char* VoidwormAudioProcessorEditor::getSelectedAmountParameterId() const noexcept
{
    constexpr std::array<const char*, 4> ids {
        "massAmount", "furnaceAmount", "arcAmount", "feedbackAmount"
    };
    return ids[reactorWorkspace.selectedReactor];
}

VoidwormAudioProcessorEditor::CharacterBinding
VoidwormAudioProcessorEditor::getSelectedCharacterBinding() const noexcept
{
    switch (reactorWorkspace.selectedReactor)
    {
        case 1: return { "furnaceStarve", "furnaceFold", "STARVE", "FOLD",
                         "Supply starvation and sag depth inside FURNACE",
                         "Reflection-fold contribution when FURNACE enters high-ROT topology" };
        case 2: return { "arcXmod", "arcFold", "XMOD", "FOLD",
                         "Source-derived cross-modulation between ARC frequency bands",
                         "Reflection-fold depth after ARC interaction" };
        case 3: return { "feedbackReturn", "feedbackDamp", "RETURN", "DAMP",
                         "Recursive signal returned through the FEEDBACK circuit",
                         "High-frequency damping inside the recursive FEEDBACK loop" };
        default: break;
    }
    return { "massSaturation", "massHarmonics", "SATURATION", "HARMONICS",
             "Nonlinear drive inside the MASS body circuit",
             "Blend between MASS body and saturated harmonic content" };
}

const char* VoidwormAudioProcessorEditor::getSelectedEnabledParameterId() const noexcept
{
    constexpr std::array<const char*, 4> ids {
        "massEnabled", "furnaceEnabled", "arcEnabled", "feedbackEnabled"
    };
    return ids[reactorWorkspace.selectedReactor];
}

void VoidwormAudioProcessorEditor::syncTransientSolo() noexcept
{
    processor.setReactorSoloTarget (reactorWorkspace.soloTarget());
}

void VoidwormAudioProcessorEditor::bindSelectedReactorControls()
{
    reactorAmountAttachment.reset();
    reactorCharacterAAttachment.reset();
    reactorCharacterBAttachment.reset();
    if (reactorWorkspace.isReactorPage())
    {
        const auto binding = getSelectedCharacterBinding();
        reactorCharacterA.setName (binding.labelA);
        reactorCharacterB.setName (binding.labelB);
        reactorCharacterA.setTooltip (binding.tooltipA);
        reactorCharacterB.setTooltip (binding.tooltipB);
        reactorAmountAttachment = std::make_unique<Attachment> (
            processor.parameters, getSelectedAmountParameterId(), reactorAmount);
        reactorCharacterAAttachment = std::make_unique<Attachment> (
            processor.parameters, binding.idA, reactorCharacterA);
        reactorCharacterBAttachment = std::make_unique<Attachment> (
            processor.parameters, binding.idB, reactorCharacterB);
    }
    for (auto* slider : { &reactorAmount, &reactorCharacterA, &reactorCharacterB })
        slider->setVisible (reactorWorkspace.isReactorPage());
}

void VoidwormAudioProcessorEditor::selectMasterWorkspace()
{
    reactorWorkspace.selectMaster();
    syncTransientSolo();
    bindSelectedReactorControls();
    resized();
    repaint();
}

void VoidwormAudioProcessorEditor::selectReactorWorkspace (size_t reactorIndex)
{
    reactorWorkspace.selectReactor (reactorIndex);
    syncTransientSolo();
    bindSelectedReactorControls();
    resized();
    repaint();
}

void VoidwormAudioProcessorEditor::setGraphParameter (juce::RangedAudioParameter* parameter, float value)
{
    if (parameter != nullptr)
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void VoidwormAudioProcessorEditor::updateGraphDrag (juce::Point<float> point)
{
    if (activeEqNode == EqNode::none || ! reactorWorkspace.isReactorPage())
        return;

    const auto plot = makeLowerWorkspaceLayout (editorLayout.lowerWorkspace).reactorPlot;
    const auto xProportion = (point.x - plot.getX()) / plot.getWidth();
    const auto frequency = proportionToFrequency (xProportion);
    const auto ids = getSelectedEqParameterIds();
    if (activeEqNode == EqNode::hp)
        setGraphParameter (graphDragXParameter, juce::jlimit (20.0f, 5000.0f, frequency));
    else if (activeEqNode == EqNode::lp)
        setGraphParameter (graphDragXParameter, juce::jlimit (200.0f, 20000.0f, frequency));
    else if (activeEqNode == EqNode::focus1 || activeEqNode == EqNode::focus2)
    {
        setGraphParameter (graphDragXParameter, juce::jlimit (30.0f, 16000.0f, frequency));
        setGraphParameter (graphDragYParameter, voidworm::ui::yToGainDb (plot, point.y));
    }
    repaint();
}

void VoidwormAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    const juce::Point<float> point (event.position.x / sx, event.position.y / sy);
    const auto header = headerGeometry();

    if (isStandaloneMode() && editorLayout.header.contains (point)
        && ! header.preset.contains (point)
        && ! header.grid.contains (point) && ! header.search.contains (point)
        && ! header.settings.contains (point) && ! header.minimise.contains (point)
        && ! header.close.contains (point))
    {
        beginStandaloneWindowDrag (event);
        return;
    }

    const auto workspace = makeLowerWorkspaceLayout (editorLayout.lowerWorkspace);
    if (workspace.masterTab.contains (point))
    {
        selectMasterWorkspace();
        return;
    }
    if (workspace.reactorTab.contains (point))
    {
        selectReactorWorkspace (reactorWorkspace.selectedReactor);
        return;
    }

    if (reactorWorkspace.isReactorPage())
    {
        for (size_t index = 0; index < workspace.reactorTabs.size(); ++index)
            if (workspace.reactorTabs[index].contains (point))
            {
                selectReactorWorkspace (index);
                return;
            }

        if (workspace.solo.contains (point))
        {
            reactorWorkspace.toggleSolo();
            syncTransientSolo();
            repaint();
            return;
        }
        if (workspace.enabled.contains (point))
        {
            toggleParameter (getSelectedEnabledParameterId());
            return;
        }
    }

    const auto graphPlot = workspace.reactorPlot;
    if (reactorWorkspace.isReactorPage() && graphPlot.contains (point))
    {
        const auto ids = getSelectedEqParameterIds();
        const auto hostRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
        const auto responseSampleRate = hostRate * getOversamplingFactor (processor.parameters);
        const auto settings = voidworm::ReactorPreEq::sanitise (responseSampleRate, {
            processor.parameters.getRawParameterValue (ids.hp)->load(),
            processor.parameters.getRawParameterValue (ids.focus1)->load(),
            processor.parameters.getRawParameterValue (ids.gain1)->load(),
            processor.parameters.getRawParameterValue (ids.lp)->load(),
            processor.parameters.getRawParameterValue (ids.focus2)->load(),
            processor.parameters.getRawParameterValue (ids.gain2)->load()
        });
        const voidworm::ui::ReactorEqGraphModel graphModel (responseSampleRate, settings, graphPlot);
        const auto nodePoints = graphModel.nodePositions();

        constexpr auto hitRadius = 10.0f;
        const auto hitNode = voidworm::ui::hitTestEqNode (point, nodePoints, hitRadius);

        if (hitNode < 0)
            return;

        activeEqNode = hitNode == 0 ? EqNode::hp
                     : hitNode == 1 ? EqNode::focus1
                     : hitNode == 2 ? EqNode::focus2 : EqNode::lp;
        graphDragXParameter = processor.parameters.getParameter (
            activeEqNode == EqNode::hp ? ids.hp
                : activeEqNode == EqNode::focus1 ? ids.focus1
                : activeEqNode == EqNode::focus2 ? ids.focus2 : ids.lp);
        graphDragYParameter = activeEqNode == EqNode::focus1
            ? processor.parameters.getParameter (ids.gain1)
            : activeEqNode == EqNode::focus2 ? processor.parameters.getParameter (ids.gain2) : nullptr;
        if (graphDragXParameter != nullptr) graphDragXParameter->beginChangeGesture();
        if (graphDragYParameter != nullptr) graphDragYParameter->beginChangeGesture();
        return;
    }

    if (editorLayout.lowerWorkspace.contains (point))
        return;

    if (header.previousPreset.contains (point))
        applyPreset (presetIndex - 1);
    else if (header.nextPreset.contains (point))
        applyPreset (presetIndex + 1);
    else if (header.presetMenu.contains (point))
        overlay.showMode (voidworm::ui::HeaderPanel::presetBrowser);
    else if (header.gate.contains (point))
        toggleParameter ("gateEnabled");
    else if (header.grid.contains (point))
        overlay.showMode (voidworm::ui::HeaderPanel::presetMatrix);
    else if (header.search.contains (point))
        overlay.showMode (voidworm::ui::HeaderPanel::presetSearch);
    else if (header.settings.contains (point))
        overlay.showMode (voidworm::ui::HeaderPanel::settings);
    else if (editorLayout.oversampleHit.contains (point))
        cycleOversampling();
    else if (editorLayout.hqHit.contains (point))
        toggleParameter ("hqMode");
}

void VoidwormAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    if (draggingStandaloneWindow)
    {
        if (auto* topLevel = getTopLevelComponent())
            standaloneWindowDragger.dragComponent (topLevel, event.getEventRelativeTo (topLevel), nullptr);
        return;
    }
    if (activeEqNode == EqNode::none)
        return;
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    updateGraphDrag ({ event.position.x / sx, event.position.y / sy });
}

void VoidwormAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    draggingStandaloneWindow = false;
    if (graphDragXParameter != nullptr) graphDragXParameter->endChangeGesture();
    if (graphDragYParameter != nullptr) graphDragYParameter->endChangeGesture();
    graphDragXParameter = nullptr;
    graphDragYParameter = nullptr;
    activeEqNode = EqNode::none;
}

void VoidwormAudioProcessorEditor::timerCallback()
{
    if (standaloneToast.isVisible()
        && juce::Time::getMillisecondCounterHiRes() >= standaloneToast.expiresAt)
        standaloneToast.setVisible (false);
    updateMeterDisplays();
    animationPhase = voidworm::ui::advanceFooterAnimationPhase (animationPhase);
    const auto smoothReduction = [] (float measured, float displayed) noexcept
    {
        measured = std::isfinite (measured) ? juce::jmax (0.0f, measured) : 0.0f;
        return measured >= displayed ? measured : displayed * 0.86f + measured * 0.14f;
    };
    weldGainReductionDisplay = smoothReduction (processor.getWeldGainReductionDb(), weldGainReductionDisplay);
    limiterGainReductionDisplay = smoothReduction (processor.getLimiterGainReductionDb(), limiterGainReductionDisplay);
    const auto hqState = processor.parameters.getRawParameterValue ("hqMode")->load() >= 0.5f;
    hqButton.setActive (hqState);
    surgeSwitch.updateAnimation(); limiterSwitch.updateAnimation();
    const auto currentSound = processor.captureParameterSnapshot();
    const auto dirty = presetAssociation == PresetAssociation::custom
                    || ! voidworm::UserPresetStore::soundStatesEqual (currentSound, cleanPresetSound);
    if (dirty != presetIsDirty) { presetIsDirty = dirty; overlay.repaint(); }
    repaint();
}

void VoidwormAudioProcessorEditor::updateMeterDisplays() noexcept
{
    // Peak hold is a persistent UI preference. Raw measurements remain host-block
    // observations; all visual timing and release behavior live on the UI thread.
    const auto holdTicks = meterHoldTicks[static_cast<size_t> (juce::jlimit (0, 3, meterHoldIndex))];
    constexpr auto releasePerTick = 0.91201084f; // 24 dB/s at 30 Hz
    const auto update = [=] (float measured, float& displayed, int& hold) noexcept
    {
        measured = std::isfinite (measured) ? juce::jmax (0.0f, measured) : 0.0f;
        if (measured >= displayed)
        {
            displayed = measured;
            hold = holdTicks;
        }
        else if (hold > 0)
        {
            --hold;
        }
        else
        {
            displayed = juce::jmax (measured, displayed * releasePerTick);
            if (displayed < 1.0e-5f)
                displayed = 0.0f;
        }
    };

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto index = static_cast<size_t> (channel);
        update (processor.getInputVisualLevel (channel), inputMeterDisplay[index], inputMeterHold[index]);
        update (processor.getOutputVisualLevel (channel), outputMeterDisplay[index], outputMeterHold[index]);
    }
}

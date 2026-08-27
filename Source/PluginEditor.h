#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UserPresetStore.h"
#include "UI/HeaderPanelModel.h"
#include "UI/ReactorWorkspaceState.h"
#include "UI/VoidLookAndFeel.h"

class VoidwormAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit VoidwormAudioProcessorEditor (VoidwormAudioProcessor&);
    ~VoidwormAudioProcessorEditor() override;
    void configureStandaloneShell (std::function<void()> showAudioMidiSettings);
    void showStandaloneToast (const juce::String& message, int durationMs = 3200);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    class TouchSlider final : public juce::Slider
    {
    public:
        std::function<void (TouchSlider&, bool)> onTouch;
        void mouseDown (const juce::MouseEvent& event) override
        {
            if (onTouch) onTouch (*this, true);
            juce::Slider::mouseDown (event);
        }
        void mouseUp (const juce::MouseEvent& event) override
        {
            juce::Slider::mouseUp (event);
            if (onTouch) onTouch (*this, false);
        }
    };

    class HotspotButton final : public juce::Button
    {
    public:
        explicit HotspotButton (const juce::String& name) : juce::Button (name) {}
        std::function<void()> onContextClick;
        void paintButton (juce::Graphics&, bool, bool) override {}
        void mouseEnter (const juce::MouseEvent& event) override
        { juce::Button::mouseEnter (event); if (auto* parent = getParentComponent()) parent->repaint(); }
        void mouseExit (const juce::MouseEvent& event) override
        { juce::Button::mouseExit (event); if (auto* parent = getParentComponent()) parent->repaint(); }
        void mouseDown (const juce::MouseEvent& event) override
        {
            if (! event.mods.isPopupMenu())
                juce::Button::mouseDown (event);
            if (auto* parent = getParentComponent()) parent->repaint();
        }
        void mouseUp (const juce::MouseEvent& event) override
        {
            if (event.mods.isPopupMenu())
            {
                if (onContextClick) onContextClick();
            }
            else
            {
                juce::Button::mouseUp (event);
            }
            if (auto* parent = getParentComponent()) parent->repaint();
        }
    };

    class AnimatedToggleButton final : public juce::ToggleButton
    {
    public:
        AnimatedToggleButton (VoidwormAudioProcessorEditor&, const juce::String&);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
        bool updateAnimation (float step = 0.62f) noexcept;
        void snapToState() noexcept;
    private:
        VoidwormAudioProcessorEditor& owner;
        float visualPosition = 0.0f;
        bool initialised = false;
    };

    class CircularPushButton final : public juce::Button
    {
    public:
        CircularPushButton (VoidwormAudioProcessorEditor&, const juce::String&);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
        void setActive (bool) noexcept;
    private:
        VoidwormAudioProcessorEditor& owner;
        bool active = false;
    };

    class StandaloneToast final : public juce::Component
    {
    public:
        explicit StandaloneToast (VoidwormAudioProcessorEditor& editor) : owner (editor)
        { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics&) override;
        VoidwormAudioProcessorEditor& owner;
        juce::String message;
        double expiresAt = 0.0;
    };

    class OverlayPanel final : public juce::Component
                             , private juce::KeyListener
    {
    public:
        explicit OverlayPanel (VoidwormAudioProcessorEditor&);
        ~OverlayPanel() override;
        void showMode (voidworm::ui::HeaderPanel);
        void hide();
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        bool keyPressed (const juce::KeyPress&, juce::Component*) override;
        void refreshTheme();
        void refreshUserPresets();
    private:
        enum class Dialog { none, save, overwrite, rename, deleteConfirm };
        juce::Rectangle<float> panelBounds() const noexcept;
        juce::Rectangle<float> bodyBounds() const noexcept;
        juce::Rectangle<float> settingsControlBounds() const noexcept;
        juce::Rectangle<float> settingsThemeBounds() const noexcept;
        juce::Rectangle<float> gateThresholdBounds() const noexcept;
        void loadSearchSelection();
        void showDialog (Dialog, const juce::String& initialText = {});
        void closeDialog();
        void performDialogAction();
        void showMessage (const juce::String&);
        void setGateThresholdFromPoint (float pointX);
        void finishGateThresholdDrag();
        VoidwormAudioProcessorEditor& owner;
        juce::TextEditor searchEditor;
        HotspotButton clearSearchButton { "Clear search" };
        juce::TextEditor presetNameEditor;
        Dialog dialog = Dialog::none;
        juce::String statusMessage;
        double statusMessageUntil = 0.0;
        juce::Point<float> hoverPoint { -1000.0f, -1000.0f };
        juce::RangedAudioParameter* draggedGateThreshold = nullptr;
        bool settingsThemesExpanded = false;
    };

    void timerCallback() override;
    void updateMeterDisplays() noexcept;
    void configureSlider (TouchSlider&, const juce::String& name, juce::Label&);
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, bool inputMeter);
    void drawEqGraph (juce::Graphics&, juce::Rectangle<float>);
    void drawTransientValue (juce::Graphics&);
    void drawSteelTexture (juce::Graphics&, juce::Rectangle<float>, int density);
    void applyPreset (int index);
    void applyUserPreset (size_t index);
    void applySoundSnapshot (voidworm::Parameters, const juce::String&, bool factory,
                             const juce::File& userFile = {});
    void toggleParameter (const char* parameterID);
    void cycleOversampling();
    void setTheme (int index);
    void configureUiPreferences();
    void saveUiPreferences();
    void cycleUiScale();
    void cycleMeterFloor();
    void cycleMeterHold();
    void applyUiScale();
    bool isStandaloneMode() const noexcept;
    struct HeaderGeometry
    {
        juce::Rectangle<float> preset, previousPreset, presetMenu, nextPreset;
        juce::Rectangle<float> gate, gateThreshold, grid, search, settings, minimise, close;
    };
    HeaderGeometry headerGeometry() const noexcept;
    void beginStandaloneWindowDrag (const juce::MouseEvent&);
    void minimiseStandaloneWindow();
    void closeStandaloneWindow();
    int getThemeIndex() const;
    bool isNeonTheme() const;
    VoidLookAndFeel::ThemeStyle currentThemeStyle() const noexcept;
    juce::Colour lowAccent() const;
    juce::Colour primaryAccent() const;
    juce::Colour secondaryAccent() const;
    juce::Colour themeText() const;
    juce::Colour themeDimText() const;

    enum class EqNode { none, hp, focus1, focus2, lp };
    struct EqParameterIds
    {
        const char* hp;
        const char* focus1;
        const char* gain1;
        const char* focus2;
        const char* gain2;
        const char* lp;
    };
    struct CharacterBinding
    {
        const char* idA;
        const char* idB;
        const char* labelA;
        const char* labelB;
        const char* tooltipA;
        const char* tooltipB;
    };
    EqParameterIds getSelectedEqParameterIds() const noexcept;
    CharacterBinding getSelectedCharacterBinding() const noexcept;
    const char* getSelectedAmountParameterId() const noexcept;
    const char* getSelectedEnabledParameterId() const noexcept;
    void selectMasterWorkspace();
    void selectReactorWorkspace (size_t reactorIndex);
    void bindSelectedReactorControls();
    void syncTransientSolo() noexcept;
    void setGraphParameter (juce::RangedAudioParameter*, float);
    void updateGraphDrag (juce::Point<float> designPoint);

    VoidwormAudioProcessor& processor;
    VoidLookAndFeel lookAndFeel;
    TouchSlider breach, tear, overload, rot, drive, mix, low, mid, high, range, output, gateThreshold;
    TouchSlider weld, limiterThreshold, limiterCeiling;
    TouchSlider reactorAmount, reactorCharacterA, reactorCharacterB;
    AnimatedToggleButton surgeSwitch;
    AnimatedToggleButton limiterSwitch;
    HotspotButton previousPresetButton { "Previous preset" };
    HotspotButton nextPresetButton { "Next preset" };
    HotspotButton presetMenuButton { "Preset menu" };
    HotspotButton gateButton { "Input noise gate" };
    HotspotButton gridButton { "Preset library" };
    HotspotButton searchButton { "Preset search" };
    HotspotButton settingsButton { "Engine settings" };
    HotspotButton minimiseButton { "Minimise" };
    HotspotButton closeButton { "Close" };
    HotspotButton oversampleButton { "Oversampling" };
    CircularPushButton hqButton;
    OverlayPanel overlay;
    StandaloneToast standaloneToast { *this };
    voidworm::UserPresetStore userPresets;
    juce::ApplicationProperties applicationProperties;
    juce::Label breachLabel, tearLabel, overloadLabel, rotLabel, driveLabel, mixLabel, surgeLabel, gateThresholdLabel;
    juce::Label lowLabel, midLabel, highLabel, rangeLabel, outputLabel;
    juce::Label weldLabel, limiterThresholdLabel, limiterCeilingLabel, limiterSwitchLabel;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<Attachment> breachAttachment, tearAttachment, overloadAttachment, rotAttachment, driveAttachment,
                                mixAttachment, gateThresholdAttachment;
    std::unique_ptr<ButtonAttachment> surgeAttachment;
    std::unique_ptr<Attachment> lowAttachment, midAttachment, highAttachment;
    std::unique_ptr<Attachment> rangeAttachment, outputAttachment;
    std::unique_ptr<Attachment> weldAttachment, limiterThresholdAttachment, limiterCeilingAttachment;
    std::unique_ptr<ButtonAttachment> limiterSwitchAttachment;
    std::unique_ptr<Attachment> reactorAmountAttachment, reactorCharacterAAttachment,
                                reactorCharacterBAttachment;
    int presetIndex = 0;
    juce::String presetName { "Default Init" };
    enum class PresetAssociation { factory, user, custom };
    PresetAssociation presetAssociation = PresetAssociation::factory;
    juce::File currentUserPresetFile;
    voidworm::Parameters cleanPresetSound;
    bool presetIsDirty = false;
    TouchSlider* activeSlider = nullptr;
    float animationPhase = 0.0f;
    std::array<float, 2> inputMeterDisplay {};
    std::array<float, 2> outputMeterDisplay {};
    std::array<int, 2> inputMeterHold {};
    std::array<int, 2> outputMeterHold {};
    float weldGainReductionDisplay = 0.0f;
    float limiterGainReductionDisplay = 0.0f;
    voidworm::ui::HeaderPanelState headerPanelState;
    int uiScaleIndex = 1;
    int meterFloorIndex = 1;
    int meterHoldIndex = 2;
    std::function<void()> showStandaloneAudioMidiSettings;
    juce::ComponentDragger standaloneWindowDragger;
    bool draggingStandaloneWindow = false;
    voidworm::ui::ReactorWorkspaceState reactorWorkspace;
    EqNode activeEqNode = EqNode::none;
    juce::RangedAudioParameter* graphDragXParameter = nullptr;
    juce::RangedAudioParameter* graphDragYParameter = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidwormAudioProcessorEditor)
};

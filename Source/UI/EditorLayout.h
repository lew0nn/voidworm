#pragma once

#include <JuceHeader.h>

namespace voidworm::ui
{
struct EditorLayout
{
    juce::Rectangle<float> header { 7.0f, 8.0f, 694.0f, 44.0f };
    juce::Rectangle<float> mainControls { 7.0f, 59.0f, 694.0f, 287.0f };
    juce::Rectangle<float> primaryControls { 94.0f, 64.0f, 521.0f, 174.0f };
    juce::Rectangle<float> secondaryControls { 94.0f, 253.0f, 521.0f, 85.0f };
    juce::Rectangle<float> lowerWorkspace { 7.0f, 357.0f, 694.0f, 199.0f };
    juce::Rectangle<float> footer { 7.0f, 565.0f, 694.0f, 38.0f };
    juce::Rectangle<float> footerBase { 2.0f, 607.0f, 704.0f, 24.0f };

    // Label rectangles account for Bahnschrift's visible ascent. These tops are
    // deliberately lower than a purely mathematical component alignment.
    float primaryLabelY = 74.0f;
    float overloadLabelY = 68.0f;
    float overloadVisibleRingTop = 88.0f;
    float primaryKnobY = 85.0f;
    float secondaryLabelY = 259.0f;
    float secondaryControlY = 280.0f;
    float masterLabelY = 417.0f;
    float groupTitleY = 397.0f;
    float contextLabelY = 405.0f;
    float masterLabelHeight = 12.0f;
    float contextLabelHeight = 10.0f;
    float visibleLabelTopInset = 2.0f;
    float visibleLabelBottomInset = 5.0f;
    float switchHousingTopInset = 2.0f;

    juce::Rectangle<float> breach { 104.0f, 85.0f, 84.0f, 92.0f };
    juce::Rectangle<float> tear { 189.0f, 85.0f, 84.0f, 92.0f };
    juce::Rectangle<float> overload { 289.0f, 75.0f, 134.0f, 151.0f };
    juce::Rectangle<float> rot { 433.0f, 85.0f, 84.0f, 92.0f };
    juce::Rectangle<float> drive { 460.0f, 197.0f, 30.0f, 28.0f };
    juce::Rectangle<float> mix { 517.0f, 85.0f, 84.0f, 92.0f };

    // INPUT opens the bottom row and OUTPUT closes it: six knobs on an 87 px pitch.
    juce::Rectangle<float> input { 103.0f, 280.0f, 68.0f, 52.0f };
    std::array<juce::Rectangle<float>, 5> secondaryKnobs {{
        { 190.0f, 280.0f, 68.0f, 52.0f }, { 277.0f, 280.0f, 68.0f, 52.0f },
        { 364.0f, 280.0f, 68.0f, 52.0f }, { 451.0f, 280.0f, 68.0f, 52.0f },
        { 538.0f, 280.0f, 68.0f, 52.0f }
    }};

    // MASTER strip: group names on their own line, then one name per control,
    // the controls on one centre line, and each gain-reduction reading right
    // under what it measures. POST REACTOR is three even cells -- WELD, then
    // SURGE and SAW, each an icon to click and its amount; FINAL STAGE keeps
    // LIMIT, CEILING and the LIMITER close together.
    juce::Rectangle<float> masterControls { 12.0f, 393.0f, 684.0f, 96.0f };
    float postGroupWidth = 306.0f;
    juce::Rectangle<float> weld { 40.0f, 430.0f, 42.0f, 39.0f };
    juce::Rectangle<float> surgeButton { 114.0f, 431.0f, 38.0f, 38.0f };
    juce::Rectangle<float> surgeAmount { 157.0f, 430.0f, 42.0f, 39.0f };
    juce::Rectangle<float> sawButton { 219.0f, 431.0f, 38.0f, 38.0f };
    juce::Rectangle<float> sawAmount { 262.0f, 430.0f, 42.0f, 39.0f };
    float stageGlyphCentreY = 450.0f;
    std::array<float, 2> stageDividers { 104.0f, 209.0f };
    juce::Rectangle<float> limiterThreshold { 404.0f, 430.0f, 42.0f, 39.0f };
    juce::Rectangle<float> limiterCeiling { 474.0f, 430.0f, 42.0f, 39.0f };
    juce::Rectangle<float> limiter { 535.0f, 429.0f, 102.0f, 40.0f };
    float gainReductionY = 473.0f;
    juce::Rectangle<float> postReactorLabel { 16.0f, 405.0f, 74.0f, 10.0f };
    juce::Rectangle<float> finalLabel { 217.0f, 405.0f, 54.0f, 10.0f };

    juce::Rectangle<float> preset { 264.0f, 16.0f, 215.0f, 28.0f };
    juce::Rectangle<float> previousPreset { 264.0f, 16.0f, 30.0f, 28.0f };
    juce::Rectangle<float> presetMenu { 294.0f, 16.0f, 155.0f, 28.0f };
    juce::Rectangle<float> nextPreset { 449.0f, 16.0f, 30.0f, 28.0f };
    juce::Rectangle<float> gateButton { 18.0f, 84.0f, 27.0f, 32.0f };
    juce::Rectangle<float> gateThreshold { 52.0f, 91.0f, 25.0f, 23.0f };
    juce::Rectangle<float> gridButton { 526.0f, 13.0f, 40.0f, 34.0f };
    juce::Rectangle<float> searchButton { 570.0f, 13.0f, 40.0f, 34.0f };
    juce::Rectangle<float> settingsButton { 614.0f, 13.0f, 45.0f, 34.0f };
    juce::Rectangle<float> oversampleHit { 18.0f, 566.0f, 119.0f, 36.0f };
    juce::Rectangle<float> hqHit { 609.0f, 566.0f, 85.0f, 36.0f };
    juce::Rectangle<float> hqButton { 665.0f, 569.0f, 28.0f, 28.0f };
};

inline const EditorLayout& editorLayout() noexcept
{
    static const EditorLayout layout;
    return layout;
}
}

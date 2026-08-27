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
    float surgeLabelY = 266.0f;
    float secondaryControlY = 280.0f;
    float masterLabelY = 404.0f;
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
    juce::Rectangle<float> surge { 101.0f, 279.0f, 102.0f, 40.0f };
    std::array<juce::Rectangle<float>, 5> secondaryKnobs {{
        { 202.0f, 280.0f, 68.0f, 52.0f }, { 285.0f, 280.0f, 68.0f, 52.0f },
        { 368.0f, 280.0f, 68.0f, 52.0f }, { 451.0f, 280.0f, 68.0f, 52.0f },
        { 534.0f, 280.0f, 68.0f, 52.0f }
    }};

    juce::Rectangle<float> masterControls { 12.0f, 393.0f, 684.0f, 66.0f };
    juce::Rectangle<float> weld { 94.0f, 417.0f, 42.0f, 39.0f };
    juce::Rectangle<float> limiterThreshold { 282.0f, 417.0f, 42.0f, 39.0f };
    juce::Rectangle<float> limiterCeiling { 382.0f, 417.0f, 42.0f, 39.0f };
    juce::Rectangle<float> limiter { 433.0f, 415.0f, 102.0f, 40.0f };
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

#include "ParameterLayout.h"
#include "DSP/ReactorPreEq.h"

juce::AudioProcessorValueTreeState::ParameterLayout createVoidwormParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto percent = [] (float value, int) { return juce::String (juce::roundToInt (value * 100.0f)) + "%"; };
    auto parsePercent = [] (const juce::String& text) { return text.getFloatValue() * 0.01f; };
    auto frequencyText = [] (float value, int)
    {
        return value >= 1000.0f ? juce::String (value / 1000.0f, value < 10000.0f ? 2 : 1) + " kHz"
                                : juce::String (juce::roundToInt (value)) + " Hz";
    };
    auto parseFrequency = [] (const juce::String& text)
    {
        const auto value = text.getFloatValue();
        return text.containsIgnoreCase ("k") ? value * 1000.0f : value;
    };
    auto makeFrequencyRange = [] (float minimum, float maximum, float centre)
    {
        juce::NormalisableRange<float> range { minimum, maximum, 0.1f };
        range.setSkewForCentre (centre);
        return range;
    };
    const auto frequencyAttributes = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction (frequencyText).withValueFromStringFunction (parseFrequency);

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "breach", 1 }, "BREACH",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.35f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tear", 1 }, "TEAR",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.04f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "rot", 1 }, "ROT",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.42f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "drive", 1 }, "DRIVE",
        juce::NormalisableRange<float> { 0.0f, 18.0f, 0.01f, 0.58f }, 4.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "overload", 1 }, "OVERLOAD",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.35f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "MIX",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.72f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "range", 1 }, "RANGE",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.92f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "low", 1 }, "LOW",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mid", 1 }, "MID",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "high", 1 }, "HIGH",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "OUTPUT",
        juce::NormalisableRange<float> { -24.0f, 6.0f, 0.01f }, -4.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "surge", 1 }, "SURGE", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "oversample", 1 }, "OVERSAMPLING",
                                                              juce::StringArray { "1X", "2X", "4X", "8X" }, 2));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "hqMode", 1 }, "HQ MODE", true));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "theme", 1 }, "UI THEME",
                                                              juce::StringArray { "UV STEEL", "HELLFORGE STEEL", "KRYPT CYAN", "XENO ACID",
                                                                  "VOID AMBER", "TOXIC BRASS", "PLASMA BLUE", "NUCLEAR LIME",
                                                                  "BLOOD COPPER", "ASHEN GOLD", "GLACIER TEAL",
                                                                  "RUPTURE RED" }, 0));
    const auto amountAttributes = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction (percent).withValueFromStringFunction (parsePercent);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "massAmount", 1 }, "MASS",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 1.0f, amountAttributes));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "furnaceAmount", 1 }, "FURNACE",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 1.0f, amountAttributes));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "arcAmount", 1 }, "ARC",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 1.0f, amountAttributes));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedbackAmount", 1 }, "FEEDBACK",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 1.0f, amountAttributes));
    const auto addCharacter = [&layout, &amountAttributes] (const char* id, const char* name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.5f, amountAttributes));
    };
    addCharacter ("massSaturation", "MASS SATURATION");
    addCharacter ("massHarmonics", "MASS HARMONICS");
    addCharacter ("furnaceStarve", "FURNACE STARVE");
    addCharacter ("furnaceFold", "FURNACE FOLD");
    addCharacter ("arcXmod", "ARC XMOD");
    addCharacter ("arcFold", "ARC FOLD");
    addCharacter ("feedbackReturn", "FEEDBACK RETURN");
    addCharacter ("feedbackDamp", "FEEDBACK DAMP");
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "massEnabled", 1 }, "MASS ENABLED", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "furnaceEnabled", 1 }, "FURNACE ENABLED", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "arcEnabled", 1 }, "ARC ENABLED", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "feedbackEnabled", 1 }, "FEEDBACK ENABLED", true));
    const auto addReactorEq = [&layout, &makeFrequencyRange, &frequencyAttributes]
        (const char* prefix, const char* displayName, voidworm::ReactorEqSettings defaults)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { juce::String (prefix) + "Hp", 1 }, juce::String (displayName) + " HP",
            makeFrequencyRange (20.0f, 5000.0f, 300.0f), defaults.hp, frequencyAttributes));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { juce::String (prefix) + "FocusFreq", 1 }, juce::String (displayName) + " FOCUS",
            makeFrequencyRange (30.0f, 16000.0f, 1000.0f), defaults.focusFrequency, frequencyAttributes));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { juce::String (prefix) + "FocusGain", 1 }, juce::String (displayName) + " FOCUS GAIN",
            juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, defaults.focusGainDb,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { juce::String (prefix) + "Lp", 1 }, juce::String (displayName) + " LP",
            makeFrequencyRange (200.0f, 20000.0f, 4000.0f), defaults.lp, frequencyAttributes));
    };
    addReactorEq ("mass", "MASS", voidworm::massEqDefaults());
    addReactorEq ("furnace", "FURNACE", voidworm::furnaceEqDefaults());
    addReactorEq ("arc", "ARC", voidworm::arcEqDefaults());
    addReactorEq ("feedback", "FEEDBACK", voidworm::feedbackEqDefaults());
    const auto addReactorFocus2 = [&layout, &makeFrequencyRange, &frequencyAttributes]
        (const char* prefix, const char* displayName, voidworm::ReactorEqSettings defaults)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { juce::String (prefix) + "Focus2Freq", 1 }, juce::String (displayName) + " FOCUS 2",
            makeFrequencyRange (30.0f, 16000.0f, 1000.0f), defaults.focus2Frequency, frequencyAttributes));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { juce::String (prefix) + "Focus2Gain", 1 }, juce::String (displayName) + " FOCUS 2 GAIN",
            juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, defaults.focus2GainDb,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));
    };
    addReactorFocus2 ("mass", "MASS", voidworm::massEqDefaults());
    addReactorFocus2 ("furnace", "FURNACE", voidworm::furnaceEqDefaults());
    addReactorFocus2 ("arc", "ARC", voidworm::arcEqDefaults());
    addReactorFocus2 ("feedback", "FEEDBACK", voidworm::feedbackEqDefaults());
    // These parameters are deliberately appended so all existing parameter
    // indices and IDs remain stable for sessions and automation.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "weld", 1 }, "WELD",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f }, 0.30f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (percent)
            .withValueFromStringFunction (parsePercent)));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "limiterEnabled", 1 }, "LIMITER ON", true));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "limiterThreshold", 1 }, "LIMIT",
        juce::NormalisableRange<float> { -18.0f, 0.0f, 0.01f }, -3.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "limiterCeiling", 1 }, "CEILING",
        juce::NormalisableRange<float> { -6.0f, 0.0f, 0.01f }, -0.8f,
        juce::AudioParameterFloatAttributes().withLabel ("dBFS")));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "gateEnabled", 1 }, "INPUT GATE", true));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gateThreshold", 1 }, "GATE THRESHOLD",
        juce::NormalisableRange<float> { -80.0f, -20.0f, 0.1f }, -50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return layout;
}

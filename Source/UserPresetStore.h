#pragma once

#include "DSP/VoidEngine.h"

namespace voidworm
{
struct UserPreset
{
    juce::String name;
    juce::File file;
    Parameters parameters;
};

struct UserPresetResult
{
    bool ok = false;
    juce::String message;
    std::optional<UserPreset> preset;
};

class UserPresetStore
{
public:
    static constexpr int formatVersion = 1;
    static constexpr int maximumNameLength = 64;

    explicit UserPresetStore (juce::File directory = defaultDirectory());

    static juce::File defaultDirectory();
    static juce::String validateName (const juce::String&);
    static bool soundStatesEqual (const Parameters&, const Parameters&, float tolerance = 1.0e-5f) noexcept;

    const juce::File& directory() const noexcept { return presetDirectory; }
    const std::vector<UserPreset>& presets() const noexcept { return cachedPresets; }
    void refresh();
    std::optional<size_t> findByName (const juce::String&) const;
    std::optional<size_t> findByFile (const juce::File&) const;

    UserPresetResult save (const juce::String& name, const Parameters&, bool overwrite);
    UserPresetResult load (const juce::File&) const;
    UserPresetResult rename (const juce::File&, const juce::String& newName);
    UserPresetResult remove (const juce::File&);

private:
    static juce::ValueTree makeTree (const juce::String&, const Parameters&);
    static bool readTree (const juce::ValueTree&, UserPreset&, juce::String& error);
    juce::File fileForName (const juce::String&) const;
    UserPresetResult write (const juce::File&, const juce::String&, const Parameters&);

    juce::File presetDirectory;
    std::vector<UserPreset> cachedPresets;
};
}

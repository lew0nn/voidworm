#pragma once

#include "../PresetDefinitions.h"
#include "../UserPresetStore.h"

#include <algorithm>
#include <numeric>
#include <optional>
#include <vector>

namespace voidworm::ui
{
enum class HeaderPanel { none, presetBrowser, presetMatrix, presetSearch, gate, settings };

enum class PresetSource { factory, user };

struct PresetListItem
{
    PresetSource source = PresetSource::factory;
    size_t index = 0;
    juce::String name;
    juce::String family;
};

inline std::vector<PresetListItem> combinedPresetItems (const std::vector<UserPreset>& users)
{
    std::vector<PresetListItem> result;
    result.reserve (factoryPresets().size() + users.size());
    for (size_t index = 0; index < factoryPresets().size(); ++index)
        result.push_back ({ PresetSource::factory, index, factoryPresets()[index].name,
                            factoryPresets()[index].family });
    for (size_t index = 0; index < users.size(); ++index)
        result.push_back ({ PresetSource::user, index, users[index].name, "USER" });
    return result;
}

inline std::vector<PresetListItem> filterPresetItems (const std::vector<UserPreset>& users,
                                                       const juce::String& family)
{
    auto items = combinedPresetItems (users);
    if (family.isEmpty() || family.equalsIgnoreCase ("ALL")) return items;
    items.erase (std::remove_if (items.begin(), items.end(), [&] (const PresetListItem& item)
        { return ! item.family.equalsIgnoreCase (family); }), items.end());
    return items;
}

inline std::vector<PresetListItem> searchPresetItems (const std::vector<UserPreset>& users,
                                                       const juce::String& rawQuery)
{
    auto items = combinedPresetItems (users);
    const auto query = rawQuery.trim();
    if (query.isEmpty()) return items;
    items.erase (std::remove_if (items.begin(), items.end(), [&] (const PresetListItem& item)
        { return ! item.name.containsIgnoreCase (query) && ! item.family.containsIgnoreCase (query); }), items.end());
    return items;
}

inline std::vector<size_t> allPresetIndices()
{
    std::vector<size_t> result (factoryPresets().size());
    std::iota (result.begin(), result.end(), size_t { 0 });
    return result;
}

inline std::vector<juce::String> presetFamilies()
{
    std::vector<juce::String> result;
    for (const auto& preset : factoryPresets())
    {
        const juce::String family (preset.family);
        if (std::find (result.begin(), result.end(), family) == result.end())
            result.push_back (family);
    }
    return result;
}

inline std::vector<size_t> presetsInFamily (const juce::String& family)
{
    if (family.isEmpty() || family.equalsIgnoreCase ("ALL"))
        return allPresetIndices();
    std::vector<size_t> result;
    const auto& presets = factoryPresets();
    for (size_t index = 0; index < presets.size(); ++index)
        if (family.equalsIgnoreCase (presets[index].family))
            result.push_back (index);
    return result;
}

inline std::vector<size_t> searchPresets (const juce::String& rawQuery)
{
    const auto query = rawQuery.trim().toLowerCase();
    if (query.isEmpty())
        return allPresetIndices();
    std::vector<size_t> result;
    const auto& presets = factoryPresets();
    for (size_t index = 0; index < presets.size(); ++index)
    {
        const auto name = juce::String (presets[index].name).toLowerCase();
        const auto family = juce::String (presets[index].family).toLowerCase();
        if (name.contains (query) || family.contains (query))
            result.push_back (index);
    }
    return result;
}

class HeaderPanelState
{
public:
    void open (HeaderPanel requested) noexcept
    {
        panel = panel == requested ? HeaderPanel::none : requested;
        if (panel == HeaderPanel::presetSearch)
            highlightedSearchResult = 0;
    }

    void close() noexcept { panel = HeaderPanel::none; }
    bool isOpen() const noexcept { return panel != HeaderPanel::none; }

    void setMatrixFamily (juce::String family)
    {
        matrixFamily = family.isEmpty() ? "ALL" : family.toUpperCase();
    }

    void setSearchQuery (juce::String query)
    {
        searchQuery = std::move (query);
        highlightedSearchResult = 0;
    }

    std::vector<size_t> matrixResults() const { return presetsInFamily (matrixFamily); }
    std::vector<size_t> searchResults() const { return searchPresets (searchQuery); }

    void moveSearchSelection (int delta)
    {
        const auto results = searchResults();
        if (results.empty())
        {
            highlightedSearchResult = 0;
            return;
        }
        const auto count = static_cast<int> (results.size());
        highlightedSearchResult = (highlightedSearchResult + delta) % count;
        if (highlightedSearchResult < 0)
            highlightedSearchResult += count;
    }

    std::optional<size_t> highlightedPreset() const
    {
        const auto results = searchResults();
        if (results.empty())
            return std::nullopt;
        return results[static_cast<size_t> (juce::jlimit (0, static_cast<int> (results.size()) - 1,
                                                          highlightedSearchResult))];
    }

    HeaderPanel panel = HeaderPanel::none;
    juce::String matrixFamily { "ALL" };
    juce::String searchQuery;
    int highlightedSearchResult = 0;
    int browserScrollRows = 0;
    int matrixScrollRows = 0;
    int searchScrollRows = 0;
    int themeScrollRows = 0;
};
}

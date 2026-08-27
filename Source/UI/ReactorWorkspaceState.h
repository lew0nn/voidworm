#pragma once

#include <array>
#include <cstddef>

namespace voidworm::ui
{
struct ReactorWorkspaceState
{
    enum class Page { master, reactor };

    void selectMaster() noexcept
    {
        page = Page::master;
        solo = false;
    }

    void selectReactor (size_t index) noexcept
    {
        page = Page::reactor;
        selectedReactor = index < 4 ? index : 3;
    }

    void toggleSolo() noexcept
    {
        if (page == Page::reactor)
            solo = ! solo;
    }

    bool isReactorPage() const noexcept { return page == Page::reactor; }
    int soloTarget() const noexcept
    {
        return isReactorPage() && solo ? static_cast<int> (selectedReactor) + 1 : 0;
    }

    Page page = Page::master;
    size_t selectedReactor = 0;
    bool solo = false;
};

inline constexpr std::array<std::array<float, 4>, 5> reactorPresetAmounts {{
    {{ 1.00f, 1.00f, 1.00f, 1.00f }},
    {{ 0.88f, 1.00f, 0.76f, 0.60f }},
    {{ 0.70f, 0.84f, 1.00f, 0.34f }},
    {{ 1.00f, 0.66f, 0.50f, 0.74f }},
    {{ 0.64f, 0.46f, 0.30f, 0.20f }}
}};

inline constexpr std::array<float, 8> neutralReactorCharacterPreset {
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f
};
}

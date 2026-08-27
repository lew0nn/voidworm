#pragma once

#include <JuceHeader.h>

namespace voidworm::ui
{
struct ThemePalette
{
    const char* name;
    const char* descriptor;
    juce::uint32 accentLow;
    juce::uint32 accentMid;
    juce::uint32 accentHot;
    juce::uint32 text;
    juce::uint32 dimText;
};

inline constexpr std::array<ThemePalette, 12> themePalettes {{
    { "UV STEEL",          "DEEP VIOLET / ULTRAVIOLET",   0xff2b0052, 0xff7200d9, 0xffc06aff, 0xffe8e0ee, 0xffa193ab },
    { "HELLFORGE STEEL",   "CRIMSON / HOT EMBER",         0xff7d160f, 0xffe43b20, 0xffffb04d, 0xffded5ce, 0xffa89186 },
    { "KRYPT CYAN",        "DEEP CYAN / COLD COBALT",     0xff006070, 0xff00c6e5, 0xff8eeaff, 0xffdcebed, 0xff82aab0 },
    { "XENO ACID",         "ACID / HOT CHARTREUSE",       0xff3d6511, 0xff94d925, 0xffe8ff78, 0xffe4e9d8, 0xffa2ad86 },
    { "VOID AMBER",        "BURNT BROWN / WHITE-HOT AMBER", 0xff64300f, 0xffd47a18, 0xffffd37c, 0xffe5dccd, 0xffaa9475 },
    { "TOXIC BRASS",       "OLIVE / OXIDIZED BRASS",      0xff53501a, 0xffae9832, 0xffddff69, 0xffe2e1d2, 0xffa5a27d },
    { "PLASMA BLUE",       "ROYAL BLUE / ICY ELECTRIC",   0xff183a8d, 0xff2c7cff, 0xff9bdfff, 0xffdce7f1, 0xff829bb5 },
    { "NUCLEAR LIME",      "EMERALD / FLUORESCENT LIME",  0xff0d5837, 0xff39d25e, 0xffcdff94, 0xffe1eadc, 0xff91aa8f },
    { "BLOOD COPPER",      "OXBLOOD / HOT COPPER",        0xff671b20, 0xffbf5a31, 0xffffba6c, 0xffe2d7d0, 0xffa5877c },
    { "ASHEN GOLD",        "ANTIQUE BRONZE / PALE GOLD",  0xff59441d, 0xffc29940, 0xfff5da99, 0xffe5dfd2, 0xffa99b7d },
    { "GLACIER TEAL",      "DEEP TEAL / ICY MINT",        0xff07595b, 0xff20b4ab, 0xffa6ffe2, 0xffd9eae7, 0xff82a9a4 },
    { "RUPTURE RED",       "ELECTRIC CRIMSON / HOT SIGNAL", 0xff52001a, 0xffff0051, 0xffff7a9d, 0xfff0dfe5, 0xffb2909b }
}};

inline constexpr int themeCount = static_cast<int> (themePalettes.size());

inline const ThemePalette& themePalette (int index) noexcept
{
    return themePalettes[static_cast<size_t> (juce::jlimit (0, themeCount - 1, index))];
}
}

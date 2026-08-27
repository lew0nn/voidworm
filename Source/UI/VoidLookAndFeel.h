#pragma once

#include <JuceHeader.h>

class VoidLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    enum class ThemeStyle
    {
        uvSteel, hellforgeSteel, kryptCyan, xenoAcid,
        voidAmber, toxicBrass, plasmaBlue, nuclearLime,
        bloodCopper, ashenGold, glacierTeal, ruptureRed
    };

    VoidLookAndFeel();
    void setMetalTexture (juce::Image);
    void setSteelPanelTexture (juce::Image);
    void setNeonPanelTexture (juce::Image);
    void setThemeStyle (ThemeStyle);
    void drawMetalTexture (juce::Graphics&, juce::Rectangle<float>, float opacity,
                           int anchorX = 0, int anchorY = 0) const;
    void drawPanelTexture (juce::Graphics&, juce::Rectangle<float>, float opacity) const;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

private:
    juce::Image createGradientArc (int diameter, float strokeWidth,
                                   float startAngle, float endAngle,
                                   juce::Colour low, juce::Colour mid,
                                   juce::Colour hot, bool glow) const;
    const juce::Image& gradientArc (int diameter, float strokeWidth,
                                    float startAngle, float endAngle,
                                    bool glow);
    void clearGradientCache();
    juce::Image metalTexture;
    juce::Image steelPanelTexture;
    juce::Image neonPanelTexture;
    ThemeStyle themeStyle = ThemeStyle::uvSteel;
    struct ArcCacheEntry
    {
        int diameter = 0;
        float strokeWidth = 0.0f;
        float startAngle = 0.0f;
        float endAngle = 0.0f;
        ThemeStyle style = ThemeStyle::uvSteel;
        bool glow = false;
        juce::Image image;
    };
    std::vector<ArcCacheEntry> arcCache;
};

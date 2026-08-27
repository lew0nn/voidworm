#include "VoidLookAndFeel.h"
#include "ThemePalette.h"

namespace
{
constexpr auto ember = 0xffff2b16;
constexpr auto orange = 0xffff6a20;
constexpr auto warmWhite = 0xffd9d1c7;

juce::Colour primaryFor (VoidLookAndFeel::ThemeStyle style) noexcept
{
    return juce::Colour (voidworm::ui::themePalette (static_cast<int> (style)).accentMid);
}

juce::Colour highlightFor (VoidLookAndFeel::ThemeStyle style) noexcept
{
    return juce::Colour (voidworm::ui::themePalette (static_cast<int> (style)).accentHot);
}

juce::Colour lowFor (VoidLookAndFeel::ThemeStyle style) noexcept
{
    return juce::Colour (voidworm::ui::themePalette (static_cast<int> (style)).accentLow);
}
}

VoidLookAndFeel::VoidLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (ember));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff080808));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff5c1d16));
    setColour (juce::Label::textColourId, juce::Colour (warmWhite));
}

void VoidLookAndFeel::setMetalTexture (juce::Image texture)
{
    metalTexture = std::move (texture);
}

void VoidLookAndFeel::setSteelPanelTexture (juce::Image texture)
{
    steelPanelTexture = std::move (texture);
}

void VoidLookAndFeel::setNeonPanelTexture (juce::Image texture)
{
    neonPanelTexture = std::move (texture);
}

void VoidLookAndFeel::setThemeStyle (ThemeStyle style)
{
    if (themeStyle != style)
        clearGradientCache();
    themeStyle = style;
    setColour (juce::Slider::textBoxTextColourId, highlightFor (themeStyle));
    setColour (juce::Slider::textBoxOutlineColourId, primaryFor (themeStyle).darker (0.58f));
}

void VoidLookAndFeel::clearGradientCache()
{
    arcCache.clear();
}

juce::Image VoidLookAndFeel::createGradientArc (int diameter, float strokeWidth,
                                                 float startAngle, float endAngle,
                                                 juce::Colour low, juce::Colour mid,
                                                 juce::Colour hot, bool strongGlow) const
{
    constexpr int supersampling = 4;
    const auto largeDiameter = juce::jmax (4, diameter * supersampling);
    juce::Image large (juce::Image::ARGB, largeDiameter, largeDiameter, true);
    juce::Image::BitmapData pixels (large, juce::Image::BitmapData::writeOnly);
    const auto centre = static_cast<float> (largeDiameter) * 0.5f;
    const auto mainWidth = strokeWidth * supersampling;
    const auto glowWidth = mainWidth * (strongGlow ? 3.2f : 2.7f);
    const auto radius = centre - glowWidth * 0.56f - 1.0f;
    const auto range = juce::jmax (0.001f, endAngle - startAngle);

    for (int y = 0; y < largeDiameter; ++y)
    {
        for (int x = 0; x < largeDiameter; ++x)
        {
            const auto dx = static_cast<float> (x) + 0.5f - centre;
            const auto dy = static_cast<float> (y) + 0.5f - centre;
            auto angle = std::atan2 (dx, -dy);
            while (angle < startAngle) angle += juce::MathConstants<float>::twoPi;
            if (angle > endAngle)
                continue;

            const auto radialDistance = std::abs (std::sqrt (dx * dx + dy * dy) - radius);
            const auto mainCoverage = juce::jlimit (0.0f, 1.0f,
                mainWidth * 0.5f + 0.75f - radialDistance);
            const auto glowPosition = radialDistance / juce::jmax (1.0f, glowWidth * 0.5f);
            const auto glowCoverage = glowPosition < 1.0f
                ? std::pow (1.0f - glowPosition, 2.15f) * (strongGlow ? 0.24f : 0.15f) : 0.0f;
            const auto alpha = juce::jlimit (0.0f, 1.0f, mainCoverage + glowCoverage);
            if (alpha <= 0.0f)
                continue;

            const auto t = juce::jlimit (0.0f, 1.0f, (angle - startAngle) / range);
            const auto colour = t < 0.5f ? low.interpolatedWith (mid, t * 2.0f)
                                         : mid.interpolatedWith (hot, (t - 0.5f) * 2.0f);
            pixels.setPixelColour (x, y, colour.withMultipliedAlpha (alpha));
        }
    }

    return large.rescaled (diameter, diameter, juce::Graphics::highResamplingQuality);
}

const juce::Image& VoidLookAndFeel::gradientArc (int diameter, float strokeWidth,
                                                 float startAngle, float endAngle,
                                                 bool strongGlow)
{
    for (auto& entry : arcCache)
        if (entry.diameter == diameter && std::abs (entry.strokeWidth - strokeWidth) < 0.01f
            && std::abs (entry.startAngle - startAngle) < 0.0001f
            && std::abs (entry.endAngle - endAngle) < 0.0001f
            && entry.style == themeStyle && entry.glow == strongGlow)
            return entry.image;

    ArcCacheEntry entry;
    entry.diameter = diameter;
    entry.strokeWidth = strokeWidth;
    entry.startAngle = startAngle;
    entry.endAngle = endAngle;
    entry.style = themeStyle;
    entry.glow = strongGlow;
    entry.image = createGradientArc (diameter, strokeWidth, startAngle, endAngle,
                                     lowFor (themeStyle), primaryFor (themeStyle),
                                     highlightFor (themeStyle), strongGlow);
    arcCache.push_back (std::move (entry));
    return arcCache.back().image;
}

void VoidLookAndFeel::drawMetalTexture (juce::Graphics& g, juce::Rectangle<float> area, float opacity,
                                        int anchorX, int anchorY) const
{
    if (! metalTexture.isValid())
        return;

    g.saveState();
    g.reduceClipRegion (area.toNearestInt());
    g.setTiledImageFill (metalTexture, anchorX, anchorY, juce::jlimit (0.0f, 1.0f, opacity));
    g.fillRect (area);
    g.restoreState();
}

void VoidLookAndFeel::drawPanelTexture (juce::Graphics& g, juce::Rectangle<float> area, float opacity) const
{
    const auto& panelTexture = themeStyle == ThemeStyle::uvSteel ? neonPanelTexture : steelPanelTexture;
    if (! panelTexture.isValid())
        return;

    g.saveState();
    g.reduceClipRegion (area.toNearestInt());
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.setOpacity (juce::jlimit (0.0f, 1.0f, opacity));
    g.drawImage (panelTexture, juce::Rectangle<float> (0.0f, 0.0f, 708.0f, 499.0f),
                 juce::RectanglePlacement (juce::RectanglePlacement::centred
                                           | juce::RectanglePlacement::fillDestination));
    if (area.getBottom() > 499.0f)
        g.drawImage (panelTexture, juce::Rectangle<float> (0.0f, 499.0f, 708.0f, 499.0f),
                     juce::RectanglePlacement (juce::RectanglePlacement::centred
                                               | juce::RectanglePlacement::fillDestination));
    g.restoreState();
}

void VoidLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float start, float end, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                 static_cast<float> (width), static_cast<float> (height));
    const auto isOverload = slider.getName() == "OVERLOAD";
    const auto isMini = slider.getName() == "DRIVE" || slider.getName() == "GATE THRESHOLD";
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * (isOverload ? 0.388f : isMini ? 0.30f : 0.345f);
    const auto centre = bounds.getCentre();
    const auto angle = start + sliderPos * (end - start);
    const auto accent = primaryFor (themeStyle);
    const auto low = lowFor (themeStyle);
    const auto hot = highlightFor (themeStyle);
    const auto face = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

    // Deep contact shadow makes the control sit above the faceplate instead of
    // reading as a flat circle printed on it.
    g.setColour (juce::Colours::black.withAlpha (0.24f));
    g.fillEllipse (face.expanded (isOverload ? 13.0f : isMini ? 4.5f : 10.0f).translated (0.0f, isMini ? 2.0f : 5.0f));
    g.setColour (juce::Colours::black.withAlpha (0.52f));
    g.fillEllipse (face.expanded (isOverload ? 9.0f : isMini ? 3.0f : 7.0f).translated (0.0f, isMini ? 1.3f : 3.0f));
    g.setColour (juce::Colours::black.withAlpha (0.88f));
    g.fillEllipse (face.expanded (isMini ? 1.7f : 4.0f).translated (0.0f, isMini ? 0.8f : 2.0f));

    const auto railOffset = isOverload ? 10.0f : isMini ? 3.5f : 7.0f;
    juce::Path rail;
    rail.addCentredArc (centre.x, centre.y, radius + railOffset,
                        radius + railOffset, 0.0f, start, end, true);
    g.setColour (low.withAlpha (0.78f));
    g.strokePath (rail, juce::PathStrokeType (isOverload ? 4.0f : isMini ? 1.25f : 2.2f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (angle > start + 0.0001f)
    {
        const auto mainStroke = isOverload ? 3.0f : isMini ? 0.95f : 1.55f;
        const auto revealStroke = isOverload ? 12.0f : isMini ? 4.0f : 7.0f;
        const auto arcRadius = radius + railOffset;
        const auto glowFactor = isOverload ? 3.2f : 2.7f;
        const auto imageDiameter = juce::roundToInt (2.0f * (arcRadius + mainStroke * glowFactor * 0.56f + 0.25f));
        const auto imageBounds = juce::Rectangle<float> (static_cast<float> (imageDiameter),
                                                          static_cast<float> (imageDiameter)).withCentre (centre);
        juce::Path activeArc, reveal;
        activeArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, start, angle, true);
        juce::PathStrokeType (revealStroke, juce::PathStrokeType::curved,
                             juce::PathStrokeType::rounded).createStrokedPath (reveal, activeArc);
        g.saveState();
        g.reduceClipRegion (reveal);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (gradientArc (imageDiameter, mainStroke, start, end, isOverload), imageBounds);
        g.restoreState();
    }

    // Matte multi-stage bezel with restrained diffuse shading and no specular shine.
    juce::ColourGradient rim (juce::Colour (0xff4b4541), face.getX() + face.getWidth() * 0.16f,
                              face.getY() + face.getHeight() * 0.10f, juce::Colour (0xff050505),
                              face.getRight(), face.getBottom(), false);
    rim.addColour (0.16, juce::Colour (0xff403b38));
    rim.addColour (0.38, juce::Colour (0xff24211f));
    rim.addColour (0.72, juce::Colour (0xff0c0b0b));
    g.setGradientFill (rim);
    g.fillEllipse (face);
    g.setColour (juce::Colour (0xff5e5752).withAlpha (0.42f));
    g.drawEllipse (face, isOverload ? 1.8f : isMini ? 0.7f : 1.15f);
    g.setColour (juce::Colours::black.withAlpha (0.86f));
    g.drawEllipse (face.reduced (isMini ? 1.2f : 2.4f), isOverload ? 2.8f : isMini ? 1.0f : 1.8f);
    g.setColour (juce::Colour (0xff594f49).withAlpha (0.74f));
    g.drawEllipse (face.reduced (isOverload ? 5.0f : isMini ? 2.1f : 3.8f), isOverload ? 1.3f : isMini ? 0.55f : 0.9f);

    const auto capArea = face.reduced (isOverload ? 7.0f : isMini ? 2.6f : 5.0f);
    const auto lightOrigin = juce::Point<float> (capArea.getX() + capArea.getWidth() * 0.32f,
                                                  capArea.getY() + capArea.getHeight() * 0.27f);
    const auto shadeEdge = juce::Point<float> (capArea.getRight() - capArea.getWidth() * 0.04f,
                                                capArea.getBottom() - capArea.getHeight() * 0.02f);
    juce::ColourGradient cap (juce::Colour (0xff302d2b), lightOrigin.x, lightOrigin.y,
                              juce::Colour (0xff050505), shadeEdge.x, shadeEdge.y, true);
    cap.addColour (0.20, juce::Colour (0xff2c2927));
    cap.addColour (0.46, juce::Colour (0xff211f1d));
    cap.addColour (0.72, juce::Colour (0xff141210));
    cap.addColour (0.90, juce::Colour (0xff090808));
    g.setGradientFill (cap);
    g.fillEllipse (capArea);

    // Keep the cast-metal grain, but below the curved lighting so it follows the
    // spherical cap rather than flattening it.
    juce::Path capClip;
    capClip.addEllipse (capArea);
    g.saveState();
    g.reduceClipRegion (capClip);
    const auto hash = static_cast<juce::uint64> (slider.getName().hashCode64());
    drawMetalTexture (g, capArea, isOverload ? 0.24f : isMini ? 0.17f : 0.20f,
                      -static_cast<int> (hash % 971u), -static_cast<int> ((hash / 971u) % 971u));

    juce::ColourGradient edgeShade (juce::Colours::transparentBlack, capArea.getCentreX(), capArea.getCentreY(),
                                     juce::Colours::black.withAlpha (0.68f), capArea.getRight(),
                                     capArea.getBottom(), true);
    edgeShade.addColour (0.62, juce::Colours::transparentBlack);
    edgeShade.addColour (0.83, juce::Colours::black.withAlpha (0.24f));
    g.setGradientFill (edgeShade);
    g.fillEllipse (capArea);

    g.restoreState();
    g.setColour (juce::Colours::black.withAlpha (0.70f));
    juce::Path lowerOcclusion;
    lowerOcclusion.addCentredArc (centre.x, centre.y, capArea.getWidth() * 0.5f, capArea.getHeight() * 0.5f,
                                  0.0f, start + 2.64f, end - 0.08f, true);
    g.strokePath (lowerOcclusion, juce::PathStrokeType (isOverload ? 2.4f : isMini ? 0.75f : 1.55f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

    // A short engraved index near the perimeter preserves the dome illusion.
    const auto pointerStartLength = radius * (isOverload ? 0.57f : 0.55f);
    const auto pointerEndLength = radius * (isOverload ? 0.78f : 0.76f);
    const juce::Point<float> pointerStart (centre.x + std::sin (angle) * pointerStartLength,
                                           centre.y - std::cos (angle) * pointerStartLength);
    const juce::Point<float> pointerEnd (centre.x + std::sin (angle) * pointerEndLength,
                                         centre.y - std::cos (angle) * pointerEndLength);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine ({ pointerStart.translated (isMini ? 0.5f : 1.0f, isMini ? 0.5f : 1.0f),
                  pointerEnd.translated (isMini ? 0.5f : 1.0f, isMini ? 0.5f : 1.0f) },
                isOverload ? 4.1f : isMini ? 1.3f : 3.0f);
    g.setColour (accent);
    g.drawLine ({ pointerStart, pointerEnd }, isOverload ? 2.25f : isMini ? 0.8f : 1.5f);
    g.setColour (hot.withAlpha (isOverload ? 0.82f : 0.48f));
    g.fillEllipse (pointerEnd.x - (isOverload ? 1.3f : 0.8f), pointerEnd.y - (isOverload ? 1.3f : 0.8f),
                   isOverload ? 2.6f : 1.6f, isOverload ? 2.6f : 1.6f);
}

void VoidLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (label.getText() == "OVERLOAD")
    {
        g.setColour (label.findColour (juce::Label::textColourId));
    }
    else
    {
        g.setColour (label.findColour (juce::Label::textColourId));
    }
    g.setFont (juce::FontOptions (label.getFont().getHeight()).withName ("Bahnschrift")
                                                          .withStyle (label.getFont().getTypefaceStyle()));
    g.drawFittedText (label.getText(), label.getLocalBounds(), label.getJustificationType(), 1);
}

void VoidLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool highlighted, bool down)
{
    const auto on = button.getToggleState();
    auto area = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto lampAccent = primaryFor (themeStyle);
    const auto lampHighlight = highlightFor (themeStyle);
    const auto trackHeight = juce::jmax (6.0f, area.getHeight() * 0.38f);
    const auto trackWidth = juce::jmin (area.getWidth() - 4.0f, area.getHeight() * 1.72f);
    const auto track = juce::Rectangle<float> (trackWidth, trackHeight).withCentre (area.getCentre());
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (track.translated (0.0f, 1.0f).expanded (0.8f, 0.4f), trackHeight * 0.5f);
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (track, track.getHeight() * 0.5f);
    if (on) { g.setColour (lampAccent.withAlpha (0.10f)); g.fillRoundedRectangle (track, track.getHeight() * 0.5f); }
    g.setColour (juce::Colour (0xff4b4541).withAlpha (highlighted ? 0.92f : 0.68f));
    g.drawRoundedRectangle (track, track.getHeight() * 0.5f, highlighted ? 0.9f : 0.65f);
    const auto lampDiameter = juce::jmin (track.getHeight() + 3.0f, area.getHeight() - 4.0f);
    const auto lampX = on ? track.getRight() - lampDiameter + 1.5f : track.getX() - 1.5f;
    const auto lampArea = juce::Rectangle<float> (lampX, area.getCentreY() - lampDiameter * 0.5f,
                                                  lampDiameter, lampDiameter);
    juce::ColourGradient lamp (juce::Colour (0xff756c67),
                                lampArea.getX(), lampArea.getY(),
                                juce::Colour (0xff1b1817),
                                lampArea.getRight(), lampArea.getBottom(), false);
    g.setGradientFill (lamp);
    g.fillEllipse (lampArea);
    g.setColour (juce::Colour (warmWhite).withAlpha (down ? 0.82f : 0.58f));
    g.drawEllipse (lampArea.reduced (0.7f), 0.75f);
    if (on) { g.setColour (lampAccent.interpolatedWith (lampHighlight, 0.12f).withAlpha (0.62f)); g.fillEllipse (lampArea.reduced (lampDiameter * 0.37f)); }
}

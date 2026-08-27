#pragma once

#include "PresetTransition.h"

namespace voidworm
{
struct FactoryPreset
{
    const char* name;
    const char* family;
    const char* description;
    Parameters parameters;
};

enum class PresetEqProfile
{
    neutral, impact, impactPunch, grindLow, grindHigh, massDeep, massBody,
    furnaceStarve, furnaceFold, arcCollision, arcScrape, feedbackRattle,
    tearImpact, synthWall, collapse, extreme
};

inline ReactorEqSettings presetEq (float hp, float f1, float g1,
                                   float f2, float g2, float lp) noexcept
{
    return { hp, f1, g1, lp, f2, g2 };
}

inline std::array<ReactorEqSettings, 4> presetEqProfile (PresetEqProfile profile) noexcept
{
    switch (profile)
    {
        case PresetEqProfile::impact:
            return {{ presetEq (28, 105, 3.8f, 430, 2.2f, 9800), presetEq (72, 410, 4.5f, 1350, 2.0f, 10500),
                      presetEq (145, 2100, 2.5f, 5200, -2.5f, 9200), presetEq (180, 1150, 1.5f, 3400, -2.0f, 7200) }};
        case PresetEqProfile::impactPunch:
            return {{ presetEq (35, 145, 2.8f, 720, -1.5f, 11800), presetEq (95, 620, 4.0f, 2100, 2.5f, 12000),
                      presetEq (180, 2850, 3.0f, 6800, -3.5f, 10800), presetEq (220, 1750, 2.0f, 4600, -2.5f, 8200) }};
        case PresetEqProfile::grindLow:
            return {{ presetEq (42, 190, 2.0f, 760, -2.0f, 10500), presetEq (120, 540, 5.2f, 1650, 3.8f, 9800),
                      presetEq (210, 1700, 4.2f, 4300, 2.0f, 9300), presetEq (260, 1200, 2.0f, 3100, -2.5f, 7000) }};
        case PresetEqProfile::grindHigh:
            return {{ presetEq (70, 260, 1.5f, 1050, -2.5f, 11800), presetEq (160, 930, 4.2f, 2900, 3.2f, 11200),
                      presetEq (310, 2550, 5.5f, 6100, 2.5f, 10500), presetEq (380, 2100, 2.8f, 5200, -2.0f, 8200) }};
        case PresetEqProfile::massDeep:
            return {{ presetEq (24, 82, 4.5f, 260, 3.0f, 7200), presetEq (82, 360, 2.8f, 980, -1.5f, 8200),
                      presetEq (260, 1850, 1.0f, 4800, -4.5f, 7600), presetEq (210, 900, 1.0f, 2600, -3.0f, 5600) }};
        case PresetEqProfile::massBody:
            return {{ presetEq (30, 150, 4.0f, 620, 2.5f, 9200), presetEq (95, 520, 3.5f, 1550, 1.0f, 9400),
                      presetEq (230, 2250, 1.8f, 5700, -3.0f, 9000), presetEq (250, 1350, 1.5f, 3600, -2.0f, 6800) }};
        case PresetEqProfile::furnaceStarve:
            return {{ presetEq (55, 180, 1.5f, 720, -2.0f, 9800), presetEq (130, 610, 5.8f, 1900, 3.6f, 10800),
                      presetEq (250, 2500, 2.2f, 6100, -2.5f, 9700), presetEq (300, 1600, 1.8f, 3900, -2.0f, 7200) }};
        case PresetEqProfile::furnaceFold:
            return {{ presetEq (65, 240, 1.0f, 900, -2.5f, 11200), presetEq (180, 980, 4.6f, 3150, 4.4f, 11800),
                      presetEq (340, 3100, 3.6f, 7200, -2.0f, 11000), presetEq (420, 2400, 2.2f, 5300, -2.5f, 8600) }};
        case PresetEqProfile::arcCollision:
            return {{ presetEq (75, 260, 1.0f, 930, -2.5f, 10500), presetEq (190, 850, 2.5f, 2600, 1.8f, 10800),
                      presetEq (330, 1900, 5.0f, 4800, 4.2f, 9900), presetEq (390, 1750, 2.4f, 4300, -1.5f, 7800) }};
        case PresetEqProfile::arcScrape:
            return {{ presetEq (95, 340, 0.5f, 1200, -3.0f, 11800), presetEq (240, 1150, 2.2f, 3500, 2.0f, 11600),
                      presetEq (480, 3250, 5.8f, 7600, 2.8f, 12100), presetEq (520, 2800, 3.0f, 6200, -1.0f, 9200) }};
        case PresetEqProfile::feedbackRattle:
            return {{ presetEq (55, 190, 1.5f, 680, -2.0f, 9000), presetEq (150, 720, 2.8f, 2200, 1.8f, 9800),
                      presetEq (290, 2400, 3.0f, 5600, -2.0f, 9200), presetEq (360, 1150, 4.8f, 3250, 4.0f, 7200) }};
        case PresetEqProfile::tearImpact:
            return {{ presetEq (38, 135, 3.2f, 520, 1.5f, 9800), presetEq (115, 580, 4.0f, 1750, 2.2f, 10300),
                      presetEq (250, 2300, 4.5f, 6000, -2.8f, 9800), presetEq (320, 1550, 3.0f, 4100, -2.0f, 7400) }};
        case PresetEqProfile::synthWall:
            return {{ presetEq (62, 210, 2.5f, 780, -1.8f, 10800), presetEq (145, 760, 4.5f, 2350, 3.5f, 11200),
                      presetEq (300, 2100, 4.0f, 5200, 1.0f, 10200), presetEq (380, 1800, 2.2f, 4500, -2.8f, 7800) }};
        case PresetEqProfile::collapse:
            return {{ presetEq (45, 120, 3.5f, 480, 1.5f, 8600), presetEq (145, 520, 5.0f, 1650, 3.0f, 9000),
                      presetEq (320, 1850, 4.0f, 4700, -1.5f, 8400), presetEq (300, 980, 3.5f, 2950, 2.0f, 6200) }};
        case PresetEqProfile::extreme:
            return {{ presetEq (48, 110, 4.8f, 410, 3.2f, 8200), presetEq (180, 620, 5.8f, 1900, 4.8f, 9400),
                      presetEq (420, 2200, 5.8f, 5900, 2.5f, 9800), presetEq (480, 1350, 4.5f, 3800, 2.8f, 7200) }};
        case PresetEqProfile::neutral:
        default:
            return {{ massEqDefaults(), furnaceEqDefaults(), arcEqDefaults(), feedbackEqDefaults() }};
    }
}

inline FactoryPreset makeFactoryPreset (const char* name, const char* family, const char* description,
                                        std::array<float, 11> main, PresetEqProfile eqProfile,
                                        std::array<float, 4> amounts, ReactorCharacterSettings character,
                                        float weld, float limitDb, float ceilingDb, bool surge = false,
                                        std::array<bool, 4> enabled = { true, true, true, true }) noexcept
{
    Parameters p;
    p.breach = main[0]; p.tear = main[1]; p.rot = main[2]; p.driveDb = main[3];
    p.overload = main[4]; p.mix = main[5]; p.lowDb = main[6]; p.midDb = main[7];
    p.highDb = main[8]; p.range = main[9]; p.outputDb = main[10];
    p.reactorEnabled = enabled;
    p.reactorAmounts = amounts;
    p.reactorCharacter = character;
    p.surge = surge;
    p.weld = weld;
    p.gateEnabled = true;
    p.limiterEnabled = true;
    p.limiterThresholdDb = limitDb;
    p.limiterCeilingDb = ceilingDb;
    const auto eq = presetEqProfile (eqProfile);
    p.massEq = eq[0]; p.furnaceEq = eq[1]; p.arcEq = eq[2]; p.feedbackEq = eq[3];
    return { name, family, description, p };
}

inline const std::array<FactoryPreset, 49>& factoryPresets() noexcept
{
    using E = PresetEqProfile;
    static const std::array<FactoryPreset, 49> presets {{
        makeFactoryPreset ("Default Init", "INIT", "Neutral starting point for sound design.",
            { .35f,.04f,.42f,4,.35f,.72f,0,0,0,.92f,-4 }, E::neutral,
            { 1,1,1,1 }, { .5f,.5f,.5f,.5f,.5f,.5f,.5f,.5f }, .30f,-3,-.8f),

        makeFactoryPreset ("Void Hammer", "IMPACT", "Huge low-note impact with a grinding furnace tail.",
            { .48f,.03f,.58f,9,.72f,.92f,1.5f,2,-2,.88f,1 }, E::impact,
            { 1,.94f,.48f,.22f }, { .76f,.70f,.74f,.55f,.48f,.38f,.38f,.70f }, .72f,-8,-.8f),
        makeFactoryPreset ("Shock Mass", "IMPACT", "Fast body hit with retained transient edge.",
            { .36f,.05f,.47f,8,.66f,.88f,2,1,-1,.91f,0 }, E::impactPunch,
            { 1,.80f,.60f,.18f }, { .82f,.62f,.58f,.40f,.62f,.44f,.32f,.76f }, .58f,-6,-.8f),
        makeFactoryPreset ("Hull Impact", "IMPACT", "Broad mechanical stab that lands before it flattens.",
            { .55f,.09f,.61f,10,.78f,.90f,1,3,-2,.84f,1.5f }, E::impactPunch,
            { .92f,1,.68f,.34f }, { .70f,.78f,.68f,.61f,.58f,.52f,.44f,.64f }, .69f,-9,-1),
        makeFactoryPreset ("Pressure Drop", "IMPACT", "Compressed reactor pressure with a short hostile return.",
            { .44f,.04f,.52f,7,.70f,.86f,3,0,-2,.80f,.5f }, E::impact,
            { 1,.76f,.40f,.52f }, { .84f,.68f,.62f,.36f,.40f,.34f,.68f,.72f }, .64f,-7,-.8f),
        makeFactoryPreset ("Core Breach", "IMPACT", "Low-mid breach and controlled arc detonation.",
            { .74f,.07f,.66f,11,.82f,.94f,1,3,-1,.86f,2 }, E::impact,
            { .94f,1,.74f,.28f }, { .72f,.80f,.76f,.63f,.66f,.58f,.42f,.62f }, .78f,-10,-1),

        makeFactoryPreset ("Grind Vector", "GRIND", "Furnace and arc carve a moving midrange gear train.",
            { .66f,.06f,.74f,10,.78f,.92f,-1,3,0,.82f,1 }, E::grindLow,
            { .58f,1,.90f,.25f }, { .52f,.64f,.82f,.72f,.78f,.66f,.38f,.58f }, .72f,-8,-.8f),
        makeFactoryPreset ("Iron Maw", "GRIND", "Dense low-mid bite with starved transistor edges.",
            { .58f,.04f,.70f,9,.74f,.90f,1,4,-2,.80f,1 }, E::grindLow,
            { .72f,1,.68f,.32f }, { .64f,.70f,.91f,.48f,.58f,.52f,.44f,.62f }, .68f,-8,-1),
        makeFactoryPreset ("Machine Wound", "GRIND", "High-mid scrape wrapped around a scorched core.",
            { .72f,.08f,.82f,11,.84f,.93f,-2,2,1,.76f,.5f }, E::grindHigh,
            { .44f,.92f,1,.46f }, { .46f,.62f,.78f,.84f,.90f,.72f,.52f,.48f }, .76f,-9,-1.2f),
        makeFactoryPreset ("Steel Teeth", "GRIND", "Hard folded articulation for harmonically rich synths.",
            { .62f,.02f,.78f,8,.70f,.86f,-1,2,-1,.84f,-.5f }, E::grindHigh,
            { .40f,.80f,1,.18f }, { .42f,.56f,.62f,.88f,.84f,.92f,.28f,.70f }, .58f,-6,-.8f),
        makeFactoryPreset ("Friction Core", "GRIND", "Slow crushing friction with a dense welded sustain.",
            { .70f,.05f,.88f,12,.88f,.95f,0,4,-2,.75f,2 }, E::grindLow,
            { .68f,1,.82f,.38f }, { .66f,.78f,.88f,.72f,.70f,.64f,.50f,.56f }, .84f,-11,-1),

        makeFactoryPreset ("Black Pressure", "MASS", "Deep fundamental pressure without loose sub bloom.",
            { .30f,.01f,.50f,8,.62f,.90f,2,1,-3,.86f,0 }, E::massDeep,
            { 1,.58f,.24f,.12f }, { .92f,.70f,.58f,.34f,.34f,.28f,.24f,.82f }, .62f,-7,-.8f),
        makeFactoryPreset ("Crush Depth", "MASS", "Sub and low-mid layers crushed into one heavy body.",
            { .42f,.03f,.62f,10,.74f,.94f,3,2,-3,.80f,1 }, E::massDeep,
            { 1,.72f,.34f,.18f }, { .88f,.86f,.68f,.46f,.42f,.36f,.30f,.78f }, .78f,-9,-1),
        makeFactoryPreset ("Reactor Drop", "MASS", "Single-note machine start with a compressed core fall.",
            { .50f,.06f,.56f,11,.82f,.91f,2,3,-2,.83f,2 }, E::massBody,
            { 1,.86f,.42f,.20f }, { .82f,.76f,.76f,.52f,.48f,.38f,.34f,.72f }, .74f,-10,-.8f),
        makeFactoryPreset ("Dense Gravity", "MASS", "Broad bass wall with controlled upper abrasion.",
            { .38f,.02f,.68f,9,.68f,.88f,1,2,-1,.90f,0 }, E::massBody,
            { 1,.64f,.36f,.14f }, { .94f,.82f,.54f,.40f,.46f,.32f,.22f,.86f }, .66f,-7,-1),
        makeFactoryPreset ("Compression Core", "MASS", "Pinned low reactor body with moderate harmonic lift.",
            { .46f,.04f,.60f,7,.72f,.86f,2,1,-2,.88f,1 }, E::massBody,
            { 1,.52f,.26f,.08f }, { .90f,.72f,.52f,.36f,.30f,.28f,.18f,.90f }, .86f,-11,-.8f,
            false, { true,true,true,false }),

        makeFactoryPreset ("Scorched Rail", "FURNACE", "Starved transistor rail with dry midrange abrasion.",
            { .52f,.03f,.76f,10,.82f,.90f,-1,3,-2,.80f,1 }, E::furnaceStarve,
            { .42f,1,.58f,.18f }, { .48f,.54f,.96f,.42f,.54f,.38f,.26f,.74f }, .68f,-8,-1),
        makeFactoryPreset ("Power Failure", "FURNACE", "Collapsed supply sag that still preserves note shape.",
            { .44f,.05f,.84f,12,.88f,.92f,0,4,-3,.76f,1.5f }, E::furnaceStarve,
            { .48f,1,.46f,.30f }, { .56f,.62f,1,.58f,.44f,.46f,.48f,.60f }, .74f,-9,-1.2f),
        makeFactoryPreset ("Furnace Mouth", "FURNACE", "Hot mechanical fuzz with an open folded throat.",
            { .62f,.04f,.72f,9,.76f,.91f,-1,2,-1,.85f,.5f }, E::furnaceFold,
            { .34f,1,.72f,.16f }, { .40f,.52f,.76f,.94f,.62f,.70f,.24f,.78f }, .66f,-7,-.8f),
        makeFactoryPreset ("Burn State", "FURNACE", "Asymmetric grind focused into the upper low-mids.",
            { .57f,.02f,.80f,11,.80f,.89f,-2,4,-2,.78f,1 }, E::furnaceStarve,
            { .28f,1,.54f,.12f }, { .38f,.48f,.92f,.72f,.50f,.42f,.20f,.82f }, .60f,-8,-1),
        makeFactoryPreset ("Molten Bus", "FURNACE", "Folded furnace output welded into a broad synth slab.",
            { .68f,.07f,.86f,13,.90f,.96f,0,3,-2,.74f,2.5f }, E::furnaceFold,
            { .56f,1,.76f,.34f }, { .52f,.68f,.90f,.98f,.68f,.82f,.44f,.62f }, .88f,-12,-1),

        makeFactoryPreset ("Arc Damage", "ARC", "Electrical collision centered on useful metallic mids.",
            { .78f,.04f,.72f,9,.74f,.90f,-2,2,-2,.82f,.5f }, E::arcCollision,
            { .30f,.62f,1,.20f }, { .34f,.46f,.58f,.54f,.94f,.72f,.28f,.76f }, .64f,-7,-1),
        makeFactoryPreset ("Grid Collision", "ARC", "Source-derived cross-mod pressure with furnace support.",
            { .86f,.06f,.78f,11,.82f,.93f,-2,3,-1,.76f,1 }, E::arcCollision,
            { .34f,.78f,1,.34f }, { .36f,.52f,.66f,.58f,1,.84f,.42f,.64f }, .72f,-9,-1),
        makeFactoryPreset ("Voltage Scar", "ARC", "Narrow high-mid reactor scrape without brittle fizz.",
            { .72f,.03f,.82f,8,.70f,.86f,-3,1,-2,.78f,-1 }, E::arcScrape,
            { .18f,.50f,1,.16f }, { .28f,.38f,.48f,.44f,.92f,.96f,.22f,.82f }, .52f,-6,-1.2f),
        makeFactoryPreset ("Mech Spark", "ARC", "Fast metallic transient violence with a short decay.",
            { .82f,.12f,.68f,10,.78f,.88f,-2,2,-2,.84f,0 }, E::arcScrape,
            { .22f,.58f,1,.28f }, { .30f,.42f,.56f,.62f,.88f,.78f,.36f,.72f }, .58f,-7,-.8f),
        makeFactoryPreset ("Crosswire", "ARC", "Unstable machinery texture built from harmonic collisions.",
            { .92f,.05f,.90f,12,.90f,.95f,-2,3,-2,.72f,2 }, E::arcCollision,
            { .26f,.66f,1,.54f }, { .32f,.48f,.72f,.68f,1,.92f,.58f,.54f }, .82f,-11,-1),

        makeFactoryPreset ("Iron Static", "FEEDBACK", "Short rattling recursion around a weighted core.",
            { .58f,.04f,.74f,9,.78f,.90f,0,2,-2,.82f,.5f }, E::feedbackRattle,
            { .54f,.58f,.42f,1 }, { .50f,.58f,.64f,.52f,.52f,.46f,.92f,.46f }, .66f,-8,-1),
        makeFactoryPreset ("Dead Circuit", "FEEDBACK", "Dark hostile return with aggressive internal damping.",
            { .66f,.03f,.82f,10,.84f,.92f,1,2,-3,.76f,1 }, E::feedbackRattle,
            { .42f,.60f,.36f,1 }, { .46f,.54f,.70f,.58f,.48f,.42f,1,.88f }, .72f,-9,-1.2f),
        makeFactoryPreset ("Return Fault", "FEEDBACK", "Mechanically resonant return that clamps into WELD.",
            { .74f,.08f,.86f,12,.90f,.95f,-1,3,-2,.72f,2 }, E::feedbackRattle,
            { .50f,.72f,.54f,1 }, { .52f,.64f,.78f,.66f,.58f,.52f,.98f,.34f }, .84f,-11,-1),
        makeFactoryPreset ("Machine Room", "FEEDBACK", "Dense recursive machinery with restrained brightness.",
            { .62f,.05f,.70f,8,.72f,.88f,1,1,-3,.86f,0 }, E::feedbackRattle,
            { .66f,.54f,.30f,.88f }, { .62f,.66f,.58f,.46f,.38f,.36f,.82f,.58f }, .58f,-7,-.8f,
            true),

        makeFactoryPreset ("Mech Rip", "TEAR", "Obvious fragmentation around a hard mechanical attack.",
            { .64f,.42f,.72f,9,.78f,.90f,-1,2,-2,.82f,.5f }, E::tearImpact,
            { .62f,.80f,.78f,.34f }, { .58f,.66f,.72f,.62f,.76f,.68f,.46f,.66f }, .68f,-8,-1),
        makeFactoryPreset ("Signal Shrapnel", "TEAR", "Fragmented synth edges compressed into moving debris.",
            { .72f,.62f,.80f,11,.84f,.94f,-2,3,-2,.76f,1.5f }, E::tearImpact,
            { .48f,.86f,.92f,.44f }, { .46f,.58f,.82f,.72f,.88f,.76f,.54f,.58f }, .76f,-10,-1),
        makeFactoryPreset ("Fragment Rail", "TEAR", "Rhythmic electrical tearing with low-mid continuity.",
            { .56f,.76f,.68f,8,.70f,.88f,0,2,-2,.86f,0 }, E::tearImpact,
            { .74f,.70f,.66f,.28f }, { .68f,.64f,.62f,.54f,.70f,.60f,.38f,.74f }, .60f,-7,-.8f),
        makeFactoryPreset ("Split Current", "TEAR", "Extreme fragmentation welded back into one circuit.",
            { .82f,.92f,.88f,13,.92f,.97f,-2,3,-3,.70f,2.5f }, E::tearImpact,
            { .58f,.92f,1,.62f }, { .52f,.70f,.88f,.78f,.94f,.88f,.66f,.50f }, .90f,-13,-1.2f),

        makeFactoryPreset ("Weld Death", "WELDED", "Extreme WELD density that still exposes reactor motion.",
            { .68f,.06f,.78f,12,.88f,.98f,0,2,-2,.78f,2 }, E::synthWall,
            { .76f,.92f,.84f,.40f }, { .68f,.76f,.82f,.70f,.76f,.68f,.48f,.64f }, 1,-12,-1),
        makeFactoryPreset ("Pressure Lock", "WELDED", "Moderate reactors forced into a tight pressure block.",
            { .46f,.03f,.60f,8,.68f,.92f,1,1,-2,.88f,0 }, E::impact,
            { .88f,.74f,.52f,.22f }, { .78f,.66f,.64f,.48f,.50f,.42f,.30f,.76f }, .88f,-10,-.8f),
        makeFactoryPreset ("Molten Block", "WELDED", "Furnace-led synth wall with controlled top-end ash.",
            { .64f,.04f,.82f,11,.86f,.96f,-1,3,-3,.74f,2 }, E::synthWall,
            { .54f,1,.72f,.30f }, { .50f,.64f,.90f,.84f,.68f,.60f,.40f,.68f }, .94f,-12,-1),
        makeFactoryPreset ("Redline Core", "WELDED", "Mass-led core driven hard into the final limiter.",
            { .52f,.03f,.72f,10,.82f,.96f,2,2,-3,.80f,3 }, E::massBody,
            { 1,.72f,.44f,.20f }, { .90f,.80f,.74f,.52f,.48f,.42f,.30f,.78f }, .90f,-13,-.8f),
        makeFactoryPreset ("System Crush", "WELDED", "Full reactor blend compressed into a finished wall.",
            { .74f,.08f,.86f,13,.92f,1,-1,3,-2,.70f,3 }, E::synthWall,
            { .78f,.96f,.90f,.58f }, { .70f,.78f,.88f,.76f,.84f,.74f,.62f,.54f }, .98f,-14,-1),

        makeFactoryPreset ("Steel Collapse", "COLLAPSE", "Starved supply folds a heavy core inward.",
            { .70f,.05f,.90f,12,.92f,.96f,1,3,-3,.72f,2 }, E::collapse,
            { .84f,1,.62f,.48f }, { .76f,.82f,1,.72f,.62f,.56f,.58f,.50f }, .86f,-11,-1),
        makeFactoryPreset ("Faultline", "COLLAPSE", "Low body fracture with an unstable arc seam.",
            { .88f,.12f,.84f,11,.88f,.94f,1,2,-2,.76f,1 }, E::collapse,
            { .92f,.78f,.88f,.36f }, { .84f,.78f,.86f,.64f,.92f,.72f,.44f,.62f }, .80f,-10,-1),
        makeFactoryPreset ("Supply Rupture", "COLLAPSE", "Violent furnace sag and compressed feedback debris.",
            { .76f,.10f,.94f,14,.96f,.98f,0,4,-3,.68f,3 }, E::collapse,
            { .66f,1,.70f,.72f }, { .60f,.74f,1,.86f,.72f,.68f,.82f,.40f }, .94f,-14,-1.2f,
            true),
        makeFactoryPreset ("Grid Failure", "COLLAPSE", "Whole-machine failure with a bounded catastrophic surge.",
            { .92f,.16f,.96f,15,1,.98f,-1,4,-3,.66f,3.5f }, E::collapse,
            { .78f,1,.92f,.78f }, { .72f,.84f,1,.90f,.94f,.86f,.88f,.34f }, .96f,-15,-1.2f,
            true),

        makeFactoryPreset ("Blackout Engine", "EXTREME", "Maximum dark reactor pressure with hard containment.",
            { .88f,.18f,.96f,16,1,1,2,3,-4,.64f,4 }, E::extreme,
            { 1,.96f,.72f,.70f }, { 1,.92f,1,.84f,.78f,.72f,.86f,.46f }, 1,-16,-1.2f,
            true),
        makeFactoryPreset ("Reactor Zero", "EXTREME", "All four reactors forced through a terminal output stage.",
            { 1,.26f,1,18,1,1,0,4,-3,.62f,4 }, E::extreme,
            { 1,1,1,1 }, { .94f,1,1,1,1,1,1,.30f }, 1,-18,-1.5f,
            true),
        makeFactoryPreset ("Terminal Grind", "EXTREME", "Midrange machinery reduced to a hostile moving slab.",
            { .94f,.14f,1,16,1,.98f,-2,5,-3,.62f,4 }, E::grindHigh,
            { .54f,1,1,.76f }, { .58f,.82f,1,.96f,1,1,.88f,.36f }, .98f,-16,-1.2f,
            true),
        makeFactoryPreset ("Catastrophe Bus", "EXTREME", "Torn full-spectrum bus with deliberate limiter violence.",
            { 1,.68f,.96f,17,1,1,-1,4,-4,.60f,5 }, E::extreme,
            { .82f,1,1,.92f }, { .80f,.92f,1,.94f,1,.96f,.98f,.28f }, 1,-18,-2,
            true),
        makeFactoryPreset ("Void Furnace", "EXTREME", "Collapsed furnace supply buried inside enormous mass.",
            { .86f,.20f,1,17,1,1,3,4,-4,.60f,4.5f }, E::furnaceFold,
            { 1,1,.72f,.68f }, { 1,.94f,1,1,.74f,.78f,.84f,.42f }, 1,-17,-1.5f,
            true),
        makeFactoryPreset ("Final Failure", "EXTREME", "The complete instrument at controlled catastrophic escalation.",
            { 1,.84f,1,18,1,1,1,5,-4,.58f,6 }, E::extreme,
            { 1,1,1,1 }, { 1,1,1,1,1,1,1,.22f }, 1,-18,-2,
            true)
    }};
    return presets;
}

inline PresetSnapshot makeFactoryPresetSnapshot (int index, Parameters base) noexcept
{
    const auto& presets = factoryPresets();
    const auto safeIndex = (index % static_cast<int> (presets.size()) + static_cast<int> (presets.size()))
                         % static_cast<int> (presets.size());
    const auto preservedOversampling = base.oversampleFactor;
    const auto preservedHqMode = base.hqMode;
    const auto preservedSolo = base.reactorSolo;
    base = presets[static_cast<size_t> (safeIndex)].parameters;
    base.oversampleFactor = preservedOversampling;
    base.hqMode = preservedHqMode;
    base.reactorSolo = preservedSolo;
    return { base };
}
}

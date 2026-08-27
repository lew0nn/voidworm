#include "UserPresetStore.h"

#include <cmath>

namespace voidworm
{
namespace
{
constexpr const char* extension = ".voidwormpreset";

bool finite (float value) noexcept { return std::isfinite (value); }

bool validEq (const ReactorEqSettings& eq) noexcept
{
    return finite (eq.hp) && finite (eq.focusFrequency) && finite (eq.focusGainDb)
        && finite (eq.focus2Frequency) && finite (eq.focus2GainDb) && finite (eq.lp)
        && eq.hp >= 20.0f && eq.hp <= 5000.0f
        && eq.focusFrequency >= 30.0f && eq.focusFrequency <= 16000.0f
        && eq.focus2Frequency >= 30.0f && eq.focus2Frequency <= 16000.0f
        && eq.focusGainDb >= -12.0f && eq.focusGainDb <= 12.0f
        && eq.focus2GainDb >= -12.0f && eq.focus2GainDb <= 12.0f
        && eq.lp >= 200.0f && eq.lp <= 20000.0f;
}

bool validSound (const Parameters& p) noexcept
{
    const std::array<float, 20> values { p.breach, p.tear, p.rot, p.driveDb, p.overload, p.mix,
        p.range, p.lowDb, p.midDb, p.highDb, p.outputDb, p.weld, p.limiterThresholdDb,
        p.limiterCeilingDb, p.gateThresholdDb, p.reactorAmounts[0], p.reactorAmounts[1], p.reactorAmounts[2],
        p.reactorAmounts[3], p.reactorCharacter.massSaturation };
    if (! std::all_of (values.begin(), values.end(), finite)) return false;
    const std::array<float, 7> characters { p.reactorCharacter.massHarmonics,
        p.reactorCharacter.furnaceStarve, p.reactorCharacter.furnaceFold,
        p.reactorCharacter.arcXmod, p.reactorCharacter.arcFold,
        p.reactorCharacter.feedbackReturn, p.reactorCharacter.feedbackDamp };
    if (! std::all_of (characters.begin(), characters.end(), [] (float value)
        { return finite (value) && value >= 0.0f && value <= 1.0f; })) return false;
    if (p.breach < 0 || p.breach > 1 || p.tear < 0 || p.tear > 1 || p.rot < 0 || p.rot > 1
        || p.driveDb < 0 || p.driveDb > 18 || p.overload < 0 || p.overload > 1 || p.mix < 0 || p.mix > 1
        || p.range < 0 || p.range > 1 || p.lowDb < -12 || p.lowDb > 12 || p.midDb < -12 || p.midDb > 12
        || p.highDb < -12 || p.highDb > 12 || p.outputDb < -24 || p.outputDb > 6 || p.weld < 0 || p.weld > 1
        || p.limiterThresholdDb < -18 || p.limiterThresholdDb > 0 || p.limiterCeilingDb < -6 || p.limiterCeilingDb > 0
        || p.gateThresholdDb < -80 || p.gateThresholdDb > -20)
        return false;
    if (! std::all_of (p.reactorAmounts.begin(), p.reactorAmounts.end(), [] (float value)
        { return finite (value) && value >= 0.0f && value <= 1.0f; })) return false;
    return validEq (p.massEq) && validEq (p.furnaceEq) && validEq (p.arcEq) && validEq (p.feedbackEq);
}

void setEq (juce::ValueTree& sound, const char* prefix, const ReactorEqSettings& eq)
{
    sound.setProperty (juce::String (prefix) + "Hp", eq.hp, nullptr);
    sound.setProperty (juce::String (prefix) + "F1", eq.focusFrequency, nullptr);
    sound.setProperty (juce::String (prefix) + "G1", eq.focusGainDb, nullptr);
    sound.setProperty (juce::String (prefix) + "F2", eq.focus2Frequency, nullptr);
    sound.setProperty (juce::String (prefix) + "G2", eq.focus2GainDb, nullptr);
    sound.setProperty (juce::String (prefix) + "Lp", eq.lp, nullptr);
}

ReactorEqSettings getEq (const juce::ValueTree& sound, const char* prefix)
{
    return { static_cast<float> (sound[juce::String (prefix) + "Hp"]),
             static_cast<float> (sound[juce::String (prefix) + "F1"]),
             static_cast<float> (sound[juce::String (prefix) + "G1"]),
             static_cast<float> (sound[juce::String (prefix) + "Lp"]),
             static_cast<float> (sound[juce::String (prefix) + "F2"]),
             static_cast<float> (sound[juce::String (prefix) + "G2"]) };
}
}

UserPresetStore::UserPresetStore (juce::File directory) : presetDirectory (std::move (directory))
{
    if (presetDirectory == defaultDirectory())
    {
        const auto legacyDirectory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
            .getChildFile ("LWNX DSP").getChildFile ("VOIDWORM").getChildFile ("Presets");
        if (legacyDirectory.isDirectory())
        {
            presetDirectory.createDirectory();
            for (const auto& source : legacyDirectory.findChildFiles (juce::File::findFiles, false,
                                                                       "*" + juce::String (extension)))
            {
                const auto destination = presetDirectory.getChildFile (source.getFileName());
                if (! destination.existsAsFile()) source.copyFileTo (destination);
            }
        }
    }
    refresh();
}

juce::File UserPresetStore::defaultDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("LWNX DSP").getChildFile ("VOIDWORM").getChildFile ("Presets");
}

juce::String UserPresetStore::validateName (const juce::String& raw)
{
    const auto name = raw.trim();
    if (name.isEmpty()) return "ENTER A PRESET NAME";
    if (name.length() > maximumNameLength) return "NAME MUST BE 64 CHARACTERS OR LESS";
    if (name.containsAnyOf ("\\/:*?\"<>|") || name.contains ("..")) return "NAME CONTAINS UNSAFE CHARACTERS";
    return {};
}

void UserPresetStore::refresh()
{
    cachedPresets.clear();
    if (! presetDirectory.isDirectory()) return;
    for (const auto& file : presetDirectory.findChildFiles (juce::File::findFiles, false, "*" + juce::String (extension)))
    {
        const auto loaded = load (file);
        if (loaded.ok && loaded.preset.has_value()) cachedPresets.push_back (*loaded.preset);
    }
    std::sort (cachedPresets.begin(), cachedPresets.end(), [] (const UserPreset& a, const UserPreset& b)
    { return a.name.compareNatural (b.name, true) < 0; });
}

std::optional<size_t> UserPresetStore::findByName (const juce::String& name) const
{
    for (size_t index = 0; index < cachedPresets.size(); ++index)
        if (cachedPresets[index].name.equalsIgnoreCase (name.trim())) return index;
    return std::nullopt;
}

std::optional<size_t> UserPresetStore::findByFile (const juce::File& file) const
{
    for (size_t index = 0; index < cachedPresets.size(); ++index)
        if (cachedPresets[index].file == file) return index;
    return std::nullopt;
}

juce::ValueTree UserPresetStore::makeTree (const juce::String& name, const Parameters& p)
{
    juce::ValueTree root ("VOIDWORM_PRESET");
    root.setProperty ("plugin", "VOIDWORM", nullptr);
    root.setProperty ("formatVersion", formatVersion, nullptr);
    root.setProperty ("presetName", name, nullptr);
    root.setProperty ("createdWithVersion", "1.0.0", nullptr);
    juce::ValueTree sound ("SOUND");
    sound.setProperty ("breach", p.breach, nullptr); sound.setProperty ("tear", p.tear, nullptr);
    sound.setProperty ("rot", p.rot, nullptr); sound.setProperty ("drive", p.driveDb, nullptr);
    sound.setProperty ("overload", p.overload, nullptr); sound.setProperty ("mix", p.mix, nullptr);
    sound.setProperty ("surge", p.surge, nullptr); sound.setProperty ("low", p.lowDb, nullptr);
    sound.setProperty ("mid", p.midDb, nullptr); sound.setProperty ("high", p.highDb, nullptr);
    sound.setProperty ("range", p.range, nullptr); sound.setProperty ("output", p.outputDb, nullptr);
    sound.setProperty ("weld", p.weld, nullptr); sound.setProperty ("limiterEnabled", p.limiterEnabled, nullptr);
    sound.setProperty ("limiterThreshold", p.limiterThresholdDb, nullptr);
    sound.setProperty ("limiterCeiling", p.limiterCeilingDb, nullptr);
    sound.setProperty ("gateEnabled", p.gateEnabled, nullptr);
    sound.setProperty ("gateThreshold", p.gateThresholdDb, nullptr);
    constexpr std::array<const char*, 4> paths { "mass", "furnace", "arc", "feedback" };
    for (size_t i = 0; i < paths.size(); ++i)
    {
        sound.setProperty (juce::String (paths[i]) + "Enabled", p.reactorEnabled[i], nullptr);
        sound.setProperty (juce::String (paths[i]) + "Amount", p.reactorAmounts[i], nullptr);
    }
    sound.setProperty ("massSaturation", p.reactorCharacter.massSaturation, nullptr);
    sound.setProperty ("massHarmonics", p.reactorCharacter.massHarmonics, nullptr);
    sound.setProperty ("furnaceStarve", p.reactorCharacter.furnaceStarve, nullptr);
    sound.setProperty ("furnaceFold", p.reactorCharacter.furnaceFold, nullptr);
    sound.setProperty ("arcXmod", p.reactorCharacter.arcXmod, nullptr);
    sound.setProperty ("arcFold", p.reactorCharacter.arcFold, nullptr);
    sound.setProperty ("feedbackReturn", p.reactorCharacter.feedbackReturn, nullptr);
    sound.setProperty ("feedbackDamp", p.reactorCharacter.feedbackDamp, nullptr);
    setEq (sound, "mass", p.massEq); setEq (sound, "furnace", p.furnaceEq);
    setEq (sound, "arc", p.arcEq); setEq (sound, "feedback", p.feedbackEq);
    root.addChild (sound, -1, nullptr);
    return root;
}

bool UserPresetStore::readTree (const juce::ValueTree& root, UserPreset& result, juce::String& error)
{
    if (! root.isValid() || ! root.hasType ("VOIDWORM_PRESET") || root["plugin"].toString() != "VOIDWORM")
    { error = "NOT A VOIDWORM PRESET"; return false; }
    if (static_cast<int> (root["formatVersion"]) != formatVersion)
    { error = "INCOMPATIBLE PRESET VERSION"; return false; }
    result.name = root["presetName"].toString().trim();
    if (validateName (result.name).isNotEmpty()) { error = "INVALID PRESET NAME"; return false; }
    const auto sound = root.getChildWithName ("SOUND");
    if (! sound.isValid()) { error = "PRESET SOUND DATA IS MISSING"; return false; }
    constexpr std::array<const char*, 40> required { "breach","tear","rot","drive","overload","mix","surge",
        "low","mid","high","range","output","weld","limiterEnabled","limiterThreshold","limiterCeiling",
        "massEnabled","furnaceEnabled","arcEnabled","feedbackEnabled","massAmount","furnaceAmount","arcAmount","feedbackAmount",
        "massSaturation","massHarmonics","furnaceStarve","furnaceFold","arcXmod","arcFold","feedbackReturn","feedbackDamp",
        "massHp","massF1","massG1","massF2","massG2","massLp","furnaceHp","furnaceF1" };
    for (const auto* property : required)
        if (! sound.hasProperty (property)) { error = "PRESET SOUND DATA IS INCOMPLETE"; return false; }
    for (const auto* prefix : { "furnace", "arc", "feedback" })
        for (const auto* suffix : { "Hp", "F1", "G1", "F2", "G2", "Lp" })
            if (! sound.hasProperty (juce::String (prefix) + suffix)) { error = "PRESET EQ DATA IS INCOMPLETE"; return false; }
    auto& p = result.parameters;
    p.breach = sound["breach"]; p.tear = sound["tear"]; p.rot = sound["rot"]; p.driveDb = sound["drive"];
    p.overload = sound["overload"]; p.mix = sound["mix"]; p.surge = sound["surge"];
    p.lowDb = sound["low"]; p.midDb = sound["mid"]; p.highDb = sound["high"];
    p.range = sound["range"]; p.outputDb = sound["output"]; p.weld = sound["weld"];
    p.limiterEnabled = sound["limiterEnabled"]; p.limiterThresholdDb = sound["limiterThreshold"];
    p.limiterCeilingDb = sound["limiterCeiling"];
    p.gateEnabled = sound.hasProperty ("gateEnabled") ? static_cast<bool> (sound["gateEnabled"]) : true;
    p.gateThresholdDb = sound.hasProperty ("gateThreshold")
        ? static_cast<float> (sound["gateThreshold"]) : -50.0f;
    constexpr std::array<const char*, 4> paths { "mass", "furnace", "arc", "feedback" };
    for (size_t i = 0; i < paths.size(); ++i)
    {
        p.reactorEnabled[i] = sound[juce::String (paths[i]) + "Enabled"];
        p.reactorAmounts[i] = sound[juce::String (paths[i]) + "Amount"];
    }
    p.reactorCharacter = { sound["massSaturation"], sound["massHarmonics"], sound["furnaceStarve"], sound["furnaceFold"],
        sound["arcXmod"], sound["arcFold"], sound["feedbackReturn"], sound["feedbackDamp"] };
    p.massEq = getEq (sound, "mass"); p.furnaceEq = getEq (sound, "furnace");
    p.arcEq = getEq (sound, "arc"); p.feedbackEq = getEq (sound, "feedback");
    if (! validSound (p)) { error = "PRESET CONTAINS INVALID VALUES"; return false; }
    return true;
}

juce::File UserPresetStore::fileForName (const juce::String& name) const
{
    auto stem = juce::File::createLegalFileName (name.trim());
    if (stem.isEmpty()) stem = "User Preset";
    auto candidate = presetDirectory.getChildFile (stem + extension);
    for (int suffix = 2; candidate.existsAsFile(); ++suffix)
        candidate = presetDirectory.getChildFile (stem + " " + juce::String (suffix) + extension);
    return candidate;
}

UserPresetResult UserPresetStore::write (const juce::File& file, const juce::String& name, const Parameters& parameters)
{
    if (const auto error = validateName (name); error.isNotEmpty()) return { false, error, std::nullopt };
    if (! validSound (parameters)) return { false, "CURRENT SOUND CONTAINS INVALID VALUES", std::nullopt };
    if (! presetDirectory.isDirectory() && ! presetDirectory.createDirectory())
        return { false, "PRESET DIRECTORY COULD NOT BE CREATED", std::nullopt };
    const auto xml = makeTree (name, parameters).createXml();
    if (xml == nullptr) return { false, "PRESET COULD NOT BE SERIALIZED", std::nullopt };
    juce::TemporaryFile temporary (file);
    if (! temporary.getFile().replaceWithText (xml->toString(), false, false, "\n"))
        return { false, "PRESET COULD NOT BE WRITTEN", std::nullopt };
    if (! temporary.overwriteTargetFileWithTemporary())
        return { false, "PRESET COULD NOT BE COMMITTED", std::nullopt };
    UserPreset saved { name.trim(), file, parameters };
    refresh();
    return { true, {}, saved };
}

UserPresetResult UserPresetStore::save (const juce::String& name, const Parameters& parameters, bool overwrite)
{
    if (const auto error = validateName (name); error.isNotEmpty()) return { false, error, std::nullopt };
    if (const auto existing = findByName (name))
    {
        if (! overwrite) return { false, "PRESET ALREADY EXISTS", cachedPresets[*existing] };
        return write (cachedPresets[*existing].file, name.trim(), parameters);
    }
    return write (fileForName (name), name.trim(), parameters);
}

UserPresetResult UserPresetStore::load (const juce::File& file) const
{
    if (! file.existsAsFile()) return { false, "PRESET FILE DOES NOT EXIST", std::nullopt };
    const auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) return { false, "PRESET FILE IS MALFORMED", std::nullopt };
    UserPreset loaded; loaded.file = file;
    juce::String error;
    if (! readTree (juce::ValueTree::fromXml (*xml), loaded, error)) return { false, error, std::nullopt };
    return { true, {}, loaded };
}

UserPresetResult UserPresetStore::rename (const juce::File& file, const juce::String& newName)
{
    if (const auto error = validateName (newName); error.isNotEmpty()) return { false, error, std::nullopt };
    const auto loaded = load (file);
    if (! loaded.ok || ! loaded.preset) return loaded;
    if (const auto duplicate = findByName (newName); duplicate && cachedPresets[*duplicate].file != file)
        return { false, "PRESET ALREADY EXISTS", cachedPresets[*duplicate] };
    const auto destination = fileForName (newName);
    const auto written = write (destination, newName.trim(), loaded.preset->parameters);
    if (! written.ok) return written;
    if (destination != file && file.existsAsFile() && ! file.deleteFile())
    {
        destination.deleteFile(); refresh();
        return { false, "OLD PRESET FILE COULD NOT BE REMOVED", std::nullopt };
    }
    refresh();
    return load (destination);
}

UserPresetResult UserPresetStore::remove (const juce::File& file)
{
    if (! findByFile (file)) return { false, "USER PRESET NOT FOUND", std::nullopt };
    if (! file.deleteFile()) return { false, "PRESET COULD NOT BE DELETED", std::nullopt };
    refresh();
    return { true, {}, std::nullopt };
}

bool UserPresetStore::soundStatesEqual (const Parameters& a, const Parameters& b, float tolerance) noexcept
{
    const auto same = [tolerance] (float x, float y) { return std::abs (x - y) <= tolerance; };
    if (! same (a.breach,b.breach) || ! same (a.tear,b.tear) || ! same (a.rot,b.rot) || ! same (a.driveDb,b.driveDb)
        || ! same (a.overload,b.overload) || ! same (a.mix,b.mix) || ! same (a.range,b.range)
        || ! same (a.lowDb,b.lowDb) || ! same (a.midDb,b.midDb) || ! same (a.highDb,b.highDb)
        || ! same (a.outputDb,b.outputDb) || ! same (a.weld,b.weld) || a.surge != b.surge
        || a.limiterEnabled != b.limiterEnabled || ! same (a.limiterThresholdDb,b.limiterThresholdDb)
        || ! same (a.limiterCeilingDb,b.limiterCeilingDb) || a.reactorEnabled != b.reactorEnabled) return false;
    if (a.gateEnabled != b.gateEnabled || ! same (a.gateThresholdDb, b.gateThresholdDb)) return false;
    for (size_t i=0;i<4;++i) if (! same (a.reactorAmounts[i],b.reactorAmounts[i])) return false;
    const std::array<float,8> ac { a.reactorCharacter.massSaturation,a.reactorCharacter.massHarmonics,a.reactorCharacter.furnaceStarve,a.reactorCharacter.furnaceFold,a.reactorCharacter.arcXmod,a.reactorCharacter.arcFold,a.reactorCharacter.feedbackReturn,a.reactorCharacter.feedbackDamp };
    const std::array<float,8> bc { b.reactorCharacter.massSaturation,b.reactorCharacter.massHarmonics,b.reactorCharacter.furnaceStarve,b.reactorCharacter.furnaceFold,b.reactorCharacter.arcXmod,b.reactorCharacter.arcFold,b.reactorCharacter.feedbackReturn,b.reactorCharacter.feedbackDamp };
    for (size_t i=0;i<ac.size();++i) if (! same(ac[i],bc[i])) return false;
    const auto sameEq = [&] (const ReactorEqSettings& x, const ReactorEqSettings& y)
    { return same(x.hp,y.hp)&&same(x.focusFrequency,y.focusFrequency)&&same(x.focusGainDb,y.focusGainDb)&&same(x.focus2Frequency,y.focus2Frequency)&&same(x.focus2GainDb,y.focus2GainDb)&&same(x.lp,y.lp); };
    return sameEq(a.massEq,b.massEq)&&sameEq(a.furnaceEq,b.furnaceEq)&&sameEq(a.arcEq,b.arcEq)&&sameEq(a.feedbackEq,b.feedbackEq);
}
}

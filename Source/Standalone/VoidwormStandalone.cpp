#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "../PluginEditor.h"

namespace
{
class VoidwormStandaloneWindow final : public juce::DocumentWindow
{
public:
    VoidwormStandaloneWindow (juce::PropertySet* settings)
        : juce::DocumentWindow ("VOIDWORM", juce::Colour (0xff050505), 0),
          holder (std::make_unique<juce::StandalonePluginHolder> (settings, false))
    {
        setUsingNativeTitleBar (false);
        setTitleBarHeight (0);
        setDropShadowEnabled (true);
        setOpaque (true);

        content = new EditorContent (*this);
        setContentOwned (content, true);
        if (auto* editor = content->editor.get())
            setResizable (editor->isResizable(), false);

        if (settings != nullptr)
        {
            const auto savedX = settings->getIntValue ("windowX", -100000);
            const auto savedY = settings->getIntValue ("windowY", -100000);
            if (savedX > -100000 && savedY > -100000)
                setTopLeftPosition (savedX, savedY);
            else
                centreWithSize (getWidth(), getHeight());
        }
        else
            centreWithSize (getWidth(), getHeight());
    }

    ~VoidwormStandaloneWindow() override
    {
        if (auto* settings = holder->settings.get())
        {
            settings->setValue ("windowX", getX());
            settings->setValue ("windowY", getY());
        }
        holder->savePluginState();
        holder->stopPlaying();
        clearContentComponent();
        holder = nullptr;
    }

    void closeButtonPressed() override
    {
        holder->savePluginState();
        juce::JUCEApplicationBase::quit();
    }

private:
    class EditorContent final : public juce::Component,
                                private juce::ComponentListener,
                                private juce::Value::Listener
    {
    public:
        explicit EditorContent (VoidwormStandaloneWindow& window) : owner (window)
        {
            editor.reset (owner.holder->processor->hasEditor()
                ? owner.holder->processor->createEditorIfNeeded()
                : new juce::GenericAudioProcessorEditor (*owner.holder->processor));
            if (editor != nullptr)
            {
                editor->addComponentListener (this);
                addAndMakeVisible (*editor);
                setSize (editor->getWidth(), editor->getHeight());
                editor->setTopLeftPosition (0, 0);
            }

            if (auto* voidworm = dynamic_cast<VoidwormAudioProcessorEditor*> (editor.get()))
            {
                voidworm->configureStandaloneShell ([safe = juce::Component::SafePointer<VoidwormAudioProcessorEditor> (voidworm)]
                {
                    if (safe != nullptr)
                        if (auto* activeHolder = juce::StandalonePluginHolder::getInstance())
                            activeHolder->showAudioSettingsDialog();
                });
            }

            muteInput.referTo (owner.holder->getMuteInputValue());
            muteInput.addListener (this);
            showInputProtectionToastIfNeeded();
        }

        ~EditorContent() override
        {
            muteInput.removeListener (this);
            if (editor != nullptr)
            {
                editor->removeComponentListener (this);
                owner.holder->processor->editorBeingDeleted (editor.get());
                editor = nullptr;
            }
        }

        void resized() override
        {
            if (editor != nullptr)
                editor->setBounds (getLocalBounds());
        }

        void componentMovedOrResized (juce::Component&, bool, bool wasResized) override
        {
            if (! syncingBounds && wasResized && editor != nullptr)
            {
                const juce::ScopedValueSetter<bool> guard (syncingBounds, true);
                setSize (editor->getWidth(), editor->getHeight());
            }
        }

        void valueChanged (juce::Value&) override { showInputProtectionToastIfNeeded(); }

        void showInputProtectionToastIfNeeded()
        {
            if (owner.holder->getProcessorHasPotentialFeedbackLoop() && (bool) muteInput.getValue())
                if (auto* voidworm = dynamic_cast<VoidwormAudioProcessorEditor*> (editor.get()))
                    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<VoidwormAudioProcessorEditor> (voidworm)]
                    {
                        if (safe != nullptr)
                            safe->showStandaloneToast ("INPUT MUTED - FEEDBACK PROTECTION");
                    });
        }

        VoidwormStandaloneWindow& owner;
        std::unique_ptr<juce::AudioProcessorEditor> editor;
        juce::Value muteInput;
        bool syncingBounds = false;
    };

    std::unique_ptr<juce::StandalonePluginHolder> holder;
    EditorContent* content = nullptr;
};

class VoidwormStandaloneApplication final : public juce::JUCEApplication
{
public:
    VoidwormStandaloneApplication()
    {
        juce::PropertiesFile::Options options;
        // Device/window state is intentionally separate from the editor's UI
        // preferences file, so each PropertiesFile has a single writer.
        options.applicationName = "VOIDWORM Standalone";
        options.filenameSuffix = ".settings";
        options.folderName = "LWNX DSP/VOIDWORM";
        options.osxLibrarySubFolder = "Application Support";
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        properties.setStorageParameters (options);
    }

    const juce::String getApplicationName() override { return "VOIDWORM"; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (window != nullptr)
        {
            window->setMinimised (false);
            window->setVisible (true);
            window->toFront (true);
            window->grabKeyboardFocus();
        }
    }

    void initialise (const juce::String&) override
    {
        if (! juce::Desktop::getInstance().getDisplays().displays.isEmpty())
        {
            window = std::make_unique<VoidwormStandaloneWindow> (properties.getUserSettings());
            window->setVisible (true);
        }
    }

    void shutdown() override
    {
        window = nullptr;
        properties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            holder->savePluginState();
        if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
            juce::Timer::callAfterDelay (100, []
            {
                if (auto* app = juce::JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        else
            quit();
    }

private:
    juce::ApplicationProperties properties;
    std::unique_ptr<VoidwormStandaloneWindow> window;
};
}

JUCE_CREATE_APPLICATION_DEFINE (VoidwormStandaloneApplication)

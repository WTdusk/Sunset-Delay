#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include "PluginEditor.h"

namespace
{

constexpr int kMuteStripHeight = 42;

class ClaySettingsPanel final : public juce::Component
{
public:
    explicit ClaySettingsPanel (juce::StandalonePluginHolder& holder)
        : deviceSelector (holder.deviceManager,
                          0,
                          holder.processor != nullptr ? holder.processor->getMainBusNumInputChannels() : 2,
                          0,
                          holder.processor != nullptr ? holder.processor->getMainBusNumOutputChannels() : 2,
                          holder.processor != nullptr && holder.processor->acceptsMidi(),
                          holder.processor != nullptr && holder.processor->producesMidi(),
                          true,
                          false),
          muteButton ("Mute audio input")
    {
        setOpaque (true);
        deviceSelector.setItemHeight (34);
        addAndMakeVisible (deviceSelector);

        muteButton.setClickingTogglesState (true);
        muteButton.getToggleStateValue().referTo (holder.getMuteInputValue());
        addAndMakeVisible (muteButton);

        // Lay the selector out at the dialog width so it reports its real content height
        // before the window is created. A tall placeholder leaves a blank clay field.
        deviceSelector.setBounds (18, 64, 524, 80);
        fitToSelector();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xffe7e2db));
    }

    void resized() override
    {
        if (adjusting)
            return;

        const juce::ScopedValueSetter<bool> lock (adjusting, true);
        auto area = getLocalBounds().reduced (18, 16);
        muteButton.setBounds (area.removeFromTop (36).removeFromLeft (320));
        area.removeFromTop (8);
        // The selector sizes itself to its rows, then JUCE adds a blank trailing row.
        // Give it its real height; fitToSelector clips that blank row out of the dialog.
        deviceSelector.setBounds (area.getX(), area.getY(), area.getWidth(),
                                  juce::jmax (72, deviceSelector.getHeight()));
    }

    void childBoundsChanged (juce::Component* child) override
    {
        if (adjusting || child != &deviceSelector)
            return;

        fitToSelector();
    }

private:
    void fitToSelector()
    {
        // 40px is the selector's own trailing spacer (one item row). Keeping it
        // inside the dialog reads as an empty clay field under the last control.
        const int visibleSelector = juce::jmax (72, deviceSelector.getHeight() - 40);
        const int wanted = 16 + 36 + 8 + visibleSelector + 14;
        if (getWidth() != 560 || std::abs (getHeight() - wanted) > 2)
            setSize (560, wanted);
    }

    juce::AudioDeviceSelectorComponent deviceSelector;
    juce::ToggleButton muteButton;
    bool adjusting = false;
};

class ClayContent final : public juce::Component,
                          private juce::Value::Listener,
                          private juce::Button::Listener,
                          private juce::ComponentListener
{
public:
    ClayContent (juce::StandalonePluginHolder& holderIn, std::function<void()> showSettingsIn)
        : holder (holderIn),
          showSettings (std::move (showSettingsIn)),
          editor (holder.processor != nullptr && holder.processor->hasEditor()
                      ? holder.processor->createEditorAndMakeActive()
                      : nullptr)
    {
        setOpaque (true);

        muteLabel.setText ("Input is muted to avoid a feedback loop", juce::dontSendNotification);
        muteLabel.setJustificationType (juce::Justification::centredLeft);
        muteLabel.setInterceptsMouseClicks (false, false);
        muteLabel.setColour (juce::Label::textColourId, juce::Colour (0xff514c46));
        muteLabel.setFont (juce::Font (juce::FontOptions ("Inter", "Medium", 14.0f)));
        addChildComponent (muteLabel);

        settingsButton.setButtonText ("Settings");
        settingsButton.addListener (this);
        settingsButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addChildComponent (settingsButton);

        if (editor != nullptr)
        {
            editor->addComponentListener (this);
            addAndMakeVisible (editor.get());
            fitToEditor();
        }

        muted.referTo (holder.getMuteInputValue());
        if (holder.getProcessorHasPotentialFeedbackLoop())
        {
            muted.addListener (this);
            showMute = static_cast<bool> (muted.getValue());
        }

        muteLabel.setVisible (showMute);
        settingsButton.setVisible (showMute);
        fitToEditor();
    }

    ~ClayContent() override
    {
        if (editor != nullptr && holder.processor != nullptr)
        {
            editor->removeComponentListener (this);
            holder.processor->editorBeingDeleted (editor.get());
            editor = nullptr;
        }
    }

    juce::AudioProcessorEditor* getEditor() const noexcept { return editor.get(); }
    int getMuteStrip() const noexcept { return showMute ? kMuteStripHeight : 0; }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xffe7e2db));

        if (! showMute)
            return;

        auto well = getLocalBounds().removeFromTop (kMuteStripHeight).reduced (10, 5).toFloat();
        const float radius = well.getHeight() * 0.5f;
        g.setColour (juce::Colour (0xffddd6cd));
        g.fillRoundedRectangle (well, radius);

        juce::ColourGradient shade (juce::Colour (0xffb3aa9f).withAlpha (0.34f), well.getX(), well.getY(),
                                    juce::Colours::transparentBlack, well.getCentreX(), well.getCentreY(), true);
        g.setGradientFill (shade);
        g.fillRoundedRectangle (well, radius);

        juce::ColourGradient light (juce::Colours::white.withAlpha (0.28f), well.getRight(), well.getBottom(),
                                    juce::Colours::transparentWhite, well.getCentreX(), well.getCentreY(), true);
        g.setGradientFill (light);
        g.fillRoundedRectangle (well, radius);
    }

    void resized() override
    {
        if (! fitting)
            layoutChildren();
    }

private:
    void layoutChildren()
    {
        auto area = getLocalBounds();

        if (showMute)
        {
            auto strip = area.removeFromTop (kMuteStripHeight).reduced (16, 6);
            settingsButton.setBounds (strip.removeFromRight (108));
            strip.removeFromRight (8);
            muteLabel.setBounds (strip);
            settingsButton.toFront (false);
            muteLabel.toFront (false);
        }

        if (editor != nullptr)
            editor->setBounds (area);
    }

    void fitToEditor()
    {
        if (editor == nullptr || fitting)
            return;

        const juce::ScopedValueSetter<bool> lock (fitting, true);
        const int width = juce::jmax (1, editor->getWidth());
        const int height = juce::jmax (1, editor->getHeight()) + (showMute ? kMuteStripHeight : 0);

        if (getWidth() != width || getHeight() != height)
            setSize (width, height);

        layoutChildren();
    }

    void componentMovedOrResized (juce::Component&, bool, bool) override
    {
        fitToEditor();
    }

    void valueChanged (juce::Value&) override
    {
        showMute = static_cast<bool> (muted.getValue());
        muteLabel.setVisible (showMute);
        settingsButton.setVisible (showMute);
        fitToEditor();
        if (onMuteChanged != nullptr)
            onMuteChanged (showMute);
    }

    void buttonClicked (juce::Button*) override
    {
        if (showSettings != nullptr)
            showSettings();
    }

    juce::StandalonePluginHolder& holder;
    std::function<void()> showSettings;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::Label muteLabel;
    juce::TextButton settingsButton;
    juce::Value muted;
    bool showMute = false;
    bool fitting = false;

public:
    std::function<void (bool)> onMuteChanged;
};

class ClayStandaloneWindow final : public juce::DocumentWindow,
                                   private juce::Button::Listener
{
public:
    ClayStandaloneWindow (const juce::String& title, juce::StandalonePluginHolder* holderIn)
        : juce::DocumentWindow (title, juce::Colour (0xffe7e2db),
                                juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton),
          holder (holderIn)
    {
        setUsingNativeTitleBar (false);
        setDropShadowEnabled (true);
        setTitleBarHeight (48);
        setTitleBarTextCentred (true);
        setConstrainer (&frameConstrainer);

        optionsButton.setButtonText ("Options");
        optionsButton.setTriggeredOnMouseDown (true);
        optionsButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        optionsButton.addListener (this);
        addAndMakeVisible (optionsButton);

        auto* content = new ClayContent (*holder, [this] { showSettings(); });
        content->onMuteChanged = [this] (bool shown)
        {
            frameConstrainer.bannerHeight = shown ? kMuteStripHeight : 0;
        };
        frameConstrainer.bannerHeight = content->getMuteStrip();
        frameConstrainer.editor = content->getEditor();
        frameConstrainer.window = this;
        setContentOwned (content, true);

        if (auto* editor = content->getEditor())
            setResizable (editor->isResizable(), false);

        restorePosition();
    }

    ~ClayStandaloneWindow() override
    {
        frameConstrainer.editor = nullptr;
        clearContentComponent();
    }

    void closeButtonPressed() override
    {
        savePosition();
        if (holder != nullptr)
            holder->savePluginState();
        juce::JUCEApplicationBase::quit();
    }

    void resized() override
    {
        juce::DocumentWindow::resized();
        auto title = getTitleBarArea();
        optionsButton.setBounds (title.getX() + 12, title.getY() + 7, 96, juce::jmax (28, title.getHeight() - 14));
    }

private:
    class FrameConstrainer final : public juce::BorderedComponentBoundsConstrainer
    {
    public:
        juce::ComponentBoundsConstrainer* getWrappedConstrainer() const override
        {
            return editor != nullptr ? editor->getConstrainer() : nullptr;
        }

        juce::BorderSize<int> getAdditionalBorder() const override
        {
            auto border = window != nullptr ? window->getContentComponentBorder() : juce::BorderSize<int>();
            if (bannerHeight > 0)
                border.setTop (border.getTop() + bannerHeight);
            return border;
        }

        juce::AudioProcessorEditor* editor = nullptr;
        juce::DocumentWindow* window = nullptr;
        int bannerHeight = 0;
    };

    void restorePosition()
    {
        int x = -1;
        int y = -1;

        if (holder != nullptr)
            if (auto* props = holder->settings.get())
            {
                x = props->getIntValue ("windowX", -1);
                y = props->getIntValue ("windowY", -1);
            }

        if (x >= 0 && y >= 0)
            setTopLeftPosition (x, y);
        else
            centreWithSize (getWidth(), getHeight());
    }

    void savePosition()
    {
        if (holder == nullptr)
            return;

        if (auto* props = holder->settings.get())
        {
            props->setValue ("windowX", getX());
            props->setValue ("windowY", getY());
        }
    }

    void showSettings()
    {
        if (holder == nullptr)
            return;

        auto panel = std::make_unique<ClaySettingsPanel> (*holder);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (panel.release());
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = juce::Colour (0xffe7e2db);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.componentToCentreAround = this;
        options.launchAsync();
    }

    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Audio settings...");
        menu.addSeparator();
        menu.addItem (2, "Save current state...");
        menu.addItem (3, "Load a saved state...");
        menu.addSeparator();
        menu.addItem (4, "Reset to default state");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&optionsButton),
                            [this] (int result)
                            {
                                if (holder == nullptr || result == 0)
                                    return;

                                if (result == 1)
                                    showSettings();
                                else if (result == 2)
                                    holder->askUserToSaveState();
                                else if (result == 3)
                                    holder->askUserToLoadState();
                                else if (result == 4)
                                    resetState();
                            });
    }

    void resetState()
    {
        if (holder == nullptr)
            return;

        savePosition();
        holder->stopPlaying();
        clearContentComponent();
        frameConstrainer.editor = nullptr;
        holder->deletePlugin();

        if (auto* props = holder->settings.get())
            props->removeValue ("filterState");

        holder->createPlugin();

        auto* content = new ClayContent (*holder, [this] { showSettings(); });
        content->onMuteChanged = [this] (bool shown)
        {
            frameConstrainer.bannerHeight = shown ? kMuteStripHeight : 0;
        };
        frameConstrainer.bannerHeight = content->getMuteStrip();
        frameConstrainer.editor = content->getEditor();
        setContentOwned (content, true);
        holder->startPlaying();
    }

    void buttonClicked (juce::Button* button) override
    {
        if (button == &optionsButton)
            showMenu();
    }

    juce::StandalonePluginHolder* holder = nullptr;
    juce::TextButton optionsButton;
    FrameConstrainer frameConstrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClayStandaloneWindow)
};

} // namespace

namespace juce
{

class ClayStandaloneApp final : public juce::JUCEApplication
{
public:
    ClayStandaloneApp()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "Sunset Delay";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
       #else
        options.folderName = "";
       #endif
        appProperties.setStorageParameters (options);
    }

    const juce::String getApplicationName() override { return "Sunset Delay"; }
    const juce::String getApplicationVersion() override { return "1.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    void initialise (const juce::String&) override
    {
        chrome.applyChromeColours();
        juce::Desktop::getInstance().setDefaultLookAndFeel (&chrome);

        pluginHolder = std::make_unique<juce::StandalonePluginHolder> (appProperties.getUserSettings(), false);
        mainWindow = std::make_unique<ClayStandaloneWindow> (getApplicationName(), pluginHolder.get());
        mainWindow->setVisible (true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        pluginHolder = nullptr;
        juce::Desktop::getInstance().setDefaultLookAndFeel (nullptr);
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (pluginHolder != nullptr)
            pluginHolder->savePluginState();

        quit();
    }

private:
    guitar_delay::PedalLookAndFeel chrome;
    juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;
    std::unique_ptr<ClayStandaloneWindow> mainWindow;
};

} // namespace juce

juce::JUCEApplicationBase* juce_CreateApplication()
{
    return new juce::ClayStandaloneApp();
}

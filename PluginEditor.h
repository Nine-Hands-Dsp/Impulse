#pragma once
#include "PluginProcessor.h"

class ImpulseContent;

class ImpulseEditor : public juce::AudioProcessorEditor
{
public:
    explicit ImpulseEditor (ImpulseProcessor&);
    ~ImpulseEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

    int  getScaleIndex() const;          // 0, 1, 2 = 60 % / 75 % / 100 %  (-1 = taille libre)
    void setScaleIndex (int idx);        // -1 = taille par defaut choisie (menu Default size)
    void applyScaleIndex (int idx);
    static constexpr int numSizes = 3;
    ImpulseProcessor& getProc() { return proc; }
    void openColourPicker (juce::Component* near);   // nuancier (couleur d'accent)
    void chooseIRFolder();                           // dossier des IR par defaut (= dossier de la bibliotheque)
    juce::LookAndFeel* getMenuLook() { return menuLook.get(); }   // menus au design d'Octopus (MenuStyle.h)
    void openLibrary (int slot);                     // bibliotheque d'IR (fenetre d'Ampire)
    bool stacked = false;                            // vue collee sous Ampire (mode stack) : la bibliotheque d'IR reste dans Impulse
   #if IMPULSE_TOOL
    void previewLibrary (int tab, bool signedIn);    // outil Linux : captures de la bibliotheque
   #endif

private:
    friend class ImpulseContent;         // son timer surveille le moteur de rendu (anti-gel v2)
    void useSoftwareRenderer();
    void checkRenderer();
    void resetRenderer();
    void* lastPeer = nullptr;
    juce::uint32 lastRendererCheck = 0;
    ImpulseProcessor& proc;
    std::unique_ptr<juce::LookAndFeel> menuLook;      // menus au design d'Octopus (MenuStyle.h), detruite apres le contenu
    std::unique_ptr<ImpulseContent> content;
    std::unique_ptr<juce::FileChooser> folderChooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpulseEditor)
};

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

private:
    friend class ImpulseContent;         // son timer surveille le moteur de rendu (anti-gel v2)
    void useSoftwareRenderer();
    void checkRenderer();
    void resetRenderer();
    void* lastPeer = nullptr;
    juce::uint32 lastRendererCheck = 0;
    ImpulseProcessor& proc;
    std::unique_ptr<ImpulseContent> content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpulseEditor)
};

#pragma once
#include <juce_core/juce_core.h>
#include "NineHandsFolder.h"

// NINE HANDS | DSP : dossier de l'utilisateur du plugin (taille par defaut des nouvelles fenetres, couleur,
// dossier des IR, presets), commun a toutes ses instances :
//   Windows : %APPDATA%\NINE HANDS DSP\<Plugin>\
//   macOS   : ~/Library/Application Support/NINE HANDS DSP/<Plugin>/
// La marque s'appelait NINE HANDS | DSP : au premier lancement, l'ancien dossier « NINE HANDS | DSP/<Plugin> » est recopie
// (reglages et presets conserves).
namespace WigaWindowSize
{
    inline juce::File folder (const juce::String& plugin)
    {
        const auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
        const auto dir = base.getChildFile ("NINE HANDS DSP").getChildFile (plugin);
        if (! dir.isDirectory())
        {
            const auto old = base.getChildFile (nhd::folderName()).getChildFile (plugin);
            if (old.isDirectory()) old.copyDirectoryTo (dir);
        }
        return dir;
    }

    inline juce::File file (const juce::String& plugin) { return folder (plugin).getChildFile ("settings.xml"); }

    // indice de la taille par defaut (0 si rien n'a ete choisi ou fichier illisible)
    inline int defaultIndex (const juce::String& plugin, int numSizes)
    {
        const auto f = file (plugin);
        if (f.existsAsFile())
            if (auto x = juce::XmlDocument::parse (f))
                return juce::jlimit (0, juce::jmax (0, numSizes - 1), x->getIntAttribute ("defaultSize", 0));
        return 0;
    }

    inline void setDefaultIndex (const juce::String& plugin, int idx)
    {
        const auto f = file (plugin);
        juce::XmlElement x ("NineHandsSettings");
        if (f.existsAsFile())
            if (auto old = juce::XmlDocument::parse (f))
                x = *old;
        x.setAttribute ("defaultSize", juce::jmax (0, idx));
        f.getParentDirectory().createDirectory();
        x.writeTo (f);
    }
}

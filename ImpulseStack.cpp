// ============================================================================
// Impulse - mode « stack » (voir NhdStack.h) : registre des instances d'Impulse et des vues affichees sous Ampire.
// Tout se passe sur le thread de messages (createView / destroyView / processeur cree ou detruit).
// ============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "NhdStack.h"
#include <map>

namespace
{
    struct View
    {
        int instance = 0;
        std::unique_ptr<juce::AudioProcessorEditor> editor;
    };

    struct Registry
    {
        std::map<int, ImpulseProcessor*> instances;
        std::map<int, View> views;
        int nextInstance = 1, nextView = 1;
        using RegisterFn = int (*) (const char*, void*);
        RegisterFn pluginRegister = nullptr;     // plugin_register de REAPER (garde pour se desinscrire)

        ~Registry() { unpublish(); }
        void unpublish();
    };

    Registry& reg()
    {
        static Registry r;   // detruit au dechargement du module : desinscription aupres de REAPER
        return r;
    }

    // ---- table C publiee aupres de REAPER
    int findOnTrack (void* track)
    {
        if (track == nullptr) return 0;
        for (auto& [id, p] : reg().instances)
            if (p != nullptr && p->reaperTrack() == track) return id;
        return 0;
    }

    void place (juce::AudioProcessorEditor& ed, int x, int y, int w, int h, double scale)
    {
       #if JUCE_WINDOWS
        ed.setScaleFactor ((float) juce::jmax (0.5, scale));   // meme echelle (DPI) que la fenetre d'Ampire
       #else
        juce::ignoreUnused (scale);
       #endif
        ed.setBounds (x, y, juce::jmax (1, w), juce::jmax (1, h));
    }

    int createView (int instance, void* parent, int x, int y, int w, int h, double scale)
    {
        auto& r = reg();
        auto it = r.instances.find (instance);
        if (it == r.instances.end() || it->second == nullptr || parent == nullptr) return 0;
        std::unique_ptr<juce::AudioProcessorEditor> ed (it->second->createEditor());
        if (ed == nullptr) return 0;
        ed->setResizable (false, false);              // la taille est pilotee par Ampire (pas de poignee dans le stack)
        if (auto* ie = dynamic_cast<ImpulseEditor*> (ed.get())) ie->stacked = true;   // bibliotheque d'IR gardee dans Impulse
        ed->setResizeLimits (1, 1, 100000, 100000);
        ed->setOpaque (true);
        place (*ed, x, y, w, h, scale);
        ed->addToDesktop (0, parent);                 // fenetre enfant de celle d'Ampire
        place (*ed, x, y, w, h, scale);
        ed->setVisible (true);
        const int id = r.nextView++;
        r.views[id] = View { instance, std::move (ed) };
        return id;
    }

    void setViewBounds (int view, int x, int y, int w, int h, double scale)
    {
        auto& r = reg();
        auto it = r.views.find (view);
        if (it != r.views.end() && it->second.editor != nullptr) place (*it->second.editor, x, y, w, h, scale);
    }

    void destroyView (int view)
    {
        auto& r = reg();
        auto it = r.views.find (view);
        if (it == r.views.end()) return;
        auto ed = std::move (it->second.editor);
        r.views.erase (it);
        if (ed != nullptr) { ed->setVisible (false); ed->removeFromDesktop(); ed.reset(); }
    }

    int viewAlive (int view)
    {
        auto& r = reg();
        auto it = r.views.find (view);
        return it != r.views.end() && it->second.editor != nullptr ? 1 : 0;
    }

    NhdStackApi gApi { 1, findOnTrack, createView, setViewBounds, destroyView, viewAlive };

    void Registry::unpublish()
    {
        if (pluginRegister != nullptr) pluginRegister ("-API_" NHD_STACK_API_NAME, (void*) &gApi);
        pluginRegister = nullptr;
    }
}

// ---------------------------------------------------------------- cote processeur
void ImpulseProcessor::stackRegister()
{
    auto& r = reg();
    for (auto& [id, p] : r.instances) if (p == this) return;
    stackId = r.nextInstance++;
    r.instances[stackId] = this;
}

void ImpulseProcessor::stackUnregister()
{
    auto& r = reg();
    // vues de cette instance (affichees sous Ampire) : detruites avant le processeur ; Ampire le voit (viewAlive = 0)
    for (auto it = r.views.begin(); it != r.views.end();)
    {
        if (it->second.instance == stackId)
        {
            auto ed = std::move (it->second.editor);
            it = r.views.erase (it);
            if (ed != nullptr) { ed->setVisible (false); ed->removeFromDesktop(); ed.reset(); }
        }
        else ++it;
    }
    r.instances.erase (stackId);
    stackId = 0;
}

void ImpulseProcessor::setIHostApplication (Steinberg::FUnknown* app)
{
    if (! reaperHost->attach (app)) return;
    auto& r = reg();
    if (r.pluginRegister == nullptr)
        if (auto fn = reinterpret_cast<Registry::RegisterFn> (reaperHost->api ("plugin_register")))
        {
            fn ("API_" NHD_STACK_API_NAME, (void*) &gApi);   // une seule fois par module
            r.pluginRegister = fn;
        }
}

void* ImpulseProcessor::reaperTrack() const
{
    // une piste seulement (pas les effets de prise) : l'Impulse de la chaine de la piste d'Ampire
    return reaperHost->isReaper() && reaperHost->take() == nullptr ? reaperHost->track() : nullptr;
}

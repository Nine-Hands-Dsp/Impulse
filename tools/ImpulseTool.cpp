// Outil de verification Linux : captures PNG de l'interface + rendu audio hors ligne.
//   ImpulseTool render <out.png> <scale> [irA] [irB] [mix] [overload] [variphi]
//   ImpulseTool process <in.wav> <out.wav> <rate> <block> [irA] [irB] [mix] [overload] [variphi] [normalize]
#include "../MenuStyle.h"
#include "../PluginProcessor.h"
#include "../PluginEditor.h"
#include "../NhdStack.h"
#include <map>
#include "../ColourPicker.h"

static void setP (ImpulseProcessor& p, const char* id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}


// ---- REAPER simule (mode stacktest) : plugin_register / plugin_getapi + piste
static std::map<std::string, void*> gSimApis;
static int simRegister (const char* name, void* p)
{
    std::string n (name);
    if (! n.empty() && n[0] == '-') { gSimApis.erase (n.substr (1)); return 1; }
    gSimApis[n] = p; return 1;
}
static void* simGetApi (const char* name)
{
    auto it = gSimApis.find (std::string ("API_") + name);
    return it != gSimApis.end() ? it->second : nullptr;
}
class SimReaper : public NhdReaperSdk::IReaperHostApplication
{
public:
    explicit SimReaper (void* tr) : trk (tr) {}
    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) override
    {
        const Steinberg::TUID rid = INLINE_UID (0x79655E36, 0x77EE4267, 0xA573FEF7, 0x4912C27C);
        if (Steinberg::FUnknownPrivate::iidEqual (iid, rid)) { addRef(); *obj = this; return Steinberg::kResultOk; }
        *obj = nullptr; return Steinberg::kNoInterface;
    }
    Steinberg::uint32 PLUGIN_API addRef() override { return ++refs; }
    Steinberg::uint32 PLUGIN_API release() override { return --refs; }
    void* PLUGIN_API getReaperApi (Steinberg::CStringA f) override
    {
        if (std::string (f) == "plugin_register") return (void*) &simRegister;
        if (std::string (f) == "plugin_getapi") return (void*) &simGetApi;
        return nullptr;
    }
    void* PLUGIN_API getReaperParent (Steinberg::uint32 w) override { return w == 1 ? trk : nullptr; }
    void* PLUGIN_API reaperExtended (Steinberg::uint32, void*, void*, void*) override { return nullptr; }
    void* trk; Steinberg::uint32 refs = 1;
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) return 1;
    const juce::String mode (argv[1]);
    auto arg = [&] (int i) { return i < argc ? juce::String (argv[i]) : juce::String(); };

    ImpulseProcessor proc;
    if (mode == "render")
    {
        const float scale = arg (3).isNotEmpty() ? arg (3).getFloatValue() : 2.0f;
        if (arg (4).isNotEmpty()) proc.loadIR (0, juce::File (arg (4)));
        if (arg (5).isNotEmpty()) proc.loadIR (1, juce::File (arg (5)));
        if (arg (6).isNotEmpty()) setP (proc, "mix", arg (6).getFloatValue());
        if (arg (7).isNotEmpty()) setP (proc, "overload", arg (7).getFloatValue());
        if (arg (8).isNotEmpty()) setP (proc, "variphi", arg (8).getFloatValue());
        if (arg (9).isNotEmpty()) setP (proc, "gainB", ImpulseProcessor::faderDbToPos (arg (9).getFloatValue()));
        proc.prepareToPlay (48000.0, 256);
        if (arg (12) == "states") { setP (proc, "muteA", 1.0f); setP (proc, "phaseB", 1.0f); setP (proc, "active", 0.0f); }   // etats allumes / eteints
        if (arg (10).isNotEmpty()) proc.accentColour = 0xff000000u | (juce::uint32) arg (10).getHexValue64();
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        // fenetre a la taille demandee (le fond est calcule a cette taille, comme dans l'hote), capture a 1:1
        ed->setSize ((int) std::lround (ed->getWidth() * scale), (int) std::lround (ed->getHeight() * scale));
        if (arg (11) == "picker") if (auto* ie = dynamic_cast<ImpulseEditor*> (ed.get())) ie->openColourPicker (nullptr);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (600);   // timer : cache du fond
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        if (arg (12) == "blur")   // apercu du flou de mouvement de la couronne : OVERLOAD tourne de +4 % entre deux images
        {
            setP (proc, "overload", arg (7).getFloatValue() + 4.0f);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
            img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        }
        juce::File out (arg (2));
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("editor %d x %d -> %s\n", ed->getWidth(), ed->getHeight(), out.getFullPathName().toRawUTF8());
        ed.reset();
        return 0;
    }
    if (mode == "menu")         // ImpulseTool menu <out.png> : apercu du menu clic droit (MenuStyle.h, design d'Octopus)
    {
        MenuStyle::MenuLnf lf;
        const char* items[] = { "MIDI Learn: Mix A/B (-)", "Clear MIDI: Mix A/B", "-", "#IR A: Cab 4x12 V30 SM57 A.wav", "Load IR...", "Previous IR in folder",
                                "Next IR in folder", "Show in folder", "Clear", "-", ">Plugin size", ">Presets", ">Settings", ">MIDI" };
        const int W = 380, n = (int) (sizeof (items) / sizeof (items[0]));
        int H = 14 + 52 + 11;
        for (auto* it : items) H += it[0] == '-' ? 11 : 32;
        juce::Image img (juce::Image::ARGB, W * 2, H * 2, true);
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::scale (2.0f));
        lf.drawPopupMenuBackground (g, W, H);
        MenuStyle::Header hd (MenuStyle::fonts(), MenuStyle::Badge::ofUi (MenuStyle::icoParams), "Impulse",
                              juce::String::fromUTF8 ("NINE HANDS | DSP  \xc2\xb7  v" JucePlugin_VersionString));
        hd.setBounds (7, 7, W - 14, 52);
        { juce::Graphics::ScopedSaveState ss (g); g.setOrigin (7, 7); hd.paint (g); }
        int y = 7 + 52;
        lf.drawPopupMenuItem (g, { 7, y, W - 14, 11 }, true, true, false, false, false, {}, {}, nullptr, nullptr); y += 11;
        for (int i = 0; i < n; ++i)
        {
            const juce::String t (items[i]);
            if (t == "-") { lf.drawPopupMenuItem (g, { 7, y, W - 14, 11 }, true, true, false, false, false, {}, {}, nullptr, nullptr); y += 11; continue; }
            if (t.startsWith ("#")) { lf.drawPopupMenuSectionHeader (g, { 7, y, W - 14, 32 }, t.substring (1)); y += 32; continue; }
            const bool sub = t.startsWith (">");
            lf.drawPopupMenuItem (g, { 7, y, W - 14, 32 }, false, true, i == 4, false, sub, sub ? t.substring (1) : t, {}, nullptr, nullptr);
            y += 32;
        }
        juce::File out (arg (2)); out.deleteFile();
        juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (mode == "menuscale")
    {
        // echelle des menus clic droit : avant (cible dans le contenu reduit) / maintenant (fenetre du plugin)
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto* inner = ed->getChildComponent (0) != nullptr && ed->getChildComponent (0)->getNumChildComponents() > 0
                        ? ed->getChildComponent (0)->getChildComponent (0) : nullptr;
        std::printf ("fenetre %d px : echelle menu avant = %.2f, maintenant = %.2f\n", ed->getWidth(),
                     inner != nullptr ? juce::Component::getApproximateScaleFactorForComponent (inner) : -1.0f,
                     juce::Component::getApproximateScaleFactorForComponent (ed.get()));
        return 0;
    }
    if (mode == "presettest")
    {
        proc.loadIR (0, juce::File (arg (2)));
        proc.loadIR (1, juce::File (arg (3)));
        setP (proc, "mix", -50.0f); setP (proc, "overload", 30.0f);
        const juce::File f ("/tmp/impulse_test/My Export.impulse");
        f.getParentDirectory().createDirectory();
        const bool ex = proc.exportPreset (f);
        proc.initPreset();
        std::printf ("export=%d after init: A loaded=%d mix=%.0f\n", (int) ex, (int) proc.getInfo (0).loaded, proc.apvts.getRawParameterValue ("mix")->load());
        const bool im = proc.importPreset (f);
        std::printf ("import=%d preset='%s' A='%s' B='%s' mix=%.0f overload=%.0f\n", (int) im, proc.getPresetName().toRawUTF8(),
                     proc.getInfo (0).name.toRawUTF8(), proc.getInfo (1).name.toRawUTF8(),
                     proc.apvts.getRawParameterValue ("mix")->load(), proc.apvts.getRawParameterValue ("overload")->load());
        const bool im2 = proc.importPreset (f);   // deuxieme import : nom numerote
        std::printf ("import again=%d preset='%s'\n", (int) im2, proc.getPresetName().toRawUTF8());
        return ex && im && im2 ? 0 : 6;
    }
    if (mode == "pickertest")
    {
        // reproduit : nuancier ouvert, case du code avec le focus, clic sur une couleur rapide, puis OK
        proc.accentColour = 0xff4fc3f7;
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto* ie = dynamic_cast<ImpulseEditor*> (ed.get());
        ie->openColourPicker (nullptr);
        juce::Component* picker = nullptr;
        std::function<void (juce::Component*)> find = [&] (juce::Component* c)
        {
            for (auto* ch : c->getChildren()) { if (dynamic_cast<WigaColourPicker*> (ch) != nullptr) picker = ch; find (ch); }
        };
        find (ed.get());
        if (picker == nullptr) { std::printf ("no picker\n"); return 4; }
        const float k = (float) picker->getWidth() / 320.0f;
        auto click = [&] (float x, float y)
        {
            auto src = juce::Desktop::getInstance().getMainMouseSource();
            const juce::Point<float> pos (x * k, y * k);
            juce::MouseEvent e (src, pos, {}, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, picker, picker, juce::Time::getCurrentTime(), pos, juce::Time::getCurrentTime(), 1, false);
            picker->mouseDown (e);
        };
        click (16.0f + 27.0f * 7 + 10.0f, 46.0f);          // rouge
        std::printf ("after swatch: %08x\n", (unsigned) proc.accentColour.load());
        click (320.0f - 14.0f - 31.0f, 247.0f);             // OK
        juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
        std::printf ("after OK: %08x\n", (unsigned) proc.accentColour.load());
        const bool ok1 = proc.accentColour.load() == 0xffff4040;
        // code tape dans la case puis OK
        ie->openColourPicker (nullptr);
        picker = nullptr; find (ed.get());
        if (auto* te = dynamic_cast<juce::TextEditor*> (picker->getChildComponent (0))) { te->setText ("#172633", true); juce::MessageManager::getInstance()->runDispatchLoopUntil (50); }
        click (320.0f - 14.0f - 31.0f, 247.0f);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
        std::printf ("typed #172633 then OK: %08x\n", (unsigned) proc.accentColour.load());
        return ok1 && proc.accentColour.load() == 0xff172633 ? 0 : 5;
    }
    if (mode == "swap")
    {
        // swap <out.wav> <rate> <block> <ir1> <ir2> [mix] : bruit blanc, IR1 sur A, puis chargement d'IR2 sur A en cours de lecture
        const double rate = arg (3).getDoubleValue();
        const int block = arg (4).getIntValue();
        if (arg (7).isNotEmpty()) setP (proc, "mix", arg (7).getFloatValue());
        proc.setPlayConfigDetails (2, 2, rate, block);
        proc.prepareToPlay (rate, block);
        std::printf ("load 1: %d\n", (int) proc.loadIR (0, juce::File (arg (5))));
        juce::MidiBuffer midi;
        const int n = (int) (rate * 4.0);
        juce::AudioBuffer<float> out (2, n);
        juce::Random rnd (1);
        for (int i = 0; i < n; ++i) { const float v = (rnd.nextFloat() * 2.0f - 1.0f) * 0.25f; out.setSample (0, i, v); out.setSample (1, i, v); }
        for (int pos = 0; pos < n; pos += block)
        {
            if (pos >= (int) (rate * 2.0) && pos - block < (int) (rate * 2.0))
                std::printf ("load 2 at %d: %d\n", pos, (int) proc.loadIR (0, juce::File (arg (6))));
            const int len = juce::jmin (block, n - pos);
            juce::AudioBuffer<float> b (out.getArrayOfWritePointers(), 2, pos, len);
            proc.processBlock (b, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (1);
            juce::Thread::sleep ((int) (1000.0 * len / rate));
        }
        juce::WavAudioFormat wav;
        juce::File of (arg (2)); of.deleteFile();
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (of), rate, 2, 32, {}, 0));
        w->writeFromAudioSampleBuffer (out, 0, n);
        return 0;
    }
    if (mode == "stacktest")
    {
        int trackA = 0, trackB = 0;
        SimReaper hostA (&trackA), hostB (&trackB);
        auto pA = std::make_unique<ImpulseProcessor>();
        auto pB = std::make_unique<ImpulseProcessor>();
        pA->setIHostApplication (&hostA);
        pB->setIHostApplication (&hostB);
        using GetApiFn = void* (*) (const char*);
        auto getApi = reinterpret_cast<GetApiFn> (hostA.getReaperApi ("plugin_getapi"));
        auto* api = static_cast<const NhdStackApi*> (getApi (NHD_STACK_API_NAME));
        std::printf ("api published=%d version=%d\n", api != nullptr, api ? api->version : 0);
        if (api == nullptr) return 2;
        int trackC = 0;
        const int iA = api->findOnTrack (&trackA), iB = api->findOnTrack (&trackB), iC = api->findOnTrack (&trackC);
        std::printf ("findOnTrack: A=%d B=%d other=%d (distinct=%d)\n", iA, iB, iC, iA != iB && iA > 0 && iB > 0);
        std::printf ("createView without parent=%d, viewAlive(999)=%d\n", api->createView (iA, nullptr, 0, 0, 10, 10, 1.0), api->viewAlive (999));
        pA.reset();
        std::printf ("after deleting A: findOnTrack(A)=%d findOnTrack(B)=%d\n", api->findOnTrack (&trackA), api->findOnTrack (&trackB));
        pB.reset();
        std::printf ("after deleting B: findOnTrack(B)=%d\n", api->findOnTrack (&trackB));
        return 0;
    }
    if (mode == "process")
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (juce::File (arg (2))));
        if (r == nullptr) { std::printf ("cannot read input\n"); return 2; }
        const double rate = arg (4).getDoubleValue();
        const int block = arg (5).getIntValue();
        const int n = (int) r->lengthInSamples;
        juce::AudioBuffer<float> in (2, n);
        r->read (&in, 0, n, 0, true, true);
        if (r->numChannels == 1) in.copyFrom (1, 0, in, 0, 0, n);
        if (arg (6).isNotEmpty() && arg (6) != "-") std::printf ("load A: %d\n", (int) proc.loadIR (0, juce::File (arg (6))));
        if (arg (7).isNotEmpty() && arg (7) != "-") std::printf ("load B: %d\n", (int) proc.loadIR (1, juce::File (arg (7))));
        if (arg (8).isNotEmpty()) setP (proc, "mix", arg (8).getFloatValue());
        if (arg (9).isNotEmpty()) setP (proc, "overload", arg (9).getFloatValue());
        if (arg (10).isNotEmpty()) setP (proc, "variphi", arg (10).getFloatValue());
        if (arg (11).isNotEmpty()) setP (proc, "normalize", arg (11).getFloatValue());
        proc.setPlayConfigDetails (2, 2, rate, block);
        proc.prepareToPlay (rate, block);
        // le moteur de convolution JUCE charge l'IR en tache de fond : on lui laisse le temps
        juce::Thread::sleep (300);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> warm (2, block);
        for (int i = 0; i < 40; ++i) { warm.clear(); proc.processBlock (warm, midi); juce::Thread::sleep (5); }
        juce::AudioBuffer<float> out (2, n);
        out.makeCopyOf (in);
        bool finite = true;
        for (int pos = 0; pos < n; pos += block)
        {
            const int len = juce::jmin (block, n - pos);
            juce::AudioBuffer<float> b (out.getArrayOfWritePointers(), 2, pos, len);
            proc.processBlock (b, midi);
        }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) if (! std::isfinite (out.getSample (c, i))) finite = false;
        juce::WavAudioFormat wav;
        juce::File of (arg (3)); of.deleteFile();
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (of), rate, 2, 32, {}, 0));
        w->writeFromAudioSampleBuffer (out, 0, n);
        std::printf ("processed %d samples, finite=%d, peak=%.4f\n", n, (int) finite, out.getMagnitude (0, n));
        return finite ? 0 : 3;
    }
    return 1;
}

// Outil de verification Linux : captures PNG de l'interface + rendu audio hors ligne.
//   ImpulseTool render <out.png> <scale> [irA] [irB] [mix] [overload] [variphi]
//   ImpulseTool process <in.wav> <out.wav> <rate> <block> [irA] [irB] [mix] [overload] [variphi] [normalize]
#include "../PluginProcessor.h"
#include "../PluginEditor.h"
#include "../ColourPicker.h"

static void setP (ImpulseProcessor& p, const char* id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

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
        if (arg (10).isNotEmpty()) proc.accentColour = 0xff000000u | (juce::uint32) arg (10).getHexValue64();
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        if (arg (11) == "picker") if (auto* ie = dynamic_cast<ImpulseEditor*> (ed.get())) ie->openColourPicker (nullptr);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (400);   // timer : cache du fond
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        juce::File out (arg (2));
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("editor %d x %d -> %s\n", ed->getWidth(), ed->getHeight(), out.getFullPathName().toRawUTF8());
        ed.reset();
        return 0;
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

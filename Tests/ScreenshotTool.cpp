// Outil de verification :
//   SatLabShot out.png [slot 0-3] [preset index]   -> rend l'editeur dans un PNG
//   SatLabShot --test                              -> teste le processeur (tous presets x oversampling x tailles de blocs)
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <random>

static int runProcessorTest()
{
    int failures = 0;
    std::mt19937 rng (7); std::uniform_real_distribution<float> U (-1.0f, 1.0f);
    const int blockSizes[] = { 1, 37, 64, 480, 512, 2048 };    // 2048 > taille preparee (512) : doit etre gere
    for (int mono = 0; mono < 2; ++mono)
    {
        SatLabProcessor proc;
        juce::AudioProcessor::BusesLayout layout;
        layout.getMainInputChannelSet() = layout.getMainOutputChannelSet() = mono ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
        proc.setBusesLayout (layout);
        proc.prepareToPlay (48000.0, 512);
        for (int pr = 0; pr < proc.getNumPrograms(); ++pr)
        {
            proc.setCurrentProgram (pr);
            for (int os = 0; os < 5; ++os)
            {
                if (auto* p = proc.apvts.getParameter ("os")) p->setValueNotifyingHost (p->convertTo0to1 ((float) os));
                float peak = 0, rmsIn = 0, rmsOut = 0; bool finite = true; int count = 0;
                for (int bs : blockSizes)
                {
                    juce::AudioBuffer<float> buf (mono ? 1 : 2, bs); juce::MidiBuffer midi;
                    for (int rep = 0; rep < std::max (2, 4096 / bs); ++rep)
                    {
                        for (int c = 0; c < buf.getNumChannels(); ++c)
                            for (int i = 0; i < bs; ++i)
                                buf.setSample (c, i, 0.35f * std::sin (0.07f * (float) (count + i)) + 0.05f * U (rng));
                        count += bs;
                        double si = 0; for (int i = 0; i < bs; ++i) si += buf.getSample (0, i) * buf.getSample (0, i);
                        proc.processBlock (buf, midi);
                        double so = 0;
                        for (int c = 0; c < buf.getNumChannels(); ++c)
                            for (int i = 0; i < bs; ++i)
                            {
                                const float v = buf.getSample (c, i);
                                if (! std::isfinite (v)) finite = false;
                                peak = std::max (peak, std::abs (v));
                                if (c == 0) so += v * v;
                            }
                        rmsIn += (float) si; rmsOut += (float) so;
                    }
                }
                const bool ok = finite && peak < 16.0f;
                if (! ok) ++failures;
                if (os == 2 || ! ok)
                    std::printf ("%s preset %2d %-28s os=%d latency=%d  peak=%.2f gain=%+.1f dB %s\n", mono ? "mono  " : "stereo", pr,
                                 proc.getProgramName (pr).toRawUTF8(), os, proc.getLatencySamples(), peak,
                                 10.0 * std::log10 ((rmsOut + 1e-12) / (rmsIn + 1e-12)), ok ? "" : "  <-- PROBLEME");
            }
        }
    }
    std::printf (failures == 0 ? "PROCESSOR TEST OK\n" : "PROCESSOR TEST FAILED (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 1 && juce::String (argv[1]) == "--test") return runProcessorTest();

    SatLabProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    const int slot = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;
    if (argc > 3) proc.setCurrentProgram (juce::String (argv[3]).getIntValue());
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    if (auto* e = dynamic_cast<SatLabEditor*> (ed.get())) e->snapshotRefresh (slot);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f (argc > 1 ? argv[1] : "shot.png");
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    png.writeImageToStream (img, os);
    return 0;
}

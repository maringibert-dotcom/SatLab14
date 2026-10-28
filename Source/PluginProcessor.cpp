#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace satlab;

SatLabProcessor::SatLabProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    params.build (apvts);
}

bool SatLabProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SatLabProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = std::max (samplesPerBlock, 32);
    work.setSize (2, maxBlock, false, true, false);

    for (int i = 0; i <= kMaxFactorLog2; ++i)
    {
        const int factor = 1 << i;
        engines[(size_t) i].prepare ((float) (sr * factor), factor, maxBlock * factor);
        if (i > 0)
        {
            auto os = std::make_unique<juce::dsp::Oversampling<float>> (
                2, (size_t) i, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
            os->initProcessing ((size_t) maxBlock);
            oversamplers[(size_t) i - 1] = std::move (os);
        }
    }
    currentOs = -1;
    prepared = true;
}

void SatLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (! prepared) return;

    const int total = buffer.getNumSamples();
    const int numCh = std::min (buffer.getNumChannels(), 2);
    for (int c = numCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, total);   // canaux en trop : silence
    if (numCh < 1) return;

    // ---- parametres globaux
    const int osIdx = juce::jlimit (0, kMaxFactorLog2, (int) std::round (params.os->load()));
    if (osIdx != currentOs)
    {
        currentOs = osIdx;
        engines[(size_t) osIdx].reset();
        if (osIdx > 0) oversamplers[(size_t) osIdx - 1]->reset();
        const int lat = osIdx > 0 ? (int) std::round (oversamplers[(size_t) osIdx - 1]->getLatencyInSamples()) : 0;
        setLatencySamples (lat);
    }

    EngineParams ep;
    ep.parallel   = params.routing->load() > 0.5f;
    ep.mix        = params.mix->load();
    ep.safetyClip = params.safety->load() > 0.5f;
    for (int s = 0; s < kNumSlots; ++s) ep.slots[(size_t) s] = readSlotParams (params, s);
    auto& engine = engines[(size_t) osIdx];
    engine.setParams (ep);

    const float inG = dbToLin (params.in->load()), outG = dbToLin (params.out->load());
    float peakIn = 0.0f, peakOut = 0.0f;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = std::min (maxBlock, total - start);
        float* wl = work.getWritePointer (0);
        float* wr = work.getWritePointer (1);
        const float* inL = buffer.getReadPointer (0) + start;
        const float* inR = buffer.getReadPointer (numCh > 1 ? 1 : 0) + start;
        for (int i = 0; i < n; ++i)
        {
            wl[i] = inL[i] * inG; wr[i] = inR[i] * inG;
            peakIn = std::max (peakIn, std::max (std::abs (wl[i]), std::abs (wr[i])));
        }

        if (osIdx == 0)
        {
            engine.process (wl, wr, n);
        }
        else
        {
            juce::dsp::AudioBlock<float> block (work.getArrayOfWritePointers(), 2, 0, (size_t) n);
            auto& os = *oversamplers[(size_t) osIdx - 1];
            auto up = os.processSamplesUp (block);
            engine.process (up.getChannelPointer (0), up.getChannelPointer (1), (int) up.getNumSamples());
            os.processSamplesDown (block);
        }

        float* outL = buffer.getWritePointer (0) + start;
        float* outR = numCh > 1 ? buffer.getWritePointer (1) + start : nullptr;
        for (int i = 0; i < n; ++i)
        {
            const float l = sanitize (wl[i] * outG), r = sanitize (wr[i] * outG);
            peakOut = std::max (peakOut, std::max (std::abs (l), std::abs (r)));
            outL[i] = l;
            if (outR) outR[i] = r;
        }
    }

    // vu-metres avec relachement
    const float decay = std::exp (-(float) total / (0.25f * (float) sr));
    inLevel.store (std::max (peakIn, inLevel.load() * decay));
    outLevel.store (std::max (peakOut, outLevel.load() * decay));
}

//==============================================================================
int SatLabProcessor::getNumPrograms() { return (int) getPresets().size(); }
const juce::String SatLabProcessor::getProgramName (int i)
{
    auto& p = getPresets();
    return (i >= 0 && i < (int) p.size()) ? p[(size_t) i].name : juce::String();
}
void SatLabProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;
    applyPreset (index);
}

void SatLabProcessor::applyPreset (int index)
{
    // 1) tout remettre aux valeurs par defaut, 2) appliquer les valeurs du preset
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& e : getPresets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (e.first))
            rp->setValueNotifyingHost (rp->convertTo0to1 (e.second));
}

void SatLabProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void SatLabProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            currentProgram = xml->getIntAttribute ("program", 0);
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessorEditor* SatLabProcessor::createEditor() { return new SatLabEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SatLabProcessor(); }

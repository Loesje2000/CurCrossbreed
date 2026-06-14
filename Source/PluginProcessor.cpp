#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
const juce::StringArray CurCrossbreedAudioProcessor::kOversampleChoices
    { "Off  1x", "2x", "4x", "8x" };
const juce::StringArray CurCrossbreedAudioProcessor::kGhClipTypeChoices
    { "Diode", "Transistor", "OpAmp Hard Clip" };
const juce::StringArray CurCrossbreedAudioProcessor::kTblTapeTypeChoices
    { "Type I  Ferric", "Type II  Chrome", "Type IV  Metal" };
const juce::StringArray CurCrossbreedAudioProcessor::kVvModeChoices
    { "Chorus", "Vibrato" };
const juce::StringArray CurCrossbreedAudioProcessor::kTgSidechainChoices
    { "None", "HP 100Hz", "HP 500Hz", "LP 2kHz", "LP 5kHz" };

//==============================================================================
// GrindhouseModule
//==============================================================================
void GrindhouseModule::prepare (double sr) noexcept { mSR = sr; reset(); }
void GrindhouseModule::reset   () noexcept
{
    mTiltLPL = mTiltLPR = mPostLPL = mPostLPR = mOpAmpLPL = mOpAmpLPR = 0.f;
}

void GrindhouseModule::processBlock (juce::AudioBuffer<float>& buf,
                                     juce::dsp::Oversampling<float>* os,
                                     float inputGainDB,   float bias,        float stage1Amount,
                                     int   clipType,      float stage2Drive, float preTilt,
                                     float postTone,      float outputLevelDB, float blend) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int N    = buf.getNumSamples();
    const int numCh = buf.getNumChannels();
    const float sr = (float)mSR;
    const float pi2 = juce::MathConstants<float>::twoPi;

    const float inGainLin  = juce::Decibels::decibelsToGain (inputGainDB);
    const float outLevelLin = juce::Decibels::decibelsToGain (outputLevelDB);

    // Tilt EQ one-pole LP at 1 kHz pivot
    const float tiltC = (pi2 * 1000.f / sr) / (1.f + pi2 * 1000.f / sr);
    // Post-tone LP: maps 0→1 to 2 kHz..12 kHz
    const float postCutHz = 2000.f + postTone * 10000.f;
    const float postC     = (pi2 * postCutHz / sr) / (1.f + pi2 * postCutHz / sr);

    // Store dry
    juce::AudioBuffer<float> dry (numCh, N);
    for (int ch = 0; ch < numCh; ++ch)
        dry.copyFrom (ch, 0, buf, ch, 0, N);

    auto* L = buf.getWritePointer (0);
    auto* R = (numCh > 1) ? buf.getWritePointer (1) : L;

    //---  Stage 1: tilt EQ + tube pre-clip (no oversampling needed)  -----------
    for (int i = 0; i < N; ++i)
    {
        float xL = L[i] * inGainLin;
        float xR = (numCh > 1 ? R[i] : L[i]) * inGainLin;

        // Tilt EQ: preTilt > 0 brightens (adds HP), < 0 darkens (subtracts HP)
        mTiltLPL += tiltC * (xL - mTiltLPL);
        mTiltLPR += tiltC * (xR - mTiltLPR);
        xL = xL + preTilt * (xL - mTiltLPL) * 0.5f;
        xR = xR + preTilt * (xR - mTiltLPR) * 0.5f;

        // Stage 1: asymmetric tanh pre-clip, blended with clean
        const float biasOff = bias * 0.3f;
        const float s1drive = 1.f + stage1Amount * 4.f;
        float s1L = std::tanh ((xL + biasOff) * s1drive);
        float s1R = std::tanh ((xR + biasOff) * s1drive);
        xL = xL * (1.f - stage1Amount) + s1L * stage1Amount;
        xR = xR * (1.f - stage1Amount) + s1R * stage1Amount;

        L[i] = xL;
        if (numCh > 1) R[i] = xR;
    }

    //--- Stage 2: selectable clip topology — oversampled if os != nullptr ------
    {
        auto doClip = [&](float x, float opAmpLP, float& opAmpLPState, float osSR) -> float
        {
            if (clipType == 0) // Diode
            {
                const float k = 1.f + stage2Drive * 7.f;
                return x / (1.f + k * std::abs (x));
            }
            else if (clipType == 1) // Transistor (asymmetric)
            {
                const float kPos = 1.f + stage2Drive * 7.f;
                const float kNeg = 1.f + stage2Drive * 5.f;
                float c = (x >= 0.f) ? x / (1.f + kPos * x)
                                      : x / (1.f - kNeg * x);
                return c + 0.05f * stage2Drive * c * c * (x >= 0.f ? 1.f : -1.f);
            }
            else // OpAmp Hard Clip (one-pole LP + hard limiter)
            {
                const float cutHz  = 8000.f - stage2Drive * 3000.f;
                const float alphaO = (pi2 * cutHz / osSR) / (1.f + pi2 * cutHz / osSR);
                opAmpLPState += alphaO * (x - opAmpLPState);
                const float thresh = 1.f - stage2Drive * 0.7f;
                return juce::jlimit (-thresh, thresh, opAmpLPState);
            }
            return x;
        };

        if (os != nullptr)
        {
            juce::dsp::AudioBlock<float> block (buf);
            auto osBlock = os->processSamplesUp (block);
            const float osSR = sr * (float)(osBlock.getNumSamples() / N);
            auto* osL = osBlock.getChannelPointer (0);
            auto* osR = (osBlock.getNumChannels() > 1) ? osBlock.getChannelPointer (1) : osL;
            const size_t osN = osBlock.getNumSamples();
            for (size_t s = 0; s < osN; ++s)
            {
                osL[s] = doClip (osL[s], mOpAmpLPL, mOpAmpLPL, osSR);
                osR[s] = doClip (osR[s], mOpAmpLPR, mOpAmpLPR, osSR);
            }
            os->processSamplesDown (block);
        }
        else
        {
            for (int i = 0; i < N; ++i)
            {
                L[i] = doClip (L[i], mOpAmpLPL, mOpAmpLPL, sr);
                if (numCh > 1)
                    R[i] = doClip (R[i], mOpAmpLPR, mOpAmpLPR, sr);
            }
        }
    }

    //--- Post-stage: LP, output level, blend  ----------------------------------
    const float* dL = dry.getReadPointer (0);
    const float* dR = (numCh > 1) ? dry.getReadPointer (1) : dL;
    for (int i = 0; i < N; ++i)
    {
        mPostLPL += postC * (L[i] - mPostLPL);
        float xL = mPostLPL * outLevelLin;
        L[i] = dL[i] * (1.f - blend) + xL * blend;

        if (numCh > 1)
        {
            mPostLPR += postC * (R[i] - mPostLPR);
            float xR = mPostLPR * outLevelLin;
            R[i] = dR[i] * (1.f - blend) + xR * blend;
        }
    }
}

//==============================================================================
// TapeBoxLiteModule
//==============================================================================
void TapeBoxLiteModule::prepare (double sr) noexcept
{
    mSR = sr;
    mDelBufL.assign (kWFBuf, 0.f);
    mDelBufR.assign (kWFBuf, 0.f);
    reset();
}
void TapeBoxLiteModule::reset () noexcept
{
    mDelWritePos = 0;
    mWowP = mWowP2 = mWowP3 = mFlutP = mFlutP2 = 0.f;
    mEQBassL = mEQBassR = mEQPresL = mEQPresR = 0.f;
    mToneLPL = mToneLPR = mDCBlkL = mDCBlkR = 0.f;
}

float TapeBoxLiteModule::softSat (float x, float drive) noexcept
{
    const float th = 1.f / (1.f + drive * 2.f);
    auto clip = [&](float v) -> float {
        if (v >  th) return  th + (v-th)  / (1.f + ((v-th) /(1.f-th)) * ((v-th) /(1.f-th)));
        if (v < -th) return -th + (v+th)  / (1.f + ((-v-th)/(1.f-th)) * ((-v-th)/(1.f-th)));
        return v;
    };
    return clip (x);
}

float TapeBoxLiteModule::tapeNL (float x, int tapeType) noexcept
{
    float drive, bb;
    if      (tapeType == 0) { drive = 0.7f;  bb = 0.0f;  }
    else if (tapeType == 1) { drive = 0.45f; bb = 0.05f; }
    else                    { drive = 0.25f; bb = 0.1f;  }
    x = softSat (x, drive);
    return x + bb * x * x * x;
}

float TapeBoxLiteModule::readLin (const std::vector<float>& buf, int wPos, float delay) noexcept
{
    float rf = (float)wPos - delay;
    while (rf < 0.f) rf += (float)kWFBuf;
    const int p0 = (int)rf % kWFBuf;
    const int p1 = (p0 + 1) % kWFBuf;
    const float f = rf - std::floor (rf);
    return buf[p0] * (1.f - f) + buf[p1] * f;
}

void TapeBoxLiteModule::processBlock (juce::AudioBuffer<float>& buf,
                                      juce::dsp::Oversampling<float>* os,
                                      int tapeType, float wowFlutter, float saturation,
                                      float tone, float mix) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int N    = buf.getNumSamples();
    const int numCh = buf.getNumChannels();
    const float sr  = (float)mSR;
    const float dt  = 1.f / sr;
    const float pi2 = juce::MathConstants<float>::twoPi;

    // Tape EQ: gentle bass boost + presence
    const float eqBassC = (pi2 * 50.f   / sr) / (1.f + pi2 * 50.f   / sr);
    const float eqPresC = (pi2 * 2200.f / sr) / (1.f + pi2 * 2200.f / sr);
    const float eqBassAmt = 0.08f;
    const float eqPresAmt = 0.04f;

    // Tone LP: maps 0→1 to 1500 Hz .. 18000 Hz
    const float toneCutHz = 1500.f + tone * 16500.f;
    const float toneC     = (pi2 * toneCutHz / sr) / (1.f + pi2 * toneCutHz / sr);

    // Wow & Flutter rates + depths (simplified from TapeBox: single combined wowFlutter param)
    const float wowDepth1  = sr * 0.002f  * wowFlutter;
    const float wowDepth2  = sr * 0.0008f * wowFlutter;
    const float wowDepth3  = sr * 0.0012f * wowFlutter;
    const float flutDepth  = sr * 0.0008f * wowFlutter;

    // Store dry for mix
    juce::AudioBuffer<float> dry (numCh, N);
    for (int ch = 0; ch < numCh; ++ch)
        dry.copyFrom (ch, 0, buf, ch, 0, N);

    auto* L = buf.getWritePointer (0);
    auto* R = (numCh > 1) ? buf.getWritePointer (1) : L;

    // Tape EQ (pre-saturation)
    for (int i = 0; i < N; ++i)
    {
        float xL = L[i], xR = (numCh > 1) ? R[i] : L[i];
        mEQBassL += eqBassC * (xL - mEQBassL);  xL += eqBassAmt * mEQBassL;
        mEQBassR += eqBassC * (xR - mEQBassR);  xR += eqBassAmt * mEQBassR;
        mEQPresL += eqPresC * (xL - mEQPresL);  xL += eqPresAmt * (xL - mEQPresL);
        mEQPresR += eqPresC * (xR - mEQPresR);  xR += eqPresAmt * (xR - mEQPresR);
        L[i] = xL; if (numCh > 1) R[i] = xR;
    }

    // Oversampled saturation
    {
        const float driveAmt  = saturation;
        const float inputGain = 1.f + driveAmt * 0.8f;

        auto saturateSample = [&](float x) -> float {
            return tapeNL (x * inputGain, tapeType);
        };

        if (os != nullptr)
        {
            juce::dsp::AudioBlock<float> block (buf);
            auto osBlock = os->processSamplesUp (block);
            auto* osL = osBlock.getChannelPointer (0);
            auto* osR = (osBlock.getNumChannels() > 1) ? osBlock.getChannelPointer (1) : osL;
            for (size_t s = 0; s < osBlock.getNumSamples(); ++s)
            {
                osL[s] = saturateSample (osL[s]);
                osR[s] = saturateSample (osR[s]);
            }
            os->processSamplesDown (block);
        }
        else
        {
            for (int i = 0; i < N; ++i)
            {
                L[i] = saturateSample (L[i]);
                if (numCh > 1) R[i] = saturateSample (R[i]);
            }
        }
    }

    // Post-saturation: tone LP, wow/flutter, DC block, mix
    juce::Random rng;
    for (int i = 0; i < N; ++i)
    {
        float xL = L[i], xR = (numCh > 1) ? R[i] : L[i];

        // Tone LP (tape darkening)
        mToneLPL += toneC * (xL - mToneLPL);  xL = mToneLPL;
        mToneLPR += toneC * (xR - mToneLPR);  xR = mToneLPR;

        // Wow & flutter delay
        mDelBufL[mDelWritePos] = xL;
        mDelBufR[mDelWritePos] = xR;

        mWowP  += 0.7f  * dt; mWowP2  += 0.28f * dt; mWowP3  += 0.45f * dt;
        mFlutP += 8.0f  * dt; mFlutP2 += 13.0f * dt;
        if (mWowP  > 1.f) mWowP  -= 1.f;
        if (mWowP2 > 1.f) mWowP2 -= 1.f;
        if (mWowP3 > 1.f) mWowP3 -= 1.f;
        if (mFlutP > 1.f) mFlutP -= 1.f;
        if (mFlutP2> 1.f) mFlutP2 -= 1.f;

        const float wowMod = std::sin(pi2*mWowP) *wowDepth1
                           + std::sin(pi2*mWowP2)*wowDepth2
                           + std::sin(pi2*mWowP3)*wowDepth3;
        const float flutRnd = rng.nextFloat() * 0.15f;
        const float flutMod = (std::sin(pi2*mFlutP) + 0.4f*std::sin(pi2*mFlutP2+1.1f)+flutRnd)
                             * flutDepth;
        const float totalMod = 128.f + wowMod + flutMod;

        xL = readLin (mDelBufL, mDelWritePos, totalMod);
        xR = readLin (mDelBufR, mDelWritePos, totalMod);
        mDelWritePos = (mDelWritePos + 1) % kWFBuf;

        // DC block
        float dbL = xL - mDCBlkL;  mDCBlkL = xL - dbL * 0.999f;  xL = dbL;
        float dbR = xR - mDCBlkR;  mDCBlkR = xR - dbR * 0.999f;  xR = dbR;

        const float* dL = dry.getReadPointer (0);
        const float* dR = (numCh > 1) ? dry.getReadPointer (1) : dL;
        L[i] = dL[i] * (1.f - mix) + xL * mix;
        if (numCh > 1) R[i] = dR[i] * (1.f - mix) + xR * mix;
    }
}

//==============================================================================
// VoodooVibeModule — LDR optical phaser/vibrato (ported from Loesje2000/VoodooVibe)
//==============================================================================
void VoodooVibeModule::prepare (double sr) noexcept
{
    mSR = sr > 0.0 ? sr : 44100.0;
    mInvSR = 1.f / (float)mSR;
    reset();
}

void VoodooVibeModule::reset() noexcept
{
    mLfoPhase  = 0.f;
    mLampState = 0.5f;
    for (auto& ch : mChan) ch.reset();
}

float VoodooVibeModule::AllpassStage::process (float x, float coeff) noexcept
{
    const float y = coeff * x + x1 - coeff * y1;
    x1 = x; y1 = y;
    return y;
}

void VoodooVibeModule::Channel::reset() noexcept
{
    for (auto& s : stages) { s.x1 = 0.f; s.y1 = 0.f; }
    dcIn = dcOut = 0.f;
}

float VoodooVibeModule::processPreamp (float x, float driveAmt) const noexcept
{
    const float gain  = 1.f + driveAmt * 3.25f;
    const float clip  = [](float v) -> float {  // soft clip
        if (v > 1.f)  return 1.f - 1.f / (1.f + (v - 1.f));
        if (v < -1.f) return -1.f + 1.f / (1.f - (v + 1.f));
        return v - v * v * v * (1.f / 3.f);
    }(x * gain);
    const float scaled = clip / juce::jmax (1.f, gain * 0.62f);
    return juce::jmap (driveAmt, x, scaled);
}

float VoodooVibeModule::processDcBlock (float x, Channel& ch) const noexcept
{
    constexpr float pole = 0.995f;
    const float y = x - ch.dcIn + pole * ch.dcOut;
    ch.dcIn = x; ch.dcOut = y;
    return y;
}

float VoodooVibeModule::computeApCoeff (float r, float c) const noexcept
{
    const float omega = 1.f / juce::jmax (1.0e-9f, r * c);
    const float g     = omega * mInvSR;
    return juce::jlimit (-0.9995f, 0.9995f, (1.f - g) / (1.f + g));
}

void VoodooVibeModule::updateLamp (float speed, float intensity, float bias) noexcept
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    const float sine   = std::sin (twoPi * mLfoPhase);
    const float second = 0.08f * std::sin (twoPi * 2.f * mLfoPhase + 0.55f);
    const float sc     = juce::jlimit (0.75f, 1.35f, 0.82f + speed * 0.045f);
    const float drive  = std::tanh ((sine + second) * intensity * sc / 0.9f) * 0.9f;
    const float target = juce::jlimit (0.f, 1.f, bias + drive * 0.42f);
    const float tau    = target > mLampState ? 0.015f : 0.025f;
    mLampState += (target - mLampState) * (1.f - std::exp (-mInvSR / tau));
    mLfoPhase  += speed * mInvSR;
    if (mLfoPhase >= 1.f) mLfoPhase -= std::floor (mLfoPhase);
}

float VoodooVibeModule::channelSpread (int ch, float width) const noexcept
{
    return width <= 0.f ? 0.f : (ch == 0 ? -1.f : 1.f) * width;
}

void VoodooVibeModule::processBlock (juce::AudioBuffer<float>& buf,
                                     float speed, float intensity, float drive,
                                     float lampBias, float stereoWidth,
                                     int mode, float mix) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = buf.getNumChannels();
    const int nS  = buf.getNumSamples();
    const bool vibratoMode = (mode >= 1);
    const float wetMix = vibratoMode ? 1.f : mix;
    const float dryMix = vibratoMode ? 0.f : (1.f - mix);
    const float level  = vibratoMode ? 0.82f : 1.f;
    const bool stereoOn = (nCh > 1) && (stereoWidth > 0.0001f);
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    for (int s = 0; s < nS; ++s)
    {
        updateLamp (speed, intensity, lampBias);
        const float brightness = juce::jlimit (0.001f, 1.f, mLampState);
        const float ldrBase    = juce::jlimit (500.f, 1000000.f,
                                               1000000.f * std::pow (brightness, -0.82f) * 0.0018f);
        const float panMotion  = std::sin (twoPi * (mLfoPhase + 0.25f));

        for (int ch = 0; ch < nCh; ++ch)
        {
            auto* data = buf.getWritePointer (ch);
            auto& state = mChan[(size_t)juce::jmin (ch, 1)];
            const float side = stereoOn ? channelSpread (ch, stereoWidth) : 0.f;

            const float dry = data[s];
            float wet = processPreamp (dry, drive);
            for (size_t i = 0; i < state.stages.size(); ++i)
            {
                const float spread = 1.f + side * (0.06f + 0.035f * (float)i);
                const float coeff  = computeApCoeff (ldrBase * kLdrOff[i] * spread, kCaps[i]);
                wet = state.stages[i].process (wet, coeff);
            }
            wet *= 1.f + side * panMotion * 0.22f;
            data[s] = processDcBlock ((dry * dryMix + wet * wetMix) * level, state);
        }
    }
}

//==============================================================================
// MemoreclLiteModule
//==============================================================================
void MemoreclLiteModule::prepare (double sr) noexcept
{
    mSR = sr;
    // Buffer for 800ms at up to 192kHz with margin
    const int bufSize = (int)(sr * 0.85);
    mDelBufL.assign (bufSize, 0.f);
    mDelBufR.assign (bufSize, 0.f);
    reset();
}
void MemoreclLiteModule::reset () noexcept
{
    std::fill (mDelBufL.begin(), mDelBufL.end(), 0.f);
    std::fill (mDelBufR.begin(), mDelBufR.end(), 0.f);
    mDelWritePos = 0;
    mDrkL = mDrkR = 0.f;
}

float MemoreclLiteModule::readLin (const std::vector<float>& buf, int wPos, float delay) noexcept
{
    const int bufSize = (int)buf.size();
    float rf = (float)wPos - delay;
    while (rf < 0.f) rf += (float)bufSize;
    const int p0 = (int)rf % bufSize;
    const int p1 = (p0 + 1) % bufSize;
    const float f = rf - std::floor (rf);
    return buf[p0] * (1.f - f) + buf[p1] * f;
}

void MemoreclLiteModule::processBlock (juce::AudioBuffer<float>& buf,
                                        float timeMs, float feedback, float tone,
                                        float mix) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int N    = buf.getNumSamples();
    const int numCh = buf.getNumChannels();
    const float sr  = (float)mSR;
    const float pi2 = juce::MathConstants<float>::twoPi;
    const int bufSize = (int)mDelBufL.size();

    timeMs = juce::jlimit (50.f, 800.f, timeMs);
    const float delaySamples = timeMs * 0.001f * sr;

    // Per-repeat darkening LP cutoff: tone 0→1 maps 800Hz..10000Hz
    const float drkCutHz = 800.f + tone * 9200.f;
    const float drkC     = (pi2 * drkCutHz / sr) / (1.f + pi2 * drkCutHz / sr);

    auto* L = buf.getWritePointer (0);
    auto* R = (numCh > 1) ? buf.getWritePointer (1) : L;

    for (int i = 0; i < N; ++i)
    {
        const float inL = L[i];
        const float inR = (numCh > 1) ? R[i] : L[i];

        // Read delayed output
        float delL = readLin (mDelBufL, mDelWritePos, delaySamples);
        float delR = readLin (mDelBufR, mDelWritePos, delaySamples);

        // Apply darkening to feedback path (BBD HF rolloff per repeat)
        mDrkL += drkC * (delL - mDrkL);
        mDrkR += drkC * (delR - mDrkR);

        // Write to delay buffer: input + darkened feedback
        mDelBufL[mDelWritePos] = inL + mDrkL * feedback;
        mDelBufR[mDelWritePos] = inR + mDrkR * feedback;
        mDelWritePos = (mDelWritePos + 1) % bufSize;

        L[i] = inL * (1.f - mix) + delL * mix;
        if (numCh > 1) R[i] = inR * (1.f - mix) + delR * mix;
    }
}

//==============================================================================
// StereoWidenerModule
//==============================================================================
void StereoWidenerModule::prepare (double sr) noexcept { mSR = sr; reset(); }
void StereoWidenerModule::reset   () noexcept
{
    mMidLP1L = mMidLP1R = mMidLP2L = mMidLP2R = 0.f;
    mSideLP1L = mSideLP1R = mSideLP2L = mSideLP2R = 0.f;
}

void StereoWidenerModule::processBlock (juce::AudioBuffer<float>& buf,
                                         float widthPct, float crossoverHz) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int N    = buf.getNumSamples();
    const int numCh = buf.getNumChannels();

    if (numCh < 2)
        return; // widening meaningless on mono

    const float sr  = (float)mSR;
    const float pi2 = juce::MathConstants<float>::twoPi;
    crossoverHz = juce::jlimit (200.f, 2000.f, crossoverHz);
    const float alpha = (pi2 * crossoverHz / sr) / (1.f + pi2 * crossoverHz / sr);
    const float widthGain = widthPct / 100.f;

    auto* L = buf.getWritePointer (0);
    auto* R = buf.getWritePointer (1);

    for (int i = 0; i < N; ++i)
    {
        const float mid  = (L[i] + R[i]) * 0.5f;
        const float side = (L[i] - R[i]) * 0.5f;

        // Linkwitz-Riley 2nd-order LP (two one-pole stages) on mid and side
        mMidLP1L  += alpha * (mid  - mMidLP1L);
        mMidLP2L  += alpha * (mMidLP1L - mMidLP2L);
        mSideLP1L += alpha * (side - mSideLP1L);
        mSideLP2L += alpha * (mSideLP1L - mSideLP2L);

        // Mid is identical for both channels so one filter chain suffices
        // (Side also splits identically, just one chain needed)
        const float lowMid  = mMidLP2L;
        const float highMid = mid - lowMid;
        const float lowSide  = mSideLP2L;
        const float highSide = side - lowSide;

        // Scale only the high-band side
        const float newSide = lowSide + highSide * widthGain;
        const float newMid  = lowMid  + highMid;

        L[i] = newMid + newSide;
        R[i] = newMid - newSide;
    }
}

//==============================================================================
// ToneGateModule — 5-band proportional EQ + gate
//==============================================================================
void ToneGateModule::prepare (double sr) noexcept { mSR = sr; reset(); }
void ToneGateModule::reset   () noexcept
{
    mLp0L = mLp0R = mLp1L = mLp1R = mLp2L = mLp2R =
    mLp3L = mLp3R = mLp4L = mLp4R = 0.f;
    mGateGain = 1.f;
    mScFltL = mScFltR = 0.f;
}

void ToneGateModule::processBlock (juce::AudioBuffer<float>& buf,
                                    float eqLow,   float eqLoMid, float eqMid,
                                    float eqHiMid, float eqHigh,
                                    float gateThreshDB, float gateReleaseMs,
                                    int sidechainFilter, bool gateBypass) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int N     = buf.getNumSamples();
    const int numCh = buf.getNumChannels();
    const float sr  = (float)mSR;
    const float pi2 = juce::MathConstants<float>::twoPi;

    // 5-band LP crossover alphas: 80, 400, 1500, 5000, 12000 Hz
    auto alpha = [&](float fc) { const float w = pi2*fc/sr; return w/(1.f+w); };
    const float a0 = alpha (80.f);
    const float a1 = alpha (400.f);
    const float a2 = alpha (1500.f);
    const float a3 = alpha (5000.f);
    const float a4 = alpha (12000.f);

    // Band gains: dB → linear proportional factor (g-1 = 0 at unity)
    auto dg = [](float db) { return juce::Decibels::decibelsToGain(db) - 1.f; };
    const float g0 = dg(eqLow), g1 = dg(eqLoMid), g2 = dg(eqMid),
                g3 = dg(eqHiMid), g4 = dg(eqHigh);

    // Gate constants
    const float threshLin  = juce::Decibels::decibelsToGain (gateThreshDB);
    const float attackCoef = std::exp (-1.f / (0.001f * sr));
    const float relCoef    = std::exp (-1.f / (juce::jlimit(5.f,500.f,gateReleaseMs) * 0.001f * sr));

    // Sidechain filter
    float scAlpha = 0.f;
    if (sidechainFilter > 0)
    {
        const float scHz = (sidechainFilter == 1) ? 100.f :
                           (sidechainFilter == 2) ? 500.f :
                           (sidechainFilter == 3) ? 2000.f : 5000.f;
        scAlpha = alpha (scHz);
    }

    auto* L = buf.getWritePointer (0);
    auto* R = (numCh > 1) ? buf.getWritePointer (1) : L;

    for (int i = 0; i < N; ++i)
    {
        float xL = L[i], xR = (numCh > 1) ? R[i] : L[i];

        //--  5-band proportional EQ  ------------------------------------------
        mLp0L += a0*(xL-mLp0L);  mLp0R += a0*(xR-mLp0R);
        mLp1L += a1*(xL-mLp1L);  mLp1R += a1*(xR-mLp1R);
        mLp2L += a2*(xL-mLp2L);  mLp2R += a2*(xR-mLp2R);
        mLp3L += a3*(xL-mLp3L);  mLp3R += a3*(xR-mLp3R);
        mLp4L += a4*(xL-mLp4L);  mLp4R += a4*(xR-mLp4R);

        // Bands: low shelf, 3 bells, high shelf
        xL += g0*mLp0L + g1*(mLp1L-mLp0L) + g2*(mLp2L-mLp1L)
            + g3*(mLp3L-mLp2L) + g4*(xL-mLp4L);
        xR += g0*mLp0R + g1*(mLp1R-mLp0R) + g2*(mLp2R-mLp1R)
            + g3*(mLp3R-mLp2R) + g4*(xR-mLp4R);

        //--  Gate  -------------------------------------------------------------
        if (!gateBypass)
        {
            float scL = xL, scR = xR;
            if (sidechainFilter > 0)
            {
                mScFltL += scAlpha * (xL - mScFltL);
                mScFltR += scAlpha * (xR - mScFltR);
                if (sidechainFilter <= 2) { scL = xL - mScFltL; scR = xR - mScFltR; }
                else                      { scL = mScFltL;       scR = mScFltR; }
            }
            const float level = std::max (std::abs (scL), std::abs (scR));
            if (level > threshLin)
                mGateGain = attackCoef * mGateGain + (1.f - attackCoef) * 1.f;
            else
                mGateGain *= relCoef;
            mGateGain = juce::jlimit (0.f, 1.f, mGateGain);
            xL *= mGateGain;  xR *= mGateGain;
        }

        L[i] = xL;
        if (numCh > 1) R[i] = xR;
    }
}

//==============================================================================
// CurCrossbreedAudioProcessor
//==============================================================================
CurCrossbreedAudioProcessor::CurCrossbreedAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    // Initialise slot atomics
    mSlot0 = 0.f; mSlot1 = 1.f; mSlot2 = 2.f;
    mSlot3 = 3.f; mSlot4 = 4.f; mSlot5 = 5.f;

    const juce::StringArray ids {
        "rack_inputGain", "rack_outputGain", "rack_mix", "rack_bypass", "rack_oversample",
        "slot0","slot1","slot2","slot3","slot4","slot5",
        "gh_inputGain","gh_bias","gh_stage1Amt","gh_clipType","gh_stage2Drive",
        "gh_preTilt","gh_postTone","gh_outputLevel","gh_blend","gh_bypass",
        "tbl_tapeType","tbl_wowFlutter","tbl_saturation","tbl_tone","tbl_mix","tbl_bypass",
        "vv_bypass","vv_speed","vv_intensity","vv_drive","vv_lampBias","vv_stereoWidth","vv_mode","vv_mix",
        "mrl_time","mrl_feedback","mrl_tone","mrl_mix","mrl_bypass",
        "sw_width","sw_crossover","sw_bypass",
        "tg_eqLow","tg_eqLoMid","tg_eqMid","tg_eqHiMid","tg_eqHigh",
        "tg_gateThresh","tg_gateRelease","tg_sidechainFilter","tg_gateBypass","tg_bypass"
    };
    for (auto& id : ids)
        apvts.addParameterListener (id, this);
}

CurCrossbreedAudioProcessor::~CurCrossbreedAudioProcessor()
{
    const juce::StringArray ids {
        "rack_inputGain", "rack_outputGain", "rack_mix", "rack_bypass", "rack_oversample",
        "slot0","slot1","slot2","slot3","slot4","slot5",
        "gh_inputGain","gh_bias","gh_stage1Amt","gh_clipType","gh_stage2Drive",
        "gh_preTilt","gh_postTone","gh_outputLevel","gh_blend","gh_bypass",
        "tbl_tapeType","tbl_wowFlutter","tbl_saturation","tbl_tone","tbl_mix","tbl_bypass",
        "vv_bypass","vv_speed","vv_intensity","vv_drive","vv_lampBias","vv_stereoWidth","vv_mode","vv_mix",
        "mrl_time","mrl_feedback","mrl_tone","mrl_mix","mrl_bypass",
        "sw_width","sw_crossover","sw_bypass",
        "tg_eqLow","tg_eqLoMid","tg_eqMid","tg_eqHiMid","tg_eqHigh",
        "tg_gateThresh","tg_gateRelease","tg_sidechainFilter","tg_gateBypass","tg_bypass"
    };
    for (auto& id : ids)
        apvts.removeParameterListener (id, this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
CurCrossbreedAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto flt = [](const char* id, const char* nm, float lo, float hi, float step, float def)
    {
        return std::make_unique<juce::AudioParameterFloat>
            (id, nm, juce::NormalisableRange<float>(lo,hi,step), def);
    };
    auto tog = [](const char* id, const char* nm)
    {
        return std::make_unique<juce::AudioParameterFloat>
            (id, nm, juce::NormalisableRange<float>(0.f,1.f,1.f), 0.f);
    };
    auto ch = [](const char* id, const char* nm, const juce::StringArray& c, int def)
    {
        return std::make_unique<juce::AudioParameterChoice>(id, nm, c, def);
    };

    // Global rack
    p.push_back (flt ("rack_inputGain",  "Rack Input Gain",  -18.f, 12.f, 0.1f, 0.f));
    p.push_back (flt ("rack_outputGain", "Rack Output Gain", -18.f, 12.f, 0.1f, 0.f));
    p.push_back (flt ("rack_mix",        "Rack Mix",          0.f,   1.f, 0.01f, 1.f));
    p.push_back (tog ("rack_bypass",     "Rack Bypass"));
    p.push_back (ch  ("rack_oversample", "Oversample", kOversampleChoices, 0));

    // Slot order (float 0..5)
    for (int s = 0; s < 6; ++s)
    {
        juce::String id ("slot"); id += s;
        juce::String nm ("Slot "); nm += s;
        p.push_back (flt (id.toRawUTF8(), nm.toRawUTF8(), 0.f, 5.f, 1.f, (float)s));
    }

    // Grindhouse
    p.push_back (flt ("gh_inputGain",   "GH Input Gain",   -12.f, 24.f,  0.1f,  0.f));
    p.push_back (flt ("gh_bias",        "GH Bias",          -1.f,  1.f,  0.01f, 0.f));
    p.push_back (flt ("gh_stage1Amt",   "GH Stage1 Amount", 0.f,   1.f,  0.01f, 0.5f));
    p.push_back (ch  ("gh_clipType",    "GH Clip Type", kGhClipTypeChoices, 0));
    p.push_back (flt ("gh_stage2Drive", "GH Stage2 Drive",  0.f,   1.f,  0.01f, 0.5f));
    p.push_back (flt ("gh_preTilt",     "GH Pre Tilt",     -1.f,  1.f,  0.01f, 0.f));
    p.push_back (flt ("gh_postTone",    "GH Post Tone",     0.f,   1.f,  0.01f, 0.5f));
    p.push_back (flt ("gh_outputLevel", "GH Output Level", -18.f, 12.f,  0.1f,  0.f));
    p.push_back (flt ("gh_blend",       "GH Blend",         0.f,   1.f,  0.01f, 1.f));
    p.push_back (tog ("gh_bypass",      "GH Bypass"));

    // TapeBox-lite
    p.push_back (ch  ("tbl_tapeType",   "TBL Tape Type",   kTblTapeTypeChoices, 0));
    p.push_back (flt ("tbl_wowFlutter", "TBL Wow/Flutter",  0.f,  1.f, 0.01f, 0.25f));
    p.push_back (flt ("tbl_saturation", "TBL Saturation",   0.f,  1.f, 0.01f, 0.4f));
    p.push_back (flt ("tbl_tone",       "TBL Tone",         0.f,  1.f, 0.01f, 0.5f));
    p.push_back (flt ("tbl_mix",        "TBL Mix",          0.f,  1.f, 0.01f, 1.f));
    p.push_back (tog ("tbl_bypass",     "TBL Bypass"));

    // Voodoo Vibe
    auto speedRange = juce::NormalisableRange<float> (0.5f, 12.f, 0.01f);
    speedRange.setSkewForCentre (3.f);
    p.push_back (tog ("vv_bypass",      "VV Bypass"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vv_speed", 1 }, "VV Speed", speedRange, 3.f, "Hz"));
    p.push_back (flt ("vv_intensity",   "VV Intensity",    0.f,  1.f,  0.01f, 0.7f));
    p.push_back (flt ("vv_drive",       "VV Drive",        0.f,  1.f,  0.01f, 0.3f));
    p.push_back (flt ("vv_lampBias",    "VV Lamp Bias",    0.2f, 0.8f, 0.01f, 0.5f));
    p.push_back (flt ("vv_stereoWidth", "VV Stereo Width", 0.f,  1.f,  0.01f, 0.35f));
    p.push_back (ch  ("vv_mode",        "VV Mode",         kVvModeChoices, 0));
    p.push_back (flt ("vv_mix",         "VV Mix",          0.f,  1.f,  0.01f, 0.5f));

    // Memorec-lite
    p.push_back (flt ("mrl_time",     "MRL Time",      50.f, 800.f, 1.f,   250.f));
    p.push_back (flt ("mrl_feedback", "MRL Feedback",   0.f,  0.95f, 0.01f, 0.35f));
    p.push_back (flt ("mrl_tone",     "MRL Tone",       0.f,  1.f,  0.01f, 0.5f));
    p.push_back (flt ("mrl_mix",      "MRL Mix",        0.f,  1.f,  0.01f, 0.3f));
    p.push_back (tog ("mrl_bypass",   "MRL Bypass"));

    // Stereo Widener
    p.push_back (flt ("sw_width",     "SW Width",      0.f, 200.f, 1.f,  100.f));
    p.push_back (flt ("sw_crossover", "SW Crossover",  200.f, 2000.f, 1.f, 500.f));
    p.push_back (tog ("sw_bypass",    "SW Bypass"));

    // Tone & Gate
    p.push_back (flt ("tg_eqLow",        "TG EQ Low",      -15.f, 15.f, 0.1f, 0.f));
    p.push_back (flt ("tg_eqLoMid",      "TG EQ Lo Mid",   -15.f, 15.f, 0.1f, 0.f));
    p.push_back (flt ("tg_eqMid",        "TG EQ Mid",      -15.f, 15.f, 0.1f, 0.f));
    p.push_back (flt ("tg_eqHiMid",      "TG EQ Hi Mid",   -15.f, 15.f, 0.1f, 0.f));
    p.push_back (flt ("tg_eqHigh",       "TG EQ High",     -15.f, 15.f, 0.1f, 0.f));
    p.push_back (flt ("tg_gateThresh",    "TG Gate Thresh", -60.f, 0.f, 0.5f, -40.f));
    p.push_back (flt ("tg_gateRelease",   "TG Gate Release",  5.f, 500.f, 1.f, 100.f));
    p.push_back (ch  ("tg_sidechainFilter","TG Sidechain", kTgSidechainChoices, 0));
    p.push_back (tog ("tg_gateBypass",    "TG Gate Bypass"));
    p.push_back (tog ("tg_bypass",        "TG Bypass"));

    return { p.begin(), p.end() };
}

//==============================================================================
void CurCrossbreedAudioProcessor::parameterChanged (const juce::String& id, float v)
{
    if      (id == "rack_inputGain")   mRackInputGain  = v;
    else if (id == "rack_outputGain")  mRackOutputGain = v;
    else if (id == "rack_mix")         mRackMix        = v;
    else if (id == "rack_bypass")      mRackBypass     = v;
    else if (id == "rack_oversample")  { mOSIndex = v; mCurrentOSIndex = (int)v; rebuildOS (mBlockSize); }
    else if (id == "slot0") { mSlot0 = v; mOrderDirty = true; }
    else if (id == "slot1") { mSlot1 = v; mOrderDirty = true; }
    else if (id == "slot2") { mSlot2 = v; mOrderDirty = true; }
    else if (id == "slot3") { mSlot3 = v; mOrderDirty = true; }
    else if (id == "slot4") { mSlot4 = v; mOrderDirty = true; }
    else if (id == "slot5") { mSlot5 = v; mOrderDirty = true; }
    else if (id == "gh_inputGain")   mGhInputGain   = v;
    else if (id == "gh_bias")        mGhBias        = v;
    else if (id == "gh_stage1Amt")   mGhStage1Amt   = v;
    else if (id == "gh_clipType")    mGhClipType    = v;
    else if (id == "gh_stage2Drive") mGhStage2Drive = v;
    else if (id == "gh_preTilt")     mGhPreTilt     = v;
    else if (id == "gh_postTone")    mGhPostTone    = v;
    else if (id == "gh_outputLevel") mGhOutputLevel = v;
    else if (id == "gh_blend")       mGhBlend       = v;
    else if (id == "gh_bypass")      mGhBypass      = v;
    else if (id == "tbl_tapeType")   mTblTapeType   = v;
    else if (id == "tbl_wowFlutter") mTblWowFlutter = v;
    else if (id == "tbl_saturation") mTblSaturation = v;
    else if (id == "tbl_tone")       mTblTone       = v;
    else if (id == "tbl_mix")        mTblMix        = v;
    else if (id == "tbl_bypass")     mTblBypass     = v;
    else if (id == "vv_bypass")      mVvBypass      = v;
    else if (id == "vv_speed")       mVvSpeed       = v;
    else if (id == "vv_intensity")   mVvIntensity   = v;
    else if (id == "vv_drive")       mVvDrive       = v;
    else if (id == "vv_lampBias")    mVvLampBias    = v;
    else if (id == "vv_stereoWidth") mVvStereoWidth = v;
    else if (id == "vv_mode")        mVvMode        = v;
    else if (id == "vv_mix")         mVvMix         = v;
    else if (id == "mrl_time")       mMrlTime       = v;
    else if (id == "mrl_feedback")   mMrlFeedback   = v;
    else if (id == "mrl_tone")       mMrlTone       = v;
    else if (id == "mrl_mix")        mMrlMix        = v;
    else if (id == "mrl_bypass")     mMrlBypass     = v;
    else if (id == "sw_width")       mSwWidth       = v;
    else if (id == "sw_crossover")   mSwCrossover   = v;
    else if (id == "sw_bypass")      mSwBypass      = v;
    else if (id == "tg_eqLow")       mTgEqLow       = v;
    else if (id == "tg_eqLoMid")     mTgEqLoMid     = v;
    else if (id == "tg_eqMid")       mTgEqMid       = v;
    else if (id == "tg_eqHiMid")     mTgEqHiMid     = v;
    else if (id == "tg_eqHigh")      mTgEqHigh      = v;
    else if (id == "tg_gateThresh")  mTgGateThresh  = v;
    else if (id == "tg_gateRelease") mTgGateRelease = v;
    else if (id == "tg_sidechainFilter") mTgSidechainFlt = v;
    else if (id == "tg_gateBypass")  mTgGateBypass  = v;
    else if (id == "tg_bypass")      mTgBypass      = v;
}

//==============================================================================
void CurCrossbreedAudioProcessor::rebuildOS (int blockSize)
{
    using OS = juce::dsp::Oversampling<float>;
    mOS1x = std::make_unique<OS>(2, 0, OS::filterHalfBandPolyphaseIIR, true);
    mOS2x = std::make_unique<OS>(2, 1, OS::filterHalfBandPolyphaseIIR, true);
    mOS4x = std::make_unique<OS>(2, 2, OS::filterHalfBandPolyphaseIIR, true);
    mOS8x = std::make_unique<OS>(2, 3, OS::filterHalfBandPolyphaseIIR, true);
    mOS1x->initProcessing ((size_t)blockSize);
    mOS2x->initProcessing ((size_t)blockSize);
    mOS4x->initProcessing ((size_t)blockSize);
    mOS8x->initProcessing ((size_t)blockSize);
}

juce::dsp::Oversampling<float>* CurCrossbreedAudioProcessor::getActiveOS() noexcept
{
    switch (mCurrentOSIndex) {
        case 1:  return mOS2x.get();
        case 2:  return mOS4x.get();
        case 3:  return mOS8x.get();
        default: return mOS1x.get();
    }
}

//==============================================================================
void CurCrossbreedAudioProcessor::resetAllState()
{
    mGrindhouse.reset();
    mTapeBoxLite.reset();
    mVoodooVibe.reset();
    mMemorecLite.reset();
    mStereoWidener.reset();
    mToneGate.reset();
}

void CurCrossbreedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mBlockSize  = samplesPerBlock;

    mGrindhouse.prepare    (sampleRate);
    mTapeBoxLite.prepare   (sampleRate);
    mVoodooVibe.prepare    (sampleRate);
    mMemorecLite.prepare   (sampleRate);
    mStereoWidener.prepare (sampleRate);
    mToneGate.prepare      (sampleRate);

    rebuildOS (samplesPerBlock);
    mCurrentOSIndex = (int)mOSIndex.load();

    auto sync = [&](const char* id, std::atomic<float>& t)
    { if (auto* param = apvts.getRawParameterValue(id)) t = param->load(); };

    sync("rack_inputGain",  mRackInputGain);
    sync("rack_outputGain", mRackOutputGain);
    sync("rack_mix",        mRackMix);
    sync("rack_bypass",     mRackBypass);
    sync("rack_oversample", mOSIndex);
    sync("slot0", mSlot0); sync("slot1", mSlot1); sync("slot2", mSlot2);
    sync("slot3", mSlot3); sync("slot4", mSlot4); sync("slot5", mSlot5);
    sync("gh_inputGain",   mGhInputGain);
    sync("gh_bias",        mGhBias);
    sync("gh_stage1Amt",   mGhStage1Amt);
    sync("gh_clipType",    mGhClipType);
    sync("gh_stage2Drive", mGhStage2Drive);
    sync("gh_preTilt",     mGhPreTilt);
    sync("gh_postTone",    mGhPostTone);
    sync("gh_outputLevel", mGhOutputLevel);
    sync("gh_blend",       mGhBlend);
    sync("gh_bypass",      mGhBypass);
    sync("tbl_tapeType",   mTblTapeType);
    sync("tbl_wowFlutter", mTblWowFlutter);
    sync("tbl_saturation", mTblSaturation);
    sync("tbl_tone",       mTblTone);
    sync("tbl_mix",        mTblMix);
    sync("tbl_bypass",     mTblBypass);
    sync("vv_bypass",      mVvBypass);
    sync("vv_speed",       mVvSpeed);
    sync("vv_intensity",   mVvIntensity);
    sync("vv_drive",       mVvDrive);
    sync("vv_lampBias",    mVvLampBias);
    sync("vv_stereoWidth", mVvStereoWidth);
    sync("vv_mode",        mVvMode);
    sync("vv_mix",         mVvMix);
    sync("mrl_time",       mMrlTime);
    sync("mrl_feedback",   mMrlFeedback);
    sync("mrl_tone",       mMrlTone);
    sync("mrl_mix",        mMrlMix);
    sync("mrl_bypass",     mMrlBypass);
    sync("sw_width",       mSwWidth);
    sync("sw_crossover",   mSwCrossover);
    sync("sw_bypass",      mSwBypass);
    sync("tg_eqLow",       mTgEqLow);
    sync("tg_eqLoMid",     mTgEqLoMid);
    sync("tg_eqMid",       mTgEqMid);
    sync("tg_eqHiMid",     mTgEqHiMid);
    sync("tg_eqHigh",      mTgEqHigh);
    sync("tg_gateThresh",  mTgGateThresh);
    sync("tg_gateRelease", mTgGateRelease);
    sync("tg_sidechainFilter", mTgSidechainFlt);
    sync("tg_gateBypass",  mTgGateBypass);
    sync("tg_bypass",      mTgBypass);

    // Rebuild order from slot params
    mModuleOrder[0] = (int)std::round(mSlot0.load());
    mModuleOrder[1] = (int)std::round(mSlot1.load());
    mModuleOrder[2] = (int)std::round(mSlot2.load());
    mModuleOrder[3] = (int)std::round(mSlot3.load());
    mModuleOrder[4] = (int)std::round(mSlot4.load());
    mModuleOrder[5] = (int)std::round(mSlot5.load());
}

void CurCrossbreedAudioProcessor::releaseResources() {}

//==============================================================================
void CurCrossbreedAudioProcessor::dispatchModule (int moduleIdx,
                                                   juce::AudioBuffer<float>& buf)
{
    auto* os = getActiveOS();

    switch (moduleIdx)
    {
        case 0: // Grindhouse
            if (mGhBypass.load() < 0.5f)
                mGrindhouse.processBlock (buf, os,
                    mGhInputGain.load(), mGhBias.load(), mGhStage1Amt.load(),
                    (int)std::round(mGhClipType.load()), mGhStage2Drive.load(),
                    mGhPreTilt.load(), mGhPostTone.load(), mGhOutputLevel.load(),
                    mGhBlend.load());
            break;
        case 1: // TapeBox-lite
            if (mTblBypass.load() < 0.5f)
                mTapeBoxLite.processBlock (buf, os,
                    (int)std::round(mTblTapeType.load()), mTblWowFlutter.load(),
                    mTblSaturation.load(), mTblTone.load(), mTblMix.load());
            break;
        case 2: // Voodoo Vibe
            if (mVvBypass.load() < 0.5f)
                mVoodooVibe.processBlock (buf,
                    mVvSpeed.load(), mVvIntensity.load(), mVvDrive.load(),
                    mVvLampBias.load(), mVvStereoWidth.load(),
                    (int)std::round (mVvMode.load()), mVvMix.load());
            break;
        case 3: // Memorec-lite
            if (mMrlBypass.load() < 0.5f)
                mMemorecLite.processBlock (buf,
                    mMrlTime.load(), mMrlFeedback.load(), mMrlTone.load(), mMrlMix.load());
            break;
        case 4: // Stereo Widener
            if (mSwBypass.load() < 0.5f)
                mStereoWidener.processBlock (buf, mSwWidth.load(), mSwCrossover.load());
            break;
        case 5: // Tone & Gate
            if (mTgBypass.load() < 0.5f)
                mToneGate.processBlock (buf,
                    mTgEqLow.load(), mTgEqLoMid.load(), mTgEqMid.load(),
                    mTgEqHiMid.load(), mTgEqHigh.load(),
                    mTgGateThresh.load(), mTgGateRelease.load(),
                    (int)std::round(mTgSidechainFlt.load()),
                    mTgGateBypass.load() > 0.5f);
            break;
        default: break;
    }
}

void CurCrossbreedAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int N    = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    if (mRackBypass.load() > 0.5f) return;

    // Rebuild order if any slot param changed
    if (mOrderDirty.exchange (false))
    {
        mModuleOrder[0] = juce::jlimit(0, 5, (int)std::round(mSlot0.load()));
        mModuleOrder[1] = juce::jlimit(0, 5, (int)std::round(mSlot1.load()));
        mModuleOrder[2] = juce::jlimit(0, 5, (int)std::round(mSlot2.load()));
        mModuleOrder[3] = juce::jlimit(0, 5, (int)std::round(mSlot3.load()));
        mModuleOrder[4] = juce::jlimit(0, 5, (int)std::round(mSlot4.load()));
        mModuleOrder[5] = juce::jlimit(0, 5, (int)std::round(mSlot5.load()));
    }

    // Update OS index if changed
    const int osIdx = (int)mOSIndex.load();
    if (osIdx != mCurrentOSIndex)
    {
        mCurrentOSIndex = osIdx;
        rebuildOS (mBlockSize);
    }

    // Apply rack input gain
    const float inGainLin = juce::Decibels::decibelsToGain (mRackInputGain.load());
    if (std::abs (inGainLin - 1.f) > 0.001f)
        for (int ch = 0; ch < numCh; ++ch)
            juce::FloatVectorOperations::multiply (buffer.getWritePointer(ch), inGainLin, N);

    // Store dry for global rack mix
    juce::AudioBuffer<float> rackDry (numCh, N);
    for (int ch = 0; ch < numCh; ++ch)
        rackDry.copyFrom (ch, 0, buffer, ch, 0, N);

    // Process each slot in current order
    for (int slot = 0; slot < 6; ++slot)
        dispatchModule (mModuleOrder[slot], buffer);

    // Apply rack output gain + global mix blend
    const float outGainLin = juce::Decibels::decibelsToGain (mRackOutputGain.load());
    const float mix        = mRackMix.load();
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* out = buffer.getWritePointer (ch);
        const auto* dry = rackDry.getReadPointer (ch);
        for (int i = 0; i < N; ++i)
            out[i] = dry[i] * (1.f - mix) + out[i] * mix * outGainLin;
    }
}

//==============================================================================
void CurCrossbreedAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CurCrossbreedAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* CurCrossbreedAudioProcessor::createEditor()
{
    return new CurCrossbreedAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CurCrossbreedAudioProcessor();
}

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>

//==============================================================================
// Module 1: Grindhouse — SansAmp-style two-stage saturation / DI
//==============================================================================
struct GrindhouseModule
{
    void prepare (double sr) noexcept;
    void reset   () noexcept;
    // os: borrowed from main processor for the Stage-2 clip (may be nullptr = no OS)
    void processBlock (juce::AudioBuffer<float>& buf,
                       juce::dsp::Oversampling<float>* os,
                       float inputGainDB,   float bias,         float stage1Amount,
                       int   clipType,      float stage2Drive,  float preTilt,
                       float postTone,      float outputLevelDB, float blend) noexcept;
private:
    double mSR        = 44100.0;
    float  mTiltLPL   = 0, mTiltLPR   = 0;   // pre-clip tilt EQ
    float  mPostLPL   = 0, mPostLPR   = 0;   // post-tone LP
    float  mOpAmpLPL  = 0, mOpAmpLPR  = 0;   // OpAmp mode internal LP
};

//==============================================================================
// Module 2: TapeBox-lite — tape saturation, wow/flutter, EQ darkening
//==============================================================================
struct TapeBoxLiteModule
{
    void prepare (double sr) noexcept;
    void reset   () noexcept;
    void processBlock (juce::AudioBuffer<float>& buf,
                       juce::dsp::Oversampling<float>* os,
                       int tapeType, float wowFlutter, float saturation,
                       float tone,   float mix) noexcept;
private:
    double mSR = 44100.0;
    static constexpr int kWFBuf = 8192;
    std::vector<float> mDelBufL, mDelBufR;
    int   mDelWritePos = 0;
    float mWowP  = 0, mWowP2 = 0, mWowP3 = 0;
    float mFlutP = 0, mFlutP2 = 0;
    float mEQBassL = 0, mEQBassR = 0;
    float mEQPresL = 0, mEQPresR = 0;
    float mToneLPL = 0, mToneLPR = 0;
    float mDCBlkL  = 0, mDCBlkR  = 0;

    float softSat (float x, float drive) noexcept;
    float tapeNL  (float x, int   tapeType) noexcept;
    float readLin (const std::vector<float>& buf, int wPos, float delay) noexcept;
};

//==============================================================================
// Module 3: Voodoo Vibe — LDR-based optical phaser / vibrato
// DSP ported from Loesje2000/VoodooVibe (PluginProcessor.cpp)
//==============================================================================
struct VoodooVibeModule
{
    void prepare (double sr) noexcept;
    void reset   () noexcept;
    void processBlock (juce::AudioBuffer<float>& buf,
                       float speed, float intensity, float drive,
                       float lampBias, float stereoWidth,
                       int mode, float mix) noexcept;
private:
    struct AllpassStage {
        float process (float x, float coeff) noexcept;
        float x1 = 0.f, y1 = 0.f;
    };
    struct Channel {
        void reset() noexcept;
        std::array<AllpassStage, 4> stages;
        float dcIn = 0.f, dcOut = 0.f;
    };

    static constexpr std::array<float, 4> kCaps   {{ 15.0e-9f, 220.0e-9f, 470.0e-12f, 4.7e-9f }};
    static constexpr std::array<float, 4> kLdrOff {{ 0.92f, 1.10f, 0.82f, 1.22f }};

    double mSR       = 44100.0;
    float  mInvSR    = 1.f / 44100.f;
    float  mLfoPhase = 0.f;
    float  mLampState = 0.5f;
    std::array<Channel, 2> mChan;

    float processPreamp    (float x, float driveAmt) const noexcept;
    float processDcBlock   (float x, Channel& ch)    const noexcept;
    float computeApCoeff   (float r, float c)        const noexcept;
    void  updateLamp       (float speed, float intensity, float bias) noexcept;
    float channelSpread    (int ch, float width)     const noexcept;
};

//==============================================================================
// Module 4: Memorec-lite — BBD-style echo with per-repeat HF darkening
//==============================================================================
struct MemoreclLiteModule
{
    void prepare (double sr) noexcept;
    void reset   () noexcept;
    void processBlock (juce::AudioBuffer<float>& buf,
                       float timeMs, float feedback, float tone, float mix) noexcept;
private:
    double mSR = 44100.0;
    std::vector<float> mDelBufL, mDelBufR;
    int   mDelWritePos = 0;
    float mDrkL = 0, mDrkR = 0;   // per-repeat darkening LP state

    float readLin (const std::vector<float>& buf, int wPos, float delay) noexcept;
};

//==============================================================================
// Module 5: Stereo Widener — frequency-dependent M-S widening above crossover
//==============================================================================
struct StereoWidenerModule
{
    void prepare (double sr) noexcept;
    void reset   () noexcept;
    void processBlock (juce::AudioBuffer<float>& buf,
                       float widthPct, float crossoverHz) noexcept;
private:
    double mSR = 44100.0;
    // Linkwitz-Riley 2nd-order: two cascaded one-pole LPs for mid and side
    float mMidLP1L  = 0, mMidLP1R  = 0, mMidLP2L  = 0, mMidLP2R  = 0;
    float mSideLP1L = 0, mSideLP1R = 0, mSideLP2L = 0, mSideLP2R = 0;
};

//==============================================================================
// Module 6: Tone & Gate — 5-band EQ + sidechain-filterable gate
//==============================================================================
struct ToneGateModule
{
    void prepare (double sr) noexcept;
    void reset   () noexcept;
    // eqLow..eqHigh: band gains in dB (±15), bands centred at ~80/400/1.5k/5k/12k Hz
    void processBlock (juce::AudioBuffer<float>& buf,
                       float eqLow,  float eqLoMid, float eqMid,
                       float eqHiMid, float eqHigh,
                       float gateThreshDB, float gateReleaseMs,
                       int sidechainFilter, bool gateBypass) noexcept;
private:
    double mSR = 44100.0;
    // Five one-pole LP integrators (crossover frequencies: 80/400/1500/5000/12000 Hz)
    float mLp0L = 0, mLp0R = 0;
    float mLp1L = 0, mLp1R = 0;
    float mLp2L = 0, mLp2R = 0;
    float mLp3L = 0, mLp3R = 0;
    float mLp4L = 0, mLp4R = 0;
    // Gate
    float mGateGain = 1.0f;
    float mScFltL = 0, mScFltR = 0;
};

//==============================================================================
// Main Processor
//==============================================================================
class CurCrossbreedAudioProcessor : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    CurCrossbreedAudioProcessor();
    ~CurCrossbreedAudioProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Cur Crossbreed"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

    // Processing order: mModuleOrder[slotIndex] = moduleIndex (0..5)
    // Slot 0 runs first. Default: sequential 0..5.
    std::array<int, 6> mModuleOrder { 0, 1, 2, 3, 4, 5 };

    // UI-facing string arrays
    static const juce::StringArray kOversampleChoices;
    static const juce::StringArray kGhClipTypeChoices;
    static const juce::StringArray kTblTapeTypeChoices;
    static const juce::StringArray kVvModeChoices;
    static const juce::StringArray kTgSidechainChoices;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    //==========================================================================
    // Oversampling — 4 objects (TapeBox pattern).
    // Active object is passed to saturation modules (Grindhouse, TapeBoxLite).
    //==========================================================================
    std::unique_ptr<juce::dsp::Oversampling<float>> mOS1x, mOS2x, mOS4x, mOS8x;
    juce::dsp::Oversampling<float>* getActiveOS() noexcept;
    int  mCurrentOSIndex = 0;
    void rebuildOS (int blockSize);

    //==========================================================================
    // Module instances — all 6 permanently allocated; mModuleOrder controls routing
    //==========================================================================
    GrindhouseModule    mGrindhouse;
    TapeBoxLiteModule   mTapeBoxLite;
    VoodooVibeModule    mVoodooVibe;
    MemoreclLiteModule  mMemorecLite;
    StereoWidenerModule mStereoWidener;
    ToneGateModule      mToneGate;

    double mSampleRate = 44100.0;
    int    mBlockSize  = 512;
    std::atomic<bool> mOrderDirty { false };

    //==========================================================================
    // Cached parameters (atomic for audio-thread safety)
    //==========================================================================
    // Rack globals
    std::atomic<float> mRackInputGain  { 0.f };
    std::atomic<float> mRackOutputGain { 0.f };
    std::atomic<float> mRackMix        { 1.f };
    std::atomic<float> mRackBypass     { 0.f };
    std::atomic<float> mOSIndex        { 0.f };
    // Slot order (float 0..5, round to int)
    std::atomic<float> mSlot0 { 0.f }, mSlot1 { 1.f }, mSlot2 { 2.f };
    std::atomic<float> mSlot3 { 3.f }, mSlot4 { 4.f }, mSlot5 { 5.f };
    // Grindhouse
    std::atomic<float> mGhInputGain   { 0.f };
    std::atomic<float> mGhBias        { 0.f };
    std::atomic<float> mGhStage1Amt   { 0.5f };
    std::atomic<float> mGhClipType    { 0.f };
    std::atomic<float> mGhStage2Drive { 0.5f };
    std::atomic<float> mGhPreTilt     { 0.f };
    std::atomic<float> mGhPostTone    { 0.5f };
    std::atomic<float> mGhOutputLevel { 0.f };
    std::atomic<float> mGhBlend       { 1.f };
    std::atomic<float> mGhBypass      { 0.f };
    // TapeBox-lite
    std::atomic<float> mTblTapeType   { 0.f };
    std::atomic<float> mTblWowFlutter { 0.25f };
    std::atomic<float> mTblSaturation { 0.4f };
    std::atomic<float> mTblTone       { 0.5f };
    std::atomic<float> mTblMix        { 1.f };
    std::atomic<float> mTblBypass     { 0.f };
    // Voodoo Vibe
    std::atomic<float> mVvBypass      { 0.f };
    std::atomic<float> mVvSpeed       { 3.f };
    std::atomic<float> mVvIntensity   { 0.7f };
    std::atomic<float> mVvDrive       { 0.3f };
    std::atomic<float> mVvLampBias    { 0.5f };
    std::atomic<float> mVvStereoWidth { 0.35f };
    std::atomic<float> mVvMode        { 0.f };
    std::atomic<float> mVvMix         { 0.5f };
    // Memorec-lite
    std::atomic<float> mMrlTime       { 250.f };
    std::atomic<float> mMrlFeedback   { 0.35f };
    std::atomic<float> mMrlTone       { 0.5f };
    std::atomic<float> mMrlMix        { 0.3f };
    std::atomic<float> mMrlBypass     { 0.f };
    // Stereo Widener
    std::atomic<float> mSwWidth       { 100.f };
    std::atomic<float> mSwCrossover   { 500.f };
    std::atomic<float> mSwBypass      { 0.f };
    // Tone & Gate
    std::atomic<float> mTgEqLow       { 0.f };
    std::atomic<float> mTgEqLoMid     { 0.f };
    std::atomic<float> mTgEqMid       { 0.f };
    std::atomic<float> mTgEqHiMid     { 0.f };
    std::atomic<float> mTgEqHigh      { 0.f };
    std::atomic<float> mTgGateThresh  { -40.f };
    std::atomic<float> mTgGateRelease { 100.f };
    std::atomic<float> mTgSidechainFlt{ 0.f };
    std::atomic<float> mTgGateBypass  { 0.f };
    std::atomic<float> mTgBypass      { 0.f };

    void resetAllState();
    void dispatchModule (int moduleIdx, juce::AudioBuffer<float>& buf);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurCrossbreedAudioProcessor)
};

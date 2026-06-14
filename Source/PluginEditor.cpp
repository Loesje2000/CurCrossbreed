#include "PluginEditor.h"
#include "RubblesonicLogo.h"

//==============================================================================
// Module colour tints per type (shown on left indicator strip)
static const juce::Colour kModuleColour[6] = {
    juce::Colour (0xFFBB4422),  // 0 Grindhouse — burnt orange
    juce::Colour (0xFF885533),  // 1 TapeBox-lite — warm brown
    juce::Colour (0xFF4477AA),  // 2 Voodoo Vibe — steel blue
    juce::Colour (0xFF557755),  // 3 Memorec-lite — sage green
    juce::Colour (0xFF7755AA),  // 4 Stereo Widener — muted violet
    juce::Colour (0xFF996633),  // 5 Tone & Gate — amber brown
};

const char* ModuleSlotComponent::moduleName (int type) noexcept
{
    switch (type) {
        case 0: return "GRINDHOUSE";
        case 1: return "TAPEBOX LITE";
        case 2: return "VOODOO VIBE";
        case 3: return "MEMOREC LITE";
        case 4: return "WIDENER";
        case 5: return "TONE & GATE";
        default: return "MODULE";
    }
}

//==============================================================================
// ModuleSlotComponent
//==============================================================================
ModuleSlotComponent::ModuleSlotComponent (int moduleType,
                                          CurCrossbreedAudioProcessor& proc)
    : mModuleType (moduleType), mProc (proc)
{
    auto& apvts = proc.apvts;

    // Common: bypass button (all module types)
    mBypassBtn.setClickingTogglesState (true);
    mBypassBtn.setName ("BYPASS");
    addAndMakeVisible (mBypassBtn);

    switch (moduleType)
    {
        //----------------------------------------------------------------------
        case 0: // Grindhouse
        {
            setupCombo (mGhClipTypeBox, mGhClipTypeLabel, "CLIP TYPE",
                        CurCrossbreedAudioProcessor::kGhClipTypeChoices);
            mGhClipTypeBox.setTooltip ("Clipping character: Soft = smooth, Hard = harsh, Asym = asymmetric, FET = transistor bite");
            setupKnob (mGhInputGain,   mGhInputGainLb,   "INPUT");   mGhInputGain.setTooltip   ("Input gain in dB before the first saturation stage");
            setupKnob (mGhBias,        mGhBiasLb,        "BIAS");    mGhBias.setTooltip        ("DC offset applied to the signal before clipping — adds even/odd harmonic asymmetry");
            setupKnob (mGhStage1Amt,   mGhStage1AmtLb,   "STAGE 1"); mGhStage1Amt.setTooltip   ("Amount of first-stage soft saturation");
            setupKnob (mGhStage2Drive, mGhStage2DriveLb, "DRIVE");   mGhStage2Drive.setTooltip  ("Drive into the second clipping stage — set the clip type first");
            setupKnob (mGhPreTilt,     mGhPreTiltLb,     "TILT");    mGhPreTilt.setTooltip      ("Tilt EQ before clipping: + boosts highs, - boosts lows");
            setupKnob (mGhPostTone,    mGhPostToneLb,    "TONE");    mGhPostTone.setTooltip     ("Post-clip tone: high-pass character, sweeps from dark to bright");
            setupKnob (mGhOutputLevel, mGhOutputLevelLb, "OUTPUT");  mGhOutputLevel.setTooltip  ("Output trim in dB after all processing");
            setupKnob (mGhBlend,       mGhBlendLb,       "BLEND");   mGhBlend.setTooltip        ("Dry/wet blend: 0 = dry, 1 = full effect");
            mBypassBtn.setTooltip ("Bypass this module");

            mBypassAttach      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "gh_bypass",      mBypassBtn);
            mGhClipTypeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "gh_clipType",  mGhClipTypeBox);
            mGhInputGainA      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_inputGain",   mGhInputGain);
            mGhBiasA           = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_bias",        mGhBias);
            mGhStage1AmtA      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_stage1Amt",   mGhStage1Amt);
            mGhStage2DriveA    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_stage2Drive", mGhStage2Drive);
            mGhPreTiltA        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_preTilt",     mGhPreTilt);
            mGhPostToneA       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_postTone",    mGhPostTone);
            mGhOutputLevelA    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_outputLevel", mGhOutputLevel);
            mGhBlendA          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gh_blend",       mGhBlend);
            break;
        }
        //----------------------------------------------------------------------
        case 1: // TapeBox-lite
        {
            setupCombo (mTblTapeTypeBox, mTblTapeTypeLabel, "TAPE",
                        CurCrossbreedAudioProcessor::kTblTapeTypeChoices);
            mTblTapeTypeBox.setTooltip ("Tape formulation: changes saturation curve and EQ colouration");
            setupKnob (mTblWowFlutter,  mTblWowFlutterLb,  "WOW/FLUT"); mTblWowFlutter.setTooltip ("Wow & flutter depth — slow and fast pitch modulation like a degraded tape machine");
            setupKnob (mTblSaturation,  mTblSaturationLb,  "SAT");       mTblSaturation.setTooltip ("Tape saturation amount — higher values compress and add harmonics");
            setupKnob (mTblTone,        mTblToneLb,        "TONE");       mTblTone.setTooltip       ("Tape head EQ: dark at low values, bright and present at high values");
            setupKnob (mTblMix,         mTblMixLb,         "MIX");        mTblMix.setTooltip        ("Dry/wet blend between clean and tape-processed signal");
            mBypassBtn.setTooltip ("Bypass this module");

            mBypassAttach       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "tbl_bypass",     mBypassBtn);
            mTblTapeTypeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "tbl_tapeType", mTblTapeTypeBox);
            mTblWowFlutterA     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tbl_wowFlutter", mTblWowFlutter);
            mTblSaturationA     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tbl_saturation", mTblSaturation);
            mTblToneA           = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tbl_tone",       mTblTone);
            mTblMixA            = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tbl_mix",        mTblMix);
            break;
        }
        //----------------------------------------------------------------------
        case 2: // Voodoo Vibe
        {
            setupCombo (mVvModeBox,      mVvModeLabel,      "MODE",
                        CurCrossbreedAudioProcessor::kVvModeChoices);
            mVvModeBox.setTooltip ("Phaser = allpass phase shift; Vibrato = pure pitch modulation with no dry signal");
            setupKnob  (mVvSpeed,        mVvSpeedLb,        "SPEED");  mVvSpeed.setTooltip       ("LFO rate in Hz — how fast the optical element sweeps");
            setupKnob  (mVvIntensity,    mVvIntensityLb,    "INTENS"); mVvIntensity.setTooltip    ("Depth of the LFO modulation into the allpass stages");
            setupKnob  (mVvDrive,        mVvDriveLb,        "DRIVE");  mVvDrive.setTooltip        ("Input drive into the lamp preamp — adds harmonic warmth");
            setupKnob  (mVvLampBias,     mVvLampBiasLb,     "LAMP");   mVvLampBias.setTooltip     ("LDR resting point — shifts the centre frequency of the sweep");
            setupKnob  (mVvStereoWidth,  mVvStereoWidthLb,  "WIDTH");  mVvStereoWidth.setTooltip  ("Phase offset between L and R LFOs — 0 = mono, 1 = wide stereo");
            setupKnob  (mVvMix,          mVvMixLb,          "MIX");    mVvMix.setTooltip          ("Dry/wet blend between clean and modulated signal");
            mBypassBtn.setTooltip ("Bypass this module");

            mBypassAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "vv_bypass",      mBypassBtn);
            mVvModeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "vv_mode",      mVvModeBox);
            mVvSpeedA      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vv_speed",       mVvSpeed);
            mVvIntensityA  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vv_intensity",   mVvIntensity);
            mVvDriveA      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vv_drive",       mVvDrive);
            mVvLampBiasA   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vv_lampBias",    mVvLampBias);
            mVvStereoWidthA= std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vv_stereoWidth", mVvStereoWidth);
            mVvMixA        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vv_mix",         mVvMix);
            break;
        }
        //----------------------------------------------------------------------
        case 3: // Memorec-lite
        {
            setupKnob (mMrlTime,     mMrlTimeLb,     "TIME");    mMrlTime.setTooltip     ("Delay time in milliseconds");
            setupKnob (mMrlFeedback, mMrlFeedbackLb, "FDBK");    mMrlFeedback.setTooltip ("Repeat feedback amount — higher values add more echoes");
            setupKnob (mMrlTone,     mMrlToneLb,     "TONE");    mMrlTone.setTooltip     ("BBD head EQ: lower cuts highs per repeat, simulating magnetic tape darkening");
            setupKnob (mMrlMix,      mMrlMixLb,      "MIX");     mMrlMix.setTooltip      ("Dry/wet blend between clean and delayed signal");
            mBypassBtn.setTooltip ("Bypass this module");

            mBypassAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "mrl_bypass",   mBypassBtn);
            mMrlTimeA       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mrl_time",     mMrlTime);
            mMrlFeedbackA   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mrl_feedback", mMrlFeedback);
            mMrlToneA       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mrl_tone",     mMrlTone);
            mMrlMixA        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mrl_mix",      mMrlMix);
            break;
        }
        //----------------------------------------------------------------------
        case 4: // Stereo Widener
        {
            setupKnob (mSwWidth,     mSwWidthLb,     "WIDTH");  mSwWidth.setTooltip    ("Stereo width percentage — 100 = normal, 200 = maximum widening above the crossover");
            setupKnob (mSwCrossover, mSwCrossoverLb, "XOVER");  mSwCrossover.setTooltip("Crossover frequency: widening is applied only to content above this point");
            mBypassBtn.setTooltip ("Bypass this module");

            mBypassAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "sw_bypass",   mBypassBtn);
            mSwWidthA      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sw_width",    mSwWidth);
            mSwCrossoverA  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sw_crossover",mSwCrossover);
            break;
        }
        //----------------------------------------------------------------------
        case 5: // Tone & Gate
        {
            setupKnob (mTgEqLow,   mTgEqLowLb,   "LOW");    mTgEqLow.setTooltip   ("Low shelf gain in dB — shelves at ~80 Hz");
            setupKnob (mTgEqLoMid, mTgEqLoMidLb, "LO MID"); mTgEqLoMid.setTooltip ("Lo-mid bell gain in dB — centred around 400 Hz");
            setupKnob (mTgEqMid,   mTgEqMidLb,   "MID");    mTgEqMid.setTooltip   ("Mid bell gain in dB — centred around 1.5 kHz");
            setupKnob (mTgEqHiMid, mTgEqHiMidLb, "HI MID"); mTgEqHiMid.setTooltip ("Hi-mid bell gain in dB — centred around 5 kHz");
            setupKnob (mTgEqHigh,  mTgEqHighLb,  "HIGH");   mTgEqHigh.setTooltip  ("High shelf gain in dB — shelves above ~12 kHz");
            setupKnob (mTgGateThresh,  mTgGateThreshLb,  "THRESH");  mTgGateThresh.setTooltip  ("Gate threshold in dB — signal below this level is attenuated");
            setupKnob (mTgGateRelease, mTgGateReleaseLb, "RELEASE"); mTgGateRelease.setTooltip ("Gate release time in milliseconds");
            setupCombo (mTgSidechainBox, mTgSidechainLabel, "SIDECHAIN",
                        CurCrossbreedAudioProcessor::kTgSidechainChoices);
            mTgSidechainBox.setTooltip ("Sidechain high-pass filter for the gate detector — filters low-end before gate sensing so bass doesn't trigger the gate");

            mTgGateBypassBtn.setClickingTogglesState (true);
            mTgGateBypassBtn.setName ("BYPASS");
            mTgGateBypassBtn.setTooltip ("Bypass the gate section only (EQ still active)");
            addAndMakeVisible (mTgGateBypassBtn);
            mBypassBtn.setTooltip ("Bypass this module");

            mBypassAttach       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "tg_bypass",          mBypassBtn);
            mTgSidechainAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "tg_sidechainFilter", mTgSidechainBox);
            mTgEqLowA           = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_eqLow",    mTgEqLow);
            mTgEqLoMidA         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_eqLoMid",  mTgEqLoMid);
            mTgEqMidA           = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_eqMid",    mTgEqMid);
            mTgEqHiMidA         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_eqHiMid",  mTgEqHiMid);
            mTgEqHighA          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_eqHigh",   mTgEqHigh);
            mTgGateThreshA      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_gateThresh",   mTgGateThresh);
            mTgGateReleaseA     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tg_gateRelease",  mTgGateRelease);
            mTgGateBypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "tg_gateBypass",   mTgGateBypassBtn);
            break;
        }
        default: break;
    }
}

ModuleSlotComponent::~ModuleSlotComponent()
{
    // Attachments must be destroyed before controls
    mBypassAttach.reset();
    mGhClipTypeAttach.reset();  mTblTapeTypeAttach.reset();
    mVvModeAttach.reset();      mTgSidechainAttach.reset();
    mGhInputGainA.reset();      mGhBiasA.reset();
    mGhStage1AmtA.reset();      mGhStage2DriveA.reset();
    mGhPreTiltA.reset();        mGhPostToneA.reset();
    mGhOutputLevelA.reset();    mGhBlendA.reset();
    mTblWowFlutterA.reset();    mTblSaturationA.reset();
    mTblToneA.reset();          mTblMixA.reset();
    mVvSpeedA.reset();          mVvIntensityA.reset();
    mVvDriveA.reset();          mVvLampBiasA.reset();
    mVvStereoWidthA.reset();    mVvMixA.reset();
    mMrlTimeA.reset();          mMrlFeedbackA.reset();
    mMrlToneA.reset();          mMrlMixA.reset();
    mSwWidthA.reset();          mSwCrossoverA.reset();
    mTgEqLowA.reset();          mTgEqLoMidA.reset();
    mTgEqMidA.reset();          mTgEqHiMidA.reset();
    mTgEqHighA.reset();         mTgGateThreshA.reset();
    mTgGateReleaseA.reset();    mTgGateBypassAttach.reset();
}

//==============================================================================
void ModuleSlotComponent::setupKnob (juce::Slider& knob, juce::Label& lbl,
                                      const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (knob);
    lbl.setText (text, juce::dontSendNotification);
    lbl.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lbl);
}

void ModuleSlotComponent::setupCombo (juce::ComboBox& box, juce::Label& lbl,
                                       const juce::String& text,
                                       const juce::StringArray& items)
{
    box.clear (juce::dontSendNotification);
    for (int i = 0; i < items.size(); ++i)
        box.addItem (items[i], i + 1);
    box.setColour (juce::ComboBox::textColourId,       juce::Colours::transparentBlack);
    box.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    box.setColour (juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
    addAndMakeVisible (box);
    lbl.setText (text, juce::dontSendNotification);
    lbl.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (lbl);
}

//==============================================================================
void ModuleSlotComponent::resized()
{
    const int H  = getHeight();   // 109
    const int kCtrl = 212;        // controls start X
    const int kKnobY = 16, kKnobW = 48, kKnobH = 48, kLblY = 66, kLblH = 12;
    const int kStep  = 56;
    const int kComboW = 110, kComboH = 22, kComboY = (H - kComboH) / 2 - 14;
    const int kComboLblY = kComboY + kComboH + 4, kComboLblH = 12;

    // Bypass button: x=26, centred vertically, 52×24
    mBypassBtn.setBounds (26, (H - 24) / 2, 52, 24);

    int x = kCtrl;

    switch (mModuleType)
    {
        //----------------------------------------------------------------------
        case 0: // Grindhouse
        {
            mGhClipTypeLabel.setBounds (x, kComboLblY, kComboW, kComboLblH);
            mGhClipTypeBox.setBounds   (x, kComboY,    kComboW, kComboH);
            x += kComboW + 8;

            auto placeKnob = [&](juce::Slider& k, juce::Label& l) {
                k.setBounds (x, kKnobY, kKnobW, kKnobH);
                l.setBounds (x, kLblY,  kKnobW, kLblH);
                x += kStep;
            };
            placeKnob (mGhInputGain,   mGhInputGainLb);
            placeKnob (mGhBias,        mGhBiasLb);
            placeKnob (mGhStage1Amt,   mGhStage1AmtLb);
            placeKnob (mGhStage2Drive, mGhStage2DriveLb);
            placeKnob (mGhPreTilt,     mGhPreTiltLb);
            placeKnob (mGhPostTone,    mGhPostToneLb);
            placeKnob (mGhOutputLevel, mGhOutputLevelLb);
            placeKnob (mGhBlend,       mGhBlendLb);
            break;
        }
        //----------------------------------------------------------------------
        case 1: // TapeBox-lite
        {
            mTblTapeTypeLabel.setBounds (x, kComboLblY, kComboW, kComboLblH);
            mTblTapeTypeBox.setBounds   (x, kComboY,    kComboW, kComboH);
            x += kComboW + 8;

            auto placeKnob = [&](juce::Slider& k, juce::Label& l) {
                k.setBounds (x, kKnobY, kKnobW, kKnobH);
                l.setBounds (x, kLblY,  kKnobW, kLblH);
                x += kStep;
            };
            placeKnob (mTblWowFlutter, mTblWowFlutterLb);
            placeKnob (mTblSaturation, mTblSaturationLb);
            placeKnob (mTblTone,       mTblToneLb);
            placeKnob (mTblMix,        mTblMixLb);
            break;
        }
        //----------------------------------------------------------------------
        case 2: // Voodoo Vibe
        {
            mVvModeLabel.setBounds (x, kComboLblY, kComboW, kComboLblH);
            mVvModeBox.setBounds   (x, kComboY,    kComboW, kComboH);
            x += kComboW + 8;

            auto placeKnob = [&](juce::Slider& k, juce::Label& l) {
                k.setBounds (x, kKnobY, kKnobW, kKnobH);
                l.setBounds (x, kLblY,  kKnobW, kLblH);
                x += kStep;
            };
            placeKnob (mVvSpeed,       mVvSpeedLb);
            placeKnob (mVvIntensity,   mVvIntensityLb);
            placeKnob (mVvDrive,       mVvDriveLb);
            placeKnob (mVvLampBias,    mVvLampBiasLb);
            placeKnob (mVvStereoWidth, mVvStereoWidthLb);
            placeKnob (mVvMix,         mVvMixLb);
            break;
        }
        //----------------------------------------------------------------------
        case 3: // Memorec-lite
        {
            auto placeKnob = [&](juce::Slider& k, juce::Label& l) {
                k.setBounds (x, kKnobY, kKnobW, kKnobH);
                l.setBounds (x, kLblY,  kKnobW, kLblH);
                x += kStep;
            };
            placeKnob (mMrlTime,     mMrlTimeLb);
            placeKnob (mMrlFeedback, mMrlFeedbackLb);
            placeKnob (mMrlTone,     mMrlToneLb);
            placeKnob (mMrlMix,      mMrlMixLb);
            break;
        }
        //----------------------------------------------------------------------
        case 4: // Stereo Widener
        {
            mSwWidth.setBounds     (x,       kKnobY, kKnobW, kKnobH);
            mSwWidthLb.setBounds   (x,       kLblY,  kKnobW, kLblH); x += kStep;
            mSwCrossover.setBounds (x,       kKnobY, kKnobW, kKnobH);
            mSwCrossoverLb.setBounds(x,      kLblY,  kKnobW, kLblH);
            break;
        }
        //----------------------------------------------------------------------
        case 5: // Tone & Gate — 5-band EQ + gate
        {
            auto placeKnob = [&](juce::Slider& k, juce::Label& l) {
                k.setBounds (x, kKnobY, kKnobW, kKnobH);
                l.setBounds (x, kLblY,  kKnobW, kLblH);
                x += kStep;
            };
            placeKnob (mTgEqLow,   mTgEqLowLb);
            placeKnob (mTgEqLoMid, mTgEqLoMidLb);
            placeKnob (mTgEqMid,   mTgEqMidLb);
            placeKnob (mTgEqHiMid, mTgEqHiMidLb);
            placeKnob (mTgEqHigh,  mTgEqHighLb);
            x += 8;

            placeKnob (mTgGateThresh,  mTgGateThreshLb);
            placeKnob (mTgGateRelease, mTgGateReleaseLb);
            x += 4;

            mTgSidechainLabel.setBounds(x, kComboLblY, kComboW, kComboLblH);
            mTgSidechainBox.setBounds  (x, kComboY,    kComboW, kComboH);
            x += kComboW + 8;

            mTgGateBypassBtn.setBounds (x, (H - 24) / 2, 58, 24);
            break;
        }
        default: break;
    }
}

//==============================================================================
void ModuleSlotComponent::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    const juce::Colour modCol = kModuleColour[mModuleType];

    // Coloured left indicator bar
    g.setColour (modCol);
    g.fillRect  (22, 0, 4, H);

    // Drag handle dots
    g.setColour (modCol.withAlpha (0.8f));
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 2; ++col)
            g.fillEllipse (6.f + col * 6.f, (float)(H/2 - 9 + row * 9), 3.f, 3.f);

    // Module name
    g.setColour (modCol.brighter (0.4f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (11.5f).withStyle ("Bold")));
    g.drawFittedText (moduleName (mModuleType), 84, 0, 126, H,
                      juce::Justification::centredLeft, 2);

    // Vertical separator after name
    g.setColour (juce::Colour (0xFF5A4030));
    g.fillRect (210, 8, 1, H - 16);

}

//==============================================================================
// CurCrossbreedAudioProcessorEditor
//==============================================================================
CurCrossbreedAudioProcessorEditor::CurCrossbreedAudioProcessorEditor
    (CurCrossbreedAudioProcessor& p)
    : AudioProcessorEditor (&p), mProc (p), mLAF (*this)
{
    setLookAndFeel (&mLAF);

    // Create 6 slot components before setSize() so resized() finds them ready
    for (int m = 0; m < 6; ++m)
    {
        mSlots[m] = std::make_unique<ModuleSlotComponent> (m, p);
        addAndMakeVisible (*mSlots[m]);
    }

    // Sync visual order from processor's module order
    mVisualOrder = p.mModuleOrder;

    // Global controls
    setupKnob  (mInputGainKnob,  mInputGainLabel,  "IN GAIN");
    mInputGainKnob.setTooltip  ("Rack input gain in dB — applied before any modules");
    setupKnob  (mOutputGainKnob, mOutputGainLabel, "OUT GAIN");
    mOutputGainKnob.setTooltip ("Rack output gain in dB — applied after all modules");
    setupKnob  (mMixKnob,        mMixLabel,        "MIX");
    mMixKnob.setTooltip        ("Global dry/wet: 0 = fully dry (all modules bypassed), 1 = fully wet");
    setupCombo (mOversampleBox,  mOversampleLabel, "OVERSAMPLE",
                CurCrossbreedAudioProcessor::kOversampleChoices);
    mOversampleBox.setTooltip  ("Oversampling factor — higher values reduce aliasing from saturation modules at the cost of CPU");

    mBypassBtn.setClickingTogglesState (true);
    mBypassBtn.setName ("BYPASS");
    mBypassBtn.setTooltip ("Bypass the entire rack — passes audio through unprocessed");
    addAndMakeVisible (mBypassBtn);

    mInputGainA  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                   (p.apvts, "rack_inputGain",  mInputGainKnob);
    mOutputGainA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                   (p.apvts, "rack_outputGain", mOutputGainKnob);
    mMixA        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                   (p.apvts, "rack_mix",        mMixKnob);
    mOversampleA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
                   (p.apvts, "rack_oversample", mOversampleBox);
    mBypassA     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                   (p.apvts, "rack_bypass",     mBypassBtn);

    setSize (kW, kH);  // triggers resized() — all children are ready by now
}

CurCrossbreedAudioProcessorEditor::~CurCrossbreedAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    mInputGainA.reset();  mOutputGainA.reset();  mMixA.reset();
    mOversampleA.reset(); mBypassA.reset();
}

//==============================================================================
void CurCrossbreedAudioProcessorEditor::setupKnob (juce::Slider& knob,
                                                    juce::Label& label,
                                                    const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (knob);
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions().withHeight (10.5f)));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void CurCrossbreedAudioProcessorEditor::setupCombo (juce::ComboBox& box,
                                                     juce::Label& label,
                                                     const juce::String& text,
                                                     const juce::StringArray& items)
{
    box.clear (juce::dontSendNotification);
    for (int i = 0; i < items.size(); ++i)
        box.addItem (items[i], i + 1);
    box.setColour (juce::ComboBox::textColourId,       juce::Colours::transparentBlack);
    box.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    box.setColour (juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
    addAndMakeVisible (box);
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions().withHeight (10.5f)));
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

//==============================================================================
void CurCrossbreedAudioProcessorEditor::updateSlotPositions()
{
    for (int row = 0; row < 6; ++row)
    {
        const int moduleType = mVisualOrder[row];
        mSlots[moduleType]->setBounds (0, rowY (row), kW, kSlotH);
    }
}

void CurCrossbreedAudioProcessorEditor::commitNewOrder (int fromRow, int toRow)
{
    if (fromRow == toRow || fromRow < 0 || toRow < 0) return;

    // Rotate entries between fromRow and toRow
    const int moduleBeingMoved = mVisualOrder[fromRow];
    if (toRow > fromRow)
        for (int i = fromRow; i < toRow; ++i) mVisualOrder[i] = mVisualOrder[i + 1];
    else
        for (int i = fromRow; i > toRow; --i) mVisualOrder[i] = mVisualOrder[i - 1];
    mVisualOrder[toRow] = moduleBeingMoved;

    updateSlotPositions();

    // Persist to APVTS (processor rebuilds mModuleOrder on next processBlock)
    for (int row = 0; row < 6; ++row)
    {
        juce::String id ("slot"); id += row;
        if (auto* param = mProc.apvts.getParameter (id))
            param->setValueNotifyingHost (
                param->convertTo0to1 ((float)mVisualOrder[row]));
    }
}

//==============================================================================
void CurCrossbreedAudioProcessorEditor::resized()
{
    // Header global controls
    const int kKnobSize = 44, kKnobY = 6, kLblY = 52, kLblH = 11;
    int x = kW - kMargin;

    // Bypass button — far right
    x -= 70;
    mBypassBtn.setBounds (x, 28, 68, 26);
    x -= 12;

    // Oversample combo
    x -= 110;
    mOversampleLabel.setBounds (x, 14, 110, 12);
    mOversampleBox.setBounds   (x, 28, 110, 22);
    x -= 12;

    // Knobs from right: Mix, Out Gain, In Gain
    for (auto [knob, lbl] : std::initializer_list<std::pair<juce::Slider*, juce::Label*>>{
        { &mMixKnob, &mMixLabel },
        { &mOutputGainKnob, &mOutputGainLabel },
        { &mInputGainKnob, &mInputGainLabel } })
    {
        x -= kKnobSize;
        knob->setBounds (x, kKnobY, kKnobSize, kKnobSize);
        lbl->setBounds  (x, kLblY,  kKnobSize, kLblH);
        x -= 12;
    }

    updateSlotPositions();
}

//==============================================================================
void CurCrossbreedAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    g.fillAll (colBg);

    // Header background
    g.setColour (colHeader);
    g.fillRect (0, 0, W, kHeaderH);
    g.setColour (colAccent);
    g.fillRect (0, kHeaderH - 2, W, 2);

    // Logo
    {
        auto logo = juce::ImageCache::getFromMemory (
            RubblesonicLogo::pngData, RubblesonicLogo::pngSize);
        if (logo.isValid())
        {
            const float lW = 118.f, lH = 44.f, lX = (float)kMargin, lY = 8.f;
            const float cx = lX + lW * 0.5f, cy = lY + lH * 0.5f;
            const float angle = -6.f * juce::MathConstants<float>::pi / 180.f;
            juce::Graphics::ScopedSaveState save (g);
            g.addTransform (juce::AffineTransform::rotation (angle, cx, cy));
            g.drawImage (logo, (int)lX, (int)lY, (int)lW, (int)lH,
                         0, 0, logo.getWidth(), logo.getHeight());
        }
    }

    // Plugin title
    g.setColour (colText);
    g.setFont   (juce::Font (juce::FontOptions().withHeight (24.f).withStyle ("Bold")));
    g.drawText  ("CUR CROSSBREED", kMargin, 42, 320, 32, juce::Justification::centredLeft);

    // Tagline
    g.setColour (colAccent.withAlpha (0.7f));
    g.setFont   (juce::Font (juce::FontOptions().withHeight (10.f)));
    g.drawText  ("Rubblesonic  /  Multi-Effects Character Rack  /  VST3+AU",
                 kMargin, 66, 500, 14, juce::Justification::centredLeft);

    // Module rows background
    for (int row = 0; row < 6; ++row)
    {
        const juce::Colour rowCol = (row % 2 == 0) ? colPanel : colPanelAlt;
        g.setColour (rowCol);
        g.fillRect  (0, rowY(row), W, kSlotH);
        // Bottom separator
        g.setColour (colSepLine);
        g.fillRect  (0, rowY(row) + kSlotH - 1, W, 1);
        // Row number
        g.setColour (colSubtext.withAlpha (0.5f));
        g.setFont   (juce::Font (juce::FontOptions().withHeight (9.f)));
        g.drawText  (juce::String (row + 1), W - 18, rowY(row) + 4, 14, 12,
                     juce::Justification::centred);
    }

    // Drag overlay while dragging
    if (mIsDragging && mDragModule >= 0)
        paintDragOverlay (g);
}

//==============================================================================
void CurCrossbreedAudioProcessorEditor::paintDragOverlay (juce::Graphics& g)
{
    // Show where the dragged slot will drop
    const int targetRow = juce::jlimit (0, 5, rowAtY (mDragCurrentY));
    g.setColour (colAccent.withAlpha (0.35f));
    g.fillRect  (0, rowY (targetRow), kW, kSlotH);
    g.setColour (colAccent);
    g.fillRect  (0, rowY (targetRow), kW, 2);
    g.fillRect  (0, rowY (targetRow) + kSlotH - 2, kW, 2);
}

//==============================================================================
void CurCrossbreedAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // Check if click is on a drag handle of a slot component
    for (int row = 0; row < 6; ++row)
    {
        const int moduleType = mVisualOrder[row];
        if (!mSlots[moduleType]) continue;
        const auto slotBounds = mSlots[moduleType]->getBounds();
        if (!slotBounds.contains (e.position.toInt())) continue;
        // Is it in the handle zone (left 22px of the slot)?
        const auto localPos = e.position.toInt() - slotBounds.getTopLeft();
        if (localPos.x < 22)
        {
            mDragModule   = moduleType;
            mDragStartRow = row;
            mDragCurrentY = e.position.toInt().y;
            mIsDragging   = false; // will set true on first drag
            return;
        }
    }
}

void CurCrossbreedAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (mDragModule < 0) return;
    mDragCurrentY = e.position.toInt().y;
    if (!mIsDragging && std::abs (mDragCurrentY - rowY(mDragStartRow)) > 4)
        mIsDragging = true;
    if (mIsDragging)
        repaint();
}

void CurCrossbreedAudioProcessorEditor::mouseUp (const juce::MouseEvent& e)
{
    if (mDragModule < 0) return;
    if (mIsDragging)
    {
        const int targetRow = juce::jlimit (0, 5, rowAtY (mDragCurrentY));
        commitNewOrder (mDragStartRow, targetRow);
        repaint();
    }
    mDragModule  = -1;
    mIsDragging  = false;
}

//==============================================================================
// CurCrossbreedLAF  (same visual language as TapeBoxLAF)
//==============================================================================
void CurCrossbreedAudioProcessorEditor::CurCrossbreedLAF::drawRotarySlider (
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const float radius = juce::jmin (w * 0.5f, h * 0.5f) - 4.f;
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (cx, cy, radius, radius, 0.f, startAngle, endAngle, true);
    g.setColour (owner.colKnobTrack);
    g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    juce::Path fill;
    fill.addCentredArc (cx, cy, radius, radius, 0.f, startAngle, angle, true);
    g.setColour (owner.colKnobFill);
    g.strokePath (fill, juce::PathStrokeType (3.f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const float kr = radius * 0.63f;
    juce::ColourGradient grad (owner.colPanel.brighter (0.15f), cx - kr*.3f, cy - kr*.3f,
                               owner.colPanel.darker  (0.2f),  cx + kr*.3f, cy + kr*.5f, true);
    g.setGradientFill (grad);
    g.fillEllipse (cx-kr, cy-kr, kr*2.f, kr*2.f);
    g.setColour (owner.colKnobTrack.brighter (0.1f));
    g.drawEllipse (cx-kr, cy-kr, kr*2.f, kr*2.f, 1.1f);

    juce::Path ptr;
    ptr.addRectangle (-1.2f, -kr, 2.4f, kr * 0.6f);
    ptr.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (owner.colKnobThumb);
    g.fillPath (ptr);
}

void CurCrossbreedAudioProcessorEditor::CurCrossbreedLAF::drawComboBox (
    juce::Graphics& g, int width, int height, bool,
    int, int, int, int, juce::ComboBox& box)
{
    const juce::Rectangle<float> b (0.f, 0.f, (float)width, (float)height);
    g.setColour (owner.colKnobTrack);   g.fillRoundedRectangle (b, 3.f);
    g.setColour (owner.colSepLine);     g.drawRoundedRectangle (b.reduced(0.5f), 3.f, 1.f);
    const float ax = width - 15.f, ay = height * 0.5f;
    juce::Path arr;
    arr.addTriangle (ax, ay-3.f, ax+5.f, ay-3.f, ax+2.5f, ay+3.f);
    g.setColour (owner.colKnobFill); g.fillPath (arr);
    g.setColour (owner.colText);
    g.setFont (juce::Font (juce::FontOptions().withHeight (10.5f)));
    g.drawFittedText (box.getText(), 6, 0, width-20, height,
                      juce::Justification::centredLeft, 1);
}

void CurCrossbreedAudioProcessorEditor::CurCrossbreedLAF::drawPopupMenuBackground (
    juce::Graphics& g, int width, int height)
{
    g.fillAll (owner.colPanel);
    g.setColour (owner.colSepLine);
    g.drawRect (0, 0, width, height);
}

void CurCrossbreedAudioProcessorEditor::CurCrossbreedLAF::drawPopupMenuItem (
    juce::Graphics& g, const juce::Rectangle<int>& area,
    bool isSep, bool, bool isHigh, bool isTick, bool,
    const juce::String& text, const juce::String&,
    const juce::Drawable*, const juce::Colour*)
{
    if (isSep)
    {
        g.setColour (owner.colSepLine);
        g.fillRect (area.getX()+4, area.getCentreY(), area.getWidth()-8, 1);
        return;
    }
    g.setColour (isHigh ? owner.colKnobTrack.brighter(0.1f) : owner.colPanel);
    g.fillRect  (area);
    g.setColour (isTick ? owner.colKnobFill : owner.colText);
    g.setFont   (juce::Font (juce::FontOptions().withHeight (10.5f)));
    g.drawFittedText (text, area.getX()+8, area.getY(),
                      area.getWidth()-12, area.getHeight(),
                      juce::Justification::centredLeft, 1);
}

void CurCrossbreedAudioProcessorEditor::CurCrossbreedLAF::drawButtonBackground (
    juce::Graphics& g, juce::Button& btn, const juce::Colour&, bool, bool)
{
    const bool on = btn.getToggleState();
    const juce::Colour bg = (btn.getName() == "BYPASS")
        ? (on ? owner.colBypassOn  : owner.colButtonOff)
        : (on ? owner.colButtonOn  : owner.colButtonOff);
    const auto b = btn.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (bg);                              g.fillRoundedRectangle (b, 3.f);
    g.setColour (on ? bg.brighter(0.3f) : owner.colSepLine);
    g.drawRoundedRectangle (b, 3.f, 1.f);
}

void CurCrossbreedAudioProcessorEditor::CurCrossbreedLAF::drawButtonText (
    juce::Graphics& g, juce::TextButton& btn, bool, bool)
{
    g.setColour (btn.getToggleState() ? owner.colKnobThumb : owner.colSubtext);
    g.setFont   (juce::Font (juce::FontOptions().withHeight (9.5f).withStyle ("Bold")));
    g.drawFittedText (btn.getButtonText(), btn.getLocalBounds(),
                      juce::Justification::centred, 1);
}

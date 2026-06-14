#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CurCrossbreedAudioProcessorEditor;

//==============================================================================
// One rack slot: owns all controls for its fixed module type.
// The parent editor manages visual row ordering and drag-to-reorder.
//==============================================================================
class ModuleSlotComponent : public juce::Component
{
public:
    explicit ModuleSlotComponent (int moduleType, CurCrossbreedAudioProcessor& proc);
    ~ModuleSlotComponent() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

    int  getModuleType () const noexcept { return mModuleType; }
    // Returns true if localPos is in the drag-handle strip (left 22px)
    bool isHandleHit   (juce::Point<int> localPos) const noexcept { return localPos.x < 22; }

    static const char* moduleName (int type) noexcept;

private:
    int mModuleType;
    CurCrossbreedAudioProcessor& mProc;

    // ── Common ───────────────────────────────────────────────────────────────
    juce::TextButton mBypassBtn { "BYP" };

    // ── Grindhouse (type 0) ──────────────────────────────────────────────────
    juce::ComboBox mGhClipTypeBox;
    juce::Label    mGhClipTypeLabel;
    juce::Slider   mGhInputGain,   mGhBias,        mGhStage1Amt,   mGhStage2Drive;
    juce::Slider   mGhPreTilt,     mGhPostTone,    mGhOutputLevel, mGhBlend;
    juce::Label    mGhInputGainLb, mGhBiasLb,      mGhStage1AmtLb, mGhStage2DriveLb;
    juce::Label    mGhPreTiltLb,   mGhPostToneLb,  mGhOutputLevelLb, mGhBlendLb;

    // ── TapeBox-lite (type 1) ────────────────────────────────────────────────
    juce::ComboBox mTblTapeTypeBox;
    juce::Label    mTblTapeTypeLabel;
    juce::Slider   mTblWowFlutter, mTblSaturation, mTblTone,  mTblMix;
    juce::Label    mTblWowFlutterLb, mTblSaturationLb, mTblToneLb, mTblMixLb;

    // ── Voodoo Vibe (type 2) ────────────────────────────────────────────────
    juce::ComboBox mVvModeBox;
    juce::Label    mVvModeLabel;
    juce::Slider   mVvSpeed, mVvIntensity, mVvDrive, mVvLampBias, mVvStereoWidth, mVvMix;
    juce::Label    mVvSpeedLb, mVvIntensityLb, mVvDriveLb, mVvLampBiasLb, mVvStereoWidthLb, mVvMixLb;

    // ── Memorec-lite (type 3) ────────────────────────────────────────────────
    juce::Slider mMrlTime, mMrlFeedback, mMrlTone, mMrlMix;
    juce::Label  mMrlTimeLb, mMrlFeedbackLb, mMrlToneLb, mMrlMixLb;

    // ── Stereo Widener (type 4) ──────────────────────────────────────────────
    juce::Slider mSwWidth, mSwCrossover;
    juce::Label  mSwWidthLb, mSwCrossoverLb;

    // ── Tone & Gate (type 5) ─────────────────────────────────────────────────
    juce::ComboBox   mTgSidechainBox;
    juce::Label      mTgSidechainLabel;
    juce::Slider     mTgEqLow, mTgEqLoMid, mTgEqMid, mTgEqHiMid, mTgEqHigh;
    juce::Label      mTgEqLowLb, mTgEqLoMidLb, mTgEqMidLb, mTgEqHiMidLb, mTgEqHighLb;
    juce::Slider     mTgGateThresh, mTgGateRelease;
    juce::Label      mTgGateThreshLb, mTgGateReleaseLb;
    juce::TextButton mTgGateBypassBtn { "GATE" };

    // ── APVTS attachments ────────────────────────────────────────────────────
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mBypassAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        mGhClipTypeAttach, mTblTapeTypeAttach, mTgSidechainAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mVvModeAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        mGhInputGainA,    mGhBiasA,        mGhStage1AmtA,   mGhStage2DriveA,
        mGhPreTiltA,      mGhPostToneA,    mGhOutputLevelA, mGhBlendA,
        mTblWowFlutterA,  mTblSaturationA, mTblToneA,       mTblMixA,
        mVvSpeedA,        mVvIntensityA,   mVvDriveA,       mVvLampBiasA,
        mVvStereoWidthA,  mVvMixA,
        mMrlTimeA,        mMrlFeedbackA,   mMrlToneA,       mMrlMixA,
        mSwWidthA,        mSwCrossoverA,
        mTgEqLowA,        mTgEqLoMidA,     mTgEqMidA,       mTgEqHiMidA,
        mTgEqHighA,       mTgGateThreshA,  mTgGateReleaseA;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mTgGateBypassAttach;

    void setupKnob  (juce::Slider&, juce::Label&, const juce::String& labelText);
    void setupCombo (juce::ComboBox&, juce::Label&, const juce::String& labelText,
                     const juce::StringArray& items);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleSlotComponent)
};

//==============================================================================
class CurCrossbreedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CurCrossbreedAudioProcessorEditor (CurCrossbreedAudioProcessor&);
    ~CurCrossbreedAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    CurCrossbreedAudioProcessor& mProc;

    // ── Layout constants ─────────────────────────────────────────────────────
    static constexpr int kW       = 1100;
    static constexpr int kH       = 760;
    static constexpr int kHeaderH = 86;
    static constexpr int kSlotH   = 109;
    static constexpr int kMargin  = 12;

    // ── Colours ──────────────────────────────────────────────────────────────
    const juce::Colour colBg        { 0xFF2C1F0E };
    const juce::Colour colHeader    { 0xFF221508 };
    const juce::Colour colPanel     { 0xFF372511 };
    const juce::Colour colPanelAlt  { 0xFF3E2A14 };
    const juce::Colour colAccent    { 0xFFC8A840 };
    const juce::Colour colText      { 0xFFE8D5A3 };
    const juce::Colour colSubtext   { 0xFF9A8060 };
    const juce::Colour colKnobTrack { 0xFF4A3520 };
    const juce::Colour colKnobFill  { 0xFFB8860B };
    const juce::Colour colKnobThumb { 0xFFE8D5A0 };
    const juce::Colour colSepLine   { 0xFF5A4030 };
    const juce::Colour colButtonOn  { 0xFFB8860B };
    const juce::Colour colButtonOff { 0xFF3D2A12 };
    const juce::Colour colBypassOn  { 0xFF8B1A1A };

    // ── Look and feel ────────────────────────────────────────────────────────
    struct CurCrossbreedLAF : public juce::LookAndFeel_V4
    {
        explicit CurCrossbreedLAF (CurCrossbreedAudioProcessorEditor& e) : owner (e) {}

        void drawRotarySlider    (juce::Graphics&, int x, int y, int w, int h,
                                  float sliderPos, float startAngle, float endAngle,
                                  juce::Slider&) override;
        void drawComboBox        (juce::Graphics&, int w, int h, bool,
                                  int, int, int, int, juce::ComboBox&) override;
        void drawPopupMenuBackground (juce::Graphics&, int, int) override;
        void drawPopupMenuItem   (juce::Graphics&, const juce::Rectangle<int>&,
                                  bool, bool, bool, bool, bool,
                                  const juce::String&, const juce::String&,
                                  const juce::Drawable*, const juce::Colour*) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&,
                                   const juce::Colour&, bool, bool) override;
        void drawButtonText       (juce::Graphics&, juce::TextButton&, bool, bool) override;
        juce::Font getLabelFont   (juce::Label&) override
        { return juce::Font (juce::FontOptions().withHeight (10.5f)); }

        CurCrossbreedAudioProcessorEditor& owner;
    };

    CurCrossbreedLAF mLAF;

    // ── Slot components (one per module type, indexed 0..5) ──────────────────
    // mVisualOrder[rowIndex] = moduleType shown at that row
    std::array<int, 6> mVisualOrder { 0, 1, 2, 3, 4, 5 };
    std::unique_ptr<ModuleSlotComponent> mSlots[6];

    // ── Drag-to-reorder state ────────────────────────────────────────────────
    int  mDragModule    = -1;   // moduleType being dragged (-1 = none)
    int  mDragStartRow  = -1;
    int  mDragCurrentY  = 0;
    bool mIsDragging    = false;

    // ── Global rack controls ─────────────────────────────────────────────────
    juce::Slider   mInputGainKnob, mOutputGainKnob, mMixKnob;
    juce::Label    mInputGainLabel, mOutputGainLabel, mMixLabel;
    juce::ComboBox mOversampleBox;
    juce::Label    mOversampleLabel;
    juce::TextButton mBypassBtn { "BYPASS" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        mInputGainA, mOutputGainA, mMixA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mOversampleA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   mBypassA;

    juce::TooltipWindow mTooltipWindow { this, 500 };

    // ── Helpers ───────────────────────────────────────────────────────────────
    int  rowY          (int row) const noexcept { return kHeaderH + row * kSlotH; }
    int  rowAtY        (int y)   const noexcept
    {
        if (y < kHeaderH) return -1;
        return juce::jlimit (0, 5, (y - kHeaderH) / kSlotH);
    }
    void updateSlotPositions ();
    void commitNewOrder      (int fromRow, int toRow);
    void setupKnob  (juce::Slider&, juce::Label&, const juce::String&);
    void setupCombo (juce::ComboBox&, juce::Label&, const juce::String&, const juce::StringArray&);
    void paintDragOverlay (juce::Graphics&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurCrossbreedAudioProcessorEditor)
};

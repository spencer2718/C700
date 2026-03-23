#include "PluginEditor.h"

#include "BinaryData.h"
#include "C700Kernel.h"
#include "C700Parameters.h"
#include "C700Properties.h"
#include "brrcodec.h"

#include <cmath>

namespace
{
const auto kValueTextColour = juce::Colour(0xffd8f8ff);
const auto kStaticTextColour = juce::Colour(0xcc141414);
const auto kFieldColour = juce::Colour(0xaa22303f);
const auto kTrackColour = juce::Colour(0x3322303f);

juce::Image loadBundledImage(const char* data, int size)
{
    return juce::ImageCache::getFromMemory(data, size);
}

juce::Image cropFilmStripFrame(const juce::Image& strip, int frameIndex, int frameCount)
{
    if (!strip.isValid() || frameCount <= 0)
        return {};

    const int frameHeight = strip.getHeight() / frameCount;
    if (frameHeight <= 0)
        return {};

    juce::Image frame(strip.getFormat(), strip.getWidth(), frameHeight, true);
    juce::Graphics g(frame);
    g.drawImageAt(strip, 0, -frameIndex * frameHeight);
    return frame;
}

juce::Image cropImageRegion(const juce::Image& source, int y, int height)
{
    if (!source.isValid() || height <= 0)
        return {};

    juce::Image frame(source.getFormat(), source.getWidth(), height, true);
    juce::Graphics g(frame);
    g.drawImageAt(source, 0, -y);
    return frame;
}

juce::Image composeImages(const juce::Image& base, const juce::Image& overlay)
{
    if (!base.isValid())
        return overlay;
    if (!overlay.isValid())
        return base;

    juce::Image composed(juce::Image::ARGB,
                         juce::jmax(base.getWidth(), overlay.getWidth()),
                         juce::jmax(base.getHeight(), overlay.getHeight()),
                         true);
    juce::Graphics g(composed);
    g.drawImageAt(base, 0, 0);
    g.drawImageAt(overlay, 0, 0);
    return composed;
}

class BitmapLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BitmapLookAndFeel(juce::Image checkStrip, juce::Image knobImage, juce::Image sliderThumb)
        : mCheckStrip(std::move(checkStrip)),
          mKnobImage(std::move(knobImage)),
          mSliderThumb(std::move(sliderThumb))
    {
    }

    void drawRotarySlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider&) override
    {
        if (mKnobImage.isValid())
            g.drawImageWithin(mKnobImage, x, y, width, height, juce::RectanglePlacement::stretchToFit);
        else
            g.fillAll(juce::Colours::darkgrey);

        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                   static_cast<float>(width), static_cast<float>(height));
        const auto centre = bounds.getCentre();
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const juce::Point<float> endPoint(centre.x + std::cos(angle) * radius,
                                          centre.y + std::sin(angle) * radius);

        g.setColour(juce::Colour(0xff43515f));
        g.drawLine(centre.x, centre.y, endPoint.x, endPoint.y, 2.2f);
    }

    int getSliderThumbRadius(juce::Slider&) override
    {
        return mSliderThumb.isValid() ? mSliderThumb.getWidth() / 2 : 8;
    }

    void drawLinearSlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider&) override
    {
        juce::ignoreUnused(minSliderPos, maxSliderPos);

        g.setColour(kTrackColour);

        if (style == juce::Slider::LinearVertical) {
            const auto trackX = static_cast<float>(x + width / 2 - 1);
            g.fillRect(trackX, static_cast<float>(y), 2.0f, static_cast<float>(height));

            const int thumbSize = mSliderThumb.isValid() ? mSliderThumb.getWidth() : 16;
            const int thumbX = x + (width - thumbSize) / 2;
            const int thumbY = juce::roundToInt(sliderPos) - thumbSize / 2;
            if (mSliderThumb.isValid())
                g.drawImageAt(mSliderThumb, thumbX, thumbY);
            else
                g.fillEllipse(static_cast<float>(thumbX), static_cast<float>(thumbY),
                              static_cast<float>(thumbSize), static_cast<float>(thumbSize));
            return;
        }

        g.fillRect(static_cast<float>(x), static_cast<float>(y + height / 2 - 1),
                   static_cast<float>(width), 2.0f);
    }

    void drawToggleButton(juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        const int frameCount = mCheckStrip.isValid() ? juce::jmax(1, mCheckStrip.getHeight() / 8) : 1;
        int frameIndex = 0;
        if (button.getToggleState())
            frameIndex = frameCount >= 3 ? 2 : frameCount - 1;
        else if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
            frameIndex = frameCount >= 2 ? 1 : 0;

        const auto tickBounds = juce::Rectangle<int>(0, 2, 8, 8);

        if (mCheckStrip.isValid()) {
            auto frame = cropFilmStripFrame(mCheckStrip, frameIndex, frameCount);
            g.drawImageAt(frame, tickBounds.getX(), tickBounds.getY());
        } else {
            g.setColour(button.getToggleState() ? kValueTextColour : juce::Colours::black);
            g.drawRect(tickBounds);
        }

        g.setColour(kValueTextColour);
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText(button.getButtonText(), 11, 0, button.getWidth() - 11, button.getHeight(),
                   juce::Justification::centredLeft);
    }

private:
    juce::Image mCheckStrip;
    juce::Image mKnobImage;
    juce::Image mSliderThumb;
};

void configureMomentaryButton(juce::ImageButton& button, const juce::Image& up, const juce::Image& down)
{
    button.setImages(false, true, true,
                     up, 1.0f, {},
                     up, 0.95f, {},
                     down, 1.0f, {});
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void configureFilmStripButton(juce::ImageButton& button, const juce::Image& strip)
{
    configureMomentaryButton(button,
                             cropFilmStripFrame(strip, 0, 2),
                             cropFilmStripFrame(strip, 1, 2));
}

void configureComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, kFieldColour);
    box.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    box.setColour(juce::ComboBox::textColourId, kValueTextColour);
    box.setColour(juce::ComboBox::arrowColourId, kValueTextColour);
    box.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff22303f));
    box.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2e4358));
    box.setColour(juce::PopupMenu::textColourId, kValueTextColour);
    box.setTextWhenNothingSelected({});
}

void configureValueLabel(juce::Label& label, juce::Justification justification, float fontSize = 11.0f)
{
    label.setFont(juce::Font(juce::FontOptions(fontSize)));
    label.setColour(juce::Label::textColourId, kValueTextColour);
    label.setJustificationType(justification);
    label.setInterceptsMouseClicks(false, false);
}

void configureUtilityButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xaa152032));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xcc243046));
    button.setColour(juce::TextButton::textColourOffId, kValueTextColour);
    button.setColour(juce::TextButton::textColourOnId, kValueTextColour);
}

void configureBitmapSlider(juce::Slider& slider,
                           juce::Slider::SliderStyle style,
                           juce::LookAndFeel& lookAndFeel)
{
    slider.setSliderStyle(style);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&lookAndFeel);
    slider.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void configureBitmapToggle(juce::ToggleButton& button, juce::LookAndFeel& lookAndFeel)
{
    button.setLookAndFeel(&lookAndFeel);
    button.setColour(juce::ToggleButton::textColourId, kValueTextColour);
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

juce::String sanitiseFileStem(juce::String stem)
{
    stem = stem.trim();
    if (stem.isEmpty())
        return {};

    static const juce::String illegal = "\\/:*?\"<>|";
    for (auto c : illegal)
        stem = stem.replaceCharacter(c, '_');
    return stem;
}

juce::String defaultBrrFileName(int program, const juce::String& programName, const juce::String& sampleName)
{
    auto stem = sanitiseFileStem(programName);
    if (stem.isEmpty())
        stem = sanitiseFileStem(sampleName);
    if (stem.isEmpty())
        stem = juce::String::formatted("program_%03d", program);
    return stem + ".brr";
}

void setDiscreteParameterValue(juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& parameterId,
                               int discreteValue)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(parameterId))) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(discreteValue)));
        parameter->endChangeGesture();
    }
}

float getParameterValue(juce::AudioProcessorValueTreeState& apvts, const juce::String& parameterId)
{
    if (auto* raw = apvts.getRawParameterValue(parameterId))
        return raw->load();
    return 0.0f;
}

juce::String formatFloatValue(float value, int decimals)
{
    const float rounded = std::round(value);
    if (decimals <= 0 || std::abs(value - rounded) < 0.0001f)
        return juce::String(static_cast<int>(rounded));
    return juce::String(value, decimals);
}

int estimateBaseCycleLength(const short* src, int length)
{
    if (src == nullptr || length < 64)
        return 0;

    const int minLag = 8;
    const int maxLag = juce::jmin(length / 2, 2048);
    const int step = juce::jmax(1, length / 2048);
    double bestScore = 0.0;
    int bestLag = 0;

    for (int lag = minLag; lag < maxLag; ++lag) {
        double corr = 0.0;
        double energyA = 0.0;
        double energyB = 0.0;

        for (int i = 0; i + lag < length; i += step) {
            const double a = src[i];
            const double b = src[i + lag];
            corr += a * b;
            energyA += a * a;
            energyB += b * b;
        }

        if (energyA <= 0.0 || energyB <= 0.0)
            continue;

        const double score = corr / std::sqrt(energyA * energyB);
        if (score > bestScore) {
            bestScore = score;
            bestLag = lag;
        }
    }

    return bestScore > 0.15 ? bestLag : 0;
}
}

C700AudioProcessorEditor::C700AudioProcessorEditor(C700AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    mBackgroundImage = loadBundledImage(BinaryData::AUBackground_png, BinaryData::AUBackground_pngSize);
    mHardwareStrip = loadBundledImage(BinaryData::hwconn_png, BinaryData::hwconn_pngSize);
    mTrackIndicatorStrip = loadBundledImage(BinaryData::ind_track_png, BinaryData::ind_track_pngSize);
    mProgramRockerStrip = loadBundledImage(BinaryData::rocker_sw_png, BinaryData::rocker_sw_pngSize);
    mCheckStrip = loadBundledImage(BinaryData::bt_check_png, BinaryData::bt_check_pngSize);
    mKnobImage = loadBundledImage(BinaryData::knobBack_png, BinaryData::knobBack_pngSize);
    mSliderThumbImage = loadBundledImage(BinaryData::sliderThumb_png, BinaryData::sliderThumb_pngSize);

    mBitmapLookAndFeel = std::make_unique<BitmapLookAndFeel>(mCheckStrip, mKnobImage, mSliderThumbImage);

    mBankStrips[0] = loadBundledImage(BinaryData::bank_a_png, BinaryData::bank_a_pngSize);
    mBankStrips[1] = loadBundledImage(BinaryData::bank_b_png, BinaryData::bank_b_pngSize);
    mBankStrips[2] = loadBundledImage(BinaryData::bank_c_png, BinaryData::bank_c_pngSize);
    mBankStrips[3] = loadBundledImage(BinaryData::bank_d_png, BinaryData::bank_d_pngSize);

    mTrackNumberStrips[0] = loadBundledImage(BinaryData::track01__png, BinaryData::track01__pngSize);
    mTrackNumberStrips[1] = loadBundledImage(BinaryData::track02__png, BinaryData::track02__pngSize);
    mTrackNumberStrips[2] = loadBundledImage(BinaryData::track03__png, BinaryData::track03__pngSize);
    mTrackNumberStrips[3] = loadBundledImage(BinaryData::track04__png, BinaryData::track04__pngSize);
    mTrackNumberStrips[4] = loadBundledImage(BinaryData::track05__png, BinaryData::track05__pngSize);
    mTrackNumberStrips[5] = loadBundledImage(BinaryData::track06__png, BinaryData::track06__pngSize);
    mTrackNumberStrips[6] = loadBundledImage(BinaryData::track07__png, BinaryData::track07__pngSize);
    mTrackNumberStrips[7] = loadBundledImage(BinaryData::track08__png, BinaryData::track08__pngSize);
    mTrackNumberStrips[8] = loadBundledImage(BinaryData::track09__png, BinaryData::track09__pngSize);
    mTrackNumberStrips[9] = loadBundledImage(BinaryData::track10__png, BinaryData::track10__pngSize);
    mTrackNumberStrips[10] = loadBundledImage(BinaryData::track11__png, BinaryData::track11__pngSize);
    mTrackNumberStrips[11] = loadBundledImage(BinaryData::track12__png, BinaryData::track12__pngSize);
    mTrackNumberStrips[12] = loadBundledImage(BinaryData::track13__png, BinaryData::track13__pngSize);
    mTrackNumberStrips[13] = loadBundledImage(BinaryData::track14__png, BinaryData::track14__pngSize);
    mTrackNumberStrips[14] = loadBundledImage(BinaryData::track15__png, BinaryData::track15__pngSize);
    mTrackNumberStrips[15] = loadBundledImage(BinaryData::track16__png, BinaryData::track16__pngSize);

    configureFilmStripButton(mLoadButton,
                             loadBundledImage(BinaryData::bt_load_png, BinaryData::bt_load_pngSize));
    mLoadButton.onClick = [this] { loadSampleClicked(); };
    addAndMakeVisible(mLoadButton);

    configureFilmStripButton(mUnloadButton,
                             loadBundledImage(BinaryData::bt_unload_png, BinaryData::bt_unload_pngSize));
    mUnloadButton.onClick = [this] { unloadSampleClicked(); };
    addAndMakeVisible(mUnloadButton);

    configureFilmStripButton(mSaveButton,
                             loadBundledImage(BinaryData::bt_save_png, BinaryData::bt_save_pngSize));
    mSaveButton.onClick = [this] { saveSampleClicked(); };
    addAndMakeVisible(mSaveButton);

    configureFilmStripButton(mSaveXiButton,
                             loadBundledImage(BinaryData::bt_save_xi_png, BinaryData::bt_save_xi_pngSize));
    mSaveXiButton.onClick = [this] { saveXiClicked(); };
    addAndMakeVisible(mSaveXiButton);

    const auto calcStrip = loadBundledImage(BinaryData::bt_calc_png, BinaryData::bt_calc_pngSize);
    configureFilmStripButton(mAutoRateButton, calcStrip);
    mAutoRateButton.onClick = [this] { autoDetectCurrentRate(); };
    addAndMakeVisible(mAutoRateButton);

    configureFilmStripButton(mAutoKeyButton, calcStrip);
    mAutoKeyButton.onClick = [this] { autoDetectCurrentBaseKey(); };
    addAndMakeVisible(mAutoKeyButton);

    for (int bank = 0; bank < static_cast<int>(mBankButtons.size()); ++bank) {
        const auto offImage = cropFilmStripFrame(mBankStrips[bank], 0, 2);
        const auto onImage = cropFilmStripFrame(mBankStrips[bank], 1, 2);
        configureMomentaryButton(mBankButtons[bank], offImage, onImage);
        mBankButtons[bank].setTriggeredOnMouseDown(true);
        mBankButtons[bank].onClick = [this, bank] { selectBank(bank); };
        addAndMakeVisible(mBankButtons[bank]);
    }

    for (int channel = 0; channel < static_cast<int>(mTrackButtons.size()); ++channel) {
        const auto offImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 0, 2),
                                            cropFilmStripFrame(mTrackNumberStrips[channel], 0, 2));
        const auto onImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 1, 2),
                                           cropFilmStripFrame(mTrackNumberStrips[channel], 1, 2));
        configureMomentaryButton(mTrackButtons[channel], offImage, onImage);
        mTrackButtons[channel].setTriggeredOnMouseDown(true);
        mTrackButtons[channel].setTooltip("Edit Track " + juce::String(channel + 1));
        mTrackButtons[channel].onClick = [this, channel] { selectEditingChannel(channel); };
        addAndMakeVisible(mTrackButtons[channel]);
    }

    const auto rockerNeutral = cropFilmStripFrame(mProgramRockerStrip, 1, 3);
    const auto rockerUp = cropFilmStripFrame(mProgramRockerStrip, 0, 3);
    const auto rockerDown = cropFilmStripFrame(mProgramRockerStrip, 2, 3);
    configureMomentaryButton(mProgramUpButton,
                             cropImageRegion(rockerNeutral, 0, 9),
                             cropImageRegion(rockerUp, 0, 9));
    configureMomentaryButton(mProgramDownButton,
                             cropImageRegion(rockerNeutral, 9, 10),
                             cropImageRegion(rockerDown, 9, 10));
    mProgramUpButton.onClick = [this] { adjustProgram(1); };
    mProgramDownButton.onClick = [this] { adjustProgram(-1); };
    addAndMakeVisible(mProgramUpButton);
    addAndMakeVisible(mProgramDownButton);

    configureMomentaryButton(mLoopPointUpButton,
                             cropImageRegion(rockerNeutral, 0, 9),
                             cropImageRegion(rockerUp, 0, 9));
    configureMomentaryButton(mLoopPointDownButton,
                             cropImageRegion(rockerNeutral, 9, 10),
                             cropImageRegion(rockerDown, 9, 10));
    mLoopPointUpButton.onClick = [this] { shiftLoopPoint(1); };
    mLoopPointDownButton.onClick = [this] { shiftLoopPoint(-1); };
    addAndMakeVisible(mLoopPointUpButton);
    addAndMakeVisible(mLoopPointDownButton);

    configureValueLabel(mProgramValueLabel, juce::Justification::centredRight);
    mProgramValueLabel.setColour(juce::Label::backgroundColourId, kFieldColour);
    mProgramValueLabel.setText("0", juce::dontSendNotification);
    addAndMakeVisible(mProgramValueLabel);

    mEngineBox.addItem("Old", 1);
    mEngineBox.addItem("Relaxed", 2);
    mEngineBox.addItem("Accurate", 3);
    configureComboBox(mEngineBox);
    addAndMakeVisible(mEngineBox);
    mEngineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "engine", mEngineBox);

    mVoiceAllocBox.addItem("Oldest", 1);
    mVoiceAllocBox.addItem("SameCh", 2);
    configureComboBox(mVoiceAllocBox);
    addAndMakeVisible(mVoiceAllocBox);
    mVoiceAllocAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "voicealloc", mVoiceAllocBox);

    configureUtilityButton(mPlayerCodeButton);
    mPlayerCodeButton.onClick = [this] { loadPlayerCodeClicked(); };
    addAndMakeVisible(mPlayerCodeButton);

    configureUtilityButton(mExportSpcButton);
    mExportSpcButton.onClick = [this] { exportSpcClicked(); };
    addAndMakeVisible(mExportSpcButton);

    configureEditorField(mProgramNameEditor, juce::Justification::centredLeft, 31);
    mProgramNameEditor.onTextChange = [this] {
        if (mProgramNameEditor.hasKeyboardFocus(true) && mProgramNameEditProgram < 0)
            mProgramNameEditProgram = currentProgram();
    };
    mProgramNameEditor.onReturnKey = [this] { commitProgramNameEditor(); };
    mProgramNameEditor.onFocusLost = [this] { commitProgramNameEditor(); };
    addAndMakeVisible(mProgramNameEditor);

    configureEditorField(mLoopPointEditor, juce::Justification::centredRight, 8, "0123456789");
    mLoopPointEditor.onTextChange = [this] {
        if (mLoopPointEditor.hasKeyboardFocus(true) && mLoopPointEditProgram < 0)
            mLoopPointEditProgram = currentProgram();
    };
    mLoopPointEditor.onReturnKey = [this] { commitLoopPointEditor(); };
    mLoopPointEditor.onFocusLost = [this] { commitLoopPointEditor(); };
    addAndMakeVisible(mLoopPointEditor);

    configureEditorField(mRateEditor, juce::Justification::centredRight, 12, "0123456789.");
    mRateEditor.onTextChange = [this] {
        if (mRateEditor.hasKeyboardFocus(true) && mRateEditProgram < 0)
            mRateEditProgram = currentProgram();
    };
    mRateEditor.onReturnKey = [this] { commitDoublePropertyEditor(kAudioUnitCustomProperty_Rate, mRateEditor, 0.0, 128000.0, 2, mRateEditProgram); };
    mRateEditor.onFocusLost = [this] { commitDoublePropertyEditor(kAudioUnitCustomProperty_Rate, mRateEditor, 0.0, 128000.0, 2, mRateEditProgram); };
    addAndMakeVisible(mRateEditor);

    configureEditorField(mBaseKeyEditor, juce::Justification::centredRight, 3, "0123456789");
    mBaseKeyEditor.onTextChange = [this] {
        if (mBaseKeyEditor.hasKeyboardFocus(true) && mBaseKeyEditProgram < 0)
            mBaseKeyEditProgram = currentProgram();
    };
    mBaseKeyEditor.onReturnKey = [this] { commitIntegerPropertyEditor(kAudioUnitCustomProperty_BaseKey, mBaseKeyEditor, 0, 127, mBaseKeyEditProgram); };
    mBaseKeyEditor.onFocusLost = [this] { commitIntegerPropertyEditor(kAudioUnitCustomProperty_BaseKey, mBaseKeyEditor, 0, 127, mBaseKeyEditProgram); };
    addAndMakeVisible(mBaseKeyEditor);

    configureEditorField(mLowKeyEditor, juce::Justification::centredRight, 3, "0123456789");
    mLowKeyEditor.onTextChange = [this] {
        if (mLowKeyEditor.hasKeyboardFocus(true) && mLowKeyEditProgram < 0)
            mLowKeyEditProgram = currentProgram();
    };
    mLowKeyEditor.onReturnKey = [this] { commitIntegerPropertyEditor(kAudioUnitCustomProperty_LowKey, mLowKeyEditor, 0, 127, mLowKeyEditProgram); };
    mLowKeyEditor.onFocusLost = [this] { commitIntegerPropertyEditor(kAudioUnitCustomProperty_LowKey, mLowKeyEditor, 0, 127, mLowKeyEditProgram); };
    addAndMakeVisible(mLowKeyEditor);

    configureEditorField(mHighKeyEditor, juce::Justification::centredRight, 3, "0123456789");
    mHighKeyEditor.onTextChange = [this] {
        if (mHighKeyEditor.hasKeyboardFocus(true) && mHighKeyEditProgram < 0)
            mHighKeyEditProgram = currentProgram();
    };
    mHighKeyEditor.onReturnKey = [this] { commitIntegerPropertyEditor(kAudioUnitCustomProperty_HighKey, mHighKeyEditor, 0, 127, mHighKeyEditProgram); };
    mHighKeyEditor.onFocusLost = [this] { commitIntegerPropertyEditor(kAudioUnitCustomProperty_HighKey, mHighKeyEditor, 0, 127, mHighKeyEditProgram); };
    addAndMakeVisible(mHighKeyEditor);

    static constexpr const char* envelopeParamIds[] = { "ar", "dr", "sl", "sr1", "sr2" };
    static constexpr double envelopeRanges[][3] = {
        { 0.0, 15.0, 1.0 },
        { 0.0, 7.0, 1.0 },
        { 0.0, 7.0, 1.0 },
        { 0.0, 31.0, 1.0 },
        { 0.0, 31.0, 1.0 }
    };

    for (int i = 0; i < static_cast<int>(mEnvelopeSliders.size()); ++i) {
        configureBitmapSlider(mEnvelopeSliders[static_cast<size_t>(i)],
                              juce::Slider::LinearVertical,
                              *mBitmapLookAndFeel);
        mEnvelopeSliders[static_cast<size_t>(i)].setRange(envelopeRanges[i][0],
                                                          envelopeRanges[i][1],
                                                          envelopeRanges[i][2]);
        addAndMakeVisible(mEnvelopeSliders[static_cast<size_t>(i)]);
        configureValueLabel(mEnvelopeValueLabels[static_cast<size_t>(i)], juce::Justification::centred, 10.0f);
        addAndMakeVisible(mEnvelopeValueLabels[static_cast<size_t>(i)]);
    }

    mArAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), envelopeParamIds[0], mEnvelopeSliders[0]);
    mDrAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), envelopeParamIds[1], mEnvelopeSliders[1]);
    mSlAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), envelopeParamIds[2], mEnvelopeSliders[2]);
    mSr1Attachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), envelopeParamIds[3], mEnvelopeSliders[3]);
    mSr2Attachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), envelopeParamIds[4], mEnvelopeSliders[4]);

    configureBitmapSlider(mVolLKnob, juce::Slider::RotaryVerticalDrag, *mBitmapLookAndFeel);
    mVolLKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                  juce::MathConstants<float>::pi * 2.8f,
                                  true);
    mVolLKnob.setRange(0.0, 127.0, 1.0);
    addAndMakeVisible(mVolLKnob);
    mVolLAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), "voll", mVolLKnob);

    configureBitmapSlider(mVolRKnob, juce::Slider::RotaryVerticalDrag, *mBitmapLookAndFeel);
    mVolRKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                  juce::MathConstants<float>::pi * 2.8f,
                                  true);
    mVolRKnob.setRange(0.0, 127.0, 1.0);
    addAndMakeVisible(mVolRKnob);
    mVolRAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), "volr", mVolRKnob);

    configureBitmapSlider(mVibDepthKnob, juce::Slider::RotaryVerticalDrag, *mBitmapLookAndFeel);
    mVibDepthKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f,
                                      true);
    mVibDepthKnob.setRange(C700Parameters::GetParameterMin(kParam_vibdepth2),
                           C700Parameters::GetParameterMax(kParam_vibdepth2),
                           0.001);
    addAndMakeVisible(mVibDepthKnob);
    mVibDepthAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), "vibdepth2", mVibDepthKnob);

    configureBitmapSlider(mVibrateKnob, juce::Slider::RotaryVerticalDrag, *mBitmapLookAndFeel);
    mVibrateKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                     juce::MathConstants<float>::pi * 2.8f,
                                     true);
    mVibrateKnob.setRange(C700Parameters::GetParameterMin(kParam_vibrate),
                          C700Parameters::GetParameterMax(kParam_vibrate),
                          0.001);
    addAndMakeVisible(mVibrateKnob);
    mVibrateAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), "vibrate", mVibrateKnob);

    configureValueLabel(mVolLValueLabel, juce::Justification::centred, 10.0f);
    configureValueLabel(mVolRValueLabel, juce::Justification::centred, 10.0f);
    configureValueLabel(mVibDepthValueLabel, juce::Justification::centred, 10.0f);
    configureValueLabel(mVibrateValueLabel, juce::Justification::centred, 10.0f);
    addAndMakeVisible(mVolLValueLabel);
    addAndMakeVisible(mVolRValueLabel);
    addAndMakeVisible(mVibDepthValueLabel);
    addAndMakeVisible(mVibrateValueLabel);

    configureBitmapToggle(mLoopToggle, *mBitmapLookAndFeel);
    configureBitmapToggle(mEchoToggle, *mBitmapLookAndFeel);
    configureBitmapToggle(mPitchModToggle, *mBitmapLookAndFeel);
    configureBitmapToggle(mNoiseToggle, *mBitmapLookAndFeel);
    configureBitmapToggle(mMonoToggle, *mBitmapLookAndFeel);
    configureBitmapToggle(mGlideToggle, *mBitmapLookAndFeel);
    addAndMakeVisible(mLoopToggle);
    addAndMakeVisible(mEchoToggle);
    addAndMakeVisible(mPitchModToggle);
    addAndMakeVisible(mNoiseToggle);
    addAndMakeVisible(mMonoToggle);
    addAndMakeVisible(mGlideToggle);

    mLoopAttachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), "loop", mLoopToggle);
    mEchoAttachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), "echo", mEchoToggle);
    mPitchModAttachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), "pitchmod", mPitchModToggle);
    mNoiseAttachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), "noise", mNoiseToggle);
    mMonoAttachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), "monomode", mMonoToggle);
    mGlideAttachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), "porta", mGlideToggle);

    configureValueLabel(mPortamentoRateLabel, juce::Justification::centred, 10.0f);
    addAndMakeVisible(mPortamentoRateLabel);

    configureValueLabel(mNotePriorityValueLabel, juce::Justification::centred, 10.0f);
    configureValueLabel(mReleasePriorityValueLabel, juce::Justification::centred, 10.0f);
    addAndMakeVisible(mNotePriorityValueLabel);
    addAndMakeVisible(mReleasePriorityValueLabel);

    configureValueLabel(mAramLabel, juce::Justification::centredRight, 10.0f);
    addAndMakeVisible(mAramLabel);

    configureValueLabel(mStatusLabel, juce::Justification::centredLeft, 10.0f);
    addAndMakeVisible(mStatusLabel);

    mLastBrowseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    mLastExportDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    setResizable(false, false);
    setSize(kEditorWidth, kEditorHeight);

    processorRef.forceParamSync();
    startTimerHz(10);
    timerCallback();
}

C700AudioProcessorEditor::~C700AudioProcessorEditor()
{
    stopTimer();

    for (auto* slider : { &mVolLKnob, &mVolRKnob, &mVibDepthKnob, &mVibrateKnob })
        slider->setLookAndFeel(nullptr);

    for (auto& slider : mEnvelopeSliders)
        slider.setLookAndFeel(nullptr);

    for (auto* toggle : { &mLoopToggle, &mEchoToggle, &mPitchModToggle, &mNoiseToggle, &mMonoToggle, &mGlideToggle })
        toggle->setLookAndFeel(nullptr);
}

void C700AudioProcessorEditor::paint(juce::Graphics& g)
{
    if (mBackgroundImage.isValid())
        g.drawImageAt(mBackgroundImage, 0, 0);
    else
        g.fillAll(juce::Colour(0xffc4c8ce));

    g.setColour(kStaticTextColour);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("Engine", 6, 63, 40, 11, juce::Justification::centredLeft);
    g.drawText("VoiceAlloc", 96, 63, 50, 11, juce::Justification::centredLeft);
    g.drawText("Bank", 12, 91, 30, 11, juce::Justification::centredLeft);
    g.drawText("High Key", 0, 113, 46, 11, juce::Justification::centred);
    g.drawText("Low Key", 2, 133, 44, 11, juce::Justification::centred);
    g.drawText("Root Key", 2, 153, 44, 11, juce::Justification::centred);
    g.drawText("Sample Rate", 4, 206, 59, 11, juce::Justification::centred);
    g.drawText("Priority", 6, 238, 59, 11, juce::Justification::centredLeft);
    g.drawText("NteOn", 7, 250, 30, 11, juce::Justification::centredLeft);
    g.drawText("Rel", 66, 250, 18, 11, juce::Justification::centredLeft);
    g.drawText("Vib.Depth", 426, 8, 49, 11, juce::Justification::centred);
    g.drawText("Vib.Rate", 474, 8, 44, 11, juce::Justification::centred);
    g.drawText("AR", 406, 180, 19, 11, juce::Justification::centred);
    g.drawText("DR", 433, 180, 19, 11, juce::Justification::centred);
    g.drawText("SL", 460, 180, 19, 11, juce::Justification::centred);
    g.drawText("SR1", 487, 180, 19, 11, juce::Justification::centred);
    g.drawText("SR2", 514, 180, 19, 11, juce::Justification::centred);
    g.drawText("L", 28, 290, 20, 11, juce::Justification::centred);
    g.drawText("R", 55, 290, 20, 11, juce::Justification::centred);
    g.drawText("Rate", 414, 137, 33, 12, juce::Justification::centredLeft);

    if (mHardwareStrip.isValid()) {
        auto frame = cropFilmStripFrame(mHardwareStrip, mHardwareConnected ? 1 : 0, 2);
        g.drawImageAt(frame, 520, 376);
    }
}

void C700AudioProcessorEditor::resized()
{
    for (int channel = 0; channel < static_cast<int>(mTrackButtons.size()); ++channel)
        mTrackButtons[channel].setBounds(60 + channel * 20, 13, 20, 20);

    for (int bank = 0; bank < static_cast<int>(mBankButtons.size()); ++bank)
        mBankButtons[bank].setBounds(51 + bank * 15, 91, 12, 12);

    mEngineBox.setBounds(40, 61, 50, 12);
    mVoiceAllocBox.setBounds(145, 61, 50, 12);

    mProgramValueLabel.setBounds(114, 92, 23, 14);
    mProgramUpButton.setBounds(137, 87, 13, 9);
    mProgramDownButton.setBounds(137, 96, 13, 10);
    mProgramNameEditor.setBounds(153, 92, 249, 14);

    mBaseKeyEditor.setBounds(45, 154, 32, 14);
    mLowKeyEditor.setBounds(45, 134, 32, 14);
    mHighKeyEditor.setBounds(45, 114, 32, 14);
    mLoopPointEditor.setBounds(12, 188, 46, 14);
    mLoopPointUpButton.setBounds(59, 184, 13, 9);
    mLoopPointDownButton.setBounds(59, 193, 13, 10);
    mRateEditor.setBounds(12, 220, 66, 14);
    mAutoKeyButton.setBounds(81, 153, 29, 14);
    mAutoRateButton.setBounds(81, 219, 29, 14);

    mLoopToggle.setBounds(76, 187, 36, 12);
    mEchoToggle.setBounds(414, 80, 33, 14);
    mPitchModToggle.setBounds(414, 91, 38, 14);
    mNoiseToggle.setBounds(414, 102, 38, 14);
    mMonoToggle.setBounds(414, 113, 36, 14);
    mGlideToggle.setBounds(414, 124, 35, 14);
    mPortamentoRateLabel.setBounds(414, 148, 24, 12);

    constexpr int sliderX[] = { 408, 435, 462, 489, 516 };
    constexpr int sliderLabelX[] = { 404, 431, 458, 485, 512 };
    for (int i = 0; i < static_cast<int>(mEnvelopeSliders.size()); ++i) {
        mEnvelopeSliders[static_cast<size_t>(i)].setBounds(sliderX[i], 191, 16, 55);
        mEnvelopeValueLabels[static_cast<size_t>(i)].setBounds(sliderLabelX[i], 248, 24, 12);
    }

    mVolLKnob.setBounds(452, 114, 32, 32);
    mVolRKnob.setBounds(492, 114, 32, 32);
    mVolLValueLabel.setBounds(456, 148, 24, 12);
    mVolRValueLabel.setBounds(496, 148, 24, 12);

    mVibDepthKnob.setBounds(434, 24, 32, 32);
    mVibrateKnob.setBounds(480, 24, 32, 32);
    mVibDepthValueLabel.setBounds(430, 61, 40, 14);
    mVibrateValueLabel.setBounds(476, 61, 40, 14);

    mNotePriorityValueLabel.setBounds(38, 248, 24, 12);
    mReleasePriorityValueLabel.setBounds(84, 248, 24, 12);

    mLoadButton.setBounds(184, 248, 40, 14);
    mSaveButton.setBounds(230, 248, 68, 14);
    mSaveXiButton.setBounds(304, 248, 49, 14);
    mUnloadButton.setBounds(361, 248, 41, 14);

    mPlayerCodeButton.setBounds(8, 382, 112, 14);
    mExportSpcButton.setBounds(180, 382, 160, 14);
    mAramLabel.setBounds(452, 392, 84, 14);
    mStatusLabel.setBounds(8, 364, 340, 14);
}

void C700AudioProcessorEditor::timerCallback()
{
    const auto state = processorRef.getRuntimeState();
    const auto program = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());

    mProgramValueLabel.setText(juce::String(program), juce::dontSendNotification);
    syncEditorFields(program);
    syncValueLabels();

    mDisplayedBank = state.bank;
    mDisplayedChannel = state.editingChannel;
    mDisplayedTrackActivity = state.noteOnTrack;
    refreshSelectionButtons();

    mHardwareConnected = state.isHwConnected;
    repaint(520, 376, 16, 16);

    const int ramMax = BRR_ENDADDR - BRR_STARTADDR;
    mAramLabel.setText(juce::String(state.aramBytes), juce::dontSendNotification);
    mAramLabel.setColour(juce::Label::textColourId,
                         state.aramBytes > ramMax ? juce::Colours::red : kValueTextColour);

    mExportSpcButton.setEnabled(state.hasPlayerCode);

    if (state.finishedRecording) {
        processorRef.getAdapter().enableSpcRecording(false);
        setStatusMessage("SPC saved!", 5000);
        return;
    }

    if (juce::Time::currentTimeMillis() < mStatusOverrideUntil)
        return;

    if (!state.hasPlayerCode)
        mStatusLabel.setText("Load playercode.bin to enable SPC export", juce::dontSendNotification);
    else
        mStatusLabel.setText("Pass 4: per-slot edit controls restored", juce::dontSendNotification);
}

void C700AudioProcessorEditor::setStatusMessage(const juce::String& message, int durationMs)
{
    mStatusLabel.setText(message, juce::dontSendNotification);
    mStatusOverrideUntil = juce::Time::currentTimeMillis() + durationMs;
}

void C700AudioProcessorEditor::configureEditorField(juce::TextEditor& editor,
                                                    juce::Justification justification,
                                                    int maxChars,
                                                    const juce::String& allowedChars)
{
    editor.setFont(juce::Font(juce::FontOptions(10.0f)));
    editor.setColour(juce::TextEditor::backgroundColourId, kFieldColour);
    editor.setColour(juce::TextEditor::textColourId, kValueTextColour);
    editor.setColour(juce::TextEditor::highlightColourId, juce::Colour(0xff2e4358));
    editor.setColour(juce::TextEditor::highlightedTextColourId, kValueTextColour);
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::CaretComponent::caretColourId, kValueTextColour);
    editor.setJustification(justification);
    if (maxChars > 0)
        editor.setInputRestrictions(maxChars, allowedChars);
}

int C700AudioProcessorEditor::currentProgram() const
{
    return juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "program"));
}

int C700AudioProcessorEditor::resolveEditProgram(const juce::TextEditor& editor, int trackedProgram) const
{
    if (trackedProgram >= 0)
        return trackedProgram;
    if (editor.hasKeyboardFocus(true))
        return currentProgram();
    return -1;
}

void C700AudioProcessorEditor::commitPendingFieldEdits()
{
    commitProgramNameEditor();
    commitLoopPointEditor();
    commitDoublePropertyEditor(kAudioUnitCustomProperty_Rate, mRateEditor, 0.0, 128000.0, 2, mRateEditProgram);
    commitIntegerPropertyEditor(kAudioUnitCustomProperty_BaseKey, mBaseKeyEditor, 0, 127, mBaseKeyEditProgram);
    commitIntegerPropertyEditor(kAudioUnitCustomProperty_LowKey, mLowKeyEditor, 0, 127, mLowKeyEditProgram);
    commitIntegerPropertyEditor(kAudioUnitCustomProperty_HighKey, mHighKeyEditor, 0, 127, mHighKeyEditProgram);
}

void C700AudioProcessorEditor::commitIntegerPropertyEditor(int propertyId,
                                                           juce::TextEditor& editor,
                                                           int minValue,
                                                           int maxValue,
                                                           int& trackedProgram)
{
    const int program = resolveEditProgram(editor, trackedProgram);
    if (program < 0)
        return;

    const int value = juce::jlimit(minValue, maxValue, editor.getText().trim().getIntValue());
    editor.setText(juce::String(value), juce::dontSendNotification);
    if (processorRef.getAdapter().setProgramPropertyValue(program, propertyId, static_cast<float>(value)))
        processorRef.forceParamSync();
    trackedProgram = -1;
}

void C700AudioProcessorEditor::commitDoublePropertyEditor(int propertyId,
                                                          juce::TextEditor& editor,
                                                          double minValue,
                                                          double maxValue,
                                                          int decimals,
                                                          int& trackedProgram)
{
    const int program = resolveEditProgram(editor, trackedProgram);
    if (program < 0)
        return;

    const double value = juce::jlimit(minValue, maxValue, static_cast<double>(editor.getText().trim().getDoubleValue()));
    editor.setText(formatFloatValue(static_cast<float>(value), decimals), juce::dontSendNotification);
    if (processorRef.getAdapter().setProgramPropertyDoubleValue(program, propertyId, value))
        processorRef.forceParamSync();
    trackedProgram = -1;
}

void C700AudioProcessorEditor::commitProgramNameEditor()
{
    const int program = resolveEditProgram(mProgramNameEditor, mProgramNameEditProgram);
    if (program < 0)
        return;

    auto text = mProgramNameEditor.getText().trim();
    if (text.isEmpty())
        text = processorRef.getRuntimeState().sampleName;

    processorRef.getAdapter().setProgramStringProperty(program,
                                                       kAudioUnitCustomProperty_ProgramName,
                                                       text.toStdString());
    processorRef.forceParamSync();
    mProgramNameEditProgram = -1;
}

void C700AudioProcessorEditor::commitLoopPointEditor()
{
    const int program = resolveEditProgram(mLoopPointEditor, mLoopPointEditProgram);
    if (program < 0)
        return;

    int loopPointSamples = juce::jmax(0, mLoopPointEditor.getText().trim().getIntValue());
    const auto brrData = processorRef.getAdapter().copyBRRData(program);
    const int maxLoopPointSamples = static_cast<int>(brrData.size() / 9) * 16;
    loopPointSamples = juce::jmin(loopPointSamples, maxLoopPointSamples);
    loopPointSamples = (loopPointSamples / 16) * 16;
    const int loopPointBrr = (loopPointSamples / 16) * 9;

    if (processorRef.getAdapter().setProgramPropertyValue(program,
                                                          kAudioUnitCustomProperty_LoopPoint,
                                                          static_cast<float>(loopPointBrr))) {
        processorRef.forceParamSync();
        mLoopPointEditor.setText(juce::String(loopPointSamples), juce::dontSendNotification);
    }
    mLoopPointEditProgram = -1;
}

void C700AudioProcessorEditor::shiftLoopPoint(int deltaBlocks)
{
    const int program = juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "program"));
    const auto brrData = processorRef.getAdapter().copyBRRData(program);
    const int maxLoopPointBrr = static_cast<int>(brrData.size());
    int currentBrr = juce::roundToInt(processorRef.getAdapter().getProgramPropertyValue(program,
                                                                                        kAudioUnitCustomProperty_LoopPoint));
    currentBrr = juce::jlimit(0, maxLoopPointBrr, currentBrr + deltaBlocks * 9);
    if (processorRef.getAdapter().setProgramPropertyValue(program,
                                                          kAudioUnitCustomProperty_LoopPoint,
                                                          static_cast<float>(currentBrr))) {
        processorRef.forceParamSync();
    }
}

void C700AudioProcessorEditor::autoDetectCurrentRate()
{
    const int program = juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "program"));
    auto brrData = processorRef.getAdapter().copyBRRData(program);
    if (brrData.empty()) {
        setStatusMessage("No BRR data in the current slot");
        return;
    }

    int loopPoint = juce::roundToInt(processorRef.getAdapter().getProgramPropertyValue(program,
                                                                                       kAudioUnitCustomProperty_LoopPoint));
    if (loopPoint >= static_cast<int>(brrData.size()))
        loopPoint = 0;

    const int key = juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "basekey"));
    std::vector<short> buffer((static_cast<int>(brrData.size()) / 9) * 16);
    if (buffer.empty())
        return;

    brrdecode(brrData.data(), buffer.data(), 0, 0);
    const int length = ((static_cast<int>(brrData.size()) - loopPoint) / 9) * 16;
    if (length <= 0)
        return;

    const int pitch = estimateBaseCycleLength(buffer.data() + (loopPoint / 9) * 16, length);
    if (pitch <= 0)
        return;

    const double sampleRate = length / static_cast<double>(pitch) * 440.0 * std::pow(2.0, (key - 69.0) / 12.0);
    if (processorRef.getAdapter().setProgramPropertyDoubleValue(program,
                                                                kAudioUnitCustomProperty_Rate,
                                                                sampleRate)) {
        processorRef.forceParamSync();
        setStatusMessage("Sample rate recalculated");
    }
}

void C700AudioProcessorEditor::autoDetectCurrentBaseKey()
{
    const int program = juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "program"));
    auto brrData = processorRef.getAdapter().copyBRRData(program);
    if (brrData.empty()) {
        setStatusMessage("No BRR data in the current slot");
        return;
    }

    int loopPoint = juce::roundToInt(processorRef.getAdapter().getProgramPropertyValue(program,
                                                                                       kAudioUnitCustomProperty_LoopPoint));
    if (loopPoint >= static_cast<int>(brrData.size()))
        loopPoint = 0;

    const double sampleRate = processorRef.getAdapter().getProgramPropertyDoubleValue(program,
                                                                                      kAudioUnitCustomProperty_Rate);
    std::vector<short> buffer((static_cast<int>(brrData.size()) / 9) * 16);
    if (buffer.empty())
        return;

    brrdecode(brrData.data(), buffer.data(), 0, 0);
    const int length = ((static_cast<int>(brrData.size()) - loopPoint) / 9) * 16;
    if (length <= 0)
        return;

    const int pitch = estimateBaseCycleLength(buffer.data() + (loopPoint / 9) * 16, length);
    if (pitch <= 0)
        return;

    const double freq = sampleRate / (length / static_cast<double>(pitch));
    if (freq <= 0.0)
        return;
    const int key = juce::jlimit(0, 127, juce::roundToInt(std::log(freq) * 17.312 - 35.874));
    processorRef.getAdapter().setProgramPropertyValue(program,
                                                      kAudioUnitCustomProperty_BaseKey,
                                                      static_cast<float>(key));
    processorRef.forceParamSync();
    setStatusMessage("Root key recalculated");
}

void C700AudioProcessorEditor::syncEditorFields(int program)
{
    if (!mProgramNameEditor.hasKeyboardFocus(true)) {
        auto programName = juce::String(processorRef.getAdapter().getProgramStringProperty(program,
                                                                                           kAudioUnitCustomProperty_ProgramName));
        if (programName.isEmpty())
            programName = processorRef.getRuntimeState().sampleName;
        if (mProgramNameEditor.getText() != programName)
            mProgramNameEditor.setText(programName, juce::dontSendNotification);
    }

    if (!mLoopPointEditor.hasKeyboardFocus(true)) {
        const int loopPointBrr = juce::roundToInt(processorRef.getAdapter().getProgramPropertyValue(program,
                                                                                                     kAudioUnitCustomProperty_LoopPoint));
        const int loopPointSamples = (loopPointBrr / 9) * 16;
        const auto text = juce::String(loopPointSamples);
        if (mLoopPointEditor.getText() != text)
            mLoopPointEditor.setText(text, juce::dontSendNotification);
    }

    if (!mRateEditor.hasKeyboardFocus(true)) {
        const auto text = formatFloatValue(getParameterValue(processorRef.getAPVTS(), "rate"), 2);
        if (mRateEditor.getText() != text)
            mRateEditor.setText(text, juce::dontSendNotification);
    }

    auto syncIntEditor = [&](juce::TextEditor& editor, const juce::String& parameterId) {
        if (editor.hasKeyboardFocus(true))
            return;
        const auto text = juce::String(juce::roundToInt(getParameterValue(processorRef.getAPVTS(), parameterId)));
        if (editor.getText() != text)
            editor.setText(text, juce::dontSendNotification);
    };

    syncIntEditor(mBaseKeyEditor, "basekey");
    syncIntEditor(mLowKeyEditor, "lowkey");
    syncIntEditor(mHighKeyEditor, "highkey");
}

void C700AudioProcessorEditor::syncValueLabels()
{
    static constexpr const char* envelopeParamIds[] = { "ar", "dr", "sl", "sr1", "sr2" };
    for (int i = 0; i < static_cast<int>(mEnvelopeValueLabels.size()); ++i) {
        const auto value = juce::roundToInt(getParameterValue(processorRef.getAPVTS(), envelopeParamIds[i]));
        mEnvelopeValueLabels[static_cast<size_t>(i)].setText(juce::String(value), juce::dontSendNotification);
    }

    mVolLValueLabel.setText(juce::String(juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "voll"))),
                            juce::dontSendNotification);
    mVolRValueLabel.setText(juce::String(juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "volr"))),
                            juce::dontSendNotification);
    mVibDepthValueLabel.setText(formatFloatValue(getParameterValue(processorRef.getAPVTS(), "vibdepth2"), 3),
                                juce::dontSendNotification);
    mVibrateValueLabel.setText(formatFloatValue(getParameterValue(processorRef.getAPVTS(), "vibrate"), 3),
                               juce::dontSendNotification);
    mPortamentoRateLabel.setText(juce::String(juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "portarate"))),
                                 juce::dontSendNotification);
    mNotePriorityValueLabel.setText(juce::String(juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "noteprio"))),
                                    juce::dontSendNotification);
    mReleasePriorityValueLabel.setText(juce::String(juce::roundToInt(getParameterValue(processorRef.getAPVTS(), "releaseprio"))),
                                       juce::dontSendNotification);
}

void C700AudioProcessorEditor::loadSampleClicked()
{
    commitPendingFieldEdits();
    const int prog = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample to Slot " + juce::String(prog),
        mLastBrowseDir,
        "*.wav;*.brr");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, prog](const juce::FileChooser& fc) {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            const auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();
            if (processorRef.getAdapter().loadSampleToSlot(prog, file.getFullPathName().toStdString())) {
                processorRef.forceParamSync();
                setStatusMessage("Loaded sample into slot " + juce::String(prog), 4000);
            } else {
                const auto err = processorRef.getAdapter().getLastLoadError();
                setStatusMessage(err.empty() ? "Failed to load sample" : juce::String(err), 4000);
            }
        });
}

void C700AudioProcessorEditor::unloadSampleClicked()
{
    commitPendingFieldEdits();
    const int prog = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());
    if (processorRef.getAdapter().unloadSlot(prog)) {
        processorRef.forceParamSync();
        setStatusMessage("Unloaded slot " + juce::String(prog));
    } else {
        const auto err = processorRef.getAdapter().getLastLoadError();
        setStatusMessage(err.empty() ? "Failed to unload sample" : juce::String(err));
    }
}

void C700AudioProcessorEditor::saveSampleClicked()
{
    commitPendingFieldEdits();
    const int prog = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());
    auto data = processorRef.getAdapter().copyBRRData(prog);
    if (data.empty()) {
        setStatusMessage("No BRR data in the current slot");
        return;
    }

    const auto runtime = processorRef.getRuntimeState();
    const auto defaultName = defaultBrrFileName(prog, runtime.programName, runtime.sampleName);

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Save BRR Sample",
        mLastExportDir.getChildFile(defaultName),
        "*.brr");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, data = std::move(data)](const juce::FileChooser& fc) mutable {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            auto file = results.getFirst();
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".brr");

            mLastExportDir = file.getParentDirectory();
            setStatusMessage(file.replaceWithData(data.data(), data.size())
                                 ? "Saved BRR sample"
                                 : "Failed to save BRR sample");
        });
}

void C700AudioProcessorEditor::saveXiClicked()
{
    commitPendingFieldEdits();
    setStatusMessage("XI export is not wired yet in the JUCE editor", 4000);
}

void C700AudioProcessorEditor::loadPlayerCodeClicked()
{
    commitPendingFieldEdits();
    mFileChooser = std::make_unique<juce::FileChooser>(
        "Load Player Code Binary",
        mLastBrowseDir,
        "*.bin");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            const auto file = results.getFirst();
            mLastBrowseDir = file.getParentDirectory();
            setStatusMessage(processorRef.getAdapter().loadPlayerCode(file.getFullPathName().toStdString())
                                 ? "Player code loaded"
                                 : "Failed to load playercode.bin");
        });
}

void C700AudioProcessorEditor::exportSpcClicked()
{
    commitPendingFieldEdits();
    if (!processorRef.getAdapter().hasPlayerCode()) {
        setStatusMessage("Load playercode.bin first");
        return;
    }

    const float recStart = processorRef.getAPVTS().getRawParameterValue("rec_start")->load();
    const float recEnd = processorRef.getAPVTS().getRawParameterValue("rec_end")->load();
    if (recStart >= recEnd) {
        setStatusMessage("Set Record Start/End beats first", 4000);
        return;
    }

    mFileChooser = std::make_unique<juce::FileChooser>(
        "Export SPC",
        mLastExportDir,
        "*.spc");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;

            auto file = results.getFirst();
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".spc");

            mLastExportDir = file.getParentDirectory();
            processorRef.getAdapter().setSpcRecordPath(
                file.getParentDirectory().getFullPathName().toStdString());

            auto* dsp = processorRef.getAdapter().getKernel()->GetDriver()->GetDsp();
            auto title = file.getFileNameWithoutExtension().toStdString();
            dsp->SetSongTitle(title.substr(0, 31).c_str());
            processorRef.getAdapter().enableSpcRecording(true);

            setStatusMessage("Recording armed. Play through the region in REAPER.", 5000);
        });
}

void C700AudioProcessorEditor::adjustProgram(int delta)
{
    commitPendingFieldEdits();
    const int current = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue("program")->load());
    const int next = juce::jlimit(0, 127, current + delta);
    if (next == current)
        return;

    setDiscreteParameterValue(processorRef.getAPVTS(), "program", next);
    processorRef.forceParamSync();
    mProgramValueLabel.setText(juce::String(next), juce::dontSendNotification);
}

void C700AudioProcessorEditor::selectBank(int bank)
{
    bank = juce::jlimit(0, 3, bank);
    if (processorRef.getAdapter().setPropertyValue(kAudioUnitCustomProperty_Bank, static_cast<float>(bank))) {
        mDisplayedBank = bank;
        refreshSelectionButtons();
    }
}

void C700AudioProcessorEditor::selectEditingChannel(int channel)
{
    channel = juce::jlimit(0, 15, channel);
    if (processorRef.getAdapter().setPropertyValue(kAudioUnitCustomProperty_EditingChannel,
                                                   static_cast<float>(channel))) {
        mDisplayedChannel = channel;
        refreshSelectionButtons();
    }
}

void C700AudioProcessorEditor::refreshSelectionButtons()
{
    for (int bank = 0; bank < static_cast<int>(mBankButtons.size()); ++bank) {
        const auto offImage = cropFilmStripFrame(mBankStrips[bank], 0, 2);
        const auto onImage = cropFilmStripFrame(mBankStrips[bank], 1, 2);
        const auto normalImage = (mDisplayedBank == bank) ? onImage : offImage;
        configureMomentaryButton(mBankButtons[bank], normalImage, onImage);
    }

    for (int channel = 0; channel < static_cast<int>(mTrackButtons.size()); ++channel) {
        const auto offImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 0, 2),
                                            cropFilmStripFrame(mTrackNumberStrips[channel], 0, 2));
        const auto onImage = composeImages(cropFilmStripFrame(mTrackIndicatorStrip, 1, 2),
                                           cropFilmStripFrame(mTrackNumberStrips[channel], 1, 2));
        const bool isActive = mDisplayedChannel == channel || mDisplayedTrackActivity[channel] > 0;
        configureMomentaryButton(mTrackButtons[channel], isActive ? onImage : offImage, onImage);
    }
}

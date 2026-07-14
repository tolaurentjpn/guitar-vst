#include "FxRackComponent.h"
#include "EffectChain.h"

namespace
{
    const char* fxOrderIds[4] = {
        GuitarSynthAudioProcessor::paramFxOrder0,
        GuitarSynthAudioProcessor::paramFxOrder1,
        GuitarSynthAudioProcessor::paramFxOrder2,
        GuitarSynthAudioProcessor::paramFxOrder3
    };
}

FxRackComponent::FxRackComponent (juce::AudioProcessorValueTreeState& state, GuitarLookAndFeel& laf)
    : apvts (state), lookAndFeel (laf)
{
    for (auto& card : cards)
    {
        card.bypass.setClickingTogglesState (true);
        card.bypass.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c2028));
        card.bypass.setColour (juce::TextButton::buttonOnColourId, lookAndFeel.accentColour());
        card.bypass.setColour (juce::TextButton::textColourOffId, juce::Colours::whitesmoke);
        card.bypass.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible (card.bypass);

        auto styleArrow = [this] (juce::TextButton& b)
        {
            b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c2028));
            b.setColour (juce::TextButton::textColourOffId, juce::Colours::whitesmoke);
            addAndMakeVisible (b);
        };
        styleArrow (card.moveLeft);
        styleArrow (card.moveRight);
    }

    cards[0].moveLeft.onClick = [this] { moveSlot (0, -1); };
    cards[0].moveRight.onClick = [this] { moveSlot (0, 1); };
    cards[1].moveLeft.onClick = [this] { moveSlot (1, -1); };
    cards[1].moveRight.onClick = [this] { moveSlot (1, 1); };
    cards[2].moveLeft.onClick = [this] { moveSlot (2, -1); };
    cards[2].moveRight.onClick = [this] { moveSlot (2, 1); };
    cards[3].moveLeft.onClick = [this] { moveSlot (3, -1); };
    cards[3].moveRight.onClick = [this] { moveSlot (3, 1); };

    for (const char* id : fxOrderIds)
        apvts.addParameterListener (id, this);

    rebuildOrderFromApvts();
    refreshFromApvts();
}

FxRackComponent::~FxRackComponent()
{
    for (const char* id : fxOrderIds)
        apvts.removeParameterListener (id, this);
}

const char* FxRackComponent::enabledParamForType (int typeId) const
{
    switch (typeId)
    {
        case 0: return GuitarSynthAudioProcessor::paramDistEnabled;
        case 1: return GuitarSynthAudioProcessor::paramCompEnabled;
        case 2: return GuitarSynthAudioProcessor::paramDelayEnabled;
        case 3: return GuitarSynthAudioProcessor::paramReverbEnabled;
        default: return GuitarSynthAudioProcessor::paramDistEnabled;
    }
}

juce::String FxRackComponent::labelForType (int typeId) const
{
    return EffectChain::nameForType (typeId);
}

void FxRackComponent::rebuildOrderFromApvts()
{
    order = {
        static_cast<int> (*apvts.getRawParameterValue (GuitarSynthAudioProcessor::paramFxOrder0)),
        static_cast<int> (*apvts.getRawParameterValue (GuitarSynthAudioProcessor::paramFxOrder1)),
        static_cast<int> (*apvts.getRawParameterValue (GuitarSynthAudioProcessor::paramFxOrder2)),
        static_cast<int> (*apvts.getRawParameterValue (GuitarSynthAudioProcessor::paramFxOrder3))
    };

    bool seen[4] = {};
    bool valid = true;
    for (int id : order)
    {
        if (id < 0 || id > 3 || seen[id])
        {
            valid = false;
            break;
        }
        seen[id] = true;
    }

    if (! valid)
        order = { 0, 1, 2, 3 };

    for (int i = 0; i < 4; ++i)
    {
        cards[static_cast<size_t> (i)].typeId = order[static_cast<size_t> (i)];
        cards[static_cast<size_t> (i)].bypassAttachment.reset();
        cards[static_cast<size_t> (i)].bypassAttachment =
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, enabledParamForType (order[static_cast<size_t> (i)]),
                cards[static_cast<size_t> (i)].bypass);
    }

    repaint();
}

void FxRackComponent::writeOrderToApvts()
{
    for (int i = 0; i < 4; ++i)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (fxOrderIds[i])))
            p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (order[static_cast<size_t> (i)])));
    }
}

void FxRackComponent::moveSlot (int slotIndex, int delta)
{
    const int other = slotIndex + delta;
    if (slotIndex < 0 || slotIndex > 3 || other < 0 || other > 3)
        return;

    std::swap (order[static_cast<size_t> (slotIndex)], order[static_cast<size_t> (other)]);
    writeOrderToApvts();
    rebuildOrderFromApvts();
}

void FxRackComponent::refreshFromApvts()
{
    rebuildOrderFromApvts();
}

void FxRackComponent::setSelectedType (int typeId)
{
    selectedType = juce::jlimit (0, 3, typeId);
    repaint();
    if (onSelectionChanged)
        onSelectionChanged();
}

void FxRackComponent::parameterChanged (const juce::String&, float)
{
    // Defer APVTS listener work off the message queue edge cases
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<FxRackComponent> (this)]
    {
        if (safe != nullptr)
            safe->rebuildOrderFromApvts();
    });
}

void FxRackComponent::paint (juce::Graphics& g)
{
    for (int i = 0; i < 4; ++i)
    {
        const auto& card = cards[static_cast<size_t> (i)];
        const bool selected = card.typeId == selectedType;
        const bool dragTarget = (dragOverSlot == i && dragSlot >= 0 && dragSlot != i);

        auto fill = lookAndFeel.panelColour().brighter (selected ? 0.12f : 0.0f);
        if (dragTarget)
            fill = fill.brighter (0.18f);

        g.setColour (fill);
        g.fillRoundedRectangle (card.bounds.toFloat(), 8.0f);
        g.setColour (selected ? lookAndFeel.accentColour() : juce::Colour (0xff3a404c));
        g.drawRoundedRectangle (card.bounds.toFloat().reduced (0.5f), 8.0f, selected ? 2.0f : 1.0f);

        g.setColour (lookAndFeel.textColour());
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        auto titleArea = card.bounds.reduced (8, 6).removeFromTop (22);
        titleArea.removeFromRight (52);
        g.drawText (labelForType (card.typeId), titleArea, juce::Justification::centredLeft, true);

        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.setColour (juce::Colours::grey);
        g.drawText ("Slot " + juce::String (i + 1),
                    card.bounds.reduced (8).removeFromBottom (16),
                    juce::Justification::centredLeft, true);
    }
}

void FxRackComponent::resized()
{
    auto area = getLocalBounds();
    const int gap = 8;
    const int cardW = (area.getWidth() - 3 * gap) / 4;

    for (int i = 0; i < 4; ++i)
    {
        auto& card = cards[static_cast<size_t> (i)];
        card.bounds = area.removeFromLeft (cardW);
        if (i < 3)
            area.removeFromLeft (gap);

        auto inner = card.bounds.reduced (6);
        auto top = inner.removeFromTop (26);
        card.bypass.setBounds (top.removeFromRight (48).reduced (1));
        top.removeFromRight (4);
        card.moveRight.setBounds (top.removeFromRight (26).reduced (1));
        card.moveLeft.setBounds (top.removeFromRight (26).reduced (1));
    }
}

int FxRackComponent::hitTestCard (juce::Point<int> pos) const
{
    for (int i = 0; i < 4; ++i)
        if (cards[static_cast<size_t> (i)].bounds.contains (pos))
            return i;
    return -1;
}

void FxRackComponent::mouseDown (const juce::MouseEvent& e)
{
    const int slot = hitTestCard (e.getPosition());
    if (slot < 0)
        return;

    dragSlot = slot;
    dragOverSlot = slot;
    setSelectedType (cards[static_cast<size_t> (slot)].typeId);
}

void FxRackComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dragSlot < 0)
        return;

    const int over = hitTestCard (e.getPosition());
    if (over != dragOverSlot)
    {
        dragOverSlot = over;
        repaint();
    }
}

void FxRackComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragSlot >= 0 && dragOverSlot >= 0 && dragOverSlot != dragSlot)
    {
        // Move dragged card into target slot (rotate/swap)
        const int from = dragSlot;
        const int to = dragOverSlot;
        const int moving = order[static_cast<size_t> (from)];

        if (from < to)
        {
            for (int i = from; i < to; ++i)
                order[static_cast<size_t> (i)] = order[static_cast<size_t> (i + 1)];
            order[static_cast<size_t> (to)] = moving;
        }
        else
        {
            for (int i = from; i > to; --i)
                order[static_cast<size_t> (i)] = order[static_cast<size_t> (i - 1)];
            order[static_cast<size_t> (to)] = moving;
        }

        writeOrderToApvts();
        rebuildOrderFromApvts();
    }

    dragSlot = -1;
    dragOverSlot = -1;
    repaint();
}

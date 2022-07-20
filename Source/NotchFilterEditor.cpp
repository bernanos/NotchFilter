/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "NotchFilterEditor.h"
#include "NotchFilterNode.h"
#include <stdio.h>


NotchFilterEditor::NotchFilterEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors=true)
    : GenericEditor(parentNode, useDefaultParameterEditors)

{
    desiredWidth = 150;

    lastCentreFreqString = " ";
    lastWidthString = " ";

    centreFreqLabel = new Label("Centre freq label", "Centre Freq:");
    centreFreqLabel->setBounds(10,65,80,20);
    centreFreqLabel->setFont(Font("Small Text", 12, Font::plain));
    centreFreqLabel->setColour(Label::textColourId, Colours::darkgrey);
    addAndMakeVisible(centreFreqLabel);

    widthLabel = new Label("Width label", "Width:");
    widthLabel->setBounds(10,25,80,20);
    widthLabel->setFont(Font("Small Text", 12, Font::plain));
    widthLabel->setColour(Label::textColourId, Colours::darkgrey);
    addAndMakeVisible(widthLabel);

    centreFreqValue = new Label("Centre Freq value", lastCentreFreqString);
    centreFreqValue->setBounds(15,42,60,18);
    centreFreqValue->setFont(Font("Default", 15, Font::plain));
    centreFreqValue->setColour(Label::textColourId, Colours::white);
    centreFreqValue->setColour(Label::backgroundColourId, Colours::grey);
    centreFreqValue->setEditable(true);
    centreFreqValue->addListener(this);
    centreFreqValue->setTooltip("Set the centre frequency for the selected channels");
    addAndMakeVisible(centreFreqValue);

    widthValue = new Label("Width value", lastWidthString);
    widthValue->setBounds(15,82,60,18);
    widthValue->setFont(Font("Default", 15, Font::plain));
    widthValue->setColour(Label::textColourId, Colours::white);
    widthValue->setColour(Label::backgroundColourId, Colours::grey);
    widthValue->setEditable(true);
    widthValue->addListener(this);
    widthValue->setTooltip("Set the bandwidth for the selected channels");
    addAndMakeVisible(widthValue);

    applyFilterOnADC = new UtilityButton("+ADCs",Font("Default", 10, Font::plain));
    applyFilterOnADC->addListener(this);
    applyFilterOnADC->setBounds(90,70,40,18);
    applyFilterOnADC->setClickingTogglesState(true);
    applyFilterOnADC->setTooltip("When this button is off, ADC channels will not be filtered");
    addAndMakeVisible(applyFilterOnADC);

    applyFilterOnChan = new UtilityButton("+CH",Font("Default", 10, Font::plain));
    applyFilterOnChan->addListener(this);
    applyFilterOnChan->setBounds(95,95,30,18);
    applyFilterOnChan->setClickingTogglesState(true);
    applyFilterOnChan->setToggleState(true, dontSendNotification);
    applyFilterOnChan->setTooltip("When this button is off, selected channels will not be filtered");
    addAndMakeVisible(applyFilterOnChan);

}

NotchFilterEditor::~NotchFilterEditor()
{

}

void NotchFilterEditor::setDefaults(double lowCut, double highCut)
{
    lastWidthString = String(roundFloatToInt(highCut));
    lastCentreFreqString = String(roundFloatToInt(lowCut));

    resetToSavedText();
}

void NotchFilterEditor::resetToSavedText()
{
    widthValue->setText(lastWidthString, dontSendNotification);
    centreFreqValue->setText(lastCentreFreqString, dontSendNotification);
}


void NotchFilterEditor::labelTextChanged(Label* label)
{
    NotchFilterNode* fn = (NotchFilterNode*) getProcessor();

    Value val = label->getTextValue();
    double requestedValue = double(val.getValue());

    if (requestedValue < 0.01 || requestedValue > 10000)
    {
        CoreServices::sendStatusMessage("Value out of range.");

        if (label == widthValue)
        {
            label->setText(lastWidthString, dontSendNotification);
            lastWidthString = label->getText();
        }
        else
        {
            label->setText(lastCentreFreqString, dontSendNotification);
            lastCentreFreqString = label->getText();
        }

        return;
    }

    Array<int> chans = getActiveChannels();

    // This needs to change, since there's not enough feedback about whether
    // or not individual channel settings were altered:

    for (int n = 0; n < chans.size(); n++)
    {

        if (label == widthValue)
        {
            double minVal = fn->getcentreFreqValueForChannel(chans[n]);

            if (requestedValue > minVal)
            {
                fn->setCurrentChannel(chans[n]);
                fn->setParameter(1, requestedValue);
            }

            lastWidthString = label->getText();

        }
        else
        {
            double maxVal = fn->getwidthValueForChannel(chans[n]);

            if (requestedValue < maxVal)
            {
                fn->setCurrentChannel(chans[n]);
                fn->setParameter(0, requestedValue);
            }

            lastCentreFreqString = label->getText();
        }

    }

}

void NotchFilterEditor::channelChanged (int channel, bool /*newState*/)
{
    NotchFilterNode* fn = (NotchFilterNode*) getProcessor();

    widthValue->setText (String (fn->getwidthValueForChannel (channel)), dontSendNotification);
    centreFreqValue->setText  (String (fn->getcentreFreqValueForChannel  (channel)), dontSendNotification);
    applyFilterOnChan->setToggleState (fn->getBypassStatusForChannel (channel), dontSendNotification);

}

void NotchFilterEditor::buttonEvent(Button* button)
{

    if (button == applyFilterOnADC)
    {
        NotchFilterNode* fn = (NotchFilterNode*) getProcessor();
        fn->setApplyOnADC(applyFilterOnADC->getToggleState());

    }
    else if (button == applyFilterOnChan)
    {
        NotchFilterNode* fn = (NotchFilterNode*) getProcessor();

        Array<int> chans = getActiveChannels();

        for (int n = 0; n < chans.size(); n++)
        {
            float newValue = button->getToggleState() ? 1.0 : 0.0;

            fn->setCurrentChannel(chans[n]);
            fn->setParameter(2, newValue);
        }
    }
}


void NotchFilterEditor::saveCustomParameters(XmlElement* xml)
{

    xml->setAttribute("Type", "NotchFilterEditor");

    lastWidthString = widthValue->getText();
    lastCentreFreqString = centreFreqValue->getText();

    XmlElement* textLabelValues = xml->createNewChildElement("VALUES");
    textLabelValues->setAttribute("HighCut",lastWidthString);
    textLabelValues->setAttribute("LowCut",lastCentreFreqString);
    textLabelValues->setAttribute("ApplyToADC",	applyFilterOnADC->getToggleState());
}

void NotchFilterEditor::loadCustomParameters(XmlElement* xml)
{

    forEachXmlChildElement(*xml, xmlNode)
    {
        if (xmlNode->hasTagName("VALUES"))
        {
            lastWidthString = xmlNode->getStringAttribute("HighCut", lastWidthString);
            lastCentreFreqString = xmlNode->getStringAttribute("LowCut", lastCentreFreqString);
            resetToSavedText();

            applyFilterOnADC->setToggleState(xmlNode->getBoolAttribute("ApplyToADC",false), sendNotification);
        }
    }


}

/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Open Ephys

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

#include <stdio.h>
#include "NotchNotchFilterNode.h"
#include "NotchNotchFilterEditor.h"


NotchFilterNode::NotchFilterNode()
    : GenericProcessor  ("Notch Filter")
    , defaultCentreFreq     (50.0f)
    , defaultWidth    (5.0f)
{
    setProcessorType (PROCESSOR_TYPE_FILTER);

    // // Deprecated "parameters" class // //
    // Array<var> CentreFreqValues;
    // CentreFreqValues.add(1.0f);
    // CentreFreqValues.add(4.0f);
    // CentreFreqValues.add(100.0f);
    // CentreFreqValues.add(600.0f);

    // parameters.add(Parameter("low cut",CentreFreqValues, 3, 0));

    // Array<var> WidthValues;
    // WidthValues.add(12.0f);
    // WidthValues.add(3000.0f);
    // WidthValues.add(6000.0f);
    // WidthValues.add(9000.0f);

    // parameters.add(Parameter("high cut",WidthValues, 2, 1));
    applyOnADC = false;
}


NotchFilterNode::~NotchFilterNode()
{
}


AudioProcessorEditor* NotchFilterNode::createEditor()
{
    editor = new NotchFilterEditor (this, true);

    NotchFilterEditor* ed = (NotchFilterEditor*) getEditor();
    ed->setDefaults (defaultCentreFreq, defaultWidth);

    return editor;
}

// ----------------------------------------------------
// From the filter library documentation:
// ----------------------------------------------------
//
// each family of filters is given its own namespace
// RBJ: filters from the RBJ cookbook
// Butterworth
// ChebyshevI: ripple in the passband
// ChebyshevII: ripple in the stop band
// Elliptic: ripple in both the passband and stopband
// Bessel: theoretically with linear phase
// Legendre: "Optimum-L" filters with steepest transition and monotonic passband
// Custom: Simple filters that allow poles and zeros to be specified directly

// within each namespace exists a set of "raw filters"
// Butterworth::LowPass
//                HighPass
//                 BandPass
//                BandStop
//                LowShelf
//                 HighShelf
//                BandShelf
//
//    class templates (such as SimpleFilter) which require FilterClasses
//    expect an identifier of a raw filter
//  raw filters do not support introspection, or the Params style of changing
//    filter settings; they only offer a setup() function for updating the IIR
//    coefficients to a given set of parameters
//

// each filter family namespace also has the nested namespace "Design"
// here we have all of the raw filter names repeated, except these classes
//  also provide the Design interface, which adds introspection, polymorphism,
//  the Params style of changing filter settings, and in general all fo the features
//  necessary to interoperate with the Filter virtual base class and its derived classes

// available methods:
//
// filter->getKind()
// filter->getName()
// filter->getNumParams()
// filter->getParamInfo()
// filter->getDefaultParams()
// filter->getParams()
// filter->getParam()

// filter->setParam()
// filter->findParamId()
// filter->setParamById()
// filter->setParams()
// filter->copyParamsFrom()

// filter->getPoleZeros()
// filter->response()
// filter->getNumChannels()
// filter->reset()
// filter->process()

void NotchFilterNode::updateSettings()
{
    //int id = nodeId;
    int numInputs = getNumInputs();
    int numfilt = filters.size();
    if (numInputs < 1024 && numInputs != numfilt)
    {
        // SO fixed this. I think values were never restored correctly because you cleared CentreFreq.
        Array<double> oldCentreFreq;
        Array<double> oldWidth;
        oldCentreFreq = CentreFreq;
        oldWidth = Width;

        filters.clear();
        CentreFreq.clear();
        Width.clear();
        shouldFilterChannel.clear();

        for (int n = 0; n < getNumInputs(); ++n)
        {
            filters.add (new Dsp::SmoothedFilterDesign
                         <Dsp::Butterworth::Design::BandStop    // design type
                         <2>,                                   // order
                         1,                                     // number of channels (must be const)
                         Dsp::DirectFormII> (1));               // realization


            //Parameter& p1 =  parameters.getReference(0);
            //p1.setValue(600.0f, n);
            //Parameter& p2 =  parameters.getReference(1);
            //p2.setValue(6000.0f, n);

            // restore defaults

            shouldFilterChannel.add (true);

            float newCentreFreq  = 0.f;
            float newWidth = 0.f;

            if (oldCentreFreq.size() > n)
            {
                newCentreFreq  = oldCentreFreq[n];
                newWidth = oldWidth[n];
            }
            else
            {
                newCentreFreq  = defaultCentreFreq;
                newWidth = defaultWidth;
            }

            CentreFreq.add  (newCentreFreq);
            Width.add (newWidth);

            setFilterParameters (newCentreFreq, newWidth, n);
        }
    }

    setApplyOnADC (applyOnADC);
}


double NotchFilterNode::getCentreFreqValueForChannel (int chan) const
{
    return CentreFreq[chan];
}


double NotchFilterNode::getWidthValueForChannel (int chan) const
{
    return Width[chan];
}


bool NotchFilterNode::getBypassStatusForChannel (int chan) const
{
    return shouldFilterChannel[chan];
}


void NotchFilterNode::setFilterParameters (double CentreFreq, double Width, int chan)
{
    if (dataChannelArray.size() - 1 < chan)
        return;

    Dsp::Params params;
    params[0] = dataChannelArray[chan]->getSampleRate(); // sample rate
    params[1] = 2;                          // order
    params[2] = CentreFreq);                // center frequency
    params[3] = Width;                      // bandwidth

    if (filters.size() > chan)
        filters[chan]->setParams (params);
}


void NotchFilterNode::setParameter (int parameterIndex, float newValue)
{
    if (parameterIndex < 2) // change filter settings
    {
        if (newValue <= 0.01 || newValue >= 10000.0f)
            return;

        if (parameterIndex == 0)
        {
            CentreFreq.set (currentChannel,newValue);
        }
        else if (parameterIndex == 1)
        {
            Width.set (currentChannel,newValue);
        }

        setFilterParameters (CentreFreq[currentChannel],
                             Width[currentChannel],
                             currentChannel);

        editor->updateParameterButtons (parameterIndex);
    }
    // change channel bypass state
    else
    {
        if (newValue == 0)
        {
            shouldFilterChannel.set (currentChannel, false);
        }
        else
        {
            shouldFilterChannel.set (currentChannel, true);
        }
    }
}


void NotchFilterNode::process (AudioSampleBuffer& buffer)
{
    for (int n = 0; n < getNumOutputs(); ++n)
    {
        if (shouldFilterChannel[n])
        {
            float* ptr = buffer.getWritePointer (n);
            filters[n]->process (getNumSamples (n), &ptr);
        }
    }
}


void NotchFilterNode::setApplyOnADC (bool state)
{
    for (int n = 0; n < dataChannelArray.size(); ++n)
    {
        if (dataChannelArray[n]->getChannelType() == DataChannel::ADC_CHANNEL
            || dataChannelArray[n]->getChannelType() == DataChannel::AUX_CHANNEL)
        {
            setCurrentChannel (n);

            if (state)
                setParameter (2,1.0);
            else
                setParameter (2,0.0);
        }
    }
}


void NotchFilterNode::saveCustomChannelParametersToXml(XmlElement* channelInfo, int channelNumber, InfoObjectCommon::InfoObjectType channelType)
{
    if (channelType == InfoObjectCommon::DATA_CHANNEL
        && channelNumber > -1
        && channelNumber < Width.size())
    {
        //std::cout << "Saving custom parameters for filter node." << std::endl;

        XmlElement* channelParams = channelInfo->createNewChildElement ("PARAMETERS");
        channelParams->setAttribute ("Width",         Width[channelNumber]);
        channelParams->setAttribute ("CentreFreq",          CentreFreq[channelNumber]);
        channelParams->setAttribute ("shouldFilter",    shouldFilterChannel[channelNumber]);
    }
}


void NotchFilterNode::loadCustomChannelParametersFromXml(XmlElement* channelInfo, InfoObjectCommon::InfoObjectType channelType)
{
    int channelNum = channelInfo->getIntAttribute ("number");

    if (channelType == InfoObjectCommon::DATA_CHANNEL)
    {
        // restore high and low cut text in case they were changed by channelChanged
        static_cast<NotchFilterEditor*>(getEditor())->resetToSavedText();

        forEachXmlChildElement (*channelInfo, subNode)
        {
            if (subNode->hasTagName ("PARAMETERS"))
            {
                Width.set (channelNum, subNode->getDoubleAttribute ("Width", defaultWidth));
                CentreFreq.set  (channelNum, subNode->getDoubleAttribute ("CentreFreq",  defaultCentreFreq));
                shouldFilterChannel.set (channelNum, subNode->getBoolAttribute ("shouldFilter", true));

                setFilterParameters (CentreFreq[channelNum], Width[channelNum], channelNum);
            }
        }
    }
}

/*
  Copyright (C) 2016 Rory Walsh

  Cabbage is free software; you can redistribute it
  and/or modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Cabbage is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include <csound.hpp>
#include <csdebug.h>
#include "csdl.h"
#include <cwindow.h>
#include "../../Utilities/CabbageUtilities.h"
#include "CabbageCsoundBreakpointData.h"


//==============================================================================
class CsoundPluginProcessor  : public AudioProcessor, public AsyncUpdater
{

public:
    //==============================================================================
    CsoundPluginProcessor (File csoundInputFile, bool debugMode = false);
    ~CsoundPluginProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    virtual void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    virtual AudioProcessorEditor* createEditor() override;
    virtual bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    //==============================================================================
    virtual void getStateInformation (MemoryBlock& destData) override;
    virtual void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    int getIntendedNumberOfChannels (String csdText);       //gets the number of chanels delcared using nchnls, regardless of errors..
    //==============================================================================
    //Csound API functions for deailing with midi input
    static int OpenMidiInputDevice (CSOUND* csnd, void** userData, const char* devName);
    static int OpenMidiOutputDevice (CSOUND* csnd, void** userData, const char* devName);
    static int ReadMidiData (CSOUND* csound, void* userData, unsigned char* mbuf, int nbytes);
    static int WriteMidiData (CSOUND* csound, void* userData, const unsigned char* mbuf, int nbytes);

    //graphing functions
    static void makeGraphCallback (CSOUND* csound, WINDAT* windat, const char* name);
    static void drawGraphCallback (CSOUND* csound, WINDAT* windat);
    static void killGraphCallback (CSOUND* csound, WINDAT* windat);
    static int exitGraphCallback (CSOUND* csound);

    //logger
    void createFileLogger (File csdFile);

    void handleAsyncUpdate() override;
    //csound breakpint function
    static void breakpointCallback (CSOUND* csound, debug_bkpt_info_t* bkpt_info, void* udata);
    CabbageCsoundBreakpointData breakPointData;

    ValueTree getBreakpointData()
    {
        return breakPointData.valueTree;
    }

    StringArray getTableStatement (int tableNum);
    const Array<float, CriticalSection> getTableFloats (int tableNum);
    int checkTable (int tableNum);
    AudioPlayHead::CurrentPositionInfo hostInfo;

    //=============================================================================
    //Implement these to init, send and receive channel data to Csound. Typically used when
    //a component is updated and its value is sent to Csound, or when a Csound channel
    //is updated in the Csound orchestra and we need to update the Cabbage components.
    //Note that sendChannelDataToCsound() if we subclass the AudioprocessorParameter clas
    //as is done in CabbagePluginprocessor.
    virtual void sendChannelDataToCsound();
    virtual void getChannelDataFromCsound() {};
    virtual void initAllCsoundChannels (ValueTree cabbageData);
    //=============================================================================
    void addMacros (String csdText);
    const String getCsoundOutput();

    void compileCsdFile (File csdFile)
    {
        csCompileResult = csound->Compile (const_cast<char*> (csdFile.getFullPathName().toUTF8().getAddress()));
    }

    bool csdCompiledWithoutError()
    {
        return csCompileResult == 0 ? true : false;
    }

    Csound* getCsound()
    {
        return csound;
    }

    CSOUND* getCsoundStruct()
    {
        return csound->GetCsound();
    }

    void setGUIRefreshRate (int rate)
    {
        guiRefreshRate = rate;
    }



    int getNumberOfCsoundChannels()
    {
        return numCsoundChannels;
    }

    MidiKeyboardState keyboardState;

    //==================================================================================
    class SignalDisplay
    {
    public:
        float yScale;
        int windid, min , max, size;
        String caption;

        SignalDisplay (String _caption, int _id, float _scale, int _min, int _max, int _size):
            caption (_caption),
            windid (_id),
            yScale (_scale),
            min (_min),
            max (_max),
            size (_size)
        {}

        ~SignalDisplay()
        {
            points.clear();
        }

        Array<float, CriticalSection> getPoints()
        {
            return points;
        }

        void setPoints (Array <float, CriticalSection > tablePoints)
        {
            points.swapWith (tablePoints);
        }

    private:
        Array <float, CriticalSection > points;
    };

    bool shouldUpdateSignalDisplay()
    {
        bool returnVal = updateSignalDisplay;
        updateSignalDisplay = false;
        return returnVal;
    };

    OwnedArray <SignalDisplay, CriticalSection> signalArrays;   //holds values from FFT function table created using dispfft
    CsoundPluginProcessor::SignalDisplay* getSignalArray (String variableName, String displayType = "");
private:
    //==============================================================================
    MidiBuffer midiOutputBuffer;
    int guiCycles = 0;
    int guiRefreshRate = 50;
    MidiBuffer midiBuffer;
    String csoundOutput;
    ScopedPointer<CSOUND_PARAMS> csoundParams;
    int csCompileResult, numCsoundChannels, pos;
    bool updateSignalDisplay = false;
    MYFLT cs_scale;
    MYFLT* CSspin, *CSspout;
    int csndIndex;
    int csdKsmps;
    File csdFile;
    ScopedPointer<Csound> csound;
    ScopedPointer<FileLogger> fileLogger;
    int busIndex = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CsoundPluginProcessor)

};


#endif  // PLUGINPROCESSOR_H_INCLUDED

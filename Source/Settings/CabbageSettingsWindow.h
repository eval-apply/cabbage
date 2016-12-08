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

#ifndef CABBAGESETTINGSWINDOW_H_INCLUDED
#define CABBAGESETTINGSWINDOW_H_INCLUDED

#include "../CabbageIds.h"
#include "CabbageSettings.h"
#include "../Utilities/CabbageColourProperty.h"
#include "../Utilities/CabbageFilePropertyComponent.h"
#include "../Utilities/CabbageUtilities.h"
#include "../BinaryData/CabbageBinaryData.h"


class CabbageSettingsWindow :
    public Component,
    public Button::Listener,
    public Value::Listener,
    public FilenameComponentListener,
    public ChangeListener
{

public:
    CabbageSettingsWindow(CabbageSettings &settings, AudioDeviceSelectorComponent* audioDevice);
    ~CabbageSettingsWindow()
    {
        audioDeviceSelector = nullptr;
    };

    void changeListenerCallback(ChangeBroadcaster *source);
    void addColourProperties();
    void addMiscProperties();
    void resized();
    void buttonClicked(Button* button);
    void paint(Graphics& g);
    void valueChanged(Value& value);
    void filenameComponentChanged (FilenameComponent*);


private:
    PropertyPanel colourPanel, miscPanel;
    ScopedPointer<AudioDeviceSelectorComponent> audioDeviceSelector;
    ValueTree valueTree;
    TextButton loadButton, saveButton;
    ImageButton audioSettingsButton, colourSettingsButton, miscSettingsButton;
    CabbageSettings &settings;
    Value alwaysOnTopValue, showLastOpenedFileValue, compileOnSaveValue, breakLinesValue;

};



#endif  // CABBAGESETTINGSWINDOW_H_INCLUDED

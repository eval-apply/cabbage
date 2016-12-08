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

#include "CabbageKeyboard.h"

CabbageKeyboard::CabbageKeyboard(ValueTree wData, MidiKeyboardState &state)
	: widgetData(wData),
      keyWidth(CabbageWidgetData::getNumProp(wData, CabbageIdentifierIds::keywidth)),
      scrollbars(CabbageWidgetData::getNumProp(wData, CabbageIdentifierIds::scrollbars)),
	  MidiKeyboardComponent(state, MidiKeyboardComponent::horizontalKeyboard)
{
	setOrientation(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::kind)=="horizontal" ? MidiKeyboardComponent::horizontalKeyboard : MidiKeyboardComponent::verticalKeyboardFacingRight);
	setName(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::name));
	widgetData.addListener(this); 				//add listener to valueTree so it gets notified when a widget's property changes
	initialiseCommonAttributes(this, wData); 	//initialise common attributes such as bounds, name, rotation, etc..	

	setLowestVisibleKey(CabbageWidgetData::getNumProp(wData, CabbageIdentifierIds::value));
	setOctaveForMiddleC(CabbageWidgetData::getNumProp(wData, CabbageIdentifierIds::middlec));
	setKeyWidth(keyWidth);
	setScrollButtonsVisible(scrollbars==1 ? true : false);
	setColour(MidiKeyboardComponent::whiteNoteColourId, Colour::fromString(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::whitenotecolour)));
	setColour(MidiKeyboardComponent::blackNoteColourId, Colour::fromString(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::blacknotecolour)));
	setColour(MidiKeyboardComponent::upDownButtonArrowColourId, Colour::fromString(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::arrowcolour)));
	setColour(MidiKeyboardComponent::upDownButtonBackgroundColourId, Colour::fromString(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::arrowbackgroundcolour)));
	setColour(MidiKeyboardComponent::keySeparatorLineColourId, Colour::fromString(CabbageWidgetData::getStringProp(wData, CabbageIdentifierIds::noteseparatorcolour)));
	

}

void CabbageKeyboard::valueTreePropertyChanged (ValueTree& valueTree, const Identifier& prop)
{
	setOrientation(CabbageWidgetData::getStringProp(valueTree, CabbageIdentifierIds::kind)=="horizontal" ? MidiKeyboardComponent::horizontalKeyboard : MidiKeyboardComponent::verticalKeyboardFacingRight);
	setColour(MidiKeyboardComponent::whiteNoteColourId, Colour::fromString(CabbageWidgetData::getStringProp(valueTree, CabbageIdentifierIds::whitenotecolour)));
	setColour(MidiKeyboardComponent::blackNoteColourId, Colour::fromString(CabbageWidgetData::getStringProp(valueTree, CabbageIdentifierIds::blacknotecolour)));
	setColour(MidiKeyboardComponent::upDownButtonArrowColourId, Colour::fromString(CabbageWidgetData::getStringProp(valueTree, CabbageIdentifierIds::arrowcolour)));
	setColour(MidiKeyboardComponent::upDownButtonBackgroundColourId, Colour::fromString(CabbageWidgetData::getStringProp(valueTree, CabbageIdentifierIds::arrowbackgroundcolour)));
	setColour(MidiKeyboardComponent::keySeparatorLineColourId, Colour::fromString(CabbageWidgetData::getStringProp(valueTree, CabbageIdentifierIds::noteseparatorcolour)));	
	handleCommonUpdates(this, valueTree);		//handle comon updates such as bounds, alpha, rotation, visible, etc	
}

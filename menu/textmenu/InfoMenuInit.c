/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "include/boot.h"
#include "BootIde.h"
#include "TextMenu.h"
#include "InfoMenuActions.h"

TEXTMENU *InfoMenuInit(void) {
    TEXTMENUITEM *itemPtr;
    TEXTMENU *menuPtr;
    int i=0;

    menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
    memset(menuPtr,0x00,sizeof(TEXTMENU));
    strcpy(menuPtr->szCaption, "Info Menu");

    //No entry in this menu will have a configurable parameter.
    //Set first character to NULL to indicate no string is to be shown.
    itemPtr->szParameter[0]=0;

    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    sprintf(itemPtr->szCaption,"Temperature");
    itemPtr->functionPtr= ShowTemperature;
      itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);

    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    sprintf(itemPtr->szCaption,"Video");
    itemPtr->functionPtr= ShowVideo;
      itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);

    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    sprintf(itemPtr->szCaption,"EEPROM");
    itemPtr->functionPtr= ShowEeprom;
      itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);

    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    sprintf(itemPtr->szCaption,"Flash Device");
    itemPtr->functionPtr= ShowFlashChip;
      itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);
        
    return menuPtr;
}

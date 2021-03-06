/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "TextMenu.h"
#include "ToolsMenuActions.h"
#include "lpcmod_v1.h"

TEXTMENU *ToolsMenuInit(void) {
    TEXTMENUITEM *itemPtr;
    TEXTMENU *menuPtr;
    int i=0;

    //No entry in this menu will have a configurable parameter.
    //Set first character to NULL to indicate no string is to be shown.
    itemPtr->szParameter[0]=0;

    menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
    memset(menuPtr,0x00,sizeof(TEXTMENU));
    strcpy(menuPtr->szCaption, "Tools");

    if(cromwell_config==CROMWELL || fHasHardware == SYSCON_ID_V1) {
        //Save EEPROM data to flash
        itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
        memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
        sprintf(itemPtr->szCaption,"Save EEPROM to modchip");
        itemPtr->functionPtr= saveEEPromToFlash;
        itemPtr->functionDataPtr = NULL;
        TextMenuAddItem(menuPtr, itemPtr);

        //Restore EEPROM data from flash
        itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
        memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
        strcpy(itemPtr->szCaption, "Restore EEPROM from modchip");
        itemPtr->functionPtr= restoreEEPromFromFlash;
        itemPtr->functionDataPtr = NULL;
        TextMenuAddItem(menuPtr, itemPtr);
    }

    //Wipe EEPROM section that holds non-vital data.
    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    strcpy(itemPtr->szCaption, "Reset system settings");
    itemPtr->functionPtr= wipeEEPromUserSettings;
    itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);

    //Do not show this entry if 1.6/1.6b
    if(mbVersion != REV1_6){
        //128MB MEMORY TEST
        itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
        memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
        strcpy(itemPtr->szCaption, "128MB RAM test");
        itemPtr->functionPtr=showMemTest;
        itemPtr->functionDataPtr = NULL;
        TextMenuAddItem(menuPtr, itemPtr);
    }
/*
    //TSOP recovery entries. Do not show if already in TSOP recovery
    if((cromwell_config==CROMWELL || fHasHardware == SYSCON_ID_V1)
       && !TSOPRecoveryMode) {
        //TSOP recovery
        itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
        memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
        strcpy(itemPtr->szCaption, "TSOP Recovery");
        itemPtr->functionPtr=TSOPRecoveryReboot;
        itemPtr->functionDataPtr = NULL;
        TextMenuAddItem(menuPtr, itemPtr);
    }
*/
    //Save xblast.cfg
    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    strcpy(itemPtr->szCaption, "Save C:\\xblast.cfg");
    itemPtr->functionPtr= saveXBlastcfg;
    itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);

    //Load xblast.cfg
    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    strcpy(itemPtr->szCaption, "Load C:\\xblast.cfg");
    itemPtr->functionPtr= loadXBlastcfg;
    itemPtr->functionDataPtr = NULL;
    TextMenuAddItem(menuPtr, itemPtr);

    //Developers tools
    itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
    memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
    strcpy(itemPtr->szCaption, "Developer tools");
    itemPtr->functionPtr= DrawChildTextMenu;
    itemPtr->functionDataPtr = (void *)DeveloperMenuInit();
    TextMenuAddItem(menuPtr, itemPtr);

    return menuPtr;
}

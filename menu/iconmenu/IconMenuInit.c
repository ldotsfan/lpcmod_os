/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* This is where you should customise the menu, by adding your own icons.
 * The code in IconMenu.c should normally be left alone.
 */
#include "include/config.h"
#include "BootIde.h"
#include "IconMenu.h"
#include "MenuActions.h"
#include "lpcmod_v1.h"

void InitFatXIcons(void);
void InitNativeIcons(void);
void InitWebBootIcons(void);
void InitNetBootIcons(void);

void IconMenuInit(void) {
    int i=0;
    ICON *iconPtr=NULL;

//Not used for lpcmod OS.




    for (i=0; i<2; ++i) {
        //Add the cdrom icon - if you have two cdroms, you'll get two icons!
        if (tsaHarddiskInfo[i].m_fAtapi) {
            char *driveName=malloc(sizeof(char)*14);
            sprintf(driveName,"CD/DVD");
            iconPtr = (ICON *)malloc(sizeof(ICON));
            iconPtr->iconSlot = ICON_SOURCE_SLOT2;
            iconPtr->szCaption = driveName;
            iconPtr->functionPtr = BootFromCD;
            iconPtr->functionDataPtr = malloc(sizeof(int));
            *(int*)iconPtr->functionDataPtr = i;
            AddIcon(iconPtr);
        }
    }


    // For the Pro/ Home Gentoox distributions.  If MCE isnt installed
    // Gentoox Pro/ Home will be selected as the default icon.
    InitFatXIcons();

    // Largely for MCE.  MCE will get selected as the default boot icon
    // if it is installed.
    InitNativeIcons();

    // For booting a Packlet from the internet.
#ifdef LWIP
//    InitWebBootIcons();
//    InitNetBootIcons();
#endif

if(!TSOPRecoveryMode){ //Do not try to boot anything if in TSOP recovery.
    if(fHasHardware == SYSCON_ID_V1 || fHasHardware == SYSCON_ID_V1_TSOP) {
        iconPtr = (ICON *)malloc(sizeof(ICON));
        iconPtr->iconSlot = ICON_SOURCE_SLOT4;
        iconPtr->szCaption = "Boot 512KB bank";
        iconPtr->bankID = BNK512;
        iconPtr->functionPtr = BootModBios;
        iconPtr->functionDataPtr = malloc(sizeof(u8));
            *(u8*)iconPtr->functionDataPtr = BNK512;
        AddIcon(iconPtr);

        iconPtr = (ICON *)malloc(sizeof(ICON));
        iconPtr->iconSlot = ICON_SOURCE_SLOT3;
        iconPtr->szCaption = "Boot 256KB bank";
        iconPtr->bankID = BNK256;
        iconPtr->functionPtr = BootModBios;
        iconPtr->functionDataPtr = malloc(sizeof(u8));
            *(u8*)iconPtr->functionDataPtr = BNK256;
        AddIcon(iconPtr);
    }
    if(((fHasHardware == SYSCON_ID_V1) || (fHasHardware == SYSCON_ID_V1_TSOP)) && 
       (LPCmodSettings.OSsettings.TSOPcontrol & 0x02)){
            iconPtr = (ICON *)malloc(sizeof(ICON));
            iconPtr->iconSlot = ICON_SOURCE_SLOT2;
            iconPtr->szCaption = "Boot OnBoard Bank0";
            iconPtr->bankID = BNKTSOPSPLIT0;
            iconPtr->functionPtr = BootOriginalBios;
            iconPtr->functionDataPtr = malloc(sizeof(u8));
                    *(u8*)iconPtr->functionDataPtr = BNKTSOPSPLIT0;    //send 0x03
            AddIcon(iconPtr);

            iconPtr = (ICON *)malloc(sizeof(ICON));
            iconPtr->iconSlot = ICON_SOURCE_SLOT2;
            iconPtr->szCaption = "Boot OnBoard Bank1";
            iconPtr->bankID = BNKTSOPSPLIT1;
            iconPtr->functionPtr = BootOriginalBios;
            iconPtr->functionDataPtr = malloc(sizeof(u8));
                    *(u8*)iconPtr->functionDataPtr = BNKTSOPSPLIT1;    //send 0x0B
            AddIcon(iconPtr);
    }
    else {                                        //No split.
        iconPtr = (ICON *)malloc(sizeof(ICON));
        iconPtr->iconSlot = ICON_SOURCE_SLOT2;
        iconPtr->szCaption = "Boot OnBoard BIOS";
        iconPtr->bankID = BNKFULLTSOP;
        iconPtr->functionPtr = BootOriginalBios;
        iconPtr->functionDataPtr = malloc(sizeof(u8));
                *(u8*)iconPtr->functionDataPtr = BNKFULLTSOP;    //send 0x01
        AddIcon(iconPtr);
    }
}

#ifdef ADVANCED_MENU
    iconPtr = (ICON *)malloc(sizeof(ICON));
    iconPtr->iconSlot = ICON_SOURCE_SLOT0;
    iconPtr->szCaption = "Setttings";
    iconPtr->bankID = NOBNKID;
    iconPtr->functionPtr = AdvancedMenu;
    iconPtr->functionDataPtr = (void *)TextMenuInit();
    AddIcon(iconPtr);
#endif

#ifdef ETHERBOOT
    //Etherboot icon - if it's compiled in, it's always available.
    iconPtr = (ICON *)malloc(sizeof(ICON));
    iconPtr->iconSlot = ICON_SOURCE_SLOT3;
    iconPtr->szCaption = "Etherboot";
    iconPtr->functionPtr = BootFromEtherboot;
    AddIcon(iconPtr);
#endif    

    //Set this to point to the icon you want to be selected by default.
    //Otherwise, leave it alone, and the first icon will be selected.
    //selectedIcon = iconPtr;
}

void InitFatXIcons(void) {
    ICON *iconPtr=NULL;
    u8 ba[512];
    int driveId=0;
    
    if (tsaHarddiskInfo[driveId].m_fDriveExists && !tsaHarddiskInfo[driveId].m_fAtapi) {
        memset(ba,0x00,512);
        BootIdeReadSector(driveId, ba, 3, 0, 512);
        if (!strncmp("BRFR",ba,4)) {
            //Got a FATX formatted HDD
            CONFIGENTRY *entry = (CONFIGENTRY*)LoadConfigFatX();
            if (entry !=NULL) {
                //There is a config file present.
                iconPtr = (ICON *)malloc(sizeof(ICON));
               iconPtr->iconSlot = ICON_SOURCE_SLOT4;
                iconPtr->szCaption="   FatX";
                iconPtr->functionPtr = DrawBootMenu;
                iconPtr->functionDataPtr = (void *)entry;
                AddIcon(iconPtr);
                //If we have fatx, mark it as default.
                //If there are natives, they'll get priority shortly
                selectedIcon = iconPtr;
            }
        }
    }
}

void InitWebBootIcons(void) {
    ICON *iconPtr=NULL;
    iconPtr = (ICON *)malloc(sizeof(ICON));
      iconPtr->iconSlot = ICON_SOURCE_SLOT3;
    iconPtr->szCaption = "Web Boot";
    iconPtr->functionPtr = AdvancedMenu;
  //  iconPtr->functionDataPtr = (void *)IPMenuInit();
    AddIcon(iconPtr);
}

void InitNetBootIcons(void) {
    ICON *iconPtr=NULL;
    iconPtr = (ICON *)malloc(sizeof(ICON));
      iconPtr->iconSlot = ICON_SOURCE_SLOT3;
    iconPtr->szCaption = "Net Boot";
  //  iconPtr->functionPtr = BootFromNet;
    iconPtr->functionDataPtr = NULL;
    AddIcon(iconPtr);
}


void InitNativeIcons(void) {
    ICON *iconPtr=NULL;
    u8 ba[512];
    int driveId;    

    for (driveId=0; driveId<2; driveId++) {
        if (tsaHarddiskInfo[driveId].m_fDriveExists && !tsaHarddiskInfo[driveId].m_fAtapi) {
            volatile u8 *pb;
            int n=0, nPos=0;
            
            memset(ba,0x00,512);
            BootIdeReadSector(driveId, ba, 0, 0, 512);
                    
            //See if there is an MBR - no MBR means no native boot options.
            if( !(ba[0x1fe]==0x55) || !(ba[0x1ff]==0xaa)) return;
    
            pb=&ba[0x1be];
            //Check the primary partitions
            for (n=0; n<4; n++,pb+=16) {
                if(pb[0]&0x80) {
                    //Bootable flag IS set on this partition.
                    CONFIGENTRY *entry = (CONFIGENTRY*)LoadConfigNative(driveId, n);
                    if (entry!=NULL) {
                        //There is a valid config file here.
                        //Add an icon for this partition 
                        iconPtr = (ICON *)malloc(sizeof(ICON));
                          iconPtr->iconSlot = ICON_SOURCE_SLOT1;
                        iconPtr->szCaption="  Native";
                        iconPtr->functionPtr = DrawBootMenu;
                        iconPtr->functionDataPtr = (void *)entry;
                          AddIcon(iconPtr);
                        selectedIcon = iconPtr;
                    }
                }
            }
        }
    }
}


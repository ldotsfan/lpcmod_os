/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "ToolsMenuActions.h"
#include "lpcmod_v1.h"
#include "boot.h"
#include "BootIde.h"
#include "video.h"
#include "BootFATX.h"

#define NBTXTPARAMS 29

char * xblastcfgstrings[NBTXTPARAMS + 1] = {
	"activebank = ",
	"altbank = ",
	"quickboot = ",
	"fanspeed = ",
	"boottimeout = ",
	"ledcolor = ",
	"tsopcontrol = ",
	"512kbname = ",
	"256kbname = ",
	"tsop0name = ",
	"tsop1name = ",
	"enablenetwork = ",
	"usedhcp = ",
	"staticip = ",
	"staticgateway = ",
	"staticdns1 = ",
	"staticdns2 = ",
	"enable5v = ",
	"lcdtype = ",
	"nblines = ",
	"linelength = ",
	"backlight = ",
	"contrast = ",
	"displaybootmsg = ",
	"customtextboot = ",
	"displaybiosnameboot = ",
	"customstring0 = ",
	"customstring1 = ",
	"customstring2 = ",
	"customstring3 = "
};



void saveEEPromToFlash(void *whatever){
    u8 i;
    u8 emptyCount = 0;
    for(i = 0; i < 4; i++) {    //Checksum2 is 4 bytes long.
        if(LPCmodSettings.bakeeprom.Checksum2[i] == 0xFF)
            emptyCount++;
    }
        if(emptyCount < 4)            //Make sure checksum2 is not 0xFFFFFFFF.
            if(ConfirmDialog("       Overwrite back up EEProm content?", 1))
                return;
    memcpy(&(LPCmodSettings.bakeeprom),&eeprom,sizeof(EEPROMDATA));
    ToolHeader("Back up to flash successful");
    ToolFooter();
}

void restoreEEPromFromFlash(void *whatever){
    u8 i;
    u8 lockPreference = 0;
    u8 emptyCount = 0;
    char hddString[6];
    for(i = 0; i < 2; i++){
        if (tsaHarddiskInfo[i].m_fDriveExists && !tsaHarddiskInfo[i].m_fAtapi) {
    	    if((tsaHarddiskInfo[i].m_securitySettings &0x0002)==0x0002){        //Drive locked.
    	        lockPreference += (i + 1);             //0=no drive locked, 1=master locked, 2= slave locked
    	    }                                          //3=both drives locked.
        }
    }
    
    for(i = 0; i < 4; i++) {         //Checksum2 is 4 bytes long.
        if(LPCmodSettings.bakeeprom.Checksum2[i] == 0xFF)
            emptyCount++;
    }
    if(emptyCount < 4){            //Make sure checksum2 is not 0xFFFFFFFF.
                        //It is practically impossible to get such value in this checksum field.
        if(ConfirmDialog("       Restore backed up EEProm content?", 1))
            return;
        if((lockPreference&1) == 1){       //Master is locked.
    	    if(!UnlockHDD(0,0)){		     //Silently unlock master.
    	        ToolHeader("ERROR: Could not unlock master HDD");
    	    	goto failed;
    	    }
        }
        if((lockPreference&2) == 2){       //Slave is locked.
            if(!UnlockHDD(1,0)){              //Silently unlock slave.
                ToolHeader("ERROR: Could not unlock slave HDD");
                goto failed;
            }
        }
        memcpy(&eeprom,&(LPCmodSettings.bakeeprom),sizeof(EEPROMDATA));
        ToolHeader("Restored back up to Xbox");
        if((lockPreference&1) == 1){       //Master was initiallylocked.
    	    LockHDD(0,0);		     //Silently lock master.
        }
        if((lockPreference&2) == 2){       //Slave was initially locked.
            LockHDD(1,0);              //Silently lock slave.
        }
    }
    else {
        ToolHeader("ERROR: No back up data on modchip");
    }
failed:
    ToolFooter();
}

void wipeEEPromUserSettings(void *whatever){
    if(ConfirmDialog("        Reset user EEProm settings(safe)?", 1))
        return;
    memset(eeprom.Checksum3,0xFF,4);    //Checksum3 need to be 0xFFFFFFFF
    memset(eeprom.TimeZoneBias,0x00,0x5b);    //Start from Checksum3 address in struct and write 0x00 up to UNKNOWN6.
    ToolHeader("Reset user EEProm settings successful");
    ToolFooter();
}

void showMemTest(void *whatever){
    ToolHeader("128MB  RAM test");
    memtest();
    ToolFooter();
}

void memtest(void){
    u8 bank = 0;
    char Bank1Text[20];
    char Bank2Text[20];
    char Bank3Text[20];
    char Bank4Text[20];
    char *BankText[4] = {Bank1Text, Bank2Text, Bank3Text, Bank4Text};
/*
    strcpy(Bank1Text,"Untested");
    strcpy(Bank2Text,"Untested");
    strcpy(Bank3Text,"Untested");
    strcpy(Bank4Text,"Untested");
*/
    if (xbox_ram == 64){
        //Unknown why this is done but has to be executed
        //It probably has to do with video memory allocation.
        (*(unsigned int*)(0xFD000000 + 0x100200)) = 0x03070103 ;
        (*(unsigned int*)(0xFD000000 + 0x100204)) = 0x11448000 ;

        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  //Force 128 MB
    }
    DisplayProgressBar(0, 4, 0xffff00ff);                      //Draw ProgressBar frame.
    for(bank = 0; bank < 4; bank++)    {
        sprintf(BankText[bank], "%s", testBank(bank)? "Failed" : "Success");
        DisplayProgressBar(bank + 1, 4, 0xffff00ff);                   //Purple progress bar.
    }
    VIDEO_ATTR=0xffc8c8c8;
    printk("\n           Bank1 : %s",Bank1Text);
    printk("\n           Bank2 : %s",Bank2Text);
    printk("\n           Bank3 : %s",Bank3Text);
    printk("\n           Bank4 : %s",Bank4Text);
    if (xbox_ram == 64) {    //Revert to 64MB RAM if previously set.
        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x3FFFFFF);  // 64 MB
    }
    return;
}

void ToolFooter(void) {
    VIDEO_ATTR=0xffc8c8c8;
    printk("\n\n           Press Button 'A' to continue.");
    while ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) != 1)) wait_ms(10);
}

void ToolHeader(char *title) {
    printk("\n\n\n\n\n           ");
    VIDEO_ATTR=0xffffef37;
    printk("\2%s\2\n\n\n\n           ", title);
    VIDEO_ATTR=0xffc8c8c8;
}

int testBank(int bank){
    u32 counter, lastValue;
    u32 *membasetop = (u32*)((64*1024*1024));
    u32 startBad = 0, stopBad = 0;
    u8 result=0;    //Start assuming everything is good.

    lastValue = 1;
    //Clear Upper 64MB
    for (counter= 0; counter < (64*1024*1024/(4*4));counter++) {
        membasetop[counter*4+bank] = lastValue;                         //Set it all to 0x1
    }

    while(lastValue < 0x80000000){                                      //Test every data bit pins.
        for (counter= 0; counter< (64*1024*1024/(4*4));counter++) {     //Test every address bit pin
            if(membasetop[counter*4+bank]!=lastValue){
                result = 1;    //1=no no
                if(startBad == 0){
                    startBad = counter*4+bank;
                    printk("\n           StartBad = 0x%08X , ",startBad);
                }
                //lastValue = 0x80000000;
                //return result;        //No need to go further. Bank is broken.
            }
            else{
                if(startBad){
                    startBad = 0;
                    stopBad = counter*4+bank - 1;
                    printk("StopBad = 0x%08X , ",stopBad);
                }
            }
            membasetop[counter*4+bank] = lastValue<<1;                  //Prepare for next read.
        }
        lastValue = lastValue << 1;                                     //Next data bit pin.
    }
    return result;
}
/*
void TSOPRecoveryReboot(void *ignored){
    if(ConfirmDialog("       Confirm reboot in TSOP recovery mode?", 1))
        return;
    WriteToIO(XODUS_CONTROL, RELEASED0 | GROUNDA15);
    WriteToIO(XBLAST_CONTROL, BNKOS);   //Make sure A19 signal is not controlled.
    BootFlashSaveOSSettings();
    assertWriteEEPROM();
    I2CTransmitWord(0x10, 0x1b00 + ( I2CTransmitByteGetReturn(0x10, 0x1b) | 0x04 )); // set noani-bit
    I2CRebootQuick();        //Retry
    while(1);
}
*/
void saveXBlastcfg(void * ignored){
    FATXFILEINFO fileinfo;
    FATXPartition *partition;
    int res = false;
    char * filebuf;
    char tempString[4];
    u32 cursorpos;

    partition = OpenFATXPartition(0, SECTOR_SYSTEM, SYSTEM_SIZE);

    if(partition != NULL){
        res = FATXFindFile(partition, "xblast.cfg", FATX_ROOT_FAT_CLUSTER, &fileinfo);
        if(res){                //File already exist
            if(ConfirmDialog("                  Overwrite C:\\xblast.cfg?", 1)){
                CloseFATXPartition(partition);
                ToolHeader("Saving to C:\\xblast.cfg aborted.");
                cromwellWarning();
                ToolFooter();
                initialSetLED(LPCmodSettings.OSsettings.LEDColor);
                return;
            }
            filebuf = (char *)malloc((u8)FATX16CLUSTERSIZE);
            memset(filebuf, 0x00, FATX16CLUSTERSIZE);
            cursorpos = 0;
            strcpy(&filebuf[cursorpos], xblastcfgstrings[0]);
            cursorpos += strlen(xblastcfgstrings[0]);
            //itoa(LPCmodSettings.OSsettings.activeBank, tempString, 10);
            cursorpos += strlen(tempString);
            strcpy(&filebuf[cursorpos],tempString);
            filebuf[cursorpos] = 0x0A;
            cursorpos += 1;
        }
        ToolHeader("Saved settings to C:\\xblast.cfg");

        CloseFATXPartition(partition);
    }
    else{
        ToolHeader("Error opening partition. Drive formatted?");
    }

    ToolFooter();
}

void loadXBlastcfg(void * ignored){
    FATXFILEINFO fileinfo;
    FATXPartition *partition;
    int res = false;

    partition = OpenFATXPartition(0, SECTOR_SYSTEM, SYSTEM_SIZE);
    if(partition != NULL){
        res = FATXFindFile(partition, "xblast.cfg", FATX_ROOT_FAT_CLUSTER, &fileinfo);
        if(res){
             if(ConfirmDialog("           Restore settings from C:\\xblast.cfg?", 1)){
                CloseFATXPartition(partition);
                ToolHeader("Loading from C:\\xblast.cfg aborted.");
                cromwellWarning();
                ToolFooter();
                initialSetLED(LPCmodSettings.OSsettings.LEDColor);
                return;
            }
            ToolHeader("Loaded settings from C:\\xblast.cfg");
        }
        else{
            ToolHeader("Error. File \"xblast.cfg\" not found.");
        }

        CloseFATXPartition(partition);
    }
    else{
        ToolHeader("Error opening partition. Drive formatted?");
    }

    ToolFooter();
}

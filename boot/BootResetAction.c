/*
 * Sequences the necessary post-reset actions from as soon as we are able to run C
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
 */

#include "boot.h"
#include "BootEEPROM.h"
#include "BootFlash.h"
#include "BootFATX.h"
#include "lib/LPCMod/BootLPCMod.h"
#include "xboxkrnl.h"
#include "xbox.h"
#include "cpu.h"
#include "config.h"
#include "video.h"
#include "memory_layout.h"
#include "lpcmod_v1.h"

//#include "lib/LPCMod/BootLCD.h"

JPEG jpegBackdrop;

int nTempCursorMbrX, nTempCursorMbrY;

extern volatile int nInteruptable;

volatile CURRENT_VIDEO_MODE_DETAILS vmode;
//extern KNOWN_FLASH_TYPE aknownflashtypesDefault[];

static int ReadFile(HANDLE Handle, PVOID Buffer, ULONG Size);
int WriteFile(HANDLE Handle, PVOID Buffer, ULONG Size);
int SaveFile(char *szFileName,PBYTE Buffer,ULONG Size);
void DismountFileSystems(void);
int RemapDrive(char *szDrive);
HANDLE OpenFile(HANDLE Root, LPCSTR Filename, LONG Length, ULONG Mode);
BOOL GetFileSize(HANDLE File, LONGLONG *Size);

int ReadFile(HANDLE Handle, PVOID Buffer, ULONG Size)
{
        IO_STATUS_BLOCK IoStatus;

        // Try to write the buffer
        if (!NT_SUCCESS(NtReadFile(Handle, NULL, NULL, NULL, &IoStatus,
                Buffer, Size, NULL)))
                return 0;

        // Verify that the amount read is the correct size
        if (IoStatus.Information != Size)
                return 0;

        return 1;
}
// Opens a file or directory for read-only access
// Length parameter is negative means use strlen()
// This was originally designed to open directories, but it turned out to be
// too much of a hassle and was scrapped.  Use only for files with the
// FILE_NON_DIRECTORY_FILE mode.
HANDLE OpenFile(HANDLE Root, LPCSTR Filename, LONG Length, ULONG Mode)
{
        ANSI_STRING FilenameString;
        OBJECT_ATTRIBUTES Attributes;
        IO_STATUS_BLOCK IoStatus;
        HANDLE Handle;

        // Initialize the filename string
        // If a length is specified, set up the string manually
        if (Length >= 0)
        {
                FilenameString.Length = (USHORT) Length;
                FilenameString.MaximumLength = (USHORT) Length;
                FilenameString.Buffer = (PSTR) Filename;
        }
        // Use RtlInitAnsiString to do it for us
        else
                RtlInitAnsiString(&FilenameString, Filename);

        // Initialize the object attributes
        Attributes.Attributes = OBJ_CASE_INSENSITIVE;
        Attributes.RootDirectory = Root;
        Attributes.ObjectName = &FilenameString;

        // Try to open the file or directory
        if (!NT_SUCCESS(NtCreateFile(&Handle, GENERIC_READ | SYNCHRONIZE,
                &Attributes, &IoStatus, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE
                | FILE_SHARE_DELETE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT |
                Mode)))
                return NULL;

        return Handle;
}


NTSTATUS GetConfig(CONFIGENTRY *entry);

void ClearScreen (void) {
    BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
}

BOOL GetFileSize(HANDLE File, LONGLONG *Size)
{
        FILE_NETWORK_OPEN_INFORMATION SizeInformation;
        IO_STATUS_BLOCK IoStatus;

        // Try to retrieve the file size
        if (!NT_SUCCESS(NtQueryInformationFile(File, &IoStatus,
                &SizeInformation, sizeof(SizeInformation),
                FileNetworkOpenInformation)))
                return FALSE;

        *Size = SizeInformation.EndOfFile.QuadPart;
        return TRUE;
}
int WriteFile(HANDLE Handle, PVOID Buffer, ULONG Size)
{
        IO_STATUS_BLOCK IoStatus;

        // Try to write the buffer
        if (!NT_SUCCESS(NtWriteFile(Handle, NULL, NULL, NULL, &IoStatus,
                Buffer, Size, NULL)))
                return 0;

        // Verify that the amount written is the correct size
        if (IoStatus.Information != Size)
                return 0;

        return 1;
}

int SaveFile(char *szFileName,PBYTE Buffer,ULONG Size) {

	ANSI_STRING DestFileName;
        IO_STATUS_BLOCK IoStatus;
        OBJECT_ATTRIBUTES Attributes;
        HANDLE DestHandle = NULL;

	RtlInitAnsiString(&DestFileName,szFileName);
        Attributes.RootDirectory = NULL;
        Attributes.ObjectName = &DestFileName;
        Attributes.Attributes = OBJ_CASE_INSENSITIVE;

	if (!NT_SUCCESS(NtCreateFile(&DestHandle,
		GENERIC_WRITE  | GENERIC_READ | SYNCHRONIZE,
		&Attributes, &IoStatus,
		NULL, FILE_RANDOM_ACCESS,
		FILE_SHARE_READ, FILE_CREATE,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE))) {
			dprintf("Error saving File\n");
			return 0;
	}

	if(!WriteFile(DestHandle, Buffer, Size)) {
		dprintf("Error saving File\n");
		return 0;
	}

	NtClose(DestHandle);

	return 1;
}

// Dismount all file systems
void DismountFileSystems(void) {

        ANSI_STRING String;

        RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition1");
        IoDismountVolumeByName(&String);
        RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition2");
        IoDismountVolumeByName(&String);
        RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition3");
        IoDismountVolumeByName(&String);
        RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition4");
        IoDismountVolumeByName(&String);
        RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition5");
        IoDismountVolumeByName(&String);
        RtlInitAnsiString(&String, "\\Device\\Harddisk0\\Partition6");
        IoDismountVolumeByName(&String);
}
long LoadFile(PVOID Filename, long *lFileSize) {

	HANDLE hFile;
	PBYTE Buffer = 0;
	ULONGLONG FileSize;

        if (!(hFile = OpenFile(NULL, Filename, -1, FILE_NON_DIRECTORY_FILE))) {
		dprintf("Error open file %s\n",Filename);
        }

	if(!GetFileSize(hFile,&FileSize)) {
		dprintf("Error getting file size %s\n",Filename);
	}

	Buffer = MmAllocateContiguousMemoryEx(FileSize,
			MIN_KERNEL, MAX_KERNEL, 0, PAGE_READWRITE);
	if (!Buffer) {
		dprintf("Error alloc memory for File %s\n",Filename);
	}

	memset(Buffer,0xff,FileSize);
	if (!ReadFile(hFile, Buffer, FileSize)) {
		dprintf("Error loading file %s\n",Filename);
	}

	NtClose(hFile);

	*lFileSize = FileSize;

	return (long)Buffer;
}
//////////////////////////////////////////////////////////////////////
//
//  BootResetAction()

extern void BootResetAction ( void ) {
    bool fMbrPresent=false;
    bool fSeenActive=false;
    int nFATXPresent=false;
    bool fFirstBoot=false;                    //Flag to indicate first boot since flash update
    int nTempCursorX, nTempCursorY;
    int n, nx, i;
    char *modName = "Unsupported modchip!";
    OBJECT_FLASH of;
    // A bit hacky, but easier to maintain.
    const KNOWN_FLASH_TYPE aknownflashtypesDefault[] = {
        #include "flashtypes.h"
    };
    u8 EjectButtonPressed=0;


    //Length of array is set depending on how many revision can be uniquely identified.
    //Modify this enum if you modify the "XBOX_REVISION" enum in boot.h
    char *xbox_mb_rev[8] = {
        "DevKit",
        "DebugKit",
        "1.0",
        "1.1",
        "1.2/1.3",
        "1.4/1.5",
        "1.6/1.6b",
        "Unknown"
    };
    
    of.m_pbMemoryMappedStartAddress=(u8 *)LPCFlashadress;

    xF70ELPCRegister = 0x03;       //Assume no control over the banks but we are booting from bank3
    x00FFLPCRegister = ReadFromIO(XODUS_CONTROL);       //Read A15 and D0 states.
                                                        //Should return 0x04 on normal boot, 0x08 on TSOP recovery.
    xF70FLPCRegister = 0x00;       //Assume no output pins activated yet.

    TSOPRecoveryMode = 0;
    //TSOPRecoveryMode = (x00FFLPCRegister & 0x08) >> 3;  //If we booted and A15 was already set.
                                                        //It means we are in TSOP recovery. Set to 1.
                                                        //We'll check later if TSOP flash is accessible.


    fHasHardware = 0;

    memcpy(&cromwell_config,(void*)(0x03A00000+0x20),4);
    memcpy(&cromwell_retryload,(void*)(0x03A00000+0x24),4);
    memcpy(&cromwell_loadbank,(void*)(0x03A00000+0x28),4);
    memcpy(&cromwell_Biostype,(void*)(0x03A00000+0x2C),4);

    VIDEO_CURSOR_POSX=40;
    VIDEO_CURSOR_POSY=140;
        
    VIDEO_AV_MODE = 0xff;
    nInteruptable = 0;

    // prep our BIOS console print state
    VIDEO_ATTR=0xffffffff;

    // init malloc() and free() structures
    MemoryManagementInitialization((void *)MEMORYMANAGERSTART, MEMORYMANAGERSIZE);

    BootInterruptsWriteIdt();

    // initialize the PCI devices
    //bprintf("BOOT: starting PCI init\n\r");
    BootPciPeripheralInitialization();
    // Reset the AGP bus and start with good condition
    BootAGPBUSInitialization();
    EjectButtonPressed = I2CTransmitByteGetReturn(0x10, 0x03) & 0x01;
    I2CTransmitByteGetReturn(0x10, 0x11);       // dummy Query IRQ
    I2CWriteBytetoRegister(0x10, 0x03,0x00);	// Clear Tray Register
    I2CTransmitWord(0x10, 0x0c01); // close DVD tray
    
    
    I2CTransmitWord(0x10, 0x1901); // no reset on eject

    LEDRed();        //Signal the user to press Eject button to avoid Quickboot.
//    if(cromwell_config==CROMWELL){              //Only check if booted from ROM.
    fHasHardware = LPCMod_HW_rev();         //Will output 0xff if no supported modchip detected.
//    }
    u32 x3probe = I2CTransmitByteGetReturn(0x51, 0x0);  //Xecuter 3 will send out 0xff
    if(x3probe != 0xff && x3probe != 0x80000002)        //Another (hacky) way to detect is to probe SMBus at addresses
        fHasHardware = SYSCON_ID_X3;                    //normally unused by the Xbox. By my own experimentation, address
                                                        //0x51 isn't used when X3 is NOT plugged. Then probing the SMBus
                                                        //offset 0 of address 0x51 will return either 0xff or 0x80000002.
                                                        //Any other value will be assumed coming from the (encrypted?)
                                                        //X3 eeprom and thus instructing the program that a X3 is detected.
                                                        //More tests will be needed to verify and confirm this theory.
                                                        //Tests have been done on NTSC-U 1.0 and 1.6(a) Xboxes so far.


    if(fHasHardware == SYSCON_ID_V1){
        sprintf(modName,"%s", "XBlast Lite V1");
        if(fHasHardware == SYSCON_ID_V1){
            //Check which flash chip is detected by system.
            BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0]);
            if(of.m_bManufacturerId == 0xbf && of.m_bDeviceId == 0x5b){     //If we detected a SST49LF080A
                //Make sure we'll be reading from OS Bank
                switchBank(BNKOS);
             }
            else {  //SST49LF080A flash chip was NOT detected.
                fHasHardware = SYSCON_ID_V1_TSOP;
                WriteToIO(XODUS_CONTROL, RELEASED0); //Make sure D0/A15 is not grounded.
            }
        }

    }
    else {
        if(fHasHardware == SYSCON_ID_XX1 || fHasHardware == SYSCON_ID_XX2)
            sprintf(modName,"%s", "SmartXX V1/V2");
        else if(fHasHardware == SYSCON_ID_XXOPX)
            sprintf(modName,"%s", "SmartXX LT OPX");
        else if(fHasHardware == SYSCON_ID_XX3)
            sprintf(modName,"%s", "SmartXX V3");
        else if(fHasHardware == SYSCON_ID_X3)
            sprintf(modName,"%s", "Xecuter 3(CE)");
        else
            fHasHardware = 0;               //Unknown device, set to 0 to indicate no known hardware.

        currentFlashBank = BNKOS;           //Make sure the system knows we're on the right bank.
        TSOPRecoveryMode = 0;               //Whatever happens, it's not possible to recover TSOP on other modchips.
    }

    //Retrieve XBlast OS settings from flash. Function checks if valid device can be read from.
    BootFlashGetOSSettings(&LPCmodSettings);


    if(LPCmodSettings.OSsettings.migrateSetttings > 1 ||
       LPCmodSettings.OSsettings.activeBank == 0xFF ||
       LPCmodSettings.OSsettings.altBank == 0xFF ||
       LPCmodSettings.OSsettings.Quickboot > 1 ||
       LPCmodSettings.OSsettings.selectedMenuItem == 0xFF ||
       LPCmodSettings.OSsettings.fanSpeed > 100 ||
       LPCmodSettings.OSsettings.bootTimeout == 0xFF ||
       LPCmodSettings.OSsettings.LEDColor == 0xFF ||
       LPCmodSettings.OSsettings.TSOPcontrol == 0xFF ||
       LPCmodSettings.OSsettings.enableNetwork == 0xFF ||
       LPCmodSettings.OSsettings.useDHCP == 0xFF ||
       LPCmodSettings.LCDsettings.migrateLCD == 0xFF ||
       LPCmodSettings.LCDsettings.enable5V > 1 ||
       LPCmodSettings.LCDsettings.lcdType == 0xFF ||
       LPCmodSettings.LCDsettings.nbLines == 0xFF ||
       LPCmodSettings.LCDsettings.lineLength == 0xFF ||
       LPCmodSettings.LCDsettings.backlight > 100 ||
       LPCmodSettings.LCDsettings.contrast > 100 ||
       LPCmodSettings.LCDsettings.displayMsgBoot > 1 ||
       LPCmodSettings.LCDsettings.customTextBoot > 1 ||
       LPCmodSettings.LCDsettings.displayBIOSNameBoot > 1){
            fFirstBoot = true;
            initialLPCModOSBoot(&LPCmodSettings);                //No settings for LPCMod were present in flash.
            //OS sometimes lock on after a fresh flash. Disabling to see if that's causing it.(probably)
            //BootFlashSaveOSSettings();        //Put some initial values in there.
            LEDFirstBoot(NULL);
            LPCmodSettings.OSsettings.bootTimeout = 0;        //No countdown since it's the first boot since a flash update.
                                                            //Configure your device first.
    }
    if(cromwell_config==XROMWELL && fHasHardware != SYSCON_ID_V1)	//If coming from XBE and no XBlast Mod is detected
    	LPCmodSettings.OSsettings.fanSpeed = I2CGetFanSpeed();		//Get previously set fan speed
    else
    	I2CSetFanSpeed(LPCmodSettings.OSsettings.fanSpeed);		//Else we're booting in ROM mode and have a fan speed to set.

    if(fHasHardware == SYSCON_ID_V1_TSOP){
    	LPCmodSettings.OSsettings.TSOPcontrol = (ReadFromIO(XODUS_CONTROL) & 0x10);
    }

    BootLCDInit();                              //Basic init. Do it even if no LCD is connected on the system.
    
    //Stuff to do right after loading persistent settings from flash.
    if(!fFirstBoot){
        if(fHasHardware == SYSCON_ID_V1 ||
           fHasHardware == SYSCON_ID_V1_TSOP ||
           fHasHardware == SYSCON_ID_XX1 ||
           fHasHardware == SYSCON_ID_XX2 ||
           fHasHardware == SYSCON_ID_XXOPX ||
           fHasHardware == SYSCON_ID_XX3 ||
           fHasHardware == SYSCON_ID_X3){
            assertInitLCD();                            //Function in charge of checking if a init of LCD is needed.
        }
        //further init here.
    }



    // We disable The CPU Cache
    cache_disable();
    // We Update the Microcode of the CPU
    display_cpuid_update_microcode();
    // We Enable The CPU Cache
    cache_enable();
    //setup_ioapic();
    // We look how much memory we have ..
    BootDetectMemorySize();

    BootEepromReadEntireEEPROM();
        
    I2CTransmitWord(0x10, 0x1a01); // unknown, done immediately after reading out eeprom data
    I2CTransmitWord(0x10, 0x1b04); // unknown
        
    /* Here, the interrupts are Switched on now */
    BootPciInterruptEnable();
    /* We allow interrupts */
    nInteruptable = 1;
    

#ifndef SILENT_MODE
    printk("           BOOT: start USB init\n");
#endif
    BootStartUSB();
    //Load up some more custom settings right before booting to OS.
    if(!fFirstBoot){
        if(fHasHardware == SYSCON_ID_V1 && cromwell_config==CROMWELL){       //Quickboot only if on the right hardware.
            if(EjectButtonPressed){              //Xbox was started from eject button.
                if(LPCmodSettings.OSsettings.altBank > BOOTFROMTSOP){
                    switchBank(LPCmodSettings.OSsettings.altBank);
              	}
                else{
                    WriteToIO(XODUS_CONTROL, RELEASED0);    //Release D0
                    if(mbVersion == REV1_6 || mbVersion == REVUNKNOWN)
                        WriteToIO(XODUS_CONTROL, KILL_MOD);    // switch to original bios. Mute modchip.
                    else{
                        WriteToIO(XBLAST_CONTROL, LPCmodSettings.OSsettings.altBank | OSBNKCTRLBIT);    // switch to original bios but modchip listen to LPC commands.
                                                                                                        // Lock flash bank control with OSBNKCTRLBIT.
                    }
                }
                I2CTransmitWord(0x10, 0x1b00 + ( I2CTransmitByteGetReturn(0x10, 0x1b) & 0xfb )); // clear noani-bit
                BootStopUSB();
                I2CRebootQuick();
                while(1);	//Hang there.
            }
            wait_ms(100);
            EjectButtonPressed = I2CTransmitByteGetReturn(0x10, 0x03) & 0x01;
            I2CTransmitByteGetReturn(0x10, 0x11);       // dummy Query IRQ
            I2CWriteBytetoRegister(0x10, 0x03,0x00);	// Clear Tray Register
            I2CTransmitWord(0x10, 0x0c01); // close DVD tray
            if(!EjectButtonPressed && LPCmodSettings.OSsettings.Quickboot == 1){       //White button NOT pressed and Quickboot ON.
                if(LPCmodSettings.OSsettings.activeBank > BOOTFROMTSOP){
                    switchBank(LPCmodSettings.OSsettings.activeBank);
              	}
                else{
                    WriteToIO(XODUS_CONTROL, RELEASED0);    //Release D0
                    if(mbVersion == REV1_6 || mbVersion == REVUNKNOWN)
                        WriteToIO(XODUS_CONTROL, KILL_MOD);    // switch to original bios. Mute modchip.
                    else{
                        WriteToIO(XBLAST_CONTROL, LPCmodSettings.OSsettings.activeBank | OSBNKCTRLBIT);    // switch to original bios but modchip listen to LPC commands.
                                                                                                           // Lock flash bank control with OSBNKCTRLBIT.
                    }
                }
                I2CTransmitWord(0x10, 0x1b00 + ( I2CTransmitByteGetReturn(0x10, 0x1b) & 0xfb )); // clear noani-bit
                BootStopUSB();
                I2CRebootQuick();
                while(1);
            }
        }
        initialSetLED(LPCmodSettings.OSsettings.LEDColor);
    }
    // Load and Init the Background image
    // clear the Video Ram
    memset((void *)FB_START,0x00,0x400000);

    BootVgaInitializationKernelNG((CURRENT_VIDEO_MODE_DETAILS *)&vmode);

    { // decode and malloc backdrop bitmap
        extern int _start_backdrop;
        BootVideoJpegUnpackAsRgb(
            (u8 *)&_start_backdrop,
            &jpegBackdrop
        );
    }
    // paint the backdrop
#ifndef DEBUG_MODE
    BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
#endif

    /* Here, the interrupts are Switched on now */
//    BootPciInterruptEnable();
        /* We allow interrupts */
//    nInteruptable = 1;

    //I2CTransmitWord(0x10, 0x1901); // no reset on eject
         
    VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
    VIDEO_CURSOR_POSY=vmode.ymargin;


//Do not activate this for now. Will probably never work...
/*
    //Now that we have something to display.
    if(fHasHardware == SYSCON_ID_V1){
    //Check which flash chip is detected by system.
        BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0]);
        if(of.m_bManufacturerId == 0xbf && of.m_bDeviceId == 0x5b){     //If we detected a SST49LF080A
            if(TSOPRecoveryMode){        //We wanted to reboot in TSOP recovery but it failed...
                TSOPRecoveryReboot(NULL);       //retry
                TSOPRecoveryMode = 0;           //We'll come back here if user do not want to retry.
                WriteToIO(XODUS_CONTROL, 0x00); //Make sure A15 is not grounded
            }
        }
        else {  //SST49LF080A flash chip was NOT detected.
            fHasHardware = SYSCON_ID_V1_TSOP;
            WriteToIO(XODUS_CONTROL, 0x00); //Make sure A15 is not grounded.
        }
    }
*/

    printk("\n\n");
    if (cromwell_config==XROMWELL) {
        printk("           \2XBlast OS (XBE) v" VERSION "\n\n\2");
    } else if (cromwell_config==CROMWELL) {
        printk("           \2XBlast OS (ROM) v" VERSION "\n\n\2");
    }

    VIDEO_ATTR=0xff00ff00;

    VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
    VIDEO_CURSOR_POSY=vmode.ymargin+64;


    VIDEO_ATTR=0xff00ff00;
    //TODO: Remove debug string print.
    printk("           Modchip: %s    DEBUG_fHasHardware: 0x%02x\n",modName, fHasHardware);
    VIDEO_ATTR=0xffc8c8c8;
    printk("           THIS IS A WIP BUILD, manID= %x  devID= %x\n", of.m_bManufacturerId, of.m_bDeviceId);

    VIDEO_ATTR=0xff00ff00;
    

   mbVersion = I2CGetXboxMBRev();
   printk("           Xbox revision: %s ", xbox_mb_rev[mbVersion]);
   if (xbox_ram > 64) {
        VIDEO_ATTR=0xff00ff00;
   } else {
        VIDEO_ATTR=0xffffa20f;
   }
   printk("  RAM: %dMiB\n", xbox_ram);
   
    VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
#ifndef SILENT_MODE
    // capture title area
    VIDEO_ATTR=0xffc8c8c8;
    printk("           Encoder: ");
    VIDEO_ATTR=0xffc8c800;
    printk("%s  ", VideoEncoderName());
    VIDEO_ATTR=0xffc8c8c8;
    printk("Cable: ");
    VIDEO_ATTR=0xffc8c800;
    printk("%s  ", AvCableName());
        
    if (I2CGetTemperature(&n, &nx)) {
        VIDEO_ATTR=0xffc8c8c8;
        printk("CPU Temp: ");
        VIDEO_ATTR=0xffc8c800;
        printk("%doC  ", n);
        VIDEO_ATTR=0xffc8c8c8;
        printk("M/b Temp: ");
        VIDEO_ATTR=0xffc8c800;
        printk("%doC  ", nx);
    }

    printk("\n");
    nTempCursorX=VIDEO_CURSOR_POSX;
    nTempCursorY=VIDEO_CURSOR_POSY;
#endif

    VIDEO_ATTR=0xffffffff;

    //busyLED();

    // set Ethernet MAC address from EEPROM
    {
        volatile u8 * pb=(u8 *)0xfef000a8;  // Ethernet MMIO base + MAC register offset (<--thanks to Anders Gustafsson)
        int n;
        for(n=5;n>=0;n--) { *pb++=    eeprom.MACAddress[n]; } // send it in backwards, its reversed by the driver
    }
#ifndef SILENT_MODE
    BootEepromPrintInfo();
#endif

    // init the IDE devices
#ifndef SILENT_MODE
    VIDEO_ATTR=0xffc8c8c8;
    printk("           Initializing IDE Controller\n");
#endif
    BootIdeWaitNotBusy(0x1f0);
    wait_ms(100);
#ifndef SILENT_MODE
    printk("           Ready\n");
#endif
    // reuse BIOS status area

#ifndef DEBUG_MODE
//    BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
#endif
    VIDEO_CURSOR_POSX=nTempCursorX;
    VIDEO_CURSOR_POSY=nTempCursorY;
    VIDEO_CURSOR_POSX=0;
    VIDEO_CURSOR_POSY=0;

    BootIdeInit();

    printk("\n\n\n\n");

    nTempCursorMbrX=VIDEO_CURSOR_POSX;
    nTempCursorMbrY=VIDEO_CURSOR_POSY;


    //Debug routine to (hopefully) identify the i2c eeprom on a Xecuter 3.
//    u8 *videosavepage = malloc(FB_SIZE);
//    memcpy(videosavepage,(void*)FB_START,FB_SIZE);
//    BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
//    printk("\n\n\n\n");
//    for(i = 0x50; i < 0x54; i++){               //Hopefully they didn't use an obscure eeprom chip
//                                                //and it will respond to top nibble 0b0101. If not we'll bruteforce it.
//            printk("\n                addr:%02x     data:%02x", i, I2CTransmitByteGetReturn(i, 0x0));
//            printk("\n                addr:%02x     data:%02x", i, I2CTransmitByteGetReturn(i, 0x1));
//
//    }
//    while ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) != 1)) wait_ms(10);
//    memcpy((void*)FB_START,videosavepage,FB_SIZE);
//    free(videosavepage);
    //Remove after success
    

    //Check for unformatted drives.
    for (i=0; i<2; ++i) {
        if (tsaHarddiskInfo[i].m_fDriveExists && !tsaHarddiskInfo[i].m_fAtapi
            && tsaHarddiskInfo[i].m_dwCountSectorsTotal >= (SECTOR_EXTEND - 1)) {
            if(tsaHarddiskInfo[i].m_enumDriveType != EDT_XBOXFS){
                // We save the complete framebuffer to memory (we restore at exit)
                u8 *videosavepage = malloc(FB_SIZE);
                memcpy(videosavepage,(void*)FB_START,FB_SIZE);
                char ConfirmDialogString[50];
                sprintf(ConfirmDialogString, "               Format new drive (%s)?\0", i ? "slave":"master");
                if(!ConfirmDialog(ConfirmDialogString, 1)){
                    FATXFormatDriveC(i, 0);                     //'0' is for non verbose
                    FATXFormatDriveE(i, 0);
                    FATXFormatCacheDrives(i, 0);
                    FATXSetBRFR(i);
                    //If there's enough sectors to make F and/or G drive(s).
                    if(tsaHarddiskInfo[i].m_dwCountSectorsTotal >= (SECTOR_EXTEND + SECTORS_SYSTEM)){
                        DrawLargeHDDTextMenu(i);//Launch LargeHDDMenuInit textmenu.
                    }
                    if(tsaHarddiskInfo[i].m_fHasMbr == 0)       //No MBR
                        FATXSetInitMBR(i);                      //Since I'm such a nice program, I will integrate the partition table to the MBR.
                }
                memcpy((void*)FB_START,videosavepage,FB_SIZE);
                free(videosavepage);
            }
        }
    }
    
    
//    printk("i2C=%d SMC=%d, IDE=%d, tick=%d una=%d unb=%d\n", nCountI2cinterrupts, nCountInterruptsSmc, nCountInterruptsIde, BIOS_TICK_COUNT, nCountUnusedInterrupts, nCountUnusedInterruptsPic2);
    IconMenuInit();
    //inputLED();
    IconMenu();

    //Should never come back here.
    while(1);
}

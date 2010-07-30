#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>
#include <ogc/machine/processor.h>

#include "loader_bin.h"
#include "tinyload_dol.h"

//Use WIILOADAPPDEBUG=1 for make input param to test wc24boottitle via wiiload directly. Without that option realmode is enabled for title installation, that doesn't work when attempting to boot via HBC.
//Uncomment these defines to test wc24boottitle via wiiload, without modifying NANDBOOTINFO.
//#define WIILOADTEST_BOOTHB "/apps/wc24app/boot.dol"//Uncomment this to test booting homebrew, by default when enabled this boots /apps/wc24app/boot.dol from SD.
//#define WIILOADTEST_BOOTDISC//Uncomment this to test booting discs.

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
u32 launchcode = 0;

void ProcessArgs(int argc, char **argv)
{
	char *path = (char*)0x900FFF00;
	void (*entry)() = (void*)0x80001800;
	printf("Processing args...\n");
	if(argc)
	{
		switch(launchcode)
		{
			case 1://Boot homebrew
				if(argc<2)break;
				memcpy((void*)0x80001800, loader_bin, loader_bin_size);
				memset(path, 0, 256);
				strncpy(path, argv[1], 255);
				DCFlushRange((void*)0x80001800, loader_bin_size);
				DCFlushRange(path, 256);
				WII_SetNANDBootInfoLaunchcode(0);
				printf("Booting homebrew from: %s\n", path);
				IOS_ReloadIOS(36);
				SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
				entry();
			break;

			case 2://Boot game disc
				memcpy((void*)0x80001800, loader_bin, loader_bin_size);
				memset(path, 0, 256);
				memcpy((void*)0x90100000, tinyload_dol, tinyload_dol_size);
				DCFlushRange((void*)0x80001800, loader_bin_size);
				DCFlushRange(path, 256);
				DCFlushRange((void*)0x90100000, tinyload_dol_size);
				WII_SetNANDBootInfoLaunchcode(0);
				printf("Booting game disc.\n");
				entry();
			break;

			default:
			break;
		}
	}

	printf("Invalid launchcode or argc: %x %x\n", launchcode, argc);
	printf("Shutting down...\n");
	WII_Shutdown();
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();
	
	// This function initialises the attached controllers
	//WPAD_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	if(usb_isgeckoalive(1))CON_EnableGecko(1, 1);

	printf("Getting NANDBOOTINFO argv...\n");
	argv = WII_GetNANDBootInfoArgv(&argc, &launchcode);
	#ifdef WIILOADAPPDEBUG

		#ifdef WIILOADTEST_BOOTDISC	
		argc = 1;
		launchcode = 2;
		#endif

		#ifdef WIILOADTEST_BOOTHB	
		launchcode = 1;
		argc = 2;
		argv[1] = ;
		#endif

	#endif
	ProcessArgs(argc, argv);

	return 0;
}
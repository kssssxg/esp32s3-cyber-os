#include <stdio.h>
#include <esp_heap_caps.h>

#include "m_argv.h"

#include "doomgeneric.h"

pixel_t* DG_ScreenBuffer = NULL;

void M_FindResponseFile(void);
void D_DoomMain (void);


void doomgeneric_Create(int argc, char **argv)
{
	// save arguments
    myargc = argc;
    myargv = argv;

	M_FindResponseFile();

	// 使用PSRAM分配framebuffer (240x240x4 = 230KB)
	DG_ScreenBuffer = heap_caps_malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4, MALLOC_CAP_SPIRAM);
	if (!DG_ScreenBuffer) {
		// fallback to internal RAM
		DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
	}

	DG_Init();

	D_DoomMain ();
}


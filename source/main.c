#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {
    // Initialize the video system
    VIDEO_Init();
    
    // Get the preferred video mode
    rmode = VIDEO_GetPreferredMode(NULL);
    
    // Allocate memory for the display in the uncached region
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    
    // Clear the garbage around the edges of the framebuffer
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
    
    // Initialize the controller system
    PAD_Init();
    
    printf("\\n\\nHello World!\\n");
    printf("Press START to exit.\\n");
    
    while(1) {
        // Scan for controller input
        PAD_ScanPads();
        
        // If START button is pressed, exit
        int buttonsDown = PAD_ButtonsDown(0);
        if(buttonsDown & PAD_BUTTON_START) {
            break;
        }
        
        VIDEO_WaitVSync();
    }
    
    return 0;
}

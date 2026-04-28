/* VIDDRV.C
  This file contains low-level graphics functions which are
  platform-specific.
*/
#include "../keen.h"
#include <SDL_getenv.h>
#include "viddrv.fdh"

#ifdef __webos__
#include "PDL.h"
#endif

void lprintf(const char *str, ...);

int border_width=0, border_height=0;

int window_width;
int window_height;
SDL_Surface *screen = NULL;              // the actual video memory/window
SDL_Surface *ScrollSurface = NULL;       // scroll buffer
// temporary buffer if using scale2x, or equal to "screen" if not
SDL_Surface *BlitSurface = NULL;

#ifdef __webos__
// webOS: off-screen surface at native 320x240; stretched to 1024x768 on flip
static SDL_Surface *webos_game_surface = NULL;
#define WEBOS_SCREEN_W  1024
#define WEBOS_SCREEN_H  768
// Touch zone boundaries (must match keydrv.c)
#define WEBOS_JUMP_Y    192
#define WEBOS_FIRE_Y    575
#define WEBOS_LEFT_X    300
#define WEBOS_RIGHT_X   723

static void webos_draw_control_overlay(void)
{
    SDL_Rect r;
    Uint8 white = 15;  // EGA bright white
    int lw = 3;

    // Horizontal dividers
    r.x = 0; r.y = WEBOS_JUMP_Y; r.w = WEBOS_SCREEN_W; r.h = lw;
    SDL_FillRect(screen, &r, white);
    r.x = 0; r.y = WEBOS_FIRE_Y; r.w = WEBOS_SCREEN_W; r.h = lw;
    SDL_FillRect(screen, &r, white);

    // Vertical dividers in mid zone: LEFT | MENU | RIGHT
    r.x = WEBOS_LEFT_X;  r.y = WEBOS_JUMP_Y; r.w = lw; r.h = WEBOS_FIRE_Y - WEBOS_JUMP_Y;
    SDL_FillRect(screen, &r, white);
    r.x = WEBOS_RIGHT_X; r.y = WEBOS_JUMP_Y; r.w = lw; r.h = WEBOS_FIRE_Y - WEBOS_JUMP_Y;
    SDL_FillRect(screen, &r, white);

    // Vertical midpoint dividers in top and bottom zones: left=directional, right=action
    r.x = WEBOS_SCREEN_W/2; r.y = 0;             r.w = lw; r.h = WEBOS_JUMP_Y;
    SDL_FillRect(screen, &r, white);
    r.x = WEBOS_SCREEN_W/2; r.y = WEBOS_FIRE_Y;  r.w = lw; r.h = WEBOS_SCREEN_H - WEBOS_FIRE_Y;
    SDL_FillRect(screen, &r, white);
}
#endif

// uncomment this line if you want to render the contents of
// the entire scrollbuffer instead of a normal "wrap" render.
// (it's a debug feature)
//#define SHOW_SCROLLBUFFER

// pointer to the line in VRAM to start blitting to when stretchblitting.
// this may not be the first line on the display as it is adjusted to
// center the image on the screen when in fullscreen.
unsigned char *VRAMPtr;


char static allocmem(void)
{
  lprintf("allocmem(): allocating %d bytes for scroll buffer...", SCROLLBUF_MEMSIZE);
  scrollbuf = malloc(SCROLLBUF_MEMSIZE);
  if (!scrollbuf)
  {
     lprintf("Failure\n");
     return 1;
  } else lprintf("OK\n");

  if (options[OPT_ZOOM])
  {
     lprintf("allocmem(): allocating %d bytes for blit buffer...", BLITBUF_MEMSIZE);
     blitbuf = malloc(BLITBUF_MEMSIZE);
     if (!blitbuf)
     {
        lprintf("Failure\n");
        return 1;
     } else lprintf("OK\n");
  }

  return 0;
}

void static freemem(void)
{
	if (scrollbuf)
	{
		free(scrollbuf);
		lprintf("  * Scrollbuffer memory released to system.\n");
	}
	if (blitbuf)
	{
		free(blitbuf);
		lprintf("  * Blitbuffer memory released to system.\n");
	}
}


SDL_Rect dstrect;

void sb_blit(void)
{
SDL_Rect srcrect;
char wraphoz, wrapvrt;
int save_dstx, save_dstw, save_srcx, save_srcw;
int blit_w, blit_h, blit_sw, blit_sh;

   #ifdef SHOW_SCROLLBUFFER
		srcrect.x = 0; srcrect.y = 0;
		srcrect.w = SCROLLBUF_XSIZE;
		srcrect.h = SCROLLBUF_YSIZE;
		SDL_BlitSurface(ScrollSurface, &srcrect, screen, &srcrect);
		return;
   #endif
   
   dstrect.x = 0; dstrect.y = 0;	//dstrect w & h is ignored (taken from src)

   blit_w = WINDOW_WIDTH;// << 1;
   blit_h = WINDOW_HEIGHT;// << 1;
   blit_sw = SCROLLBUF_XSIZE;// << 1;
   blit_sh = SCROLLBUF_YSIZE;// << 1;
   
   srcrect.x = scrollx_buf;
   srcrect.y = scrolly_buf;
   if (scrollx_buf > (blit_sw-blit_w))
   { // need to wrap right side
     srcrect.w = (blit_sw-scrollx_buf);
     wraphoz = 1;
   }
   else
   { // single blit needed for whole horizontal copy
     srcrect.w = blit_w;
     wraphoz = 0;
   }

   if (scrolly_buf > (blit_sh-blit_h))
   { // need to wrap on bottom
     srcrect.h = (blit_sh-scrolly_buf);
     wrapvrt = 1;
   }
   else
   { // single blit for whole bottom copy
     srcrect.h = blit_h;
     wrapvrt = 0;
   }
   
   SDL_BlitSurface(ScrollSurface, &srcrect, BlitSurface, &dstrect);

   if (wraphoz && wrapvrt)
   {
      // first do same thing we do for wraphoz
      save_dstx = dstrect.x;
      save_dstw = dstrect.w;
      save_srcx = srcrect.x;
      save_srcw = srcrect.w;
      dstrect.x = srcrect.w;
      dstrect.w = blit_w - dstrect.x;
      srcrect.x = 0;
      srcrect.w = (blit_w - srcrect.w);
      SDL_BlitSurface(ScrollSurface, &srcrect, BlitSurface, &dstrect);
      // now repeat for the bottom
      // (lower-right square)
      dstrect.y = srcrect.h;
      dstrect.h = blit_h - dstrect.y;
      srcrect.y = 0;
      srcrect.h = (blit_h - srcrect.h);
      SDL_BlitSurface(ScrollSurface, &srcrect, BlitSurface, &dstrect);
      // (lower-left square)
      dstrect.x = save_dstx;
      dstrect.w = save_dstw;
      srcrect.x = save_srcx;
      srcrect.w = save_srcw;
      SDL_BlitSurface(ScrollSurface, &srcrect, BlitSurface, &dstrect);
   }
   else if (wraphoz)
   {
      dstrect.x = srcrect.w;
      dstrect.w = blit_w - dstrect.x;
      srcrect.x = 0;
      srcrect.w = (blit_w - srcrect.w);
      SDL_BlitSurface(ScrollSurface, &srcrect, BlitSurface, &dstrect);
   }
   else if (wrapvrt)
   {
      dstrect.y = srcrect.h;
      dstrect.h = blit_h - dstrect.y;
      srcrect.y = 0;
      srcrect.h = (blit_h - srcrect.h);
      SDL_BlitSurface(ScrollSurface, &srcrect, BlitSurface, &dstrect);
   }

	// if we're doing zoom then we have copied the scroll buffer into
	// another offscreen buffer, and must now stretchblit it to the screen
	if (options[OPT_ZOOM])
	{
		scale2x(VRAMPtr, (WINDOW_2X_WIDTH+border_width), BlitSurface->pixels, WINDOW_WIDTH, 1, WINDOW_WIDTH, WINDOW_HEIGHT);
	}
}

// update the contents of the display
void VidDrv_flipbuffer()
{
#ifdef __webos__
	// Stretch 320x240 game surface to fill 1024x768 screen (exact 3.2x scale)
	SDL_SoftStretch(webos_game_surface, NULL, screen, NULL);
	webos_draw_control_overlay();
#endif
	SDL_Flip(screen);
}


// functions to directly set and retrieve pixels from display
void setpixel(int x, int y, unsigned char c)
{
SDL_Rect rect;
	rect.x = x; rect.y = y;
	rect.w = rect.h = 1;
	SDL_FillRect(BlitSurface, &rect, c);
}
unsigned char getpixel(int x, int y)
{
Uint8 *ubuff8;

	if (x < 0 || y < 0 || x >= WINDOW_WIDTH || y >= WINDOW_HEIGHT) return 0;

    ubuff8 = (Uint8*) BlitSurface->pixels;
    ubuff8 += (y * BlitSurface->pitch) + x;
    return *ubuff8;
}

SDL_Color MyPalette[256];

// alter the color palette. the palette is not actually altered
// on-screen until pal_apply() is called.
void VidDrv_pal_set(uchar colour, uchar red, uchar green, uchar blue)
{
	MyPalette[colour].r = red;
	MyPalette[colour].g = green;
	MyPalette[colour].b = blue;
}

// applies all changes to the palette made with pal_set
void VidDrv_pal_apply(void)
{
	SDL_SetColors(screen, (SDL_Color *)&MyPalette, 0, 256);
	if (ScrollSurface) SDL_SetColors(ScrollSurface, (SDL_Color *)&MyPalette, 0, 256);
	if (BlitSurface) SDL_SetColors(BlitSurface, (SDL_Color *)&MyPalette, 0, 256);
#ifdef __webos__
	if (webos_game_surface) SDL_SetColors(webos_game_surface, (SDL_Color *)&MyPalette, 0, 256);
#endif
}

// starts up the video driver
char VidDrv_Start(void)
{
	if (allocmem()) return 1;
	
	putenv("SDL_VIDEO_CENTERED=1");
	
	if (!scrollbuf)
	{
		lprintf("VidDrv_Start: You cannot initilize SDL graphics until the scroll buffer is allocated (allocmem())!\n");
		return 1;
	}
	
	lprintf("VidDrv_Start: calling SDL_Init\n");
#ifdef __webos__
	PDL_Init(0);
	// Deliver raw touch events immediately rather than waiting for the system to
	// classify them as taps or gestures (which can swallow events entirely in a
	// non-GL PDK app).
	PDL_SetTouchAggression(PDL_AGGRESSION_MORETOUCHES);
	// Disable the bezel quick-launch gesture so swipes don't interrupt gameplay.
	PDL_GesturesEnable(PDL_FALSE);
	// webOS handles scaling itself via SDL_SoftStretch (320x240 -> 1024x768).
	// scale2x would write with a 640-wide pitch into our 320-wide game surface.
	options[OPT_ZOOM] = 0;
	options[OPT_ZOOMONRESTART] = 0;
#endif
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
	{
		lprintf("VidDrv_Start(): Could not initialize SDL: %s\n", SDL_GetError());
		return 1;
	}
	
	SDL_WM_SetCaption("CloneKeen", NULL);
	
	atexit(SDL_Quit);
	return VidDrv_CreateSurfaces();
}

char VidDrv_CreateSurfaces(void)
{
//create the initial window
	window_is_fullscreen = 7;
	VidDrv_SetFullscreen(options[OPT_FULLSCREEN]);


  lprintf("Creating ScrollSurface (%dx%d)\n", SCROLLBUF_XSIZE, SCROLLBUF_YSIZE);
  if (!scrollbuf) { crash("VidDrv_CreateSurfaces: 'scrollbuf' was never allocated!"); return 1; }
  ScrollSurface = SDL_CreateRGBSurfaceFrom(scrollbuf, SCROLLBUF_XSIZE, SCROLLBUF_YSIZE, 8, SCROLLBUF_XSIZE, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
  if (!ScrollSurface)
  {
     lprintf("VidDrv_Start(): Couldn't create ScrollSurface!\n");
     return 1;
  }

  if (options[OPT_ZOOM])
  {
    lprintf("Creating BlitSurface (%dx%d)\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!blitbuf) { crash("VidDrv_CreateSurfaces: 'blitbuf' was never allocated!"); return 1; }
    BlitSurface = SDL_CreateRGBSurfaceFrom(blitbuf, WINDOW_WIDTH, WINDOW_HEIGHT, 8, WINDOW_WIDTH, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
    if (!BlitSurface)
    {
      lprintf("VidDrv_Start(): Couldn't create BlitSurface!\n");
      return 1;
    }
  }
  else BlitSurface = screen;

#ifdef __webos__
  BlitSurface = webos_game_surface;
#endif

  dstrect.x = 0; dstrect.y = 0;
  dstrect.w = WINDOW_WIDTH;
  dstrect.h = WINDOW_HEIGHT;

  return 0;
}

void VidDrv_SetFullscreen(int fs_on)
{
int opts;
int w,h;
//	if (fs_on==window_is_fullscreen) return;
	lprintf("VidDrv_SetFullscreen(%d)\n", fs_on);

#ifdef __webos__
	// webOS: always 1024x768 fullscreen regardless of fs_on.
	// Store fs_on in window_is_fullscreen so the gamedo_RenderScreen mismatch
	// check doesn't fire every frame and re-grab the PDL surface (which causes
	// the compositor to SIGSTOP the app).
	window_is_fullscreen = fs_on;

	if (!screen)
	{
		screen = SDL_SetVideoMode(WEBOS_SCREEN_W, WEBOS_SCREEN_H, 8,
		                          SDL_SWSURFACE | SDL_FULLSCREEN);
		if (!screen)
		{
			crash("VidDrv_SetFullscreen: Couldn't set webOS video mode: %s\n", SDL_GetError());
			return;
		}
	}
	if (!webos_game_surface)
	{
		webos_game_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
		                     WINDOW_WIDTH, WINDOW_HEIGHT, 8, 0, 0, 0, 0);
		if (!webos_game_surface)
		{
			crash("VidDrv_SetFullscreen: Couldn't create webOS game surface\n");
			return;
		}
	}
	BlitSurface = webos_game_surface;
	VRAMPtr = webos_game_surface->pixels;
	window_width = WINDOW_WIDTH;
	window_height = WINDOW_HEIGHT;
	border_width = border_height = 0;
	SDL_ShowCursor(SDL_DISABLE);
	VidDrv_pal_apply();
	lprintf("VidDrv_SetFullscreen: webOS %dx%d fullscreen, game surface %dx%d\n",
	        WEBOS_SCREEN_W, WEBOS_SCREEN_H, WINDOW_WIDTH, WINDOW_HEIGHT);
	return;
#endif
	
	#ifdef SHOW_SCROLLBUFFER
		window_width = SCROLLBUF_XSIZE;
		window_height = SCROLLBUF_YSIZE;
	#else
		window_width = WINDOW_WIDTH;
		window_height = WINDOW_HEIGHT;
		if (options[OPT_ZOOM])
		{
			window_width <<= 1; window_height <<= 1;
		}
	#endif
	
	// add border if we're in map editor mode
	if (editor)
	{
		border_width = 80; border_height = 130+32;
		fs_on = 0;
	}
	else
	{
		border_width = border_height = 0;
	}
	
	opts = SDL_HWSURFACE | SDL_HWPALETTE;
	if (fs_on)
	{
		opts |= SDL_FULLSCREEN;
	}
	
	if (screen)
	{
		lprintf("VidDrv_SetFullscreen: deleting old screen surface\n");
		SDL_FreeSurface(screen);
	}
	
	w = window_width+border_width;
	h = window_height+border_height;
	lprintf("VidDrv_SetFullscreen: Setting window size to %dx%d\n", w, h);
	screen = SDL_SetVideoMode(w,h, 8, opts);
	if (!screen)
	{
		crash("VidDrv_Start(): Couldn't create a SDL surface: %s\n", SDL_GetError());
		return;
	}
	lprintf("VidDrv_SetFullscreen: SDL_SetVideoMode successful.\n");
	VidDrv_pal_apply();
	
	window_is_fullscreen = fs_on;
	if (window_is_fullscreen)
		SDL_ShowCursor(SDL_DISABLE);
	else
		SDL_ShowCursor(SDL_ENABLE);
	
	VRAMPtr = screen->pixels;
	if (!options[OPT_ZOOM]) BlitSurface = screen;
	
	lprintf("VidDrv_SetFullscreen: full-screen mode now %s.\n", window_is_fullscreen?"on":"off");
}

// shuts down the video driver
void VidDrv_Stop(void)
{
	// workaround for Haiku SDL bug
	if (window_is_fullscreen)
	{
		lprintf("VidDrv_Stop: leaving full screen mode\n");
		VidDrv_SetFullscreen(0);
	}
	
#ifdef __webos__
	if (webos_game_surface) { SDL_FreeSurface(webos_game_surface); webos_game_surface = NULL; }
	BlitSurface = NULL;
#endif
	if (screen) { SDL_FreeSurface(screen); screen = NULL; }
	if (ScrollSurface) { SDL_FreeSurface(ScrollSurface); }
	if (BlitSurface) { SDL_FreeSurface(BlitSurface); }
	freemem();
}

// resets graphics to allow changing of resolution or zoom settings
void VidDrv_reset(void)
{
     lprintf("restart: shutting down viddrv\n");
     VidDrv_Stop();

     // re-allocate memory as blitbuffer may appear/disappear
     freemem();
     if (allocmem()) { crashflag = 1; return; }

     lprintf("restart: restarting viddrv\n");
     VidDrv_CreateSurfaces();   // don't re-call sdl_init
     pal_fade(PAL_FADE_SHADES);
}

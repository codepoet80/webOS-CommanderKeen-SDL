/* KEYDRV.C
  The low-level keyboard driver for the SDL port.
  
  Sorry I know the mapping stuff is kind of confusing.
  I think that's just the way it goes.
  It works though right?
*/

#include "../keen.h"
#include <SDL.h>
#include "keydrv.fdh"

int mouse_x, mouse_y;
uchar mouseL, mouseR;

uchar setting_keymap_ktindex = 0;
uchar setting_keymap_keyno = 0;
uchar sdl_keysdown[SDLK_LAST];		// this is used by the menus
uchar sdl_lastkeysdown[SDLK_LAST];	// this is used by the editor

// this is a lookup table of SDL keys -> keytable keys.
// for example keymappings[SDLK_LEFT] might contain the value 'KLEFT'.
uchar keymappings[SDLK_LAST];
#define MAX_KEYS_PER_CONTROL		2
typedef struct
{
	int sdl_keysym[MAX_KEYS_PER_CONTROL];
} stRevKeyMap;
stRevKeyMap reverse_keymappings[KEYTABLE_SIZE];

void KeyDrv_SetDefaultMappings(void)
{
	lprintf("keydrv: Setting default keyboard mappings.\n");
	
	memset(keymappings, 0, sizeof(keymappings));
	keymappings[SDLK_LEFT] = KLEFT;
	keymappings[SDLK_RIGHT] = KRIGHT;
	keymappings[SDLK_UP] = KUP;
	keymappings[SDLK_DOWN] = KDOWN;
	keymappings[SDLK_LCTRL] = KCTRL; keymappings[SDLK_RALT] = KCTRL;
	keymappings[SDLK_LALT] = KALT; keymappings[SDLK_RCTRL] = KALT;
	keymappings[SDLK_SPACE] = KSPACE;
	keymappings[SDLK_RETURN] = KENTER;
	
	keymappings[SDLK_ESCAPE] = KESC;
	keymappings[SDLK_DELETE] = KDEL;
	
	keymappings[SDLK_KP_PLUS] = keymappings[SDLK_EQUALS] = KPLUS;
	keymappings[SDLK_KP_MINUS] = keymappings[SDLK_MINUS] = KMINUS;
	keymappings[SDLK_TAB] = KTAB;
	
	keymappings[SDLK_F1] = KF1;
	keymappings[SDLK_F2] = KF2;
	keymappings[SDLK_F3] = KF3;
	keymappings[SDLK_F4] = KF4;
	keymappings[SDLK_F5] = KF5;
	keymappings[SDLK_F6] = KF6;
	keymappings[SDLK_F7] = KF7;
	keymappings[SDLK_F8] = KF8;
	keymappings[SDLK_F9] = KF9;
	keymappings[SDLK_F10] = KF10;
	
	mappings_to_reverse_mappings();
}

void static mappings_to_reverse_mappings(void)
{
int i,j,k;
	lprintf("keydrv: Generating reverse mapping chart.\n");
	for(i=0;i<KEYTABLE_SIZE;i++)
	{
		for(k=0;k<MAX_KEYS_PER_CONTROL;k++)
			reverse_keymappings[i].sdl_keysym[k] = 0;
			
		k = 0;
		// backwards so Lctrl comes up before Rctrl
		for(j=SDLK_LAST-1;j>=0;j--)
		{
			if (keymappings[j]==i)
			{
				reverse_keymappings[i].sdl_keysym[k] = j;
				if (++k >= MAX_KEYS_PER_CONTROL)
				{
					break;
				}
			}
		}
	}
}

void static reverse_mappings_to_mappings(void)
{
int i,k;
	memset(keymappings, 0, sizeof(keymappings));
	
	for(i=0;i<KEYTABLE_SIZE;i++)
	{
		for(k=0;k<MAX_KEYS_PER_CONTROL;k++)
		{
			if (reverse_keymappings[i].sdl_keysym[k])
			{
				keymappings[reverse_keymappings[i].sdl_keysym[k]] = i;
			}
		}
	}
}

// try to load the key mappings from the config file, or if we can't,
// set the defaults.
void KeyDrv_LoadKeyMappings(void)
{
int i,k;
char key[40];
	if (Ini_GetNumericKey(NULL, "KEYMAPVER") != KEYMAPPINGVERSION)
	{
		KeyDrv_SetDefaultMappings();
		KeyDrv_SaveKeyMappings();
		return;
	}
	
	memset(reverse_keymappings, NOKEY, sizeof(reverse_keymappings));
	for(i=1;i<KEYTABLE_SIZE;i++)	// start at 1 because 0=NOKEY
	{
		for(k=0;k<MAX_KEYS_PER_CONTROL;k++)
		{
			sprintf(key, "KEYMAP%d_%d", i, k+1);
			reverse_keymappings[i].sdl_keysym[k] = Ini_GetNumericKey(NULL, key);
			if (reverse_keymappings[i].sdl_keysym[k]==-1)
			{
				// don't save, this way if it was caused by accidental
				// editing of the config file it can be easily fixed.
				// if it's corrupted some other reason, then when the user
				// resets up the keyboard it'll be fixed then.
				KeyDrv_SetDefaultMappings();
				return;
			}
		}
	}
	reverse_mappings_to_mappings();
}

// save the current keymappings to the config file
void KeyDrv_SaveKeyMappings(void)
{
int i,k;
char key[40];
	Ini_WriteNumericKey(NULL, "KEYMAPVER", KEYMAPPINGVERSION);
	
	for(i=1;i<KEYTABLE_SIZE;i++)	// start at 1 because 0=NOKEY
	{
		for(k=0;k<MAX_KEYS_PER_CONTROL;k++)
		{
			sprintf(key, "KEYMAP%d_%d", i, k+1);
			Ini_WriteNumericKey(NULL, key, reverse_keymappings[i].sdl_keysym[k]);
		}
	}
}



// returns a text string describing the name of which actual key on the
// keyboard is currently mapped to keytable[keytable_index].
// key_no defines whether to retrieve the first or second key which is
// mapped to that keytable index.
// if no key is mapped at the specified position, returns NULL.
char *noksym = "";
char *beingset = "*****";
char knbuf[40];
uchar *KeyDrv_GetKeyMapping(uchar keytable_index, uchar key_no)
{
SDLKey ksym;
char *kname;

	if (KeyDrv_KeyMappingBeingSet(keytable_index, key_no))
	{
		return beingset;
	}
	
	ksym = reverse_keymappings[keytable_index].sdl_keysym[key_no];
	if (!ksym) return noksym;
	
	kname = SDL_GetKeyName(ksym);
	// change e.g. "left ctrl" to "Lctrl".
	if (strstr(kname, "right "))
	{
		sprintf(knbuf, "R%s", &kname[6]);
		return knbuf;
	}
	else if (strstr(kname, "left "))
	{
		sprintf(knbuf, "L%s", &kname[5]);
		return knbuf;
	}
	return kname;
}

// used in menus (so you can't screw up being able to navigate the menu
// by messing up the keyboard mapping).
char KeyDrv_KeyIsDown(int sdl_key)
{
	return sdl_keysdown[sdl_key];
}

char KeyDrv_LastKeyIsDown(int sdl_key)
{
	return sdl_lastkeysdown[sdl_key];
}

void KeyDrv_CopyLastKeys(void)
{
	memcpy(sdl_lastkeysdown, sdl_keysdown, sizeof(sdl_lastkeysdown));
}

// initilizes KeyDrv_SetKeyMapping
uchar lastkeychanged = -1;
void KeyDrv_EnteredKeySetupMenu(void)
{
	lastkeychanged = -1;
}

// specifies that the next key to be pushed down is to become the new
// key to control the specified keytable[] index.
void KeyDrv_SetKeyMapping(uchar keytable_index)
{
uchar knum;
	if (keytable_index==lastkeychanged)
	{
		knum = 1;
		lastkeychanged = -1;
	}
	else
	{
		knum = 0;
		lastkeychanged = keytable_index;
	}
	setting_keymap_ktindex = keytable_index;
	setting_keymap_keyno = knum;
}

// returns nonzero if the specified keytable and key_no is currently
// waiting on a key to be set to
char KeyDrv_KeyMappingBeingSet(uchar keytable_index, uchar key_no)
{
	return (setting_keymap_ktindex==keytable_index && setting_keymap_keyno==key_no);
}


char KeyDrv_Start(void)
{
	lprintf("Starting keyboard driver...\n");
	KeyDrv_LoadKeyMappings();
	
	memset(keytable, 0, sizeof(keytable));
	memset(last_keytable, 1, sizeof(last_keytable));
	memset(sdl_keysdown, 0, sizeof(sdl_keysdown));
	setting_keymap_ktindex = 0;
	
	return 0;
}

void KeyDrv_Stop(void)
{
}

// we process the GOD cheat code here so that it's not necessary to
// have a seperate keytable[] index for each cheat code letter
char key_g=0, key_o=0, key_d=0;

#ifdef __webos__
// D-pad + button layout on 1024x768:
//
//  [MENU]                              ← small rect, top-left
//
//    ╭──────────╮        ╭──────╮
//    │    ↑     │        │ JUMP │  ← (900,600)
//    │  ←   →  │        ╰──────╯
//    │    ↓     │  ╭──────╮
//    ╰──────────╯  │ FIRE │  ← (790,680)
//   center(120,655) ╰──────╯
//
// SDL 1.2 on webOS: event.button.which / event.motion.which = finger index (0-4).
// Each finger is tracked independently; d-pad + button can be held simultaneously.

// D-pad
#define DPAD_CENTER_X  120
#define DPAD_CENTER_Y  655
#define DPAD_RADIUS    110
#define DPAD_DEADZONE  28
#define DPAD_SLACK     50   // extra hit radius beyond DPAD_RADIUS to keep tracking

// Action buttons
#define JUMP_CENTER_X  900
#define JUMP_CENTER_Y  600
#define JUMP_RADIUS    60

#define FIRE_CENTER_X  790
#define FIRE_CENTER_Y  680
#define FIRE_RADIUS    60

// Menu / ESC button
#define MENU_BTN_X     10
#define MENU_BTN_Y     10
#define MENU_BTN_W     100
#define MENU_BTN_H     38

// Minimum hold time (ms) so a fast tap still registers in the logic timer.
#define WEBOS_MIN_HOLD_MS 150

#define ZONE_NONE  0
#define ZONE_DPAD  1
#define ZONE_JUMP  2
#define ZONE_FIRE  3
#define ZONE_MENU  4

#define MAX_FINGERS 5

typedef struct {
	int    zone;
	int    dpad_h;       // KLEFT, KRIGHT, or 0
	int    dpad_v;       // KUP, KDOWN, or 0
	int    action_key;   // KCTRL, KALT, KESC, or 0
	int    pending_up;
	Uint32 key_down_time;
} WebOSFinger;

static WebOSFinger webos_fingers[MAX_FINGERS];  // zero-initialized

static int webos_in_circle(int x, int y, int cx, int cy, int r)
{
	int dx = x - cx, dy = y - cy;
	return dx*dx + dy*dy <= r*r;
}

// Mirror keytable keys into sdl_keysdown for menu navigation.
static void webos_set_sdl_keysdown(int key, int state)
{
	switch (key) {
		case KESC:   sdl_keysdown[SDLK_ESCAPE] = state; break;
		case KUP:    sdl_keysdown[SDLK_UP]     = state; break;
		case KDOWN:  sdl_keysdown[SDLK_DOWN]   = state; break;
		case KRIGHT: sdl_keysdown[SDLK_RETURN] = state; break;
		case KCTRL:  sdl_keysdown[SDLK_RETURN] = state; break;
		default: break;
	}
}

// Return 1 if any finger other than fi is actively holding key k.
static int webos_any_holding(int fi, int k)
{
	int i;
	if (!k) return 0;
	for (i = 0; i < MAX_FINGERS; i++) {
		if (i == fi) continue;
		if (webos_fingers[i].dpad_h    == k ||
		    webos_fingers[i].dpad_v    == k ||
		    webos_fingers[i].action_key == k) return 1;
	}
	return 0;
}

// Set/clear a key for finger fi, respecting other fingers still holding it.
static void webos_finger_set_key(int fi, int key, int state)
{
	if (!key) return;
	if (state) {
		keytable[key] = 1;
		webos_set_sdl_keysdown(key, 1);
	} else if (!webos_any_holding(fi, key)) {
		keytable[key] = 0;
		webos_set_sdl_keysdown(key, 0);
	}
}

static void webos_finger_release(int fi)
{
	WebOSFinger *f = &webos_fingers[fi];
	webos_finger_set_key(fi, f->dpad_h,    0); f->dpad_h    = 0;
	webos_finger_set_key(fi, f->dpad_v,    0); f->dpad_v    = 0;
	webos_finger_set_key(fi, f->action_key, 0); f->action_key = 0;
	f->zone       = ZONE_NONE;
	f->pending_up = 0;
}

static void webos_dpad_update(int fi, int x, int y)
{
	WebOSFinger *f = &webos_fingers[fi];
	int dx = x - DPAD_CENTER_X;
	int dy = y - DPAD_CENTER_Y;
	int new_h = 0, new_v = 0;

	if (dx*dx + dy*dy >= DPAD_DEADZONE * DPAD_DEADZONE) {
		if (dx < -DPAD_DEADZONE)     new_h = KLEFT;
		else if (dx > DPAD_DEADZONE) new_h = KRIGHT;
		if (dy < -DPAD_DEADZONE)     new_v = KUP;
		else if (dy > DPAD_DEADZONE) new_v = KDOWN;
	}
	if (f->dpad_h != new_h) {
		webos_finger_set_key(fi, f->dpad_h, 0);
		f->dpad_h = new_h;
		webos_finger_set_key(fi, new_h, 1);
	}
	if (f->dpad_v != new_v) {
		webos_finger_set_key(fi, f->dpad_v, 0);
		f->dpad_v = new_v;
		webos_finger_set_key(fi, new_v, 1);
	}
}

static void webos_touch_down(int fi, int x, int y)
{
	WebOSFinger *f = &webos_fingers[fi];
	webos_finger_release(fi);  // clear any prior state for this finger slot
	f->key_down_time = SDL_GetTicks();

	if (x >= MENU_BTN_X && x <= MENU_BTN_X + MENU_BTN_W &&
	    y >= MENU_BTN_Y && y <= MENU_BTN_Y + MENU_BTN_H) {
		f->zone       = ZONE_MENU;
		f->action_key = KESC;
		webos_finger_set_key(fi, KESC, 1);
		return;
	}
	if (webos_in_circle(x, y, DPAD_CENTER_X, DPAD_CENTER_Y, DPAD_RADIUS + DPAD_SLACK)) {
		f->zone = ZONE_DPAD;
		webos_dpad_update(fi, x, y);
		return;
	}
	if (webos_in_circle(x, y, JUMP_CENTER_X, JUMP_CENTER_Y, JUMP_RADIUS + 20)) {
		f->zone       = ZONE_JUMP;
		f->action_key = KCTRL;
		webos_finger_set_key(fi, KCTRL, 1);
		return;
	}
	if (webos_in_circle(x, y, FIRE_CENTER_X, FIRE_CENTER_Y, FIRE_RADIUS + 20)) {
		f->zone       = ZONE_FIRE;
		f->action_key = KALT;
		webos_finger_set_key(fi, KALT, 1);
		return;
	}
	f->zone = ZONE_NONE;
}

static void webos_touch_up(int fi)
{
	webos_fingers[fi].pending_up = 1;
}

static void webos_touch_move(int fi, int x, int y)
{
	WebOSFinger *f = &webos_fingers[fi];
	if (f->pending_up || f->zone != ZONE_DPAD) return;
	if (webos_in_circle(x, y, DPAD_CENTER_X, DPAD_CENTER_Y, DPAD_RADIUS + DPAD_SLACK))
		webos_dpad_update(fi, x, y);
}
#endif /* __webos__ */

// update the event processing
void poll_events(void)
{
char newState;
SDL_Event event;

#ifdef __webos__
	{
		int fi;
		for (fi = 0; fi < MAX_FINGERS; fi++) {
			if (webos_fingers[fi].pending_up &&
			    SDL_GetTicks() - webos_fingers[fi].key_down_time >= WEBOS_MIN_HOLD_MS)
				webos_finger_release(fi);
		}
	}
#endif

	while(SDL_PollEvent(&event))
	{
		switch( event.type )
		{
			case SDL_QUIT:
				crash("SDL: Got quit event!");
				break;
			
			case SDL_KEYUP:
				newState = 0;
				goto prockey;
			case SDL_KEYDOWN:
				newState = 1;
				if (setting_keymap_ktindex)	// for input configuration
				{
					if (event.key.keysym.sym==SDLK_ESCAPE)
					{
						lastkeychanged = -1;
						if (setting_keymap_keyno > 0)
							reverse_keymappings[setting_keymap_ktindex].sdl_keysym[setting_keymap_keyno] = NOKEY;
					}
					else
						reverse_keymappings[setting_keymap_ktindex].sdl_keysym[setting_keymap_keyno] = event.key.keysym.sym;
						
					reverse_mappings_to_mappings();
					setting_keymap_ktindex = 0;
					break;
				}
prockey: ;

				if (keymappings[event.key.keysym.sym])
				{
					keytable[keymappings[event.key.keysym.sym]] = newState;
				}
				
				sdl_keysdown[event.key.keysym.sym] = newState;				
				break;
			
			case SDL_MOUSEMOTION:
				mouse_x = event.motion.x;
				mouse_y = event.motion.y;
#ifdef __webos__
				{
					int fi = event.motion.which < MAX_FINGERS ? event.motion.which : 0;
					webos_touch_move(fi, event.motion.x, event.motion.y);
				}
#endif
				break;

			case SDL_MOUSEBUTTONDOWN:
				mouse_x = event.button.x;
				mouse_y = event.button.y;
				if (event.button.button==SDL_BUTTON_LEFT)
				{
					mouseL = 1;
#ifdef __webos__
					{
						int fi = event.button.which < MAX_FINGERS ? event.button.which : 0;
						webos_touch_down(fi, event.button.x, event.button.y);
					}
#endif
				}
				else if (event.button.button==SDL_BUTTON_RIGHT)
					mouseR = 1;
				break;

			case SDL_MOUSEBUTTONUP:
				mouse_x = event.button.x;
				mouse_y = event.button.y;
				if (event.button.button==SDL_BUTTON_LEFT)
				{
					mouseL = 0;
#ifdef __webos__
					{
						int fi = event.button.which < MAX_FINGERS ? event.button.which : 0;
						webos_touch_up(fi);
					}
#endif
				}
				else if (event.button.button==SDL_BUTTON_RIGHT)
					mouseR = 0;
				break;
        }
    }
}

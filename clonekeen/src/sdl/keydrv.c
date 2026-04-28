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
// Screen is 1024x768. Map touch zones to game controls.
//
//  ┌──────────────────────────────────┐
//  │            JUMP (Ctrl)           │  y < 192
//  ├────────┬─────────────────┬───────┤
//  │        │                 │       │
//  │  LEFT  │   ESC (menu)    │ RIGHT │  192 <= y <= 575
//  │        │                 │       │
//  ├────────┴─────────────────┴───────┤
//  │         FIRE/POGO (Alt)          │  y > 575
//  └──────────────────────────────────┘
//     x<300         300-723       x>723
//
// Only one finger tracked at a time (SDL 1.2 limitation).

#define WEBOS_JUMP_Y    192
#define WEBOS_FIRE_Y    575
#define WEBOS_LEFT_X    300
#define WEBOS_RIGHT_X   723
#define WEBOS_SPLIT_X   512   // splits top/bottom zones: left=directional, right=action

// Minimum time (ms) a key stays active after the finger lifts.
// This decouples touch timing from the render/logic timer rates so that even a
// fast tap registers across at least one logic frame regardless of the
// render-vs-logic timer phase relationship. 150ms matches the PCSX-ReARMed
// webOS port which solved the same problem on this hardware.
#define WEBOS_MIN_HOLD_MS 150

static int      webos_touch_key     = 0;
static int      webos_pending_up    = 0;
static int      webos_pending_up_key = 0;
static Uint32   webos_key_down_time = 0;  // SDL_GetTicks() when current key went down

// Mirror a keytable key into sdl_keysdown (used by the menu system).
// Zone → menu function:
//   KESC  (center) = close/cancel
//   KCTRL (top)    = navigate up
//   KALT  (bottom) = navigate down
//   KRIGHT (right) = confirm/select  (right arrow doubles as Enter in menus)
static void webos_set_sdl_keysdown(int key, int state)
{
	switch (key) {
		case KESC:
			sdl_keysdown[SDLK_ESCAPE] = state;
			break;
		case KCTRL:
			sdl_keysdown[SDLK_UP] = state;
			break;
		case KALT:
			sdl_keysdown[SDLK_DOWN] = state;
			break;
		case KRIGHT:
			sdl_keysdown[SDLK_RETURN] = state;
			break;
		default:
			break;
	}
}

static void webos_set_touch_key(int key, int state)
{
	if (webos_touch_key && webos_touch_key != key) {
		keytable[webos_touch_key] = 0;
		webos_set_sdl_keysdown(webos_touch_key, 0);
	}
	webos_touch_key = key;
	if (key) {
		keytable[key] = state;
		webos_set_sdl_keysdown(key, state);
		if (state)
			webos_key_down_time = SDL_GetTicks();
	}
}

static void webos_touch_down(int x, int y)
{
	int key;
	if (y < WEBOS_JUMP_Y)
		// Top zone split: left = look/enter-door (KUP), right = jump (KCTRL)
		key = (x < WEBOS_SPLIT_X) ? KUP : KCTRL;
	else if (y > WEBOS_FIRE_Y)
		// Bottom zone split: left = duck (KDOWN), right = fire/pogo (KALT)
		key = (x < WEBOS_SPLIT_X) ? KDOWN : KALT;
	else if (x < WEBOS_LEFT_X)
		key = KLEFT;
	else if (x > WEBOS_RIGHT_X)
		key = KRIGHT;
	else
		key = KESC;
	webos_set_touch_key(key, 1);
}

static void webos_touch_up(void)
{
	// Schedule release but hold the key for at least WEBOS_MIN_HOLD_MS so that
	// the logic timer sees the press regardless of render/logic phase alignment.
	webos_pending_up = 1;
	webos_pending_up_key = webos_touch_key;
	webos_touch_key = 0;
}

static void webos_touch_move(int x, int y)
{
	if (y >= WEBOS_JUMP_Y && y <= WEBOS_FIRE_Y)
	{
		if (x < WEBOS_LEFT_X)
			webos_set_touch_key(KLEFT, 1);
		else if (x > WEBOS_RIGHT_X)
			webos_set_touch_key(KRIGHT, 1);
	}
}
#endif /* __webos__ */

// update the event processing
void poll_events(void)
{
char newState;
SDL_Event event;

#ifdef __webos__
	if (webos_pending_up &&
	    SDL_GetTicks() - webos_key_down_time >= WEBOS_MIN_HOLD_MS)
	{
		keytable[webos_pending_up_key] = 0;
		webos_set_sdl_keysdown(webos_pending_up_key, 0);
		webos_pending_up = 0;
		webos_pending_up_key = 0;
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
				if (event.motion.state & SDL_BUTTON(1))
					webos_touch_move(event.motion.x, event.motion.y);
#endif
				break;

			case SDL_MOUSEBUTTONDOWN:
				mouse_x = event.button.x;
				mouse_y = event.button.y;
				if (event.button.button==SDL_BUTTON_LEFT)
				{
					mouseL = 1;
#ifdef __webos__
					webos_touch_down(event.button.x, event.button.y);
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
					webos_touch_up();
#endif
				}
				else if (event.button.button==SDL_BUTTON_RIGHT)
					mouseR = 0;
				break;
        }
    }
}

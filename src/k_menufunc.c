/// \file  k_menufunc.c
/// \brief SRB2Kart's menu functions

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "k_menu.h"

#include "doomdef.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "console.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"
#include "m_argv.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

// Addfile
#include "filesrch.h"

#include "v_video.h"
#include "i_video.h"
#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_setup.h"
#include "f_finale.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "mserv.h"
#include "m_misc.h"
#include "m_anigif.h"
#include "byteptr.h"
#include "st_stuff.h"
#include "i_sound.h"
#include "k_kart.h" // SRB2kart
#include "d_player.h" // KITEM_ constants
#include "doomstat.h" // MAXSPLITSCREENPLAYERS
#include "k_grandprix.h" // MAXSPLITSCREENPLAYERS

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#include "r_skins.h"

#if defined(HAVE_SDL)
#include "SDL.h"
#if SDL_VERSION_ATLEAST(2,0,0)
#include "sdl/sdlmain.h" // JOYSTICK_HOTPLUG
#endif
#endif

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

// ==========================================================================
// GLOBAL VARIABLES
// ==========================================================================

#ifdef HAVE_THREADS
I_mutex k_menu_mutex;
#endif

boolean menuactive = false;
boolean fromlevelselect = false;

// current menudef
menu_t *currentMenu = &MainDef;

char dummystaffname[22];

INT16 itemOn = 0; // menu item skull is on, Hack by Tails 09-18-2002
INT16 skullAnimCounter = 8; // skull animation counter
struct menutransition_s menutransition; // Menu transition properties

// finish wipes between screens
boolean menuwipe = false;

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

// Consvar onchange functions
static void Dummymenuplayer_OnChange(void);
static void Dummystaff_OnChange(void);

consvar_t cv_showfocuslost = CVAR_INIT ("showfocuslost", "Yes", CV_SAVE, CV_YesNo, NULL);

#if 0
static CV_PossibleValue_t map_cons_t[] = {
	{0,"MIN"},
	{NUMMAPS, "MAX"},
	{0, NULL}
};
consvar_t cv_nextmap = CVAR_INIT ("nextmap", "1", CV_HIDEN|CV_CALL, map_cons_t, Nextmap_OnChange);
#endif

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = CVAR_INIT ("chooseskin", DEFAULTSKIN, CV_HIDDEN, skins_cons_t, NULL);

// This gametype list is integral for many different reasons.
// When you add gametypes here, don't forget to update them in dehacked.c and doomstat.h!
CV_PossibleValue_t gametype_cons_t[NUMGAMETYPES+1];

static CV_PossibleValue_t serversort_cons_t[] = {
	{0,"Ping"},
	{1,"Modified State"},
	{2,"Most Players"},
	{3,"Least Players"},
	{4,"Max Player Slots"},
	{5,"Gametype"},
	{0,NULL}
};
consvar_t cv_serversort = CVAR_INIT ("serversort", "Ping", CV_CALL, serversort_cons_t, M_SortServerList);

// first time memory
consvar_t cv_tutorialprompt = CVAR_INIT ("tutorialprompt", "On", CV_SAVE, CV_OnOff, NULL);

// autorecord demos for time attack
static consvar_t cv_autorecord = CVAR_INIT ("autorecord", "Yes", 0, CV_YesNo, NULL);

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show Character"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_besttime  = CVAR_INIT ("ghost_besttime",  "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_bestlap   = CVAR_INIT ("ghost_bestlap",   "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_last      = CVAR_INIT ("ghost_last",      "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_guest     = CVAR_INIT ("ghost_guest",     "Show", CV_SAVE, ghost2_cons_t, NULL);
consvar_t cv_ghost_staff     = CVAR_INIT ("ghost_staff",     "Show", CV_SAVE, ghost2_cons_t, NULL);

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.
static void Splitplayers_OnChange(void);
CV_PossibleValue_t splitplayers_cons_t[] = {{1, "One"}, {2, "Two"}, {3, "Three"}, {4, "Four"}, {0, NULL}};
consvar_t cv_splitplayers = CVAR_INIT ("splitplayers", "One", CV_CALL, splitplayers_cons_t, Splitplayers_OnChange);

static CV_PossibleValue_t dummymenuplayer_cons_t[] = {{0, "NOPE"}, {1, "P1"}, {2, "P2"}, {3, "P3"}, {4, "P4"}, {0, NULL}};
static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static CV_PossibleValue_t dummyspectate_cons_t[] = {{0, "Spectator"}, {1, "Playing"}, {0, NULL}};
static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static CV_PossibleValue_t dummystaff_cons_t[] = {{0, "MIN"}, {100, "MAX"}, {0, NULL}};
static CV_PossibleValue_t dummygametype_cons_t[] = {{0, "Race"}, {1, "Battle"}, {0, NULL}};

//static consvar_t cv_dummymenuplayer = CVAR_INIT ("dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange);
static consvar_t cv_dummyteam = CVAR_INIT ("dummyteam", "Spectator", CV_HIDDEN, dummyteam_cons_t, NULL);
//static cv_dummyspectate = CVAR_INITconsvar_t  ("dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL);
static consvar_t cv_dummyscramble = CVAR_INIT ("dummyscramble", "Random", CV_HIDDEN, dummyscramble_cons_t, NULL);
static consvar_t cv_dummystaff = CVAR_INIT ("dummystaff", "0", CV_HIDDEN|CV_CALL, dummystaff_cons_t, Dummystaff_OnChange);
consvar_t cv_dummygametype = CVAR_INIT ("dummygametype", "Race", CV_HIDDEN, dummygametype_cons_t, NULL);
consvar_t cv_dummyip = CVAR_INIT ("dummyip", "", CV_HIDDEN, NULL, NULL);
consvar_t cv_dummymenuplayer = CVAR_INIT ("dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange);
consvar_t cv_dummyspectate = CVAR_INIT ("dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL);

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

#if 0
// Nextmap.  Used for Time Attack.
static void Nextmap_OnChange(void)
{
	char *leveltitle;

	// Update the string in the consvar.
	Z_Free(cv_nextmap.zstring);
	leveltitle = G_BuildMapTitle(cv_nextmap.value);
	cv_nextmap.string = cv_nextmap.zstring = leveltitle ? leveltitle : Z_StrDup(G_BuildMapName(cv_nextmap.value));

	if (currentMenu == &SP_TimeAttackDef)
	{
		// see also p_setup.c's P_LoadRecordGhosts
		const size_t glen = strlen(srb2home)+1+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
		char *gpath = malloc(glen);
		INT32 i;
		UINT8 active;

		if (!gpath)
			return;

		sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));

		CV_StealthSetValue(&cv_dummystaff, 0);

		active = false;
		SP_TimeAttackMenu[taguest].status = IT_DISABLED;
		SP_TimeAttackMenu[tareplay].status = IT_DISABLED;
		//SP_TimeAttackMenu[taghost].status = IT_DISABLED;

		// Check if file exists, if not, disable REPLAY option
		for (i = 0; i < 4; i++)
		{
			SP_ReplayMenu[i].status = IT_DISABLED;
			SP_GuestReplayMenu[i].status = IT_DISABLED;
		}
		SP_ReplayMenu[4].status = IT_DISABLED;

		SP_GhostMenu[3].status = IT_DISABLED;
		SP_GhostMenu[4].status = IT_DISABLED;

		if (FIL_FileExists(va("%s-%s-time-best.lmp", gpath, cv_chooseskin.string))) {
			SP_ReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active |= 3;
		}
		if (FIL_FileExists(va("%s-%s-lap-best.lmp", gpath, cv_chooseskin.string))) {
			SP_ReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active |= 3;
		}
		if (FIL_FileExists(va("%s-%s-last.lmp", gpath, cv_chooseskin.string))) {
			SP_ReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active |= 3;
		}

		if (FIL_FileExists(va("%s-guest.lmp", gpath)))
		{
			SP_ReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_GhostMenu[3].status = IT_STRING|IT_CVAR;
			active |= 3;
		}

		CV_SetValue(&cv_dummystaff, 1);
		if (cv_dummystaff.value)
		{
			SP_ReplayMenu[4].status = IT_WHITESTRING|IT_KEYHANDLER;
			SP_GhostMenu[4].status = IT_STRING|IT_CVAR;
			CV_StealthSetValue(&cv_dummystaff, 1);
			active |= 1;
		}

		if (active) {
			if (active & 1)
				SP_TimeAttackMenu[tareplay].status = IT_WHITESTRING|IT_SUBMENU;
			if (active & 2)
				SP_TimeAttackMenu[taguest].status = IT_WHITESTRING|IT_SUBMENU;
		}
		else if (itemOn == tareplay) // Reset lastOn so replay isn't still selected when not available.
		{
			currentMenu->lastOn = itemOn;
			itemOn = tastart;
		}

		if (mapheaderinfo[cv_nextmap.value-1] && mapheaderinfo[cv_nextmap.value-1]->forcecharacter[0] != '\0')
			CV_Set(&cv_chooseskin, mapheaderinfo[cv_nextmap.value-1]->forcecharacter);

		free(gpath);
	}
}
#endif

static void Dummymenuplayer_OnChange(void)
{
	if (cv_dummymenuplayer.value < 1)
		CV_StealthSetValue(&cv_dummymenuplayer, splitscreen+1);
	else if (cv_dummymenuplayer.value > splitscreen+1)
		CV_StealthSetValue(&cv_dummymenuplayer, 1);
}

static void Dummystaff_OnChange(void)
{
#if 0
	lumpnum_t l;

	dummystaffname[0] = '\0';

	if ((l = W_CheckNumForName(va("%sS01",G_BuildMapName(cv_nextmap.value)))) == LUMPERROR)
	{
		CV_StealthSetValue(&cv_dummystaff, 0);
		return;
	}
	else
	{
		char *temp = dummystaffname;
		UINT8 numstaff = 1;
		while (numstaff < 99 && (l = W_CheckNumForName(va("%sS%02u",G_BuildMapName(cv_nextmap.value),numstaff+1))) != LUMPERROR)
			numstaff++;

		if (cv_dummystaff.value < 1)
			CV_StealthSetValue(&cv_dummystaff, numstaff);
		else if (cv_dummystaff.value > numstaff)
			CV_StealthSetValue(&cv_dummystaff, 1);

		if ((l = W_CheckNumForName(va("%sS%02u",G_BuildMapName(cv_nextmap.value), cv_dummystaff.value))) == LUMPERROR)
			return; // shouldn't happen but might as well check...

		G_UpdateStaffGhostName(l);

		while (*temp)
			temp++;

		sprintf(temp, " - %d", cv_dummystaff.value);
	}
#endif
}

void Screenshot_option_Onchange(void)
{
#if 0
	OP_ScreenshotOptionsMenu[op_screenshot_folder].status =
		(cv_screenshot_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
#endif
}

void Moviemode_mode_Onchange(void)
{
#if 0
	INT32 i, cstart, cend;
	for (i = op_screenshot_gif_start; i <= op_screenshot_apng_end; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_DISABLED;

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			cstart = op_screenshot_gif_start;
			cend = op_screenshot_gif_end;
			break;
		case MM_APNG:
			cstart = op_screenshot_apng_start;
			cend = op_screenshot_apng_end;
			break;
		default:
			return;
	}
	for (i = cstart; i <= cend; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_STRING|IT_CVAR;
#endif
}

void Moviemode_option_Onchange(void)
{
#if 0
	OP_ScreenshotOptionsMenu[op_movie_folder].status =
		(cv_movie_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
#endif
}

void Addons_option_Onchange(void)
{
#if 0
	OP_AddonsOptionsMenu[op_addons_folder].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
#endif
}

void M_SortServerList(void)
{
#if 0
#ifndef NONET
	switch(cv_serversort.value)
	{
	case 0:		// Ping.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_time);
		break;
	case 1:		// Modified state.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_modified);
		break;
	case 2:		// Most players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer_reverse);
		break;
	case 3:		// Least players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer);
		break;
	case 4:		// Max players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_maxplayer_reverse);
		break;
	case 5:		// Gametype.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametype);
		break;
	}
#endif
#endif
}

// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

void M_AddMenuColor(UINT16 color) {
	menucolor_t *c;

	// SRB2Kart: I do not understand vanilla doesn't need this but WE do???!?!??!
	if (!skincolors[color].accessible) {
		return;
	}

	if (color >= numskincolors) {
		CONS_Printf("M_AddMenuColor: color %d does not exist.",color);
		return;
	}

	c = (menucolor_t *)malloc(sizeof(menucolor_t));
	c->color = color;
	if (menucolorhead == NULL) {
		c->next = c;
		c->prev = c;
		menucolorhead = c;
		menucolortail = c;
	} else {
		c->next = menucolorhead;
		c->prev = menucolortail;
		menucolortail->next = c;
		menucolorhead->prev = c;
		menucolortail = c;
	}
}

void M_MoveColorBefore(UINT16 color, UINT16 targ) {
	menucolor_t *look, *c = NULL, *t = NULL;

	if (color == targ)
		return;
	if (color >= numskincolors) {
		CONS_Printf("M_MoveColorBefore: color %d does not exist.",color);
		return;
	}
	if (targ >= numskincolors) {
		CONS_Printf("M_MoveColorBefore: target color %d does not exist.",targ);
		return;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			c = look;
		else if (look->color == targ)
			t = look;
		if (c != NULL && t != NULL)
			break;
		if (look==menucolortail)
			return;
	}

	if (c == t->prev)
		return;

	if (t==menucolorhead)
		menucolorhead = c;
	if (c==menucolortail)
		menucolortail = c->prev;

	c->prev->next = c->next;
	c->next->prev = c->prev;

	c->prev = t->prev;
	c->next = t;
	t->prev->next = c;
	t->prev = c;
}

void M_MoveColorAfter(UINT16 color, UINT16 targ) {
	menucolor_t *look, *c = NULL, *t = NULL;

	if (color == targ)
		return;
	if (color >= numskincolors) {
		CONS_Printf("M_MoveColorAfter: color %d does not exist.\n",color);
		return;
	}
	if (targ >= numskincolors) {
		CONS_Printf("M_MoveColorAfter: target color %d does not exist.\n",targ);
		return;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			c = look;
		else if (look->color == targ)
			t = look;
		if (c != NULL && t != NULL)
			break;
		if (look==menucolortail)
			return;
	}

	if (t == c->prev)
		return;

	if (t==menucolortail)
		menucolortail = c;
	else if (c==menucolortail)
		menucolortail = c->prev;

	c->prev->next = c->next;
	c->next->prev = c->prev;

	c->next = t->next;
	c->prev = t;
	t->next->prev = c;
	t->next = c;
}

UINT16 M_GetColorBefore(UINT16 color) {
	menucolor_t *look;

	if (color >= numskincolors) {
		CONS_Printf("M_GetColorBefore: color %d does not exist.\n",color);
		return 0;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			return look->prev->color;
		if (look==menucolortail)
			return 0;
	}
}

UINT16 M_GetColorAfter(UINT16 color) {
	menucolor_t *look;

	if (color >= numskincolors) {
		CONS_Printf("M_GetColorAfter: color %d does not exist.\n",color);
		return 0;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			return look->next->color;
		if (look==menucolortail)
			return 0;
	}
}

void M_InitPlayerSetupColors(void) {
	UINT8 i;
	numskincolors = SKINCOLOR_FIRSTFREESLOT;
	menucolorhead = menucolortail = NULL;
	for (i=0; i<numskincolors; i++)
		M_AddMenuColor(i);
}

void M_FreePlayerSetupColors(void) {
	menucolor_t *look = menucolorhead, *tmp;

	if (menucolorhead==NULL)
		return;

	while (true) {
		if (look != menucolortail) {
			tmp = look;
			look = look->next;
			free(tmp);
		} else {
			free(look);
			return;
		}
	}

	menucolorhead = menucolortail = NULL;
}

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

	// Backspace sets values to default value
	if (choice == -1)
	{
		CV_Set(cv, cv->defaultvalue);
		return;
	}

	choice = (choice<<1) - 1;

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
		|| ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_INVISSLIDER)
		|| ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		CV_SetValue(cv, cv->value+choice);
	}
	else if (cv->flags & CV_FLOAT)
	{
		char s[20];
		sprintf(s, "%f", FIXED_TO_FLOAT(cv->value) + (choice) * (1.0f / 16.0f));
		CV_Set(cv, s);
	}
	else
	{
#ifndef NONET
		if (cv == &cv_nettimeout || cv == &cv_jointimeout)
			choice *= (TICRATE/7);
		else if (cv == &cv_maxsend)
			choice *= 512;
		else if (cv == &cv_maxping)
			choice *= 50;
#endif

		CV_AddValue(cv, choice);
	}
}

static boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
	char buf[MAXSTRINGLENGTH];
	size_t len;

	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				S_StartSound(NULL, sfx_s3k5b); // Tails
				M_Memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		case KEY_DEL:
			if (cv->string[0])
			{
				S_StartSound(NULL, sfx_s3k5b); // Tails
				CV_Set(cv, "");
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					S_StartSound(NULL, sfx_s3k5b); // Tails
					M_Memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}
	return false;
}

static void M_NextOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		((consvar_t *)currentMenu->menuitems[itemOn].itemaction)->value = 0;

	do
	{
		if (itemOn + 1 > currentMenu->numitems - 1)
			itemOn = 0;
		else
			itemOn++;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

	M_UpdateMenuBGImage(false);
}

static void M_PrevOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		((consvar_t *)currentMenu->menuitems[itemOn].itemaction)->value = 0;

	do
	{
		if (!itemOn)
			itemOn = currentMenu->numitems - 1;
		else
			itemOn--;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

	M_UpdateMenuBGImage(false);
}

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	INT32 ch = -1;
//	INT32 i;
	static tic_t joywait = 0, mousewait = 0;
	static INT32 pjoyx = 0, pjoyy = 0;
	static INT32 pmousex = 0, pmousey = 0;
	static INT32 lastx = 0, lasty = 0;
	void (*routine)(INT32 choice); // for some casting problem

	if (dedicated || (demo.playback && demo.title)
	|| gamestate == GS_INTRO || gamestate == GS_CUTSCENE || gamestate == GS_GAMEEND
	|| gamestate == GS_CREDITS || gamestate == GS_EVALUATION)
		return false;

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}
	else if (ev->type == ev_keydown)
	{
		ch = ev->data1;

		// added 5-2-98 remap virtual keys (mouse & joystick buttons)
		switch (ch)
		{
			case KEY_MOUSE1:
				//case KEY_JOY1:
				//case KEY_JOY1 + 2:
				ch = KEY_ENTER;
				break;
				/*case KEY_JOY1 + 3: // Brake can function as 'n' for message boxes now.
					ch = 'n';
					break;*/
			case KEY_MOUSE1 + 1:
				//case KEY_JOY1 + 1:
				ch = KEY_BACKSPACE;
				break;
			case KEY_HAT1:
				ch = KEY_UPARROW;
				break;
			case KEY_HAT1 + 1:
				ch = KEY_DOWNARROW;
				break;
			case KEY_HAT1 + 2:
				ch = KEY_LEFTARROW;
				break;
			case KEY_HAT1 + 3:
				ch = KEY_RIGHTARROW;
				break;
		}
	}
	else if (menuactive)
	{
		if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			const INT32 jdeadzone = ((JOYAXISRANGE-1) * cv_deadzone[0].value) >> FRACBITS;
			if (ev->data3 != INT32_MAX)
			{
				if (Joystick[0].bGamepadStyle || abs(ev->data3) > jdeadzone)
				{
					if (ev->data3 < 0 && pjoyy >= 0)
					{
						ch = KEY_UPARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					else if (ev->data3 > 0 && pjoyy <= 0)
					{
						ch = KEY_DOWNARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					pjoyy = ev->data3;
				}
				else
					pjoyy = 0;
			}

			if (ev->data2 != INT32_MAX)
			{
				if (Joystick[0].bGamepadStyle || abs(ev->data2) > jdeadzone)
				{
					if (ev->data2 < 0 && pjoyx >= 0)
					{
						ch = KEY_LEFTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					else if (ev->data2 > 0 && pjoyx <= 0)
					{
						ch = KEY_RIGHTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					pjoyx = ev->data2;
				}
				else
					pjoyx = 0;
			}
		}
		else if (ev->type == ev_mouse && mousewait < I_GetTime())
		{
			pmousey += ev->data3;
			if (pmousey < lasty-30)
			{
				ch = KEY_DOWNARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty -= 30;
			}
			else if (pmousey > lasty + 30)
			{
				ch = KEY_UPARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty += 30;
			}

			pmousex += ev->data2;
			if (pmousex < lastx - 30)
			{
				ch = KEY_LEFTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx -= 30;
			}
			else if (pmousex > lastx+30)
			{
				ch = KEY_RIGHTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx += 30;
			}
		}
	}

	if (ch == -1)
		return false;
	else if (ch == gamecontrol[0][gc_systemmenu][0] || ch == gamecontrol[0][gc_systemmenu][1]) // allow remappable ESC key
		ch = KEY_ESCAPE;
	else if ((ch == gamecontrol[0][gc_accelerate][0] || ch == gamecontrol[0][gc_accelerate][1])  && ch >= KEY_MOUSE1)
		ch = KEY_ENTER;

	// F-Keys
	if (!menuactive)
	{
		noFurtherInput = true;

		switch (ch)
		{
#if 0
			case KEY_F1: // Help key
				Command_Manual_f();
				return true;

			case KEY_F2: // Empty
				return true;

			case KEY_F3: // Toggle HUD
				CV_SetValue(&cv_showhud, !cv_showhud.value);
				return true;

			case KEY_F4: // Sound Volume
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				currentMenu = &OP_SoundOptionsDef;
				itemOn = 0;
				return true;

#ifndef DC
			case KEY_F5: // Video Mode
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_VideoModeMenu(0);
				return true;
#endif

			case KEY_F6: // Empty
				return true;

			case KEY_F7: // Options
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_SetupNextMenu(&OP_MainDef, false);
				return true;

			// Screenshots on F8 now handled elsewhere
			// Same with Moviemode on F9

			case KEY_F10: // Quit SRB2
				M_QuitSRB2(0);
				return true;

			case KEY_F11: // Fullscreen
				CV_AddValue(&cv_fullscreen, 1);
				return true;

			// Spymode on F12 handled in game logic
#endif

			case KEY_ESCAPE: // Pop up menu
				if (chat_on)
				{
					HU_clearChatChars();
					chat_on = false;
				}
				else
					M_StartControlPanel();
				return true;
		}
		noFurtherInput = false; // turns out we didn't care
		return false;
	}

	if ((ch == gamecontrol[0][gc_brake][0] || ch == gamecontrol[0][gc_brake][1]) && ch >= KEY_MOUSE1) // do this here, otherwise brake opens the menu mid-game
		ch = KEY_ESCAPE;

	routine = currentMenu->menuitems[itemOn].itemaction;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		if (shiftdown && ch >= 32 && ch <= 127)
			ch = shiftxform[ch];
		routine(ch);
		return true;
	}

	// Handle menu-specific input handling. If this returns true we skip regular input handling.
	if (currentMenu->inputroutine)
	{
		INT32 res = 0;
		if (shiftdown && ch >= 32 && ch <= 127)
			ch = shiftxform[ch];

		res = currentMenu->inputroutine(ch);

		if (res)
			return true;
	}

	if (currentMenu->menuitems[itemOn].status == IT_MSGHANDLER)
	{
		if (currentMenu->menuitems[itemOn].mvar1 != MM_EVENTHANDLER)
		{
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER)
			{
				if (routine)
					routine(ch);
				M_StopMessage(0);
				noFurtherInput = true;
				return true;
			}
			return true;
		}
		else
		{
			// dirty hack: for customising controls, I want only buttons/keys, not moves
			if (ev->type == ev_mouse
				|| ev->type == ev_joystick
				|| ev->type == ev_joystick2
				|| ev->type == ev_joystick3
				|| ev->type == ev_joystick4)
				return true;
			if (routine)
			{
				void (*otherroutine)(event_t *sev) = currentMenu->menuitems[itemOn].itemaction;
				otherroutine(ev); //Alam: what a hack
			}
			return true;
		}
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING)
		{

			if (shiftdown && ch >= 32 && ch <= 127)
				ch = shiftxform[ch];
			if (M_ChangeStringCvar(ch))
				return true;
			else
				routine = NULL;
		}
		else
			routine = M_ChangeCvar;
	}

	if (currentMenu == &PAUSE_PlaybackMenuDef && !con_destlines)
	{
		playback_last_menu_interaction_leveltime = leveltime;
		// Flip left/right with up/down for the playback menu, since it's a horizontal icon row.
		switch (ch)
		{
			case KEY_LEFTARROW: ch = KEY_UPARROW; break;
			case KEY_UPARROW: ch = KEY_RIGHTARROW; break;
			case KEY_RIGHTARROW: ch = KEY_DOWNARROW; break;
			case KEY_DOWNARROW: ch = KEY_LEFTARROW; break;

			// arbitrary keyboard shortcuts because fuck you

			case '\'':	// toggle freecam
				M_PlaybackToggleFreecam(0);
				break;

			case ']':	// ffw / advance frame (depends on if paused or not)
				if (paused)
					M_PlaybackAdvance(0);
				else
					M_PlaybackFastForward(0);
				break;

			case '[':	// rewind /backupframe, uses the same function
				M_PlaybackRewind(0);
				break;

			case '\\':	// pause
				M_PlaybackPause(0);
				break;

			// viewpoints, an annoyance (tm)
			case '-':	// viewpoint minus
				M_PlaybackSetViews(-1);	// yeah lol.
				break;

			case '=':	// viewpoint plus
				M_PlaybackSetViews(1);	// yeah lol.
				break;

			// switch viewpoints:
			case '1':	// viewpoint for p1 (also f12)
				// maximum laziness:
				if (!demo.freecam)
					G_AdjustView(1, 1, true);
				break;
			case '2':	// viewpoint for p2
				if (!demo.freecam)
					G_AdjustView(2, 1, true);
				break;
			case '3':	// viewpoint for p3
				if (!demo.freecam)
					G_AdjustView(3, 1, true);
				break;
			case '4':	// viewpoint for p4
				if (!demo.freecam)
					G_AdjustView(4, 1, true);
				break;

			default: break;
		}
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_s3k5b);
			return true;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_s3k5b);
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
#if 0
				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
#endif
					S_StartSound(NULL, sfx_s3k5b);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
#if 0
				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
#endif
					S_StartSound(NULL, sfx_s3k5b);
				routine(1);
			}
			return true;

		case KEY_ENTER:
			noFurtherInput = true;
			currentMenu->lastOn = itemOn;

#if 0
			if (currentMenu == &PAUSE_PlaybackMenuDef)
			{
				boolean held = (boolean)playback_enterheld;
				if (held)
					return true;
				playback_enterheld = 3;
			}
#endif

			if (routine)
			{
				S_StartSound(NULL, sfx_s3k5b);

				if (((currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_CALL
				 || (currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_SUBMENU)
				 && (currentMenu->menuitems[itemOn].status & IT_CALLTYPE))
				{
					if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && majormods)
					{
						M_StartMessage(M_GetText("This cannot be done with complex addons\nor in a cheated game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
						return true;
					}
				}

				switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
				{
					case IT_CVAR:
					case IT_ARROWS:
						routine(1); // right arrow
						break;
					case IT_CALL:
						routine(itemOn);
						break;
					case IT_SUBMENU:
						currentMenu->lastOn = itemOn;
						M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction, false);
						break;
				}
			}
			return true;

		case KEY_ESCAPE:
		//case KEY_JOY1 + 2:
			M_GoBack(0);
			return true;

		case KEY_BACKSPACE:
#if 0
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].mvar1);
				S_StartSound(NULL, sfx_shldls);
				return true;
			}
#endif

			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

				if (cv == &cv_chooseskin
					|| cv == &cv_dummystaff
					/*
					|| cv == &cv_nextmap
					|| cv == &cv_newgametype
					*/
					)
					return true;

#if 0
				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
#endif
					S_StartSound(NULL, sfx_s3k5b);
				routine(-1);
				return true;
			}

			return false;

		default:
			break;
	}

	return true;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	// intro might call this repeatedly
	if (menuactive)
	{
		CON_ToggleOff(); // move away console
		return;
	}

	if (gamestate == GS_TITLESCREEN) // Set up menu state
	{
		G_SetGamestate(GS_MENU);

		gameaction = ga_nothing;
		paused = false;
		CON_ToggleOff();

		S_ChangeMusicInternal("menu", true);
	}

	menuactive = true;

	if (demo.playback)
	{
		currentMenu = &PAUSE_PlaybackMenuDef;
	}
	else if (!Playing())
	{
		currentMenu = &MainDef;
		itemOn = 0;
	}
	else
	{
		// For now let's just always open the same pause menu.
		M_OpenPauseMenu();
	}

#if 0
	else if (modeattacking)
	{
		currentMenu = &MAPauseDef;
		itemOn = mapause_continue;
	}
	else if (!(netgame || multiplayer)) // Single Player
	{
		if (gamestate != GS_LEVEL) // intermission, so gray out stuff.
			SPauseMenu[spause_retry].status = IT_GRAYEDOUT;
		else
		{
			//INT32 numlives = 2;

			/*if (&players[consoleplayer])
			{
				numlives = players[consoleplayer].lives;
				if (players[consoleplayer].playerstate != PST_LIVE)
					++numlives;
			}

			// The list of things that can disable retrying is (was?) a little too complex
			// for me to want to use the short if statement syntax
			if (numlives <= 1 || G_IsSpecialStage(gamemap))
				SPauseMenu[spause_retry].status = (IT_GRAYEDOUT);
			else*/
				SPauseMenu[spause_retry].status = (IT_STRING | IT_CALL);
		}

		currentMenu = &SPauseDef;
		itemOn = spause_continue;
	}
	else // multiplayer
	{
		MPauseMenu[mpause_switchmap].status = IT_DISABLED;
		MPauseMenu[mpause_addons].status = IT_DISABLED;
		MPauseMenu[mpause_scramble].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit2].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit3].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit4].status = IT_DISABLED;
		MPauseMenu[mpause_spectate].status = IT_DISABLED;
		MPauseMenu[mpause_entergame].status = IT_DISABLED;
		MPauseMenu[mpause_canceljoin].status = IT_DISABLED;
		MPauseMenu[mpause_switchteam].status = IT_DISABLED;
		MPauseMenu[mpause_switchspectate].status = IT_DISABLED;
		MPauseMenu[mpause_psetup].status = IT_DISABLED;
		MISC_ChangeTeamMenu[0].status = IT_DISABLED;
		MISC_ChangeSpectateMenu[0].status = IT_DISABLED;
		// Reset these in case splitscreen messes things up
		MPauseMenu[mpause_switchteam].mvar1 = 48;
		MPauseMenu[mpause_switchspectate].mvar1 = 48;
		MPauseMenu[mpause_options].mvar1 = 64;
		MPauseMenu[mpause_title].mvar1 = 80;
		MPauseMenu[mpause_quit].mvar1 = 88;
		Dummymenuplayer_OnChange();

		if ((server || IsPlayerAdmin(consoleplayer)))
		{
			MPauseMenu[mpause_switchmap].status = IT_STRING | IT_CALL;
			MPauseMenu[mpause_addons].status = IT_STRING | IT_CALL;
			if (G_GametypeHasTeams())
				MPauseMenu[mpause_scramble].status = IT_STRING | IT_SUBMENU;
		}

		if (splitscreen)
		{
			MPauseMenu[mpause_psetupsplit].status = MPauseMenu[mpause_psetupsplit2].status = IT_STRING | IT_CALL;
			MISC_ChangeTeamMenu[0].status = MISC_ChangeSpectateMenu[0].status = IT_STRING|IT_CVAR;

			if (netgame)
			{
				if (G_GametypeHasTeams())
				{
					MPauseMenu[mpause_switchteam].status = IT_STRING | IT_SUBMENU;
					MPauseMenu[mpause_switchteam].mvar1 += ((splitscreen+1) * 8);
					MPauseMenu[mpause_options].mvar1 += 8;
					MPauseMenu[mpause_title].mvar1 += 8;
					MPauseMenu[mpause_quit].mvar1 += 8;
				}
				else if (G_GametypeHasSpectators())
				{
					MPauseMenu[mpause_switchspectate].status = IT_STRING | IT_SUBMENU;
					MPauseMenu[mpause_switchspectate].mvar1 += ((splitscreen+1) * 8);
					MPauseMenu[mpause_options].mvar1 += 8;
					MPauseMenu[mpause_title].mvar1 += 8;
					MPauseMenu[mpause_quit].mvar1 += 8;
				}
			}

			if (splitscreen > 1)
			{
				MPauseMenu[mpause_psetupsplit3].status = IT_STRING | IT_CALL;

				MPauseMenu[mpause_options].mvar1 += 8;
				MPauseMenu[mpause_title].mvar1 += 8;
				MPauseMenu[mpause_quit].mvar1 += 8;

				if (splitscreen > 2)
				{
					MPauseMenu[mpause_psetupsplit4].status = IT_STRING | IT_CALL;
					MPauseMenu[mpause_options].mvar1 += 8;
					MPauseMenu[mpause_title].mvar1 += 8;
					MPauseMenu[mpause_quit].mvar1 += 8;
				}
			}
		}
		else
		{
			MPauseMenu[mpause_psetup].status = IT_STRING | IT_CALL;

			if (G_GametypeHasTeams())
				MPauseMenu[mpause_switchteam].status = IT_STRING | IT_SUBMENU;
			else if (G_GametypeHasSpectators())
			{
				if (!players[consoleplayer].spectator)
					MPauseMenu[mpause_spectate].status = IT_STRING | IT_CALL;
				else if (players[consoleplayer].pflags & PF_WANTSTOJOIN)
					MPauseMenu[mpause_canceljoin].status = IT_STRING | IT_CALL;
				else
					MPauseMenu[mpause_entergame].status = IT_STRING | IT_CALL;
			}
			else // in this odd case, we still want something to be on the menu even if it's useless
				MPauseMenu[mpause_spectate].status = IT_GRAYEDOUT;
		}

		currentMenu = &MPauseDef;
		itemOn = mpause_continue;
	}
#endif

	CON_ToggleOff(); // move away console
}

//
// M_ClearMenus
//
void M_ClearMenus(boolean callexitmenufunc)
{
	if (!menuactive)
		return;

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

	if (currentMenu == &MessageDef) // Oh sod off!
		currentMenu = &MainDef; // Not like it matters

	if (gamestate == GS_MENU) // Back to title screen
		D_StartTitle();

	menuactive = false;
}

void M_SelectableClearMenus(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef, boolean notransition)
{
	INT16 i;

	if (!notransition)
	{
		if (currentMenu->transitionID == menudef->transitionID
			&& currentMenu->transitionTics)
		{
			menutransition.startmenu = currentMenu;
			menutransition.endmenu = menudef;

			menutransition.tics = 0;
			menutransition.dest = currentMenu->transitionTics;
			menutransition.in = false;
			return; // Don't change menu yet, the transition will call this again
		}
		else if (gamestate == GS_MENU)
		{
			menuwipe = true;
			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();
			F_RunWipe(wipedefs[wipe_menu_toblack], false, "FADEMAP0", false, false);
		}
	}

	if (currentMenu->quitroutine)
	{
		// If you're going from a menu to itself, why are you running the quitroutine? You're not quitting it! -SH
		if (currentMenu != menudef && !currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}

	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = currentMenu->numitems - 1;

	// the curent item can be disabled,
	// this code go up until an enabled item found
	if ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE)
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if ((currentMenu->menuitems[i].status & IT_TYPE) != IT_SPACE)
			{
				itemOn = i;
				break;
			}
		}
	}

	M_UpdateMenuBGImage(false);
}

void M_GoBack(INT32 choice)
{
	(void)choice;

	noFurtherInput = true;
	currentMenu->lastOn = itemOn;

	if (currentMenu->prevMenu)
	{
		//If we entered the game search menu, but didn't enter a game,
		//make sure the game doesn't still think we're in a netgame.
		if (!Playing() && netgame && multiplayer)
		{
			netgame = false;
			multiplayer = false;
		}

		M_SetupNextMenu(currentMenu->prevMenu, false);
	}
	else
		M_ClearMenus(true);

	S_StartSound(NULL, sfx_s3k5b);
}

//
// M_Ticker
//
void M_Ticker(void)
{
	if (menutransition.tics != 0 || menutransition.dest != 0)
	{
		noFurtherInput = true;

		if (menutransition.tics < menutransition.dest)
			menutransition.tics++;
		else if (menutransition.tics > menutransition.dest)
			menutransition.tics--;

		// If dest is non-zero, we've started transition and want to switch menus
		// If dest is zero, we're mid-transition and want to end it
		if (menutransition.tics == menutransition.dest
			&& menutransition.endmenu != NULL
			&& currentMenu != menutransition.endmenu
		)
		{
			if (menutransition.startmenu->transitionID == menutransition.endmenu->transitionID
				&& menutransition.endmenu->transitionTics)
			{
				menutransition.tics = menutransition.endmenu->transitionTics;
				menutransition.dest = 0;
				menutransition.in = true;
			}
			else if (gamestate == GS_MENU)
			{
				memset(&menutransition, 0, sizeof(menutransition));

				menuwipe = true;
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(wipedefs[wipe_menu_toblack], false, "FADEMAP0", false, false);
			}

			M_SetupNextMenu(menutransition.endmenu, true);
		}
	}
	else
	{
		if (menuwipe)
		{
			// try not to let people input during the fadeout
			noFurtherInput = true;
		}
		else
		{
			// reset input trigger
			noFurtherInput = false;
		}
	}

	if (currentMenu->tickroutine)
		currentMenu->tickroutine();

	if (dedicated)
		return;

	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8;

#if 0
	if (currentMenu == &PAUSE_PlaybackMenuDef)
	{
		if (playback_enterheld > 0)
			playback_enterheld--;
	}
	else
		playback_enterheld = 0;

	//added : 30-01-98 : test mode for five seconds
	if (vidm_testingmode > 0)
	{
		// restore the previous video mode
		if (--vidm_testingmode == 0)
			setmodeneeded = vidm_previousmode + 1;
	}
#endif
}

//
// M_Init
//
void M_Init(void)
{
#if 0
	CV_RegisterVar(&cv_nextmap);
#endif
	CV_RegisterVar(&cv_chooseskin);
	CV_RegisterVar(&cv_autorecord);

	if (dedicated)
		return;

	//COM_AddCommand("manual", Command_Manual_f);

	// Menu hacks
	CV_RegisterVar(&cv_dummymenuplayer);
	CV_RegisterVar(&cv_dummyteam);
	CV_RegisterVar(&cv_dummyspectate);
	CV_RegisterVar(&cv_dummyscramble);
	CV_RegisterVar(&cv_dummystaff);
	CV_RegisterVar(&cv_dummygametype);
	CV_RegisterVar(&cv_dummyip);

	M_UpdateMenuBGImage(true);

#if 0
#ifdef HWRENDER
	// Permanently hide some options based on render mode
	if (rendermode == render_soft)
		OP_VideoOptionsMenu[op_video_ogl].status =
			OP_VideoOptionsMenu[op_video_kartman].status =
			OP_VideoOptionsMenu[op_video_md2]    .status = IT_DISABLED;
#endif
#endif

#ifndef NONET
	CV_RegisterVar(&cv_serversort);
#endif
}

// ==================================================
// MESSAGE BOX (aka: a hacked, cobbled together menu)
// ==================================================
// Because this is just a "fake" menu, these definitions are not with the others
static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0, NULL, NULL, NULL, NULL, 0, 0}
};

menu_t MessageDef =
{
	1,                  // # of menu items
	NULL,               // previous menu       (TO HACK)
	0,                  // lastOn, flags       (TO HACK)
	MessageMenu,        // menuitem_t ->
	0, 0,               // x, y                (TO HACK)
	0, 0,               // transition tics
	M_DrawMessageMenu,  // drawing routine ->
	NULL,               // ticker routine
	NULL,               // quit routine
	NULL				// input routine
};

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

// default message handler
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype)
{
	size_t max = 0, start = 0, i, strlines;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, colors, etc. but who cares.
	strlines = 0;
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
		}
		else if (message[i] == '\n')
		{
			strlines = i;
			start = 0;
			max = 0;
			continue;
		}
		else
			max += 8;

		// Start trying to wrap if presumed length exceeds the screen width.
		if (max >= BASEVIDWIDTH && start > 0)
		{
			message[start] = '\n';
			max -= (start-strlines)*8;
			strlines = start;
			start = 0;
		}
	}

	start = 0;
	max = 0;

	M_StartControlPanel(); // can't put menuactive to true

	if (currentMenu == &MessageDef) // Prevent recursion
		MessageDef.prevMenu = ((demo.playback) ? &PAUSE_PlaybackMenuDef : &MainDef);
	else
		MessageDef.prevMenu = currentMenu;

	MessageDef.menuitems[0].text     = message;
	MessageDef.menuitems[0].mvar1 = (UINT8)itemtype;
	if (!routine && itemtype != MM_NOTHING) itemtype = MM_NOTHING;
	switch (itemtype)
	{
		case MM_NOTHING:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = M_StopMessage;
			break;
		case MM_YESNO:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
		case MM_EVENTHANDLER:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
	}
	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i;
				i = (size_t)-1; //added : 07-02-98 : damned!
				start++;
				break;
			}
		}

		if (i == strlen(message+start))
			start += i;
	}

	MessageDef.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	MessageDef.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	MessageDef.lastOn = (INT16)((strlines<<8)+max);

	//M_SetupNextMenu();
	currentMenu = &MessageDef;
	itemOn = 0;
}

void M_StopMessage(INT32 choice)
{
	(void)choice;
	if (menuactive)
		M_SetupNextMenu(MessageDef.prevMenu, true);
}

// =========
// IMAGEDEFS
// =========

// Handles the ImageDefs.  Just a specialized function that
// uses left and right movement.
void M_HandleImageDef(INT32 choice)
{
	boolean exitmenu = false;

	switch (choice)
	{
		case KEY_RIGHTARROW:
			if (itemOn >= (INT16)(currentMenu->numitems-1))
				break;
			S_StartSound(NULL, sfx_s3k5b);
			itemOn++;
			break;

		case KEY_LEFTARROW:
			if (!itemOn)
				break;

			S_StartSound(NULL, sfx_s3k5b);
			itemOn--;
			break;

		case KEY_ESCAPE:
		case KEY_ENTER:
			exitmenu = true;
			break;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

// =========
// MAIN MENU
// =========

// Quit Game
static INT32 quitsounds[] =
{
	// holy shit we're changing things up!
	// srb2kart: you ain't seen nothing yet
	sfx_kc2e,
	sfx_kc2f,
	sfx_cdfm01,
	sfx_ddash,
	sfx_s3ka2,
	sfx_s3k49,
	sfx_slip,
	sfx_tossed,
	sfx_s3k7b,
	sfx_itrolf,
	sfx_itrole,
	sfx_cdpcm9,
	sfx_s3k4e,
	sfx_s259,
	sfx_3db06,
	sfx_s3k3a,
	sfx_peel,
	sfx_cdfm28,
	sfx_s3k96,
	sfx_s3kc0s,
	sfx_cdfm39,
	sfx_hogbom,
	sfx_kc5a,
	sfx_kc46,
	sfx_s3k92,
	sfx_s3k42,
	sfx_kpogos,
	sfx_screec
};

void M_QuitResponse(INT32 ch)
{
	tic_t ptime;
	INT32 mrand;

	if (ch != 'y' && ch != KEY_ENTER)
		return;

	if (!(netgame || cv_debug))
	{
		mrand = M_RandomKey(sizeof(quitsounds) / sizeof(INT32));
		if (quitsounds[mrand])
			S_StartSound(NULL, quitsounds[mrand]);

		//added : 12-02-98: do that instead of I_WaitVbl which does not work
		ptime = I_GetTime() + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
		while (ptime > I_GetTime())
		{
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_CACHE)); // Demo 3 Quit Screen Tails 06-16-2001
			I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
			I_Sleep();
		}
	}

	I_Quit();
}

void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	(void)choice;
	M_StartMessage("Are you sure you want to quit playing?\n\n(Press 'Y' to exit)", M_QuitResponse, MM_YESNO);
}

// =========
// PLAY MENU
// =========

// Character Select!
struct setup_chargrid_s setup_chargrid[9][9];
setup_player_t setup_player[MAXSPLITSCREENPLAYERS];
struct setup_explosions_s setup_explosions[48];

UINT8 setup_numplayers = 0;
tic_t setup_animcounter = 0;

void M_CharacterSelectInit(INT32 choice)
{
	UINT8 i, j;

	(void)choice;

	memset(setup_chargrid, -1, sizeof(setup_chargrid));
	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
			setup_chargrid[i][j].numskins = 0;
	}

	memset(setup_player, 0, sizeof(setup_player));
	setup_player[0].mdepth = CSSTEP_CHARS;
	setup_numplayers = 1;

	memset(setup_explosions, 0, sizeof(setup_explosions));
	setup_animcounter = 0;

	for (i = 0; i < numskins; i++)
	{
		UINT8 x = skins[i].kartspeed-1;
		UINT8 y = skins[i].kartweight-1;

		if (setup_chargrid[x][y].numskins >= MAXCLONES)
			CONS_Alert(CONS_ERROR, "Max character alts reached for %d,%d\n", x+1, y+1);
		else
		{
			setup_chargrid[x][y].skinlist[setup_chargrid[x][y].numskins] = i;
			setup_chargrid[x][y].numskins++;
		}

		for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
		{
			if (!strcmp(cv_skin[j].string, skins[i].name))
			{
				setup_player[j].gridx = x;
				setup_player[j].gridy = y;
				setup_player[j].color = skins[i].prefcolor;
			}
		}
	}

	M_SetupNextMenu(&PLAY_CharSelectDef, false);
}

static void M_SetupReadyExplosions(setup_player_t *p)
{
	UINT8 i, j;
	UINT8 e = 0;

	while (setup_explosions[e].tics)
	{
		e++;
		if (e == CSEXPLOSIONS)
			return;
	}

	for (i = 0; i < 3; i++)
	{
		UINT8 t = 5 + (i*2);
		UINT8 offset = (i+1);

		for (j = 0; j < 4; j++)
		{
			SINT8 x = p->gridx, y = p->gridy;

			switch (j)
			{
				case 0: x += offset; break;
				case 1: x -= offset; break;
				case 2: y += offset; break;
				case 3: y -= offset; break;
			}

			if ((x < 0 || x > 8) || (y < 0 || y > 8))
				continue;

			setup_explosions[e].tics = t;
			setup_explosions[e].color = p->color;
			setup_explosions[e].x = x;
			setup_explosions[e].y = y;

			while (setup_explosions[e].tics)
			{
				e++;
				if (e == CSEXPLOSIONS)
					return;
			}
		}
	}
}

static void M_HandleCharacterGrid(INT32 choice, setup_player_t *p, UINT8 num)
{
	switch (choice)
	{
		case KEY_DOWNARROW:
			p->gridy++;
			if (p->gridy > 8)
				p->gridy = 0;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_UPARROW:
			p->gridy--;
			if (p->gridy < 0)
				p->gridy = 8;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_RIGHTARROW:
			p->gridx++;
			if (p->gridx > 8)
				p->gridx = 0;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_LEFTARROW:
			p->gridx--;
			if (p->gridx < 0)
				p->gridx = 8;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_ENTER:
			if (setup_chargrid[p->gridx][p->gridy].numskins == 0)
				S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
			else
			{
				if (setup_chargrid[p->gridx][p->gridy].numskins == 1)
					p->mdepth = CSSTEP_COLORS; // Skip clones menu
				else
					p->mdepth = CSSTEP_ALTS;

				S_StartSound(NULL, sfx_s3k63);
			}
			break;
		case KEY_ESCAPE:
			if (num == setup_numplayers-1)
			{
				p->mdepth = CSSTEP_NONE;
				S_StartSound(NULL, sfx_s3k5b);
			}
			else
			{
				S_StartSound(NULL, sfx_s3kb2);
			}
			break;
		default:
			break;
	}
}

static void M_HandleCharRotate(INT32 choice, setup_player_t *p)
{
	UINT8 numclones = setup_chargrid[p->gridx][p->gridy].numskins;

	switch (choice)
	{
		case KEY_RIGHTARROW:
			p->clonenum++;
			if (p->clonenum >= numclones)
				p->clonenum = 0;
			p->rotate = CSROTATETICS;
			p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3kc3s);
			break;
		case KEY_LEFTARROW:
			p->clonenum--;
			if (p->clonenum < 0)
				p->clonenum = numclones-1;
			p->rotate = -CSROTATETICS;
			p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3kc3s);
			break;
		case KEY_ENTER:
			p->mdepth = CSSTEP_COLORS;
			S_StartSound(NULL, sfx_s3k63);
			break;
		case KEY_ESCAPE:
			p->mdepth = CSSTEP_CHARS;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		default:
			break;
	}
}

static void M_HandleColorRotate(INT32 choice, setup_player_t *p)
{
	switch (choice)
	{
		case KEY_RIGHTARROW:
			p->color++;
			if (p->color >= numskincolors)
				p->color = 1;
			p->rotate = CSROTATETICS;
			//p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
			break;
		case KEY_LEFTARROW:
			p->color--;
			if (p->color < 1)
				p->color = numskincolors-1;
			p->rotate = -CSROTATETICS;
			//p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
			break;
		case KEY_ENTER:
			p->mdepth = CSSTEP_READY;
			p->delay = TICRATE;
			M_SetupReadyExplosions(p);
			S_StartSound(NULL, sfx_s3k4e);
			break;
		case KEY_ESCAPE:
			if (setup_chargrid[p->gridx][p->gridy].numskins == 1)
				p->mdepth = CSSTEP_CHARS; // Skip clones menu
			else
				p->mdepth = CSSTEP_ALTS;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		default:
			break;
	}
}

void M_CharacterSelectHandler(INT32 choice)
{
	UINT8 i;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		setup_player_t *p = &setup_player[i];

		if (i > 0)
			break; // temp

		if (p->delay == 0)
		{
			switch (p->mdepth)
			{
				case CSSTEP_NONE: // Enter Game
					if (choice == KEY_ENTER && i == setup_numplayers)
					{
						p->mdepth = CSSTEP_CHARS;
						S_StartSound(NULL, sfx_s3k65);
					}
					break;
				case CSSTEP_CHARS: // Character Select grid
					M_HandleCharacterGrid(choice, p, i);
					break;
				case CSSTEP_ALTS: // Select clone
					M_HandleCharRotate(choice, p);
					break;
				case CSSTEP_COLORS: // Select color
					M_HandleColorRotate(choice, p);
					break;
				case CSSTEP_READY:
				default: // Unready
					if (choice == KEY_ESCAPE)
					{
						p->mdepth = CSSTEP_COLORS;
						S_StartSound(NULL, sfx_s3k5b);
					}
					break;
			}
		}

		if (p->mdepth < CSSTEP_ALTS)
			p->clonenum = 0;

		// Just makes it easier to access later
		p->skin = setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum];

		if (p->mdepth < CSSTEP_COLORS)
			p->color = skins[p->skin].prefcolor;
	}

	// Setup new numplayers
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].mdepth == CSSTEP_NONE)
			break;
		else
			setup_numplayers = i+1;
	}

	// If the first player unjoins, then we get outta here
	if (setup_player[0].mdepth == CSSTEP_NONE)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

void M_CharacterSelectTick(void)
{
	UINT8 i;
	boolean setupnext = true;

	setup_animcounter++;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].delay)
			setup_player[i].delay--;

		if (setup_player[i].rotate > 0)
			setup_player[i].rotate--;
		else if (setup_player[i].rotate < 0)
			setup_player[i].rotate++;

		if (i >= setup_numplayers)
			continue;

		if (setup_player[i].mdepth < CSSTEP_READY || setup_player[i].delay > 0)
		{
			// Someone's not ready yet.
			setupnext = false;
		}
	}

	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		if (setup_explosions[i].tics > 0)
			setup_explosions[i].tics--;
	}

	if (setupnext)
	{
		for (i = 0; i < setup_numplayers; i++)
		{
			CV_StealthSet(&cv_skin[i], skins[setup_player[i].skin].name);
			CV_StealthSetValue(&cv_playercolor[i], setup_player[i].color);
		}

		CV_StealthSetValue(&cv_splitplayers, setup_numplayers);
		M_SetupNextMenu(&PLAY_MainDef, false);
	}
}

boolean M_CharacterSelectQuit(void)
{
	M_CharacterSelectInit(0);
	return true;
}

// LEVEL SELECT

//
// M_CanShowLevelInList
//
// Determines whether to show a given map in the various level-select lists.
// Set gt = -1 to ignore gametype.
//
boolean M_CanShowLevelInList(INT16 mapnum, UINT8 gt)
{
	// Does the map exist?
	if (!mapheaderinfo[mapnum])
		return false;

	// Does the map have a name?
	if (!mapheaderinfo[mapnum]->lvlttl[0])
		return false;

	if (M_MapLocked(mapnum+1))
		return false; // not unlocked

	// Should the map be hidden?
	if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU /*&& mapnum+1 != gamemap*/)
		return false;

	if (gt == GT_BATTLE && (mapheaderinfo[mapnum]->typeoflevel & TOL_BATTLE))
		return true;

	if (gt == GT_RACE && (mapheaderinfo[mapnum]->typeoflevel & TOL_RACE))
	{
		if (levellist.selectedcup && levellist.selectedcup->numlevels)
		{
			UINT8 i;

			for (i = 0; i < levellist.selectedcup->numlevels; i++)
			{
				if (mapnum == levellist.selectedcup->levellist[i])
					break;
			}

			if (i == levellist.selectedcup->numlevels)
				return false;
		}

		return true;
	}

	// Hmm? Couldn't decide?
	return false;
}

INT16 M_CountLevelsToShowInList(UINT8 gt)
{
	INT16 mapnum, count = 0;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, gt))
			count++;

	return count;
}

INT16 M_GetFirstLevelInList(UINT8 gt)
{
	INT16 mapnum;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, gt))
			return mapnum;

	return 0;
}

struct cupgrid_s cupgrid;
struct levellist_s levellist;

static void M_LevelSelectScrollDest(void)
{
	UINT16 m = M_CountLevelsToShowInList(levellist.newgametype)-1;

	levellist.dest = (6*levellist.cursor);

	if (levellist.dest < 3)
		levellist.dest = 3;

	if (levellist.dest > (6*m)-3)
		levellist.dest = (6*m)-3;
}

//  Builds the level list we'll be using from the gametype we're choosing and send us to the apropriate menu.
static void M_LevelListFromGametype(INT16 gt)
{
	levellist.newgametype = gt;
	PLAY_CupSelectDef.prevMenu = currentMenu;

	// Obviously go to Cup Select in gametypes that have cups.
	// Use a really long level select in gametypes that don't use cups.

	if (levellist.newgametype == GT_RACE)
	{
		cupheader_t *cup = kartcupheaders;
		UINT8 highestid = 0;

		// Make sure there's valid cups before going to this menu.
		if (cup == NULL)
			I_Error("Can you really call this a racing game, I didn't recieve any Cups on my pillow or anything");

		while (cup)
		{
			if (cup->unlockrequired == -1 || unlockables[cup->unlockrequired].unlocked)
				highestid = cup->id;
			cup = cup->next;
		}

		cupgrid.numpages = (highestid / (CUPMENU_COLUMNS * CUPMENU_ROWS)) + 1;

		PLAY_LevelSelectDef.prevMenu = &PLAY_CupSelectDef;
		M_SetupNextMenu(&PLAY_CupSelectDef, false);

		return;
	}

	// Reset position properly if you go back & forth between gametypes
	if (levellist.selectedcup)
	{
		levellist.cursor = 0;
		levellist.selectedcup = NULL;
	}

	M_LevelSelectScrollDest();
	levellist.y = levellist.dest;

	PLAY_LevelSelectDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PLAY_LevelSelectDef, false);

}

// Init level select for use in local play using the last choice we made.
// For the online MP version used to START HOSTING A GAME, see M_MPSetupNetgameMapSelect()
// (We still use this one midgame)

void M_LevelSelectInit(INT32 choice)
{
	(void)choice;

	levellist.netgame = false;	// Make sure this is reset as we'll only be using this function for offline games!
	cupgrid.netgame = false;	// Ditto

	switch (currentMenu->menuitems[itemOn].mvar1)
	{
		case 0:
			cupgrid.grandprix = false;
			levellist.timeattack = false;
			break;
		case 1:
			cupgrid.grandprix = false;
			levellist.timeattack = true;
			break;
		case 2:
			cupgrid.grandprix = true;
			levellist.timeattack = false;
			break;
		default:
			CONS_Alert(CONS_WARNING, "Bad level select init\n");
			return;
	}

	levellist.newgametype = currentMenu->menuitems[itemOn].mvar2;

	M_LevelListFromGametype(levellist.newgametype);
}

void M_CupSelectHandler(INT32 choice)
{
	cupheader_t *newcup = kartcupheaders;

	while (newcup)
	{
		if (newcup->id == CUPMENU_CURSORID)
			break;
		newcup = newcup->next;
	}

	switch (choice)
	{
		case KEY_RIGHTARROW:
			cupgrid.x++;
			if (cupgrid.x >= CUPMENU_COLUMNS)
			{
				cupgrid.x = 0;
				cupgrid.pageno++;
				if (cupgrid.pageno >= cupgrid.numpages)
					cupgrid.pageno = 0;
			}
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_LEFTARROW:
			cupgrid.x--;
			if (cupgrid.x < 0)
			{
				cupgrid.x = CUPMENU_COLUMNS-1;
				cupgrid.pageno--;
				if (cupgrid.pageno < 0)
					cupgrid.pageno = cupgrid.numpages-1;
			}
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_UPARROW:
			cupgrid.y++;
			if (cupgrid.y >= CUPMENU_ROWS)
				cupgrid.y = 0;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_DOWNARROW:
			cupgrid.y--;
			if (cupgrid.y < 0)
				cupgrid.y = CUPMENU_ROWS-1;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_ENTER:
			if ((!newcup) || (newcup && newcup->unlockrequired != -1 && !unlockables[newcup->unlockrequired].unlocked))
			{
				S_StartSound(NULL, sfx_s3kb2);
				break;
			}

			if (cupgrid.grandprix == true)
			{
				S_StartSound(NULL, sfx_s3k63);

				// Early fadeout to let the sound finish playing
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

				memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));

				// TODO: game settings screen
				grandprixinfo.gamespeed = KARTSPEED_NORMAL;
				grandprixinfo.masterbots = false;
				grandprixinfo.encore = false;

				grandprixinfo.cup = newcup;

				grandprixinfo.gp = true;
				grandprixinfo.roundnum = 1;
				grandprixinfo.initalize = true;

				paused = false;

				// Don't restart the server if we're already in a game lol
				if (gamestate == GS_MENU)
				{
					SV_StartSinglePlayerServer();
					multiplayer = true; // yeah, SV_StartSinglePlayerServer clobbers this...
					netgame = levellist.netgame;	// ^ ditto.
				}

				D_MapChange(
					grandprixinfo.cup->levellist[0] + 1,
					GT_RACE,
					grandprixinfo.encore,
					true,
					1,
					false,
					false
				);

				M_ClearMenus(true);
			}
			else
			{
				// Keep cursor position if you select the same cup again, reset if it's a different cup
				if (!levellist.selectedcup || newcup->id != levellist.selectedcup->id)
				{
					levellist.cursor = 0;
					levellist.selectedcup = newcup;
				}

				M_LevelSelectScrollDest();
				levellist.y = levellist.dest;

				M_SetupNextMenu(&PLAY_LevelSelectDef, false);
				S_StartSound(NULL, sfx_s3k63);
			}
			break;
		case KEY_ESCAPE:
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu, false);
			else
				M_ClearMenus(true);
			break;
		default:
			break;
	}
}

void M_CupSelectTick(void)
{
	cupgrid.previewanim++;
}

void M_LevelSelectHandler(INT32 choice)
{
	INT16 start = M_GetFirstLevelInList(levellist.newgametype);
	INT16 maxlevels = M_CountLevelsToShowInList(levellist.newgametype);

	if (levellist.y != levellist.dest)
		return;

	switch (choice)
	{
		case KEY_UPARROW:
			levellist.cursor--;
			if (levellist.cursor < 0)
				levellist.cursor = maxlevels-1;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_DOWNARROW:
			levellist.cursor++;
			if (levellist.cursor >= maxlevels)
				levellist.cursor = 0;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_ENTER:
			{
				INT16 map = start;
				INT16 add = levellist.cursor;

				while (add > 0)
				{
					map++;

					while (!M_CanShowLevelInList(map, levellist.newgametype) && map < NUMMAPS)
						map++;

					if (map >= NUMMAPS)
						break;

					add--;
				}

				if (map >= NUMMAPS)
					break;

				levellist.choosemap = map;

				if (levellist.timeattack)
				{
					M_SetupNextMenu(&PLAY_TimeAttackDef, false);
					S_StartSound(NULL, sfx_s3k63);
				}
				else
				{
					if (gamestate == GS_MENU)
					{
						UINT8 ssplayers = cv_splitplayers.value-1;

						netgame = false;
						multiplayer = true;

						strncpy(connectedservername, cv_servername.string, MAXSERVERNAME);

						// Still need to reset devmode
						cv_debug = 0;

						if (demo.playback)
							G_StopDemo();
						if (metalrecording)
							G_StopMetalDemo();

						/*if (levellist.choosemap == 0)
							levellist.choosemap = G_RandMap(G_TOLFlag(levellist.newgametype), -1, false, 0, false, NULL);*/

						if (cv_maxplayers.value < ssplayers+1)
							CV_SetValue(&cv_maxplayers, ssplayers+1);

						if (splitscreen != ssplayers)
						{
							splitscreen = ssplayers;
							SplitScreen_OnChange();
						}

						S_StartSound(NULL, sfx_s3k63);

						paused = false;

						// Early fadeout to let the sound finish playing
						F_WipeStartScreen();
						V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
						F_WipeEndScreen();
						F_RunWipe(wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

						SV_StartSinglePlayerServer();
						multiplayer = true; // yeah, SV_StartSinglePlayerServer clobbers this...
						netgame = levellist.netgame;	// ^ ditto.
					}

					D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_kartencore.value == 1), 1, 1, false, false);

					M_ClearMenus(true);
				}
			}
			break;
		case KEY_ESCAPE:
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu, false);
			else
				M_ClearMenus(true);
			break;
		default:
			break;
	}

	M_LevelSelectScrollDest();
}

void M_LevelSelectTick(void)
{
	UINT8 times = 1 + (abs(levellist.dest - levellist.y) / 21);

	while (times) // increase speed as you're farther away
	{
		if (levellist.y > levellist.dest)
			levellist.y--;
		else if (levellist.y < levellist.dest)
			levellist.y++;

		if (levellist.y == levellist.dest)
			break;

		times--;
	}
}



struct mpmenu_s mpmenu;

// MULTIPLAYER OPTION SELECT

// Use this as a quit routine within the HOST GAME and JOIN BY IP "sub" menus
boolean M_MPResetOpts(void)
{
	UINT8 i = 0;

	for (; i < 3; i++)
		mpmenu.modewinextend[i][0] = 0;	// Undo this

	return true;
}

void M_MPOptSelectInit(INT32 choice)
{
	INT16 arrcpy[3][3] = {{0,68,0}, {0,12,0}, {0,64,0}};
	UINT8 i = 0, j = 0;	// To copy the array into the struct

	(void)choice;

	mpmenu.modechoice = 0;
	mpmenu.ticker = 0;

	for (; i < 3; i++)
		for (j = 0; j < 3; j++)
			mpmenu.modewinextend[i][j] = arrcpy[i][j];	// I miss Lua already

	M_SetupNextMenu(&PLAY_MP_OptSelectDef, false);
}

void M_MPOptSelectTick(void)
{
	UINT8 i = 0;

	// 3 Because we have 3 options in the menu
	for (; i < 3; i++)
	{
		if (mpmenu.modewinextend[i][0])
			mpmenu.modewinextend[i][2] += 8;
		else
			mpmenu.modewinextend[i][2] -= 8;

		mpmenu.modewinextend[i][2] = min(mpmenu.modewinextend[i][1], max(0, mpmenu.modewinextend[i][2]));
		//CONS_Printf("%d - %d,%d,%d\n", i, mpmenu.modewinextend[i][0], mpmenu.modewinextend[i][1], mpmenu.modewinextend[i][2]);
	}
}


// MULTIPLAYER HOST
void M_MPHostInit(INT32 choice)
{

	(void)choice;
	mpmenu.modewinextend[0][0] = 1;
	M_SetupNextMenu(&PLAY_MP_HostDef, true);
}

void M_MPSetupNetgameMapSelect(INT32 choice)
{

	INT16 gt = GT_RACE;
	(void)choice;

	levellist.netgame = true;		// Yep, we'll be starting a netgame.
	cupgrid.netgame = true;			// Ditto
	levellist.timeattack = false;	// Make sure we reset those
	cupgrid.grandprix = false;	// Ditto

	// In case we ever want to add new gamemodes there somehow, have at it!
	switch (cv_dummygametype.value)
	{
		case 1:	// Battle
		{
			gt = GT_BATTLE;
			break;
		}

		default:
		{
			gt = GT_RACE;
			break;
		}
	}

	M_LevelListFromGametype(gt); // Setup the level select.
	// (This will also automatically send us to the apropriate menu)
}

// MULTIPLAYER JOIN BY IP
void M_MPJoinIPInit(INT32 choice)
{

	(void)choice;
	mpmenu.modewinextend[2][0] = 1;
	M_SetupNextMenu(&PLAY_MP_JoinIPDef, true);
}

// Attempts to join a given IP from the menu.
void M_JoinIP(const char *ipa)
{
	if (*(ipa) == '\0')	// Jack shit
	{
		M_StartMessage("Please specify an address.\n", NULL, MM_NOTHING);
		return;
	}

	COM_BufAddText(va("connect \"%s\"\n", ipa));
	M_ClearMenus(true);

	// A little "please wait" message.
	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Connecting to server...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
}

boolean M_JoinIPInputs(INT32 ch)
{
	if (itemOn == 0)	// connect field
	{
		// enter: connect
		if (ch == KEY_ENTER)
		{
			M_JoinIP(cv_dummyip.string);
			return true;
		}
		// ctrl+v -> copy paste!
		else if (ctrldown && (ch == 'v' || ch == 'V'))
		{
			const char *paste = I_ClipboardPaste();
			UINT16 i;
			for (i=0; i < strlen(paste); i++)
				M_ChangeStringCvar(paste[i]);	// We can afford to do this since we're currently on that cvar.

			return true;	// Don't input the V obviously lol.
		}

	}
	else if (currentMenu->numitems - itemOn <= NUMLOGIP && ch == KEY_ENTER)	// On one of the last 3 options for IP rejoining
	{
		UINT8 index = NUMLOGIP - (currentMenu->numitems - itemOn);

		// Is there an address at this part of the table?
		if (joinedIPlist[index][0] && strlen(joinedIPlist[index][0]))
			M_JoinIP(joinedIPlist[index][0]);
		else
			S_StartSound(NULL, sfx_lose);

		return true;	// eat input.
	}

	return false;
}

// MULTIPLAYER ROOM SELECT MENU

void M_MPRoomSelect(INT32 choice)
{

	switch (choice)
	{

		case KEY_LEFTARROW:
		case KEY_RIGHTARROW:
		{

			mpmenu.room = (!mpmenu.room) ? 1 : 0;
			S_StartSound(NULL, sfx_s3k5b);

			break;
		}

		case KEY_ESCAPE:
		{
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu, false);
			else
				M_ClearMenus(true);
			break;
		}

	}
}

void M_MPRoomSelectTick(void)
{
	mpmenu.ticker++;
}

void M_MPRoomSelectInit(INT32 choice)
{
	(void)choice;
	mpmenu.room = 0;
	mpmenu.ticker = 0;

	M_SetupNextMenu(&PLAY_MP_RoomSelectDef, false);
}

// =====================
// PAUSE / IN-GAME MENUS
// =====================
void M_EndModeAttackRun(void)
{
#if 0
	M_ModeAttackEndGame(0);
#endif
}

struct pausemenu_s pausemenu;

// Pause menu!
void M_OpenPauseMenu(void)
{
	boolean singleplayermode = (modeattacking || grandprixinfo.gp);
	currentMenu = &PAUSE_MainDef;

	// Ready the variables
	pausemenu.ticker = 0;

	pausemenu.offset = 0;
	pausemenu.openoffset = 256;
	pausemenu.closing = false;

	itemOn = mpause_continue;	// Make sure we select "RESUME GAME" by default


	// Now the hilarious balancing act of deciding what options should be enabled and which ones shouldn't be!
	// By default, disable anything sensitive:

	PAUSE_Main[mpause_addons].status = IT_DISABLED;
	PAUSE_Main[mpause_switchmap].status = IT_DISABLED;
#ifdef HAVE_DISCORDRPC
	PAUSE_Main[mpause_discordrequests].status = IT_DISABLED;
#endif

	PAUSE_Main[mpause_spectate].status = IT_DISABLED;
	PAUSE_Main[mpause_entergame].status = IT_DISABLED;
	PAUSE_Main[mpause_canceljoin].status = IT_DISABLED;
	PAUSE_Main[mpause_spectatemenu].status = IT_DISABLED;
	PAUSE_Main[mpause_psetup].status = IT_DISABLED;

	Dummymenuplayer_OnChange();	// Make sure the consvar is within bounds of the amount of splitscreen players we have.

	if (!singleplayermode && (server || IsPlayerAdmin(consoleplayer)))
	{
		PAUSE_Main[mpause_switchmap].status = IT_STRING | IT_SUBMENU;
		PAUSE_Main[mpause_addons].status = IT_STRING | IT_CALL;
	}

	if (!singleplayermode)
		PAUSE_Main[mpause_psetup].status = IT_STRING | IT_CALL;

	if (G_GametypeHasSpectators())
	{

		if (splitscreen)
			PAUSE_Main[mpause_spectatemenu].status = IT_STRING|IT_SUBMENU;
		else
		{
			if (!players[consoleplayer].spectator)
				PAUSE_Main[mpause_spectate].status = IT_STRING | IT_CALL;
			else if (players[consoleplayer].pflags & PF_WANTSTOJOIN)
				PAUSE_Main[mpause_canceljoin].status = IT_STRING | IT_CALL;
			else
				PAUSE_Main[mpause_entergame].status = IT_STRING | IT_CALL;
		}
	}



}

void M_QuitPauseMenu(void)
{
	// M_PauseTick actually handles the quitting when it's been long enough.
	pausemenu.closing = true;
	pausemenu.openoffset = 4;
}

void M_PauseTick(void)
{
	pausemenu.offset /= 2;

	if (pausemenu.closing)
	{
		pausemenu.openoffset *= 2;
		if (pausemenu.openoffset > 255)
			M_ClearMenus(true);

	}
	else
		pausemenu.openoffset /= 2;
}

boolean M_PauseInputs(INT32 ch)
{

	if (pausemenu.closing)
		return true;	// Don't allow inputs.

	switch (ch)
	{

		case KEY_UPARROW:
		{
			pausemenu.offset -= 50; // Each item is spaced by 50 px
			M_PrevOpt();
			return true;
		}

		case KEY_DOWNARROW:
		{
			pausemenu.offset += 50;	// Each item is spaced by 50 px
			M_NextOpt();
			return true;
		}

		case KEY_ESCAPE:
		{
			M_QuitPauseMenu();
			return true;
		}

	}

	return false;
}

// Pause spectate / join functions
void M_ConfirmSpectate(INT32 choice)
{
	(void)choice;
	// We allow switching to spectator even if team changing is not allowed
	M_QuitPauseMenu();
	COM_ImmedExecute("changeteam spectator");
}

void M_ConfirmEnterGame(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value)
	{
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\nPress a key.\n"), NULL, MM_NOTHING);
		return;
	}
	M_QuitPauseMenu();
	COM_ImmedExecute("changeteam playing");
}

static void M_ExitGameResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	//Command_ExitGame_f();
	G_SetExitGameFlag();
	M_ClearMenus(true);
}

void M_EndGame(INT32 choice)
{
	(void)choice;
	if (demo.playback)
		return;

	if (!Playing())
		return;

	M_StartMessage(M_GetText("Are you sure you want to return\nto the title screen?\n(Press 'Y' to confirm)\n"), M_ExitGameResponse, MM_YESNO);
}


// Replay Playback Menu
void M_SetPlaybackMenuPointer(void)
{
	itemOn = playback_pause;
}

void M_PlaybackRewind(INT32 choice)
{
	static tic_t lastconfirmtime;

	(void)choice;

	if (!demo.rewinding)
	{
		if (paused)
		{
			G_ConfirmRewind(leveltime-1);
			paused = true;
			S_PauseAudio();
		}
		else
			demo.rewinding = paused = true;
	}
	else if (lastconfirmtime + TICRATE/2 < I_GetTime())
	{
		lastconfirmtime = I_GetTime();
		G_ConfirmRewind(leveltime);
	}

	CV_SetValue(&cv_playbackspeed, 1);
}

void M_PlaybackPause(INT32 choice)
{
	(void)choice;

	paused = !paused;

	if (demo.rewinding)
	{
		G_ConfirmRewind(leveltime);
		paused = true;
		S_PauseAudio();
	}
	else if (paused)
		S_PauseAudio();
	else
		S_ResumeAudio();

	CV_SetValue(&cv_playbackspeed, 1);
}

void M_PlaybackFastForward(INT32 choice)
{
	(void)choice;

	if (demo.rewinding)
	{
		G_ConfirmRewind(leveltime);
		paused = false;
		S_ResumeAudio();
	}
	CV_SetValue(&cv_playbackspeed, cv_playbackspeed.value == 1 ? 4 : 1);
}

void M_PlaybackAdvance(INT32 choice)
{
	(void)choice;

	paused = false;
	TryRunTics(1);
	paused = true;
}

void M_PlaybackSetViews(INT32 choice)
{
	if (choice > 0)
	{
		if (splitscreen < 3)
			G_AdjustView(splitscreen + 2, 0, true);
	}
	else if (splitscreen)
	{
		splitscreen--;
		R_ExecuteSetViewSize();
	}
}

void M_PlaybackAdjustView(INT32 choice)
{
	G_AdjustView(itemOn - playback_viewcount, (choice > 0) ? 1 : -1, true);
}

// this one's rather tricky
void M_PlaybackToggleFreecam(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);

	// remove splitscreen:
	splitscreen = 0;
	R_ExecuteSetViewSize();

	P_InitCameraCmd();	// init camera controls
	if (!demo.freecam)	// toggle on
	{
		demo.freecam = true;
		democam.cam = &camera[0];	// this is rather useful
	}
	else	// toggle off
	{
		demo.freecam = false;
		// reset democam vars:
		democam.cam = NULL;
		//democam.turnheld = false;
		democam.keyboardlook = false;	// reset only these. localangle / aiming gets set before the cam does anything anyway
	}
}

void M_PlaybackQuit(INT32 choice)
{
	(void)choice;
	G_StopDemo();

	if (demo.inreplayhut)
		M_ReplayHut(choice);
	else if (modeattacking)
		M_EndModeAttackRun();
	else
		D_StartTitle();
}


void M_ReplayHut(INT32 choice)
{
	(void)choice;
}

static void Splitplayers_OnChange(void)
{
#if 0
	if (cv_splitplayers.value < setupm_pselect)
		setupm_pselect = 1;
#endif
}

// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      DOOM selection menu, options, episode etc.
//      Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <SDL3/SDL_platform_defines.h>

#include "d_main.h"
#include "m_menu.h"
#include "doomdef.h"
#include "i_sdlinput.h"
#include "d_englsh.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "i_system.h"
#include "i_system_io.h"
#include "i_video.h"
#include "z_zone.h"
#include "st_stuff.h"
#include "g_actions.h"
#include "g_game.h"
#include "s_sound.h"
#include "doomstat.h"
#include "sounds.h"
#include "m_shift.h"
#include "m_password.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "gl_texture.h"
#include "gl_draw.h"
#include "r_lights.h"
#include "r_main.h"
#include "net_server.h"
#include "dgl.h"

//
// definitions
//

//#define HAS_MENU_MOUSE_LOOK

#define MENUSTRINGSIZE      32
#define SKULLXOFF           -40    //villsa: changed from -32 to -40
#define SKULLXTEXTOFF       -24
#define LINEHEIGHT          18
#define TEXTLINEHEIGHT      18
#define MENUCOLORRED        D_RGBA(255, 0, 0, menualphacolor)
#define MENUCOLORWHITE      D_RGBA(255, 255, 255, menualphacolor)
#define MENUCOLORYELLOW     D_RGBA(194, 174, 28, menualphacolor)
#define QUICKSAVESLOT		7
#define QUICKSAVEFILE		"doomsav7.dsg"

//
// defaulted values
//

boolean            allowmenu = true;                   // can menu be accessed?
boolean            menuactive = false;
boolean            mainmenuactive = false;
boolean            allowclearmenu = true;              // can user hit escape to clear menu?

static boolean     newmenu = false;    // 20120323 villsa
static char* messageBindCommand;
static int          quickSaveSlot;                      // -1 = no quicksave slot picked!
static int          saveSlot;                           // which slot to save in
static char         savegamestrings[10][MENUSTRINGSIZE];
static boolean     alphaprevmenu = false;
static int          menualphacolor = 0xff;

static char         inputString[MENUSTRINGSIZE];
static char         oldInputString[MENUSTRINGSIZE];
static boolean     inputEnter = false;
static int          inputCharIndex;
static int          inputMax = 0;

static float cursor_x;
static float cursor_y;

//
// fade-in/out stuff
//

void M_MenuFadeIn(void);
void M_MenuFadeOut(void);
void(*menufadefunc)(void) = NULL;

//
// static variables
//

static char     MenuBindBuff[256];
static char     MenuBindMessage[256];
static boolean MenuBindActive = false;

static int MenuBindIgnoreUntilTic = 0;
static boolean MenuBindEntryWasMouse = false;
static int MenuBindIgnoreMouseButtons = 0;
static boolean showfullitemvalue[3] = { false, false, false };
static int      thermowait = 0;
static int      m_ScreenSize = 1;
static int      levelwarp = 0;

//------------------------------------------------------------------------
//
// MENU TYPEDEFS
//
//------------------------------------------------------------------------

typedef struct {
	// -3 = disabled/hidden item, -2 = enter key ok, -1 = disabled, 0 = no cursor here,
	// 1 = ok, 2 = arrows ok, 3 = for sliders
	short status;

	char name[64];

	// choice = menu item #
	void (*routine)(int choice);

	// hotkey in menu
	char alphaKey;
} menuitem_t;

typedef struct {
	cvar_t* mitem;
	float    mdefault;
} menudefault_t;

typedef struct {
	int item;
	float width;
	cvar_t* mitem;
} menuthermobar_t;

typedef struct menu_s {
	short               numitems;           // # of menu items
	boolean            textonly;
	struct menu_s* prevMenu;          // previous menu
	menuitem_t* menuitems;         // menu items
	void (*routine)(void);                  // draw routine
	char                title[64];
	short               x;
	short               y;                  // x,y of menu
	short               lastOn;             // last item user was on in menu
	boolean            smallfont;          // draw text using small fonts
	menudefault_t* defaultitems;      // pointer to default values for cvars
	short               numpageitems;       // number of items to display per page
	short               menupageoffset;
	float               scale;
	char** hints;
	menuthermobar_t* thermobars;
} menu_t;

typedef struct {
	char* name;
	char* action;
} menuaction_t;

short           itemOn;                 // menu item skull is on
short           itemSelected;
short           skullAnimCounter;       // skull animation counter
short           whichSkull;             // which skull to draw

char    msgNames[2][4] = { "Off","On" };

// current menudef
static menu_t* currentMenu;
static menu_t* nextmenu;

extern menu_t ControlMenuDef;

//------------------------------------------------------------------------
//
// PROTOTYPES
//
//------------------------------------------------------------------------

void M_SetupNextMenu(menu_t* menudef);

void M_QuickSave(void);
void M_QuickLoad(void);

static int M_StringWidth(const char* string);
static int M_BigStringWidth(const char* string);

static void M_DrawThermo(int x, int y, int thermWidth, float thermDot);
static void M_DoDefaults(int choice);
static void M_Return(int choice);
static void M_ReturnToOptions(int choice);
static void M_SetCvar(cvar_t* cvar, float value);
static void M_SetOptionValue(int choice, float min, float max, float inc, cvar_t* cvar);
static void M_DrawSmbString(const char* text, menu_t* menu, int item);
static void M_DrawSmbStringSmall(const char* text, menu_t* menu, int item);
static void M_DrawSaveGameFrontend(menu_t* def);
static void M_SetInputString(char* string, int len);
static void M_Scroll(menu_t* menu, boolean up);

static boolean M_SetThumbnail(int which);

CVAR_CMD(m_menufadetime, 0) {
	if (cvar->value < 0) {
		cvar->value = 0;
	}

	if (cvar->value > 80) {
		cvar->value = 80;
	}
}

CVAR_CMD(m_menumouse, 1) {
	if (cvar->value <= 0) {
		itemSelected = -1;
	}
	I_SetMenuCursorMouseRect();
}


CVAR_CMD(m_cursorscale, 8) {
	if (cvar->value < 0) {
		cvar->value = 0;
	}

	if (cvar->value > 50) {
		cvar->value = 50;
	}
}

//------------------------------------------------------------------------
//
// MAIN MENU
//
//------------------------------------------------------------------------

void M_NewGame(int choice);
void M_Options(int choice);
void M_LoadGame(int choice);
void M_QuitDOOM(int choice);
void M_QuitDOOM2(int choice);

enum {
	newgame = 0,
	options,
	loadgame,
	quitdoom,
	main_end
} main_e;

menuitem_t MainMenu[] = {
	{1,"New Game",M_NewGame,'n'},
	{1,"Load Game",M_LoadGame,'l'},
	{1,"Options",M_Options,'o'},
	{1,"Quit Game",M_QuitDOOM2,'q'},
};

menu_t MainDef = {
	main_end,
	false,
	NULL,
	MainMenu,
	NULL,
	"",
	112,150,
	0,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

//------------------------------------------------------------------------
//
// IN GAME MENU
//
//------------------------------------------------------------------------

void M_SaveGame(int choice);
void M_QuitMainMenu(int choice);
void M_ConfirmRestart(int choice);
void M_Features(int choice);

enum {
	pause_options = 0,
	pause_mainmenu,
	pause_restartlevel,
	pause_features,
	pause_loadgame,
	pause_savegame,
	pause_quitdoom,
	pause_end
} pause_e;

menuitem_t PauseMenu[] = {
	{1,"Options",M_Options,'o'},
	{1,"Main Menu",M_QuitMainMenu,'m'},
	{1,"Restart Level",M_ConfirmRestart,'r'},
	{1,"Features",M_Features,'f'},
	{1,"Load Game",M_LoadGame,'l'},
	{1,"Save Game",M_SaveGame,'s'},
	{1,"Quit Game",M_QuitDOOM,'q'},
};

menu_t PauseDef = {
	pause_end,
	false,
	NULL,
	PauseMenu,
	NULL,
	"Pause",
	112,80,
	0,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

//------------------------------------------------------------------------
//
// QUIT GAME PROMPT
//
//------------------------------------------------------------------------

void M_QuitGame(int choice);
void M_QuitGameBack(int choice);

enum {
	quityes = 0,
	quitno,
	quitend
};

menuitem_t QuitGameMenu[] = {
	{1,"Yes",M_QuitGame,'y'},
	{1,"No",M_QuitGameBack,'n'},
};

menu_t QuitDef = {
	quitend,
	false,
	&PauseDef,
	QuitGameMenu,
	NULL,
	"Quit DOOM?",
	144,112,
	quitno,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_QuitDOOM(int choice) {
	M_SetupNextMenu(&QuitDef);
}

void M_QuitGame(int choice) {
	I_Quit();
}

void M_QuitGameBack(int choice) {
	M_SetupNextMenu(currentMenu->prevMenu);
}

//------------------------------------------------------------------------
//
// QUIT GAME PROMPT (From Main Menu)
//
//------------------------------------------------------------------------

void M_QuitGame2(int choice);
void M_QuitGameBack2(int choice);

enum {
	quit2yes = 0,
	quit2no,
	quit2end
};

menuitem_t QuitGameMenu2[] = {
	{1,"Yes",M_QuitGame2,'y'},
	{1,"No",M_QuitGameBack2,'n'},
};

menu_t QuitDef2 = {
	quit2end,
	false,
	&MainDef,
	QuitGameMenu2,
	NULL,
	"Quit DOOM?",
	144,112,
	quit2no,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_QuitDOOM2(int choice) {
	M_SetupNextMenu(&QuitDef2);
}

void M_QuitGame2(int choice) {
	I_Quit();
}

void M_QuitGameBack2(int choice) {
	M_SetupNextMenu(currentMenu->prevMenu);
}

//------------------------------------------------------------------------
//
// EXIT TO MAIN MENU PROMPT
//
//------------------------------------------------------------------------

void M_EndGame(int choice);

enum {
	PMainYes = 0,
	PMainNo,
	PMain_end
} prompt_e;

menuitem_t PromptMain[] = {
	{1,"Yes",M_EndGame,'y'},
	{1,"No",M_ReturnToOptions,'n'},
};

menu_t PromptMainDef = {
	PMain_end,
	false,
	&PauseDef,
	PromptMain,
	NULL,
	"Quit To Main Menu?",
	144,112,
	PMainNo,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_QuitMainMenu(int choice) {
	M_SetupNextMenu(&PromptMainDef);
}

void M_EndGame(int choice) {
	if (choice) {
		currentMenu->lastOn = itemOn;
		if (currentMenu->prevMenu) {
			menufadefunc = M_MenuFadeOut;
			alphaprevmenu = true;
		}
	}
	else {
		currentMenu->lastOn = itemOn;
		M_ClearMenus();
		gameaction = ga_title;
		currentMenu = &MainDef;
	}
}

//------------------------------------------------------------------------
//
// RESTART LEVEL PROMPT
//
//------------------------------------------------------------------------

void M_RestartLevel(int choice);

enum {
	RMainYes = 0,
	RMainNo,
	RMain_end
};

menuitem_t RestartConfirmMain[] = {
	{1,"Yes",M_RestartLevel,'y'},
	{1,"No",M_ReturnToOptions,'n'},
};

menu_t RestartDef = {
	RMain_end,
	false,
	&PauseDef,
	RestartConfirmMain,
	NULL,
	"Quit Current Game?",
	144,112,
	RMainNo,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_ConfirmRestart(int choice) {
	M_SetupNextMenu(&RestartDef);
}

void M_RestartLevel(int choice) {
	if (!netgame) {
		gameaction = ga_loadlevel;
		nextmap = gamemap;
		players[consoleplayer].playerstate = PST_REBORN;
	}

	currentMenu->lastOn = itemOn;
	M_ClearMenus();
}

//------------------------------------------------------------------------
//
// START NEW IN NETGAME NOTIFY
//
//------------------------------------------------------------------------

void M_DrawStartNewNotify(void);
void M_NewGameNotifyResponse(int choice);

enum {
	SNN_Ok = 0,
	SNN_End
};

menuitem_t StartNewNotify[] = {
	{1,"Ok",M_NewGameNotifyResponse,'o'}
};

menu_t StartNewNotifyDef = {
	SNN_End,
	false,
	&PauseDef,
	StartNewNotify,
	M_DrawStartNewNotify,
	" ",
	144,112,
	SNN_Ok,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_NewGameNotifyResponse(int choice) {
	M_SetupNextMenu(&MainDef);
}

void M_DrawStartNewNotify(void) {
	Draw_BigText(-1, 16, MENUCOLORRED, "You Cannot Start");
	Draw_BigText(-1, 32, MENUCOLORRED, "A New Game On A Network");
}

//------------------------------------------------------------------------
//
// MAP SELECT MENU
//
//------------------------------------------------------------------------

int map = 1;
void M_ChooseMap(int choice);

menu_t MapSelectDef = {
	0,
	false,
	&MainDef,
	NULL,
	NULL,
	"Choose Campaign",
	112,80,
	0,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

//------------------------------------------------------------------------
//
// NEW GAME MENU
//
//------------------------------------------------------------------------

void M_ChooseSkill(int choice);

enum {
	killthings,
	toorough,
	hurtme,
	violence,
	nightmare,
	newg_end
} newgame_e;

menuitem_t NewGameMenu[] = {
	{1,"Be Gentle!",M_ChooseSkill, 'b'},
	{1,"Bring It On!",M_ChooseSkill, 'r'},
	{1,"I Own Doom!",M_ChooseSkill, 'i'},
	{1,"Watch Me Die!",M_ChooseSkill, 'w'},
	{1,"Hardcore!",M_ChooseSkill, 'h'},
};

menu_t NewDef = {
	newg_end,
	false,
	&MainDef,
	NewGameMenu,
	NULL,
	"Choose Your Skill...",
	112,80,
	toorough,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_NewGame(int choice) {
	if (netgame && !demoplayback) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&StartNewNotifyDef);
		return;
	}

	if (MapSelectDef.numitems > 0) {
		M_SetupNextMenu(&MapSelectDef);
		return;
	}

	M_SetupNextMenu(&NewDef);
}

void M_ChooseMap(int choice) {
	if (netgame && !demoplayback) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&StartNewNotifyDef);
		return;
	}
	map = P_GetEpisode(choice)->mapid;
	M_SetupNextMenu(&NewDef);
}

void M_ChooseSkill(int choice) {
	G_DeferedInitNew(choice, map);
	M_ClearMenus();
	dmemset(passwordData, 0xff, 16);
	allowmenu = false;
}

//------------------------------------------------------------------------
//
// OPTIONS MENU
//
//------------------------------------------------------------------------

void M_DrawOptions(void);
void M_Controls(int choice);
void M_Sound(int choice);
void M_Display(int choice);
void M_Video(int choice);
void M_Misc(int choice);
void M_Password(int choice);
void M_Network(int choice);

enum {
	options_controls,
	options_misc,
	options_soundvol,
	options_display,
	options_video,
	options_password,
	//options_network,
	options_return,
	opt_end
} options_e;

menuitem_t OptionsMenu[] = {
	{1,"Controls",M_Controls, 'c'},
	{1,"Sound",M_Sound,'s'},
	{1,"Setup",M_Misc, 'e'},
	{1,"HUD",M_Display, 'd'},
	{1,"Video",M_Video, 'v'},
	{1,"Password",M_Password, 'p'},
	//{1,"Network",M_Network, 'n'},
	{1,"/r Return",M_Return, 0x20}
};

char* OptionHints[opt_end] = {
	"control configuration",
	"adjust sound volume",
	"miscellaneous options for gameplay and other features",
	"settings for the heads-up display",
	"configure video-specific options",
	"enter a password to access a level",
	//"setup options for a hosted session",
	NULL
};

menu_t OptionsDef = {
	opt_end,
	false,
	&PauseDef,
	OptionsMenu,
	M_DrawOptions,
	"Options",
	170,80,
	0,
	false,
	NULL,
	-1,
	0,
	0.75f,
	OptionHints,
	NULL
};

void M_Options(int choice) {
	M_SetupNextMenu(&OptionsDef);
}

void M_DrawOptions(void) {
	if (OptionsDef.hints[itemOn] != NULL) {
		GL_SetOrthoScale(0.5f);
		Draw_BigText(-1, 410, MENUCOLORWHITE, OptionsDef.hints[itemOn]);
		GL_SetOrthoScale(OptionsDef.scale);
	}
}

//------------------------------------------------------------------------
//
// NETWORK MENU
//
//------------------------------------------------------------------------

void M_NetworkChoice(int choice);
void M_PlayerSetName(int choice);
void M_DrawNetwork(void);

CVAR_EXTERNAL(m_playername);
CVAR_EXTERNAL(sv_nomonsters);
CVAR_EXTERNAL(sv_fastmonsters);
CVAR_EXTERNAL(sv_respawnitems);
CVAR_EXTERNAL(sv_respawn);
CVAR_EXTERNAL(sv_allowcheats);
CVAR_EXTERNAL(sv_friendlyfire);
CVAR_EXTERNAL(sv_keepitems);

enum {
	network_header1,
	network_playername,
	network_header2,
	network_allowcheats,
	network_friendlyfire,
	network_keepitems,
	network_header3,
	network_nomonsters,
	network_fastmonsters,
	network_respawnmonsters,
	network_respawnitems,
	network_default,
	network_return,
	network_end
} network_e;

menuitem_t NetworkMenu[] = {
	{-1,"Player Setup",0 },
	{1,"Player Name:", M_PlayerSetName, 'p'},
	{-1,"Player Rules",0 },
	{2,"Allow Cheats:", M_NetworkChoice, 'c'},
	{2,"Friendly Fire:", M_NetworkChoice, 'f'},
	{2,"Keep Items:", M_NetworkChoice, 'k'},
	{-1,"Gameplay Rules",0 },
	{2,"No Monsters:", M_NetworkChoice, 'n'},
	{2,"Fast Monsters:", M_NetworkChoice, 'f'},
	{2,"Respawn Monsters:", M_NetworkChoice, 'r'},
	{2,"Respawn Items:", M_NetworkChoice, 'i'},
	{-2,"Default",M_DoDefaults,'d'},
	{1,"/r Return",M_Return, 0x20}
};

menudefault_t NetworkDefault[] = {
	{ &sv_allowcheats, 0 },
	{ &sv_friendlyfire, 0 },
	{ &sv_keepitems, 0 },
	{ &sv_nomonsters, 0 },
	{ &sv_fastmonsters, 0 },
	{ &sv_respawn, 0 },
	{ &sv_respawnitems, 0 },
	{ NULL, -1 }
};

char* NetworkHints[network_end] = {
	NULL,
	"set a name for yourself",
	NULL,
	"allow clients and host to use cheats",
	"allow players to damage other players",
	"players keep items when respawned from death",
	NULL,
	"no monsters will appear",
	"increased speed for monsters and projectiles",
	"monsters will respawn after death",
	"items will respawn after pickup",
	NULL,
	//	NULL
};

menu_t NetworkDef = {
	network_end,
	false,
	&OptionsDef,
	NetworkMenu,
	M_DrawNetwork,
	"Network",
	208,108,
	0,
	false,
	NetworkDefault,
	-1,
	0,
	0.5f,
	NetworkHints,
	NULL
};

void M_Network(int choice) {
	M_SetupNextMenu(&NetworkDef);
	dstrcpy(inputString, m_playername.string);
}

void M_PlayerSetName(int choice) {
	M_SetInputString(m_playername.string, 16);
}

void M_NetworkChoice(int choice) {
	switch (itemOn) {
	case network_allowcheats:
		M_SetOptionValue(choice, 0, 1, 1, &sv_allowcheats);
		break;
	case network_friendlyfire:
		M_SetOptionValue(choice, 0, 1, 1, &sv_friendlyfire);
		break;
	case network_keepitems:
		M_SetOptionValue(choice, 0, 1, 1, &sv_keepitems);
		break;
	case network_nomonsters:
		M_SetOptionValue(choice, 0, 1, 1, &sv_nomonsters);
		break;
	case network_fastmonsters:
		M_SetOptionValue(choice, 0, 1, 1, &sv_fastmonsters);
		break;
	case network_respawnmonsters:
		M_SetOptionValue(choice, 0, 1, 1, &sv_respawn);
		break;
	case network_respawnitems:
		M_SetOptionValue(choice, 0, 10, 1, &sv_respawnitems);
		break;
	}
}

void M_DrawNetwork(void) {
	int y;
	static const char* respawnitemstrings[11] = {
		"Off",
		"1 Minute",
		"2 Minutes",
		"3 Minutes",
		"4 Minutes",
		"5 Minutes",
		"6 Minutes",
		"7 Minutes",
		"8 Minutes",
		"9 Minutes",
		"10 Minutes"
	};

#define DRAWNETWORKITEM(a, b, c) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(NetworkDef.x + 194, NetworkDef.y+LINEHEIGHT*y, MENUCOLORRED, \
            c[(int)b]); \
    }

	DRAWNETWORKITEM(network_playername, 0, &inputString);
	DRAWNETWORKITEM(network_allowcheats, sv_allowcheats.value, msgNames);
	DRAWNETWORKITEM(network_friendlyfire, sv_friendlyfire.value, msgNames);
	DRAWNETWORKITEM(network_keepitems, sv_keepitems.value, msgNames);
	DRAWNETWORKITEM(network_nomonsters, sv_nomonsters.value, msgNames);
	DRAWNETWORKITEM(network_fastmonsters, sv_fastmonsters.value, msgNames);
	DRAWNETWORKITEM(network_respawnmonsters, sv_respawn.value, msgNames);
	DRAWNETWORKITEM(network_respawnitems, sv_respawnitems.value, respawnitemstrings);

#undef DRAWNETWORKITEM

	if (inputEnter) {
		int i;

		y = network_playername - currentMenu->menupageoffset;
		i = ((int)(160.0f / NetworkDef.scale) - Center_Text(inputString)) * 2;
		Draw_BigText(NetworkDef.x + i + 198, (NetworkDef.y + LINEHEIGHT * y), MENUCOLORWHITE, "/r");
	}

	if (NetworkDef.hints[itemOn] != NULL) {
		GL_SetOrthoScale(0.5f);
		Draw_BigText(-1, 410, MENUCOLORWHITE, NetworkDef.hints[itemOn]);
		GL_SetOrthoScale(NetworkDef.scale);
	}
}

//------------------------------------------------------------------------
//
// MISC MENU
//
//------------------------------------------------------------------------

void M_MiscChoice(int choice);
void M_DrawMisc(void);

CVAR_EXTERNAL(am_showkeymarkers);
CVAR_EXTERNAL(am_showkeycolors);
CVAR_EXTERNAL(am_drawobjects);
CVAR_EXTERNAL(am_overlay);
CVAR_EXTERNAL(r_weaponswitch);
CVAR_EXTERNAL(p_autorun);
CVAR_EXTERNAL(p_usecontext);
CVAR_EXTERNAL(compat_mobjpass);
CVAR_EXTERNAL(r_wipe);
CVAR_EXTERNAL(hud_disablesecretmessages);
CVAR_EXTERNAL(m_nospawnsound);
CVAR_EXTERNAL(m_obituaries);
CVAR_EXTERNAL(m_brutal);
CVAR_EXTERNAL(m_extendedcast);

enum {
	misc_header1,
	misc_menufade,
	misc_empty1,
	misc_menumouse,
	misc_cursorsize,
	misc_empty2,
	misc_header2,
	misc_autorun,
	misc_context,
	misc_header3,
	misc_wipe,
	misc_weaponswitch,
	misc_header4,
	misc_showkey,
	misc_showlocks,
	misc_amobjects,
	misc_amoverlay,
	misc_header5,
	misc_nospawnsound,
	misc_obituaries,
	misc_brutal,
	misc_extendedcast,
	misc_header6,
	misc_comp_pass,
	misc_disablesecretmessages,
	misc_default,
	misc_return,
	misc_end
} misc_e;

menuitem_t MiscMenu[] = {
	{-1,"Menu Options",0 },
	{3,"Menu Fade Speed",M_MiscChoice, 'm' },
	{-1,"",0 },
	{2,"Show Cursor:",M_MiscChoice, 'h'},
	{3,"Cursor Scale:",M_MiscChoice,'u'},
	{-1,"",0 },
	{-1,"Gameplay",0 },
	{2,"Always Run:",M_MiscChoice, 'z' },
	{2,"Use Context:",M_MiscChoice, 'u'},
	{-1,"Misc",0 },
	{2,"Screen Melt:",M_MiscChoice, 's' },
	{2,"Weapon Switch:",M_MiscChoice, 'a'},
	{-1,"Automap",0 },
	{2,"Key Pickups:",M_MiscChoice },
	{2,"Locked Doors:",M_MiscChoice },
	{2,"Draw Objects:",M_MiscChoice },
	{2,"Overlay:",M_MiscChoice },
	{-1,"Extras",0 },
	{2,"Spawn Sound:",M_MiscChoice },
	{2,"Obituaries:",M_MiscChoice },
	{2,"Brutal Mode:",M_MiscChoice },
	{2,"New Cast Roll:",M_MiscChoice },
	{-1,"N64 Compatibility",0 },
	{2,"Tall Actors:",M_MiscChoice,'i'},
	{2,"Secret Messages:",M_MiscChoice,'x'},
	{-2,"Default",M_DoDefaults,'d'},
	{1,"/r Return",M_Return, 0x20}
};

char* MiscHints[misc_end] = {
	NULL,
	"change transition speeds between switching menus",
	NULL,
	"enable menu cursor",
	"set the size of the menu cursor",
	NULL,
	NULL,
	"enable autorun",
	"if enabled interactive objects will highlight when near",
	NULL,
	"enable the melt effect when completing a level",
	"auto switch to a newly picked up weapon",
	NULL,
	"display key pickups in automap",
	"colorize locked doors accordingly to the key in automap",
	"set how objects are rendered in automap",
	"render the automap into the player hud",
	NULL,
	"spawn sound toggle",
	"death messages",
	"get knee deep in the gibs",
	"enable new monsters in the cast of characters sequence",
	NULL,
	"emulate infinite height bug for all solid actors",
	"disable the secret message text",
	NULL,
	//NULL
};

menudefault_t MiscDefault[] = {
	{ &m_menufadetime, 0 },
	{ &m_menumouse, 1 },
	{ &m_cursorscale, 8 },
	{ &p_autorun, 1 },
	{ &p_usecontext, 0 },
	{ &r_wipe, 1 },
	{ &r_weaponswitch, 1 },
	{ &hud_disablesecretmessages, 0 },
	{ &am_showkeymarkers, 0 },
	{ &am_showkeycolors, 0 },
	{ &am_drawobjects, 0 },
	{ &am_overlay, 0 },
	{ &m_nospawnsound, 0 },
	{ &m_obituaries, 0 },
	{ &m_brutal, 0 },
	{ &m_extendedcast, 0 },
	{ &compat_mobjpass, 1 },
	{ NULL, -1 }
};

menuthermobar_t SetupBars[] = {
	{ misc_empty1, 80, &m_menufadetime },
	{ misc_empty2, 50, &m_cursorscale },
	{ -1, 0 }
};

menu_t MiscDef = {
	misc_end,
	false,
	&OptionsDef,
	MiscMenu,
	M_DrawMisc,
	"Setup",
	216,108,
	0,
	false,
	MiscDefault,
	14,
	0,
	0.5f,
	MiscHints,
	SetupBars
};

void M_Misc(int choice) {
	M_SetupNextMenu(&MiscDef);
}

void M_MiscChoice(int choice) {
	switch (itemOn) {
	case misc_menufade:
		if (choice) {
			if (m_menufadetime.value < 80.0f) {
				M_SetCvar(&m_menufadetime, m_menufadetime.value + 0.8f);
			}
			else {
				CON_CvarSetValue(m_menufadetime.name, 80);
			}
		}
		else {
			if (m_menufadetime.value > 0.0f) {
				M_SetCvar(&m_menufadetime, m_menufadetime.value - 0.8f);
			}
			else {
				CON_CvarSetValue(m_menufadetime.name, 0);
			}
		}
		break;

	case misc_cursorsize:
		if (choice) {
			if (m_cursorscale.value < 50) {
				M_SetCvar(&m_cursorscale, m_cursorscale.value + 0.5f);
			}
			else {
				CON_CvarSetValue(m_cursorscale.name, 50);
			}
		}
		else {
			if (m_cursorscale.value > 0.0f) {
				M_SetCvar(&m_cursorscale, m_cursorscale.value - 0.5f);
			}
			else {
				CON_CvarSetValue(m_cursorscale.name, 0);
			}
		}
		break;

	case misc_menumouse:
		M_SetOptionValue(choice, 0, 1, 1, &m_menumouse);
		break;

	case misc_context:
		M_SetOptionValue(choice, 0, 1, 1, &p_usecontext);
		break;

	case misc_wipe:
		M_SetOptionValue(choice, 0, 1, 1, &r_wipe);
		break;

	case misc_autorun:
		M_SetOptionValue(choice, 0, 1, 1, &p_autorun);
		break;

	case misc_weaponswitch:
		M_SetOptionValue(choice, 0, 1, 1, &r_weaponswitch);
		break;

	case misc_disablesecretmessages:
		M_SetOptionValue(choice, 0, 1, 1, &hud_disablesecretmessages);
		break;

	case misc_showkey:
		M_SetOptionValue(choice, 0, 1, 1, &am_showkeymarkers);
		break;

	case misc_showlocks:
		M_SetOptionValue(choice, 0, 1, 1, &am_showkeycolors);
		break;

	case misc_amobjects:
		M_SetOptionValue(choice, 0, 2, 1, &am_drawobjects);
		break;

	case misc_amoverlay:
		M_SetOptionValue(choice, 0, 1, 1, &am_overlay);
		break;

	case misc_nospawnsound:
		M_SetOptionValue(choice, 0, 1, 1, &m_nospawnsound);
		break;

	case misc_obituaries:
		M_SetOptionValue(choice, 0, 1, 1, &m_obituaries);
		break;

	case misc_brutal:
		M_SetOptionValue(choice, 0, 1, 1, &m_brutal);
		break;

	case misc_extendedcast:
		M_SetOptionValue(choice, 0, 1, 1, &m_extendedcast);
		break;

	case misc_comp_pass:
		M_SetOptionValue(choice, 0, 1, 1, &compat_mobjpass);
		break;
	}
}

void M_DrawMisc(void) {
	static const char* autoruntype[2] = { "Off", "On" };
	static const char* mapdisplaytype[2] = { "Hide", "Show" };
	static const char* objectdrawtype[3] = { "Arrows", "Sprites", "Both" };
	static const char* disablesecretmessages[2] = { "Enabled", "Disabled" };
	int y;

	if (currentMenu->menupageoffset <= misc_menufade + 1 &&
		(misc_menufade + 1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
		y = misc_menufade - currentMenu->menupageoffset;
		M_DrawThermo(MiscDef.x, MiscDef.y + LINEHEIGHT * (y + 1), 80, m_menufadetime.value);
	}

	if (currentMenu->menupageoffset <= misc_cursorsize + 1 &&
		(misc_cursorsize + 1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
		y = misc_cursorsize - currentMenu->menupageoffset;
		M_DrawThermo(MiscDef.x, MiscDef.y + LINEHEIGHT * (y + 1), 50, m_cursorscale.value);
	}

#define DRAWMISCITEM(a, b, c) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(MiscDef.x + 176, MiscDef.y+LINEHEIGHT*y, MENUCOLORRED, \
            c[(int)b]); \
    }

	DRAWMISCITEM(misc_menumouse, m_menumouse.value, msgNames);
	DRAWMISCITEM(misc_context, p_usecontext.value, mapdisplaytype);
	DRAWMISCITEM(misc_wipe, r_wipe.value, msgNames);
	DRAWMISCITEM(misc_autorun, p_autorun.value, autoruntype);
	DRAWMISCITEM(misc_weaponswitch, r_weaponswitch.value, msgNames);
	DRAWMISCITEM(misc_showkey, am_showkeymarkers.value, mapdisplaytype);
	DRAWMISCITEM(misc_showlocks, am_showkeycolors.value, mapdisplaytype);
	DRAWMISCITEM(misc_amobjects, am_drawobjects.value, objectdrawtype);
	DRAWMISCITEM(misc_amoverlay, am_overlay.value, msgNames);
	DRAWMISCITEM(misc_nospawnsound, m_nospawnsound.value, disablesecretmessages);
	DRAWMISCITEM(misc_obituaries, m_obituaries.value, autoruntype);
	DRAWMISCITEM(misc_brutal, m_brutal.value, autoruntype);
	DRAWMISCITEM(misc_extendedcast, m_extendedcast.value, autoruntype);
	DRAWMISCITEM(misc_comp_pass, !compat_mobjpass.value, msgNames);
	DRAWMISCITEM(misc_disablesecretmessages, hud_disablesecretmessages.value, disablesecretmessages);

#undef DRAWMISCITEM

	if (MiscDef.hints[itemOn] != NULL) {
		GL_SetOrthoScale(0.5f);
		Draw_BigText(-1, 410, MENUCOLORWHITE, MiscDef.hints[itemOn]);
		GL_SetOrthoScale(MiscDef.scale);
	}
}

//------------------------------------------------------------------------
//
// MOUSE MENU
//
//------------------------------------------------------------------------

void M_ChangeSensitivity(int choice);
void M_ChangeMouseAccel(int choice);
#ifdef HAS_MENU_MOUSE_LOOK
void M_ChangeMouseLook(int choice);
void M_ChangeMouseInvert(int choice);
#endif
void M_ChangeYAxisMove(int choice);
void M_ChangeXAxisMove(int choice);
void M_DrawMouse(void);

CVAR_EXTERNAL(v_msensitivityx);
CVAR_EXTERNAL(v_msensitivityy);
#ifdef HAS_MENU_MOUSE_LOOK
CVAR_EXTERNAL(v_mlook);
CVAR_EXTERNAL(v_mlookinvert);
#endif
CVAR_EXTERNAL(v_yaxismove);
CVAR_EXTERNAL(v_xaxismove);
CVAR_EXTERNAL(v_macceleration);

enum {
	mouse_sensx,
	mouse_empty1,
	mouse_sensy,
	mouse_empty2,
	mouse_accel,
	mouse_empty3,
#ifdef HAS_MENU_MOUSE_LOOK
	mouse_look,
	mouse_invert,
#endif
	mouse_yaxismove,
	mouse_xaxismove,
	mouse_default,
	mouse_return,
	mouse_end
} mouse_e;

menuitem_t MouseMenu[] = {
	{3,"Mouse Sensitivity X",M_ChangeSensitivity, 'x'},
	{-1,"",0},
	{3,"Mouse Sensitivity Y",M_ChangeSensitivity, 'y'},
	{-1,"",0},
	{3, "Mouse Acceleration",M_ChangeMouseAccel, 'a'},
	{-1, "",0},
#ifdef HAS_MENU_MOUSE_LOOK
	{2,"Mouse Look:",M_ChangeMouseLook,'l'},
	{2,"Invert Look:",M_ChangeMouseInvert, 'i'},
#endif
	{2,"Y-Axis Move:",M_ChangeYAxisMove, 'y'},
		{2, "X-Axis Move:", M_ChangeXAxisMove, 'x'},
	{-2,"Default",M_DoDefaults,'d'},
	{1,"/r Return",M_Return, 0x20}
};

menudefault_t MouseDefault[] = {
	{ &v_msensitivityx, 5 },
	{ &v_msensitivityy, 5 },
	{ &v_macceleration, 0 },
#ifdef HAS_MENU_MOUSE_LOOK
	{ &v_mlook, 0 },
	{ &v_mlookinvert, 0 },
#endif
	{ &v_yaxismove, 0 },
	{ &v_xaxismove },
	{ NULL, -1 }
};

menuthermobar_t MouseBars[] = {
	{ mouse_empty1, 32, &v_msensitivityx },
	{ mouse_empty2, 32, &v_msensitivityy },
	{ mouse_empty3, 20, &v_macceleration },
	{ -1, 0 }
};

menu_t MouseDef = {
	mouse_end,
	false,
	&ControlMenuDef,
	MouseMenu,
	M_DrawMouse,
	"Mouse",
	104,52,
	0,
	false,
	MouseDefault,
	-1,
	0,
	0.8f,
	NULL,
	MouseBars
};

void M_DrawMouse(void) {
	M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * (mouse_sensx + 1), MAXSENSITIVITY, v_msensitivityx.value);
	M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * (mouse_sensy + 1), MAXSENSITIVITY, v_msensitivityy.value);

	M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * (mouse_accel + 1), 20, v_macceleration.value);
#ifdef HAS_MENU_MOUSE_LOOK
	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_look, MENUCOLORRED,
		msgNames[(int)v_mlook.value]);
	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_invert, MENUCOLORRED,
		msgNames[(int)v_mlookinvert.value]);
#endif
	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_yaxismove, MENUCOLORRED,
		msgNames[(int)v_yaxismove.value]);
	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_xaxismove, MENUCOLORRED,
		msgNames[(int)v_xaxismove.value]);

}

void M_ChangeSensitivity(int choice) {
	float slope = (float)MAXSENSITIVITY / 100.0f;
	switch (choice) {
	case 0:
		switch (itemOn) {
		case mouse_sensx:
			if (v_msensitivityx.value > 0.0f) {
				M_SetCvar(&v_msensitivityx, v_msensitivityx.value - slope);
			}
			else {
				CON_CvarSetValue(v_msensitivityx.name, 0);
			}
			break;
		case mouse_sensy:
			if (v_msensitivityy.value > 0.0f) {
				M_SetCvar(&v_msensitivityy, v_msensitivityy.value - slope);
			}
			else {
				CON_CvarSetValue(v_msensitivityy.name, 0);
			}
			break;
		}
		break;
	case 1:
		switch (itemOn) {
		case mouse_sensx:
			if (v_msensitivityx.value < (float)MAXSENSITIVITY) {
				M_SetCvar(&v_msensitivityx, v_msensitivityx.value + slope);
			}
			else {
				CON_CvarSetValue(v_msensitivityx.name, (float)MAXSENSITIVITY);
			}
			break;
		case mouse_sensy:
			if (v_msensitivityy.value < (float)MAXSENSITIVITY) {
				M_SetCvar(&v_msensitivityy, v_msensitivityy.value + slope);
			}
			else {
				CON_CvarSetValue(v_msensitivityy.name, (float)MAXSENSITIVITY);
			}
			break;
		}
		break;
	}
}

void M_ChangeMouseAccel(int choice)
{
	switch (choice) {
	case 0:
		if (v_macceleration.value > 0.0f) {
			M_SetCvar(&v_macceleration, v_macceleration.value - 1);
		}
		else {
			CON_CvarSetValue(v_macceleration.name, 0);
		}
		break;
	case 1:
		if (v_macceleration.value < 20.0f) {
			M_SetCvar(&v_macceleration, v_macceleration.value + 1);
		}
		else {
			CON_CvarSetValue(v_macceleration.name, 20);
		}
		break;
	}
	I_MouseAccelChange();
}

#ifdef HAS_MENU_MOUSE_LOOK
void M_ChangeMouseLook(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_mlook);
}

void M_ChangeMouseInvert(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_mlookinvert);
}
#endif

void M_ChangeYAxisMove(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_yaxismove);
}

void M_ChangeXAxisMove(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_xaxismove);
}
//------------------------------------------------------------------------
//
// DISPLAY MENU
//
//------------------------------------------------------------------------

void M_ChangeMessages(int choice);
void M_ToggleHudDraw(int choice);
void M_ToggleFlashOverlay(int choice);
void M_ToggleDamageHud(int choice);
void M_ToggleWpnDisplay(int choice);
void M_ToggleShowStats(int choice);
void M_ToggleShowStatsAlwaysOn(int choice);
void M_ChangeCrosshair(int choice);
void M_ChangeOpacity(int choice);
void M_DrawDisplay(void);
void M_ChangeHUDColor(int choice);
void M_ChangeFOV(int choice);

CVAR_EXTERNAL(st_drawhud);
CVAR_EXTERNAL(st_crosshair);
CVAR_EXTERNAL(st_crosshairopacity);
CVAR_EXTERNAL(st_flashoverlay);
CVAR_EXTERNAL(st_showpendingweapon);
CVAR_EXTERNAL(st_showstats);
CVAR_EXTERNAL(st_showstatsalwayson);
CVAR_EXTERNAL(m_messages);
CVAR_EXTERNAL(p_damageindicator);
CVAR_EXTERNAL(st_hud_color);
CVAR_EXTERNAL(r_fov);

enum {
	messages,
	statusbar,
	display_flash,
	display_damage,
	display_weapon,
	display_stats,
	display_stats_always_on,
	display_hud_color,
	display_fov,
	display_empty2,
	display_crosshair,
	display_opacity,
	display_empty1,
	e_default,
	display_return,
	display_end
} display_e;

menuitem_t DisplayMenu[] = {
	{2,"Messages:",M_ChangeMessages, 'm'},
	{2,"Status Bar:",M_ToggleHudDraw, 's'},
	{2,"Hud Flash:",M_ToggleFlashOverlay, 'f'},
	{2,"Damage Hud:",M_ToggleDamageHud, 'd'},
	{2,"Show Weapon:",M_ToggleWpnDisplay, 'w'},
	{2,"Show Stats:",M_ToggleShowStats, 't'},
	{2,"Show Stats Always:",M_ToggleShowStatsAlwaysOn, 'a'},
	{3,"HUD Colour",M_ChangeHUDColor, 'o'},
	{3,"Field of View",M_ChangeFOV, 'v'},
	{-1,"",0},
	{2,"Crosshair:",M_ChangeCrosshair, 'c'},
	{3,"Crosshair Opacity",M_ChangeOpacity, 'o'},
	{-1,"",0},
	{-2,"Default",M_DoDefaults, 'd'},
	{1,"/r Return",M_Return, 0x20}
};

char* DisplayHints[display_end] = {
	"toggle messages displaying on hud",
	"change look and style for hud",
	"use texture environment or a simple overlay for flashes",
	"toggle hud indicators when taking damage",
	"shows the next or previous pending weapon",
	"display level stats in automap",
	"display level stats in-game",
	"change the hud text colour",
	"change the field of view",
	NULL,
	"toggle crosshair",
	"change opacity for crosshairs",
	NULL,
	NULL,
	NULL
};

menudefault_t DisplayDefault[] = {
	{ &m_messages, 1 },
	{ &st_drawhud, 1 },
	{ &p_damageindicator, 0 },
	{ &st_showpendingweapon, 1 },
	{ &st_showstats, 0 },
	{ &st_showstatsalwayson, 0 },
	{ &st_hud_color, 0 },
	{ &r_fov, 75 },
	{ &st_crosshair, 0 },
	{ &st_crosshairopacity, 80 },
	{ NULL, -1 }
};

menuthermobar_t DisplayBars[] = {
	{ display_empty1, 255, &st_crosshairopacity },
	{ display_empty2, 255, &r_fov },
	{ -1, 0 }
};

menu_t DisplayDef = {
	display_end,
	false,
	&OptionsDef,
	DisplayMenu,
	M_DrawDisplay,
	"HUD",
	135,65,
	0,
	false,
	DisplayDefault,
	-1,
	0,
	0.6f,
	DisplayHints,
	DisplayBars
};

void M_Display(int choice) {
	M_SetupNextMenu(&DisplayDef);
}

void M_DrawDisplay(void) {
	static const char* hudtype[3] = { "Off", "Classic", "Arranged" };
	static const char* flashtype[2] = { "Environment", "Overlay" };
	static const char* hud_color[2] = { "Red", "White" };

	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * messages, MENUCOLORRED,
		msgNames[(int)m_messages.value]);
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * statusbar, MENUCOLORRED,
		hudtype[(int)st_drawhud.value]);
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_flash, MENUCOLORRED,
		flashtype[(int)st_flashoverlay.value]);
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_damage, MENUCOLORRED,
		msgNames[(int)p_damageindicator.value]);
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_weapon, MENUCOLORRED,
		msgNames[(int)st_showpendingweapon.value]);
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_stats, MENUCOLORRED,
		msgNames[(int)st_showstats.value]);
	Draw_BigText(DisplayDef.x + 210, DisplayDef.y + LINEHEIGHT * display_stats_always_on, MENUCOLORRED,
		msgNames[(int)st_showstatsalwayson.value]);
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_hud_color, MENUCOLORRED,
		hud_color[(int)st_hud_color.value]);

	M_DrawThermo(DisplayDef.x, DisplayDef.y + LINEHEIGHT * (display_fov + 1),
		255, r_fov.value);

	if (st_crosshair.value <= 0) {
		Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_crosshair, MENUCOLORRED,
			msgNames[0]);
	}
	else {
		ST_DrawCrosshair(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_crosshair,
			(int)st_crosshair.value, 1, MENUCOLORWHITE);
	}

	M_DrawThermo(DisplayDef.x, DisplayDef.y + LINEHEIGHT * (display_opacity + 1),
		255, st_crosshairopacity.value);

	if (DisplayDef.hints[itemOn] != NULL) {
		GL_SetOrthoScale(0.5f);
		Draw_BigText(-1, 432, MENUCOLORWHITE, DisplayDef.hints[itemOn]);
		GL_SetOrthoScale(DisplayDef.scale);
	}

}

void M_ChangeMessages(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &m_messages);

	if (choice) {
		players[consoleplayer].message = MSGON;
	}
	else {
		players[consoleplayer].message = MSGOFF;
	}
}

void M_ToggleHudDraw(int choice) {
	M_SetOptionValue(choice, 0, 2, 1, &st_drawhud);
}

void M_ToggleDamageHud(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &p_damageindicator);
}

void M_ToggleWpnDisplay(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &st_showpendingweapon);
}

void M_ToggleShowStats(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &st_showstats);
	st_showstatsalwayson.value = 0;
}

void M_ToggleShowStatsAlwaysOn(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &st_showstatsalwayson);
	st_showstats.value = 0;
}

void M_ToggleFlashOverlay(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &st_flashoverlay);
}

void M_ChangeCrosshair(int choice) {
	M_SetOptionValue(choice, 0, (float)st_crosshairs, 1, &st_crosshair);
}

void M_ChangeFOV(int choice)
{
	if (choice) {
		if (r_fov.value < 255.0f) {
			M_SetCvar(&r_fov, r_fov.value + 1);
		}
		else {
			CON_CvarSetValue(r_fov.name, 255);
		}
	}
	else {
		if (r_fov.value > 0.0f) {
			M_SetCvar(&r_fov, r_fov.value - 1);
		}
		else {
			CON_CvarSetValue(r_fov.name, 0);
		}
	}
}

void M_ChangeOpacity(int choice)
{
	if (choice) {
		if (st_crosshairopacity.value < 255.0f) {
			M_SetCvar(&st_crosshairopacity, st_crosshairopacity.value + 10);
		}
		else {
			CON_CvarSetValue(st_crosshairopacity.name, 255);
		}
	}
	else {
		if (st_crosshairopacity.value > 0.0f) {
			M_SetCvar(&st_crosshairopacity, st_crosshairopacity.value - 10);
		}
		else {
			CON_CvarSetValue(st_crosshairopacity.name, 0);
		}
	}
}

void M_ChangeHUDColor(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &st_hud_color);
}

//------------------------------------------------------------------------
//
// VIDEO MENU
//
//------------------------------------------------------------------------

void M_ChangeBrightness(int choice);
void M_ChangeGammaLevel(int choice);
void M_ChangeFullscreen(int choice);
void M_ChangeWeaponFilter(int choice);
void M_ChangeHUDFilter(int choice);
void M_ChangeSkyFilter(int choice);
void M_ChangeAnisotropic(int choice);
void M_ChangeInterpolateFrames(int choice);
void M_ChangeAccessibility(int choice);
void M_ChangeFadeIn(int choice);
void M_DrawVideo(void);
void M_ChangeVsync(int choice);

CVAR_EXTERNAL(v_checkratio);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(i_gamma);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(v_fullscreen);
CVAR_EXTERNAL(r_weaponFilter);
CVAR_EXTERNAL(r_hudFilter);
CVAR_EXTERNAL(r_skyFilter);
CVAR_EXTERNAL(r_anisotropic);
CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(v_accessibility);
CVAR_EXTERNAL(v_fadein);
CVAR_EXTERNAL(v_vsync);

enum {
	video_dbrightness,
	video_empty1,
	video_dgamma,
	video_empty2,
	video_fullscreen,
	weapon_filter,
	hud_filter,
	sky_filter,
	anisotropic,
	interpolate_frames,
	vsync,
	accessibility,
	fadein,
	v_default,
	video_return,
	video_end
} video_e;

menuitem_t VideoMenu[] = {
	{3,"Brightness",M_ChangeBrightness, 'b'},
	{-1,"",0},
	{3,"Gamma Correction",M_ChangeGammaLevel, 'g'},
	{-1,"",0},
	{2,"Fullscreen:",M_ChangeFullscreen, 'f'},
	{2,"Weapon Filter:",M_ChangeWeaponFilter, 't'},
	{2,"HUD Filter:",M_ChangeHUDFilter, 't'},
	{2,"Sky Filter:",M_ChangeSkyFilter, 's'},
	{2,"Anisotropy:",M_ChangeAnisotropic, 'a'},
	{2,"Interpolation:",M_ChangeInterpolateFrames, 'i'},
	{2,"VSync:",M_ChangeVsync, 'v'},
	{2,"Accessibility:",M_ChangeAccessibility, 'y'},
	{2,"Fade In:",M_ChangeFadeIn, 'd'},
	{-2,"Default",M_DoDefaults, 'e'},
	{1,"/r Return",M_Return, 0x20}
};

char* VideoHints[video_end] = {
	"change light color intensity",
	NULL,
	"adjust screen gamma",
	NULL,
	"toggle fullscreen mode - requires restart",
	"toggle texture filtering on the weapon",
	"toggle texture filtering on hud and text",
	"toggle texture filtering on skies",
	"toggle blur reduction on textures",
	"toggle frame interpolation to\n achieve smooth framerates",
	"toggle vsync on or off to prevent screen tearing",
	"toggle accessibility to\n remove flashing lights",
	"toggle fade in animation that plays when starting a level"
};

menudefault_t VideoDefault[] = {
	{ &i_brightness, 0 },
	{ &i_gamma, 0 },
	{ &v_fullscreen, 0 },
	{ &r_weaponFilter, 0 },
	{ &r_hudFilter, 0 },
	{ &r_skyFilter, 0 },
	{ &r_anisotropic, 1 },
	{ &i_interpolateframes, 1 },
	{ &v_vsync, 1 },
	{ &v_accessibility, 0 },
	{ &v_fadein, 1 },
	{ NULL, -1 }
};

menuthermobar_t VideoBars[] = {
	{ video_empty1, 300, &i_brightness },
	{ video_empty2, 20,  &i_gamma },
	{ -1, 0 }
};

menu_t VideoDef = {
	video_end,
	false,
	&OptionsDef,
	VideoMenu,
	M_DrawVideo,
	"Video",
	136,80,
	0,
	false,
	VideoDefault,
	12,
	0,
	0.65f,
	VideoHints,
	VideoBars
};

static char gammamsg[21][28] = {
	GAMMALVL0,
	GAMMALVL1,
	GAMMALVL2,
	GAMMALVL3,
	GAMMALVL4,
	GAMMALVL5,
	GAMMALVL6,
	GAMMALVL7,
	GAMMALVL8,
	GAMMALVL9,
	GAMMALVL10,
	GAMMALVL11,
	GAMMALVL12,
	GAMMALVL13,
	GAMMALVL14,
	GAMMALVL15,
	GAMMALVL16,
	GAMMALVL17,
	GAMMALVL18,
	GAMMALVL19,
	GAMMALVL20
};

void M_Video(int choice) {
	M_SetupNextMenu(&VideoDef);
	m_ScreenSize = 1;
}

void M_DrawVideo(void) {
	static const char* filterType2[2] = { "Linear", "Nearest" };
	static const char* onofftype[2] = { "Off", "On" };
	static const char* fullscreenType[2] = { "Resizable Window", "Exclusive" };
	int y;

	if (currentMenu->menupageoffset <= video_dbrightness + 1 &&
		(video_dbrightness + 1) - currentMenu->menupageoffset < currentMenu->numpageitems)
	{
		y = video_dbrightness - currentMenu->menupageoffset;
		M_DrawThermo(VideoDef.x, VideoDef.y + LINEHEIGHT * (y + 1), 300, i_brightness.value);
	}
	if (currentMenu->menupageoffset <= video_dgamma + 1 &&
		(video_dgamma + 1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
		y = video_dgamma - currentMenu->menupageoffset;
		M_DrawThermo(VideoDef.x, VideoDef.y + LINEHEIGHT * (y + 1), 20, i_gamma.value);
	}

#define DRAWVIDEOITEM(a, b) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(VideoDef.x + 176, VideoDef.y+LINEHEIGHT*y, MENUCOLORRED, b); \
    }

#define DRAWVIDEOITEM2(a, b, c) DRAWVIDEOITEM(a, c[(int)b])

	DRAWVIDEOITEM2(video_fullscreen, v_fullscreen.value, fullscreenType);
	DRAWVIDEOITEM2(weapon_filter, r_weaponFilter.value, filterType2);
	DRAWVIDEOITEM2(hud_filter, r_hudFilter.value, filterType2);
	DRAWVIDEOITEM2(sky_filter, r_skyFilter.value, filterType2);
	DRAWVIDEOITEM2(anisotropic, r_anisotropic.value, msgNames);
	DRAWVIDEOITEM2(interpolate_frames, i_interpolateframes.value, onofftype);
	DRAWVIDEOITEM2(vsync, v_vsync.value, onofftype);
	DRAWVIDEOITEM2(accessibility, v_accessibility.value, onofftype);
	DRAWVIDEOITEM2(fadein, v_fadein.value, onofftype);

#undef DRAWVIDEOITEM
#undef DRAWVIDEOITEM2
	if (VideoDef.hints[itemOn] != NULL)
	{
		GL_SetOrthoScale(0.55f);
		Draw_BigText(-1, 380, MENUCOLORWHITE, VideoDef.hints[itemOn]);
		GL_SetOrthoScale(VideoDef.scale);
	}
}

void M_ChangeBrightness(int choice)
{
	switch (choice)
	{
	case 0:
		if (i_brightness.value > 0.0f)
		{
			M_SetCvar(&i_brightness, i_brightness.value - 1);
		}
		else
		{
			CON_CvarSetValue(i_brightness.name, 0);
		}
		break;
	case 1:
		if (i_brightness.value < 300.0f)
		{
			M_SetCvar(&i_brightness, i_brightness.value + 1);
		}
		else
		{
			CON_CvarSetValue(i_brightness.name, 300);
		}
		break;
	}

	R_RefreshBrightness();
}

void M_ChangeGammaLevel(int choice)
{
	switch (choice) {
	case 0:
		if (i_gamma.value > 0.0f) {
			M_SetCvar(&i_gamma, i_gamma.value - 1);
		}
		else {
			CON_CvarSetValue(i_gamma.name, 0);
		}
		break;
	case 1:
		if (i_gamma.value < 20.0f) {
			M_SetCvar(&i_gamma, i_gamma.value + 1);
		}
		else {
			CON_CvarSetValue(i_gamma.name, 20);
		}
		break;
	case 2:
		if (i_gamma.value >= 20) {
			CON_CvarSetValue(i_gamma.name, 0);
		}
		else {
			CON_CvarSetValue(i_gamma.name, i_gamma.value + 1);
		}

		players[consoleplayer].message = gammamsg[(int)i_gamma.value];
		break;
	}
}

void M_ChangeFullscreen(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_fullscreen);
}

void M_ChangeWeaponFilter(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &r_weaponFilter);
}

void M_ChangeHUDFilter(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &r_hudFilter);
}

void M_ChangeSkyFilter(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &r_skyFilter);
}

void M_ChangeAnisotropic(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &r_anisotropic);
}

void M_ChangeInterpolateFrames(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &i_interpolateframes);
}

void M_ChangeVsync(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &v_vsync);
}

void M_ChangeAccessibility(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &v_accessibility);
}

void M_ChangeFadeIn(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &v_fadein);
}

//------------------------------------------------------------------------
//
// PASSWORD MENU
//
//------------------------------------------------------------------------

void M_DrawPassword(void);

menuitem_t PasswordMenu[32];

menu_t PasswordDef = {
	32,
	false,
	&OptionsDef,
	PasswordMenu,
	M_DrawPassword,
	"Password",
	92,60,
	0,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

static boolean passInvalid = false;
static int        curPasswordSlot = 0;
static int        passInvalidTic = 0;

void M_Password(int choice) {
	M_SetupNextMenu(&PasswordDef);
	passInvalid = false;
	passInvalidTic = 0;

	for (curPasswordSlot = 0; curPasswordSlot < 15; curPasswordSlot++) {
		if (passwordData[curPasswordSlot] == 0xff) {
			break;
		}
	}
}

void M_DrawPassword(void) {
	char password[2];
	byte* passData;
	int i = 0;

	Draw_BigText(-1, 240 - 48, MENUCOLORWHITE, "Press Delete To Change");
	Draw_BigText(-1, 240 - 32, MENUCOLORWHITE, "Press Escape To Return");

	if (passInvalid) {
		if (!passInvalidTic--) {
			passInvalidTic = 0;
			passInvalid = false;
		}

		if (passInvalidTic & 16) {
			Draw_BigText(-1, 240 - 80, MENUCOLORWHITE, "Invalid Password");
			return;
		}
	}

	dmemset(password, 0, 2);
	passData = passwordData;

	for (i = 0; i < 19; i++) {
		if (i == 4 || i == 9 || i == 14) {
			password[0] = 0x20;
		}
		else {
			if (*passData == 0xff) {
				password[0] = '.';
			}
			else {
				password[0] = passwordChar[*passData];
			}

			passData++;
		}

		Draw_BigText((currentMenu->x + (i * 12)) - 48, 240 - 80, MENUCOLORRED, password);
	}
}

static void M_PasswordSelect(void) {
	S_StartSound(NULL, sfx_switch2);
	passwordData[curPasswordSlot++] = (byte)itemOn;
	if (curPasswordSlot > 15) {
		static const char* hecticdemo = "rvnh3ct1cd3m0???";
		int i;

		for (i = 0; i < 16; i++) {
			if (passwordChar[passwordData[i]] != hecticdemo[i]) {
				break;
			}
		}

		if (i >= 16) {
			rundemo4 = true;
			M_Return(0);

			return;
		}

		if (!M_DecodePassword(1)) {
			passInvalid = true;
			passInvalidTic = 64;
		}
		else {
			M_DecodePassword(0);
			G_DeferedInitNew(gameskill, gamemap);
			doPassword = true;
			currentMenu->lastOn = itemOn;
			S_StartSound(NULL, sfx_pistol);
			M_ClearMenus();

			return;
		}

		curPasswordSlot = 15;
	}
}

static void M_PasswordDeSelect(void) {
	S_StartSound(NULL, sfx_switch2);
	if (passwordData[curPasswordSlot] == 0xff) {
		curPasswordSlot--;
	}

	if (curPasswordSlot < 0) {
		curPasswordSlot = 0;
	}

	passwordData[curPasswordSlot] = 0xff;
}

//------------------------------------------------------------------------
//
// SOUND MENU
//
//------------------------------------------------------------------------

void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_DrawSound(void);

CVAR_EXTERNAL(s_sfxvol);
CVAR_EXTERNAL(s_musvol);

enum {
	sfx_vol,
	sfx_empty1,
	music_vol,
	sfx_empty2,
	sound_default,
	sound_return,
	sound_end
} sound_e;

menuitem_t SoundMenu[] = {
	{3,"Sound Volume",M_SfxVol,'s'},
	{-1,"",0},
	{3,"Music Volume",M_MusicVol,'m'},
	{-1,"",0},
	{-2,"Default",M_DoDefaults,'d'},
	{1,"/r Return",M_Return, 0x20}
};

menudefault_t SoundDefault[] = {
	{ &s_sfxvol, 120 },
	{ &s_musvol, 180 },
	{ NULL, -1 }
};

menuthermobar_t SoundBars[] = {
	{ sfx_empty1, 255, &s_sfxvol },
	{ sfx_empty2, 255, &s_musvol },
	{ -1, 0 }
};

menu_t SoundDef = {
	sound_end,
	false,
	&OptionsDef,
	SoundMenu,
	M_DrawSound,
	"Sound",
	96,60,
	0,
	false,
	SoundDefault,
	-1,
	0,
	1.0f,
	NULL,
	SoundBars
};

void M_Sound(int choice) {
	M_SetupNextMenu(&SoundDef);
}

void M_DrawSound(void) {
	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 255, s_sfxvol.value);
	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), 255, s_musvol.value);
}

void M_SfxVol(int choice)
{
	switch (choice) {
	case 0:
		if (s_sfxvol.value > 0.0f) {
			M_SetCvar(&s_sfxvol, s_sfxvol.value - 1);
		}
		else {
			CON_CvarSetValue(s_sfxvol.name, 0);
		}
		break;
	case 1:
		if (s_sfxvol.value < 255.0f) {
			M_SetCvar(&s_sfxvol, s_sfxvol.value + 1);
		}
		else {
			CON_CvarSetValue(s_sfxvol.name, 255);
		}
		break;
	}
}

void M_MusicVol(int choice)
{
	switch (choice) {
	case 0:
		if (s_musvol.value > 0.0f) {
			M_SetCvar(&s_musvol, s_musvol.value - 1);
		}
		else {
			CON_CvarSetValue(s_musvol.name, 0);
		}
		break;
	case 1:
		if (s_musvol.value < 255.0f) {
			M_SetCvar(&s_musvol, s_musvol.value + 1);
		}
		else {
			CON_CvarSetValue(s_musvol.name, 255);
		}
		break;
	}
}

//------------------------------------------------------------------------
//
// FEATURES MENU
//
//------------------------------------------------------------------------

void M_DoFeature(int choice);
void M_DrawFeaturesMenu(void);

CVAR_EXTERNAL(sv_lockmonsters);

enum {
	features_levels = 0,
	features_newline,
	features_invulnerable,
	features_healthboost,
	features_securitykeys,
	features_weapons,
	features_mapeverything,
	features_lockmonsters,
	features_noclip,
	features_end
} features_e;

menuitem_t FeaturesMenu[] = {
	{2,"Warp To Level " ,M_DoFeature,'l'},
	{2, "" },
	{2,"Invulnerable",M_DoFeature,'i'},
	{2,"Health Boost",M_DoFeature,'h'},
	{2,"Security Keys",M_DoFeature,'k'},
	{2,"Weapons",M_DoFeature,'w'},
	{2,"Map Everything",M_DoFeature,'m'},
	{2,"Lock Monsters",M_DoFeature,'o'},
	{2,"Wall Blocking",M_DoFeature,'w'},
};

menu_t featuresDef = {
	features_end,
	false,
	&PauseDef,
	FeaturesMenu,
	M_DrawFeaturesMenu,
	"Features",
	56,56,
	0,
	true,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_Features(int choice) {
	M_SetupNextMenu(&featuresDef);

	showfullitemvalue[0] = showfullitemvalue[1] = showfullitemvalue[2] = false;
}

void M_DrawFeaturesMenu(void) {
	mapdef_t* map = P_GetMapInfo(levelwarp + 1);

	/*Warp To Level*/
	char levelDisplay[64];
	char mapNameDisplay[64];

	sprintf(levelDisplay, "MAP%02d", map->mapid);
	sprintf(mapNameDisplay, "%s", map->mapname);

	M_DrawSmbString(levelDisplay, &featuresDef, features_levels);

	menu_t secondLine = featuresDef;
	secondLine.y += 10;
	M_DrawSmbStringSmall(mapNameDisplay, &secondLine, features_levels);

	/*Lock Monsters Mode*/
	M_DrawSmbString(msgNames[(int)sv_lockmonsters.value], &featuresDef, features_lockmonsters);

	/*Invulnerable*/
	M_DrawSmbString(msgNames[players[consoleplayer].cheats & CF_GODMODE ? 1 : 0],
		&featuresDef, features_invulnerable);

	/*No Clip*/
	M_DrawSmbString(msgNames[players[consoleplayer].cheats & CF_NOCLIP ? 1 : 0],
		&featuresDef, features_noclip);

	/*Map Everything*/
	M_DrawSmbString(msgNames[amCheating == 2 ? 1 : 0], &featuresDef, features_mapeverything);

	/*Full Health*/
	M_DrawSmbString(showfullitemvalue[0] ? "100%%" : "-", &featuresDef, features_healthboost);

	/*Full Weapons*/
	M_DrawSmbString(showfullitemvalue[1] ? "100%%" : "-", &featuresDef, features_weapons);

	/*Full Keys*/
	M_DrawSmbString(showfullitemvalue[2] ? "100%%" : "-", &featuresDef, features_securitykeys);
}

void M_DoFeature(int choice) {
	int i = 0;

	switch (itemOn) {

	case features_levels:
		if (choice) {
			int testWarp = levelwarp + 1;
			mapdef_t* testMap = P_GetMapInfo(testWarp + 1);

			if (testMap) {
				levelwarp = testWarp;
			}
			else {
				if (choice == 2) {
					levelwarp = 0;
				}
			}
		}
		else {
			if (levelwarp > 0) {
				levelwarp--;
			}
		}
		break;

	case features_invulnerable:
		if (choice) {
			players[consoleplayer].cheats |= CF_GODMODE;
		}
		else {
			players[consoleplayer].cheats &= ~CF_GODMODE;
		}
		break;

	case features_noclip:
		if (choice) {
			players[consoleplayer].cheats |= CF_NOCLIP;
		}
		else {
			players[consoleplayer].cheats &= ~CF_NOCLIP;
		}
		break;

	case features_healthboost:
		showfullitemvalue[0] = true;
		players[consoleplayer].health = 100;
		players[consoleplayer].mo->health = 100;
		break;

	case features_weapons:
		showfullitemvalue[1] = true;

		for (i = 0; i < NUMWEAPONS; i++) {
			players[consoleplayer].weaponowned[i] = true;
		}

		if (!players[consoleplayer].backpack) {
			players[consoleplayer].backpack = true;
			for (i = 0; i < NUMAMMO; i++) {
				players[consoleplayer].maxammo[i] *= 2;
			}
		}

		for (i = 0; i < NUMAMMO; i++) {
			players[consoleplayer].ammo[i] = players[consoleplayer].maxammo[i];
		}

		break;

	case features_mapeverything:
		amCheating = choice ? 2 : 0;
		break;

	case features_securitykeys:
		showfullitemvalue[2] = true;

		for (i = 0; i < NUMCARDS; i++) {
			players[consoleplayer].cards[i] = true;
		}

		break;

	case features_lockmonsters:
		if (choice) {
			CON_CvarSetValue(sv_lockmonsters.name, 1);
		}
		else {
			CON_CvarSetValue(sv_lockmonsters.name, 0);
		}
		break;
	}

	S_StartSound(NULL, sfx_switch2);
}


//------------------------------------------------------------------------
//
// CONTROLS MENU
//
//------------------------------------------------------------------------

void M_ChangeKeyBinding(int choice);
void M_BuildControlMenu(void);
void M_DrawControls(void);

#define NUM_CONTROL_ACTIONS     51
#define NUM_CONTROL_ITEMS       NUM_CONTROL_ACTIONS

menuaction_t* PlayerActions;
menu_t          ControlsDef;
menuitem_t      ControlsItem[NUM_CONTROL_ITEMS];

static menuaction_t mPlayerActionsDef[NUM_CONTROL_ITEMS] = {
	{"Movement", NULL},
	{"Forward", "+forward"},
	{"Backward", "+back"},
	{"Left", "+left"},
	{"Right", "+right"},
	{"Strafe Modifier", "+strafe"},
	{"Strafe Left", "+strafeleft"},
	{"Strafe Right", "+straferight"},
	{"Action", NULL},
	{"Fire", "+fire"},
	{"Use", "+use"},
	{"Run", "+run"},
	{"Look Up", "+lookup"},
	{"Look Down", "+lookdown"},
	{"Center View", "+center"},
	{"Weapons", NULL},
	{"Next Weapon", "nextweap"},
	{"Previous Weapon", "prevweap"},
	{"Fist/Chainsaw", "weapon 2"},
	{"Pistol", "weapon 3"},
	{"Shotgun(s)", "weapon 4"},
	{"Chaingun", "weapon 6"},
	{"Rocket Launcher", "weapon 7"},
	{"Plasma Rifle", "weapon 8"},
	{"BFG 9000", "weapon 9"},
	{"Unmaker", "weapon 10"},
	{"Fist", "weapon 14"},
	{"Chainsaw", "weapon 1"},
	{"Shotgun", "weapon 13"},
	{"Super Shotgun", "weapon 5"},
	{"Automap", NULL},
	{"Toggle", "automap"},
	{"Zoom In", "+automap_in"},
	{"Zoom Out", "+automap_out"},
	{"Pan Left", "+automap_left"},
	{"Pan Right", "+automap_right"},
	{"Pan Up", "+automap_up"},
	{"Pan Down", "+automap_down"},
	{"Mouse Pan/Zoom", "+automap_freepan"},
	{"Follow Mode", "automap_follow"},
	{"Other", NULL},
	{"Quick Save", "quicksave"},
	{"Quick Load", "quickload"},
	{"Save", "save"},
	{"Load", "load"},
	{"Screen Shot", "screenshot"},
	{"Gamma", "gamma"},
	{"Detach Camera", "setcamerastatic"},
	{"Chasecam", "setcamerachase"},
	{NULL, NULL}
};

void M_Controls(int choice) {
	M_SetupNextMenu(&ControlMenuDef);
}

void M_BuildControlMenu(void) {
	menu_t* menu;
	int            actions;
	int            item;
	int            i;

	PlayerActions = mPlayerActionsDef;

	actions = 0;
	while (PlayerActions[actions].name) {
		actions++;
	}

	menu = &ControlsDef;
	// add extra menu items for non-bindable items (display only)
	menu->numitems = actions;
	menu->textonly = false;
	menu->numpageitems = 16;
	menu->prevMenu = &ControlMenuDef;
	menu->menuitems = ControlsItem;
	menu->routine = M_DrawControls;
	menu->x = 120;
	menu->y = 80;
	menu->smallfont = true;
	menu->menupageoffset = 0;
	menu->scale = 0.75f;
	sprintf(menu->title, "Bindings");
	menu->lastOn = itemOn;

	for (item = 0; item < actions; item++) {
		dstrcpy(menu->menuitems[item].name, PlayerActions[item].name);
		if (PlayerActions[item].action) {
			for (i = dstrlen(PlayerActions[item].name); i < 17; i++) {
				menu->menuitems[item].name[i] = ' ';
			}

			menu->menuitems[item].status = 1;
			menu->menuitems[item].routine = M_ChangeKeyBinding;

			G_GetActionBindings(&menu->menuitems[item].name[17], PlayerActions[item].action);

		}
		else {
			menu->menuitems[item].status = -1;
			menu->menuitems[item].routine = NULL;
		}

		menu->menuitems[item].alphaKey = 0;
	}
}

void M_ChangeKeyBinding(int choice) {
	char action[128];
	sprintf(action, "%s %d", PlayerActions[choice].action, 1);
	dstrcpy(MenuBindBuff, action);
	messageBindCommand = MenuBindBuff;
	sprintf(MenuBindMessage, "%s", PlayerActions[choice].name);
	MenuBindActive = true;
	MenuBindIgnoreUntilTic = gametic + 1;
}

void M_DrawControls(void) {
	Draw_BigText(-1, 264, MENUCOLORWHITE, "Press Escape To Return");
	Draw_BigText(-1, 280, MENUCOLORWHITE, "Press Delete To Unbind");
}

//------------------------------------------------------------------------
//
// CONTROLS MENU
//
//------------------------------------------------------------------------

void M_ControlChoice(int choice);
void M_DrawControlMenu(void);

enum {
	controls_keyboard,
	controls_mouse,
	controls_return,
	controls_end
} controls_e;

menuitem_t ControlsMenu[] = {
	{1,"Bindings",M_ControlChoice, 'k'},
	{1,"Mouse",M_ControlChoice, 'm'},
	{1,"/r Return",M_Return, 0x20}
};

char* ControlsHints[controls_end] = {
	"configure bindings",
	"configure mouse functionality",
	NULL
};

menu_t ControlMenuDef = {
	controls_end,
	false,
	&OptionsDef,
	ControlsMenu,
	M_DrawControlMenu,
	"Controls",
	120,64,
	0,
	false,
	NULL,
	-1,
	0,
	1,
	ControlsHints,
	NULL
};

void M_ControlChoice(int choice) {
	switch (itemOn) {
	case controls_keyboard:
		M_BuildControlMenu();
		M_SetupNextMenu(&ControlsDef);
		break;
	case controls_mouse:
		M_SetupNextMenu(&MouseDef);
		break;
	}
}

void M_DrawControlMenu(void) {
	if (ControlMenuDef.hints[itemOn] != NULL) {
		GL_SetOrthoScale(0.5f);
		Draw_BigText(-1, 410, MENUCOLORWHITE, ControlMenuDef.hints[itemOn]);
		GL_SetOrthoScale(ControlMenuDef.scale);
	}
}

//------------------------------------------------------------------------
//
// LOAD IN NETGAME NOTIFY
//
//------------------------------------------------------------------------

void M_DrawNetLoadNotify(void);

enum {
	NLN_Ok = 0,
	NLN_End
};

menuitem_t NetLoadNotify[] = {
	{1,"Ok",M_ReturnToOptions,'o'}
};

menu_t NetLoadNotifyDef = {
	NLN_End,
	false,
	&PauseDef,
	NetLoadNotify,
	M_DrawNetLoadNotify,
	" ",
	144,112,
	NLN_Ok,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_DrawNetLoadNotify(void) {
	Draw_BigText(-1, 16, MENUCOLORRED, "You Cannot Load While");
	Draw_BigText(-1, 32, MENUCOLORRED, "In A Net Game!");
}

//------------------------------------------------------------------------
//
// SAVEDEAD NOTIFY
//
//------------------------------------------------------------------------

void M_DrawSaveDeadNotify(void);

enum {
	SDN_Ok = 0,
	SDN_End
};

menuitem_t SaveDeadNotify[] = {
	{1,"Ok",M_ReturnToOptions,'o'}
};

menu_t SaveDeadDef = {
	SDN_End,
	false,
	&PauseDef,
	SaveDeadNotify,
	M_DrawSaveDeadNotify,
	" ",
	144,112,
	SDN_Ok,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_DrawSaveDeadNotify(void) {
	Draw_BigText(-1, 16, MENUCOLORRED, "You Cannot Save");
	Draw_BigText(-1, 32, MENUCOLORRED, "While Not In Game");
}

//------------------------------------------------------------------------
//
// SAVE GAME MENU
//
//------------------------------------------------------------------------

void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_DrawSave(void);

enum {
	load1,
	load2,
	load3,
	load4,
	load5,
	load6,
	load7,
	load8,
	load_end
} load_e;

menuitem_t SaveMenu[] = {
	{1,"", M_SaveSelect,'1'},
	{1,"", M_SaveSelect,'2'},
	{1,"", M_SaveSelect,'3'},
	{1,"", M_SaveSelect,'4'},
	{1,"", M_SaveSelect,'5'},
	{1,"", M_SaveSelect,'6'},
	{1,"", M_SaveSelect,'7'},
	{1,"", M_SaveSelect,'8'},
};

menu_t SaveDef = {
	load_end,
	false,
	&PauseDef,
	SaveMenu,
	M_DrawSave,
	"Save Game",
	112,144,
	0,
	false,
	NULL,
	-1,
	0,
	0.5f,
	NULL,
	NULL
};

//
//  M_SaveGame & Cie.
//
void M_DrawSave(void) {
	int i;

	M_DrawSaveGameFrontend(&SaveDef);

	for (i = 0; i < load_end; i++) {
		char* string;

		if (i == saveSlot && inputEnter) {
			string = inputString;
		}
		else {
			string = savegamestrings[i];
		}

		Draw_BigText(SaveDef.x, SaveDef.y + LINEHEIGHT * i, MENUCOLORYELLOW, string);
	}

	if (inputEnter) {
		i = ((int)(160.0f / SaveDef.scale) - Center_Text(inputString)) * 2;
		Draw_BigText(SaveDef.x + i, (SaveDef.y + LINEHEIGHT * saveSlot) - 2, MENUCOLORYELLOW, "/r");
	}
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot) {
	G_SaveGame(slot, savegamestrings[slot]);
	M_ClearMenus();

	// PICK QUICKSAVE SLOT YET?
	if (quickSaveSlot == -2) {
		quickSaveSlot = slot;
	}
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice) {
	saveSlot = choice;
	dstrcpy(inputString, savegamestrings[choice]);
	M_SetInputString(savegamestrings[choice], (SAVESTRINGSIZE - 1));
}

//
// Selected from DOOM menu
//
void M_SaveGame(int choice) {
	if (!usergame) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&SaveDeadDef);
		return;
	}

	if (gamestate != GS_LEVEL) {
		return;
	}

	M_SetupNextMenu(&SaveDef);
	M_ReadSaveStrings();
}

//------------------------------------------------------------------------
//
// LOAD GAME MENU
//
//------------------------------------------------------------------------

void M_LoadSelect(int choice);
void M_DrawLoad(void);

menuitem_t DoomLoadMenu[] = { //LoadMenu conflicts with Win32 API
	{1,"", M_LoadSelect,'1'},
	{1,"", M_LoadSelect,'2'},
	{1,"", M_LoadSelect,'3'},
	{1,"", M_LoadSelect,'4'},
	{1,"", M_LoadSelect,'5'},
	{1,"", M_LoadSelect,'6'},
	{1,"", M_LoadSelect,'7'},
	{1,"", M_LoadSelect,'8'}
};

menu_t LoadMainDef = {
	load_end,
	false,
	&MainDef,
	DoomLoadMenu,
	M_DrawLoad,
	"Load Game",
	112,144,
	0,
	false,
	NULL,
	-1,
	0,
	0.5f,
	NULL,
	NULL
};

menu_t LoadDef = {
	load_end,
	false,
	&PauseDef,
	DoomLoadMenu,
	M_DrawLoad,
	"Load Game",
	112,144,
	0,
	false,
	NULL,
	-1,
	0,
	0.5f,
	NULL,
	NULL
};

//
// M_LoadGame & Cie.
//
void M_DrawLoad(void) {
	int i;

	M_DrawSaveGameFrontend(&LoadDef);

	for (i = 0; i < load_end; i++)
		Draw_BigText(LoadDef.x, LoadDef.y + LINEHEIGHT * i,
			MENUCOLORYELLOW, savegamestrings[i]);
}

//
// User wants to load this game
//
void M_LoadSelect(int choice) {
	//char name[256];

	// sprintf(name, SAVEGAMENAME"%d.dsg", choice);
	// G_LoadGame(name);
	char* filename = P_GetSaveGameName(choice);
	G_LoadGame(filename);
	free(filename);
	M_ClearMenus();
}

//
// Selected from DOOM menu
//
void M_LoadGame(int choice) {
	if (netgame) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&NetLoadNotifyDef);
		return;
	}

	if (currentMenu == &MainDef) {
		M_SetupNextMenu(&LoadMainDef);
	}
	else {
		M_SetupNextMenu(&LoadDef);
	}

	M_ReadSaveStrings();
}

//
// M_ReadSaveStrings
//
// Read the strings from the savegame files
//
void M_ReadSaveStrings(void) {
	int     handle;
	int     i;
	// char    name[256];

	for (i = 0; i < load_end; i++) {
		// sprintf(name, SAVEGAMENAME"%d.dsg", i);

		// handle = open(name, O_RDONLY | 0, 0666);
		char* filename = P_GetSaveGameName(i);
		handle = open(filename, O_RDONLY | 0, 0666);

		free(filename);
		if (handle == -1) {
			dstrcpy(&savegamestrings[i][0], EMPTYSTRING);
			DoomLoadMenu[i].status = 0;
			continue;
		}
		read(handle, &savegamestrings[i], MENUSTRINGSIZE);
		close(handle);
		DoomLoadMenu[i].status = 1;
	}
}

//------------------------------------------------------------------------
//
// COMMON MENU FUNCTIONS
//
//------------------------------------------------------------------------

//
// M_SetCvar
//

static void M_SetCvar(cvar_t* cvar, float value) {
	if (cvar->value == value) {
		return;
	}

	CON_CvarSetValue(cvar->name, value);
}

//
// M_SetOptionValue
//

static void M_SetOptionValue(int choice, float min, float max, float inc, cvar_t* cvar) {
	if (choice) {
		if (cvar->value < max) {
			M_SetCvar(cvar, cvar->value + inc);
		}
		else if (choice == 2) {
			M_SetCvar(cvar, min);
		}
		else {
			CON_CvarSetValue(cvar->name, max);
		}
	}
	else {
		if (cvar->value > min) {
			M_SetCvar(cvar, cvar->value - inc);
		}
		else if (choice == 2) {
			M_SetCvar(cvar, max);
		}
		else {
			CON_CvarSetValue(cvar->name, min);
		}
	}
}

//
// M_DoDefaults
//

static void M_DoDefaults(int choice) {
	int i = 0;

	for (i = 0; currentMenu->defaultitems[i].mitem != NULL; i++) {
		CON_CvarSetValue(currentMenu->defaultitems[i].mitem->name, currentMenu->defaultitems[i].mdefault);
	}

	if (currentMenu == &DisplayDef) {
		R_RefreshBrightness();
	}

	if (currentMenu == &VideoDef) {
		GL_DumpTextures();
		GL_SetTextureFilter();
	}

	S_StartSound(NULL, sfx_switch2);
}

//
// M_ReturnToOptions
//

void M_ReturnToOptions(int choice) {
	M_SetupNextMenu(&PauseDef);
}

//
// M_Return
//

static void M_Return(int choice) {
	currentMenu->lastOn = itemOn;
	if (currentMenu->prevMenu) {
		menufadefunc = M_MenuFadeOut;
		alphaprevmenu = true;
		S_StartSound(NULL, sfx_pistol);
	}
}

//
// M_ReturnInstant
//

static void M_ReturnInstant(void) {
	if (currentMenu->prevMenu) {
		currentMenu = currentMenu->prevMenu;
		itemOn = currentMenu->lastOn;

		S_StartSound(NULL, sfx_switch2);
	}
	else {
		M_ClearMenus();
	}
}

//
// M_SetInputString
//

static void M_SetInputString(char* string, int len) {
	inputEnter = true;
	dstrcpy(oldInputString, string);

	// hack
	if (!dstrcmp(string, EMPTYSTRING)) {
		inputString[0] = 0;
	}

	inputCharIndex = dstrlen(inputString);
	inputMax = len;
}

//
// M_DrawSmbString
//

static void M_DrawSmbString(const char* text, menu_t* menu, int item) {
	int x;
	int y;

	x = menu->x + 128;
	y = menu->y + (ST_FONTWHSIZE + 1) * item;
	Draw_Text(x, y, MENUCOLORWHITE, 1.0f, false, text);
}

static void M_DrawSmbStringSmall(const char* text, menu_t* menu, int item) {
	int x;
	int y;

	x = menu->x + 206;
	y = menu->y + 28;
	Draw_Text(x, y, MENUCOLORWHITE, 0.7f, false, text);
}

//
// M_StringWidth
// Find string width from hu_font chars
//

static int M_StringWidth(const char* string) {
	int i;
	int w = 0;
	int c;

	for (i = 0; i < dstrlen(string); i++) {
		c = SDL_toupper(string[i]) - ST_FONTSTART;
		if (c < 0 || c >= ST_FONTSIZE) {
			w += 4;
		}
		else {
			w += ST_FONTWHSIZE;
		}
	}

	return w;
}

//
// M_BigStringWidth
// Find string width from bigfont chars
//

static int M_BigStringWidth(const char* string) {
	int width = 0;
	char t = 0;
	int id = 0;
	int len = 0;
	int i = 0;

	len = dstrlen(string);

	for (i = 0; i < len; i++) {
		t = string[i];

		switch (t) {
		case 0x20:
			width += 6;
			break;
		case '-':
			width += symboldata[SM_MISCFONT].w;
			break;
		case '%':
			width += symboldata[SM_MISCFONT + 1].w;
			break;
		case '!':
			width += symboldata[SM_MISCFONT + 2].w;
			break;
		case '.':
			width += symboldata[SM_MISCFONT + 3].w;
			break;
		case '?':
			width += symboldata[SM_MISCFONT + 4].w;
			break;
		case ':':
			width += symboldata[SM_MISCFONT + 5].w;
			break;
		default:
			if (t >= 'A' && t <= 'Z') {
				id = t - 'A';
				width += symboldata[SM_FONT1 + id].w;
			}
			if (t >= 'a' && t <= 'z') {
				id = t - 'a';
				width += symboldata[SM_FONT2 + id].w;
			}
			if (t >= '0' && t <= '9') {
				id = t - '0';
				width += symboldata[SM_NUMBERS + id].w;
			}
			break;
		}
	}

	return width;
}

//
// M_Scroll
//
// Allow scrolling through multi-page menus via mouse wheel
//

static void M_Scroll(menu_t* menu, boolean up) {
	if (menu->numpageitems != -1) {
		if (!up) {
			menu->menupageoffset++;
			if (menu->numpageitems + menu->menupageoffset >=
				menu->numitems) {
				menu->menupageoffset =
					menu->numitems - menu->numpageitems;
			}
			else if (++itemOn >= menu->numitems) {
				itemOn = menu->numitems - 1;
			}
		}
		else {
			menu->menupageoffset--;
			if (menu->menupageoffset < 0) {
				menu->menupageoffset = 0;
				if (itemOn < 0) {
					itemOn = 0;
				}
			}
			else if (--itemOn < 0) {
				itemOn = 0;
			}
		}
	}
}

//
// M_CheckDragThermoBar
//
// Allow user to click and drag thermo bar
//
// To be fair, this is a horrid mess and a awful hack...
// Really need a better and more efficient menu system
//

static void M_CheckDragThermoBar(event_t* ev, menu_t* menu) {
	menuthermobar_t* bar;
	float startx;
	float scrny;
	float x;
	float y;
	int i;
	float mx;
	float my;
	float width;
	float scalex;
	float scaley;
	float value;
	float lineheight;

	// must be a mouse held event and menu must have thermobar settings
	if (ev->type != ev_mouse || menu->thermobars == NULL) {
		return;
	}

	// mouse buttons must be held and moving
	if (!(ev->data1 & 1)) {
		return;
	}

	bar = menu->thermobars;
	x = menu->x;
	y = menu->y;
	mx = cursor_x;
	my = cursor_y;
	scalex = ((float)video_width /
		((float)SCREENHEIGHT * video_ratio)) * menu->scale;
	scaley = ((float)video_height /
		(float)SCREENHEIGHT) * menu->scale;
	startx = x * scalex;
	width = startx + (100.0f * scalex);

	// check if cursor is within range
	for (i = 0; bar[i].item != -1; i++) {
		lineheight = (float)(LINEHEIGHT * bar[i].item);
		scrny = (y + lineheight + 10) * scaley;

		if (my < scrny) {
			scrny = (y + lineheight + 2) * scaley;
			if (my >= scrny) {
				// dragged all the way to the left?
				if (mx < startx) {
					CON_CvarSetValue(bar[i].mitem->name, 0);
					return;
				}

				// dragged all the way to the right?
				if (mx > width) {
					CON_CvarSetValue(bar[i].mitem->name, bar[i].width);
					return;
				}

				// convert mouse x coordinate into thermo bar position
				// set cvar as well
				value = (mx / scalex) - x;
				CON_CvarSetValue(bar[i].mitem->name,
					value * (bar[i].width / 100.0f));

				return;
			}
		}
	}
}

//
// M_CursorHighlightItem
//
// Highlight effects when positioning mouse cursor
// over menu item
//
// To be fair, this is a horrid mess and a awful hack...
// Really need a better and more efficient menu system
//

static boolean M_CursorHighlightItem(menu_t* menu) {
	float scrnx;
	float scrny;
	float mx;
	float my;
	int width;
	int height;
	float scalex;
	float scaley;
	int item;
	int max;
	int start;
	int x;
	int y;
	int lineheight;

	start = menu->menupageoffset;
	max = (menu->numpageitems == -1) ? menu->numitems : menu->numpageitems;
	x = menu->x;
	y = menu->y;
	mx = cursor_x;
	my = cursor_y;
	scalex = ((float)video_width /
		((float)SCREENHEIGHT * video_ratio)) * menu->scale;
	scaley = ((float)video_height /
		(float)SCREENHEIGHT) * menu->scale;

	if (menu->textonly) {
		lineheight = TEXTLINEHEIGHT;
	}
	else {
		lineheight = LINEHEIGHT;
	}

	if (menu->smallfont) {
		lineheight /= 2;
	}

	itemSelected = -1;

	for (item = start; item < max + start; item++) {
		// skip hidden items
		if (menu->menuitems[item].status == -3) {
			continue;
		}

		if (menu == &PasswordDef) {
			if (item > 0) {
				if (!(item & 7)) {
					y += lineheight;
					x = menu->x;
				}
				else {
					x += TEXTLINEHEIGHT;
				}
			}
		}

		// highlight non-static items
		if (menu->menuitems[item].status != -1) {
			// guess the bounding box size based on string length and font type
			width = (menu->smallfont ?
				M_StringWidth(menu->menuitems[item].name) :
				M_BigStringWidth(menu->menuitems[item].name));
			height = (menu->smallfont ? 8 : LINEHEIGHT);
			scrnx = (float)(x + width) * scalex;
			scrny = (float)(y + height) * scaley;

			// do extra checks for columns if we're in the password menu
			// otherwise we'll just care about rows

			if (mx < scrnx || menu != &PasswordDef) {
				scrnx = (float)x * scalex;
				if (mx >= scrnx || menu != &PasswordDef) {
					if (my < scrny) {
						scrny = (float)y * scaley;
						if (my >= scrny) {
							itemSelected = item;
							return true;
						}
					}
				}
			}
		}

		if (menu != &PasswordDef) {
			y += lineheight;
		}
	}

	return false;
}

//
// M_QuickSave
//

//
// M_QuickSave
//

void M_QuickSave(void)
{
	if (!usergame) {
		S_StartSound(NULL, sfx_oof);
		return;
	}

	if (gamestate != GS_LEVEL)
		return;

	G_SaveGame(QUICKSAVESLOT, "quicksave");
}

void M_QuickLoad(void)
{
	char* filepath = I_GetUserFile(QUICKSAVEFILE);

	if (M_FileExists(filepath)) {
		G_LoadGame(filepath);
	}

	free(filepath);
}

//
// Menu Functions
//

static void M_DrawThermo(int x, int y, int thermWidth, float thermDot) {
	float slope = 100.0f / (float)thermWidth;

	Draw_BigText(x, y, MENUCOLORWHITE, "/t");
	Draw_BigText(x + (int)(thermDot * slope) * (symboldata[SM_THERMO].w / 100), y, MENUCOLORWHITE, "/s");
}

//
// M_SetThumbnail
//

static char thumbnail_date[32];
static int thumbnail_skill = -1;
static int thumbnail_map = -1;

static boolean M_SetThumbnail(int which) {
	byte* data;

	data = Z_Malloc(SAVEGAMETBSIZE, PU_STATIC, 0);

	//
	// poke into savegame file and fetch
	// date and stats
	//
	char* filename = P_GetSaveGameName(which);
	boolean ret = P_QuickReadSaveHeader(filename, thumbnail_date, (int*)data,
		&thumbnail_skill, &thumbnail_map);
	free(filename);
	Z_Free(data);

	return ret;
}

//
// M_DrawSaveGameFrontend
//

static void M_DrawSaveGameFrontend(menu_t* def) {
	GL_SetState(GLSTATE_BLEND, 1);
	GL_SetOrtho(0);

	dglDisable(GL_TEXTURE_2D);

	//
	// draw back panels
	//
	dglColor4ub(4, 4, 4, menualphacolor);
	//
	// save game panel
	//
	dglRecti(
		def->x - 48,
		def->y - 12,
		def->x + 256,
		def->y + 156
	);
	//
	// stats panel
	//
	dglRecti(
		def->x + 272,
		def->y - 12,
		def->x + 464,
		def->y + 116
	);

	//
	// draw outline for panels
	//
	dglColor4ub(240, 86, 84, menualphacolor);
	dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//
	// save game panel
	//
	dglRecti(
		def->x - 48,
		def->y - 12,
		def->x + 256,
		def->y + 156
	);
	//
	// stats panel
	//
	dglRecti(
		def->x + 272,
		def->y - 12,
		def->x + 464,
		def->y + 116
	);
	dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	dglEnable(GL_TEXTURE_2D);

	//
	// draw thumbnail texture and stats
	//
	if (M_SetThumbnail(itemOn)) {
		char string[128];

		curgfx = -1;

		GL_SetOrthoScale(0.35f);

		Draw_BigText(def->x + 440, def->y + 60, MENUCOLORYELLOW, thumbnail_date);

		sprintf(string, "Skill: %s", NewGameMenu[thumbnail_skill].name);
		Draw_BigText(def->x + 440, def->y + 168, MENUCOLORYELLOW, string);

		sprintf(string, "Map: %s", P_GetMapInfo(thumbnail_map)->mapname);
		Draw_BigText(def->x + 440, def->y + 196, MENUCOLORYELLOW, string);

		GL_SetOrthoScale(def->scale);
	}

	GL_SetState(GLSTATE_BLEND, 0);
}

//------------------------------------------------------------------------
//
// CONTROL PANEL
//
//------------------------------------------------------------------------


//
// M_Responder
//

static boolean shiftdown = false;

boolean M_Responder(event_t* ev) {
	int ch;
	int i;

	ch = -1;

	if (menufadefunc || !allowmenu || demoplayback) {
		return false;
	}

	if (ev->type == ev_mousedown) {
		if (ev->data1 & 1) {
			ch = KEY_ENTER;
		}

		if (ev->data1 & 4) {
			ch = KEY_BACKSPACE;
		}
	}
	else if (ev->type == ev_keydown) {
		ch = ev->data1;

		if (ch == KEY_SHIFT) {
			shiftdown = true;
		}
	}
	else if (ev->type == ev_keyup) {
		thermowait = 0;
		if (ev->data1 == KEY_SHIFT) {
			ch = ev->data1;
			shiftdown = false;
		}
	}
	else if (ev->type == ev_mouse && (ev->data2 != 0.0 || ev->data3 != 0.0)) {
		// handle mouse-over selection
		if (m_menumouse.value) {
			M_CheckDragThermoBar(ev, currentMenu);
			if (M_CursorHighlightItem(currentMenu))
				itemOn = itemSelected;
		}
	}

	// atsb fixes a long time EX bug where the mouse didn't allow binding as it swallowed the inputs
	if (MenuBindActive == true) { //key Bindings

		if (gametic <= MenuBindIgnoreUntilTic) {
			return true;
		}
		if (ev->type == ev_mouse) {
			if (MenuBindEntryWasMouse) {
				if ((ev->data1 & MenuBindIgnoreMouseButtons) == 0) {
					MenuBindEntryWasMouse = false;
				}
			}
			return true;
		}
		if (MenuBindEntryWasMouse && ev->type == ev_mousedown && (ev->data1 & MenuBindIgnoreMouseButtons)) {
			return true;
		}

		switch (ch) {
		case KEY_ESCAPE:
			MenuBindActive = false;
			M_BuildControlMenu();
			break;
		case KEY_CONSOLE:
			// cannot bind reserved console key
			break;
		default:
			if (G_BindActionByEvent(ev, messageBindCommand)) {
				MenuBindActive = false;
				M_BuildControlMenu();
			}
		}

		return true;
	}

	if (ch == -1) {
		return false;
	}

	// save game / player name string input
	if (inputEnter) {
		switch (ch) {
		case KEY_BACKSPACE:
			if (inputCharIndex > 0) {
				inputCharIndex--;
				inputString[inputCharIndex] = 0;
			}
			break;

		case KEY_ESCAPE:
			inputEnter = false;
			dstrcpy(inputString, oldInputString);
			break;

		case KEY_ENTER:
			inputEnter = false;
			if (currentMenu == &NetworkDef) {
				CON_CvarSet(m_playername.name, inputString);
				if (netgame) {
					NET_SV_UpdateCvars(&m_playername);
				}
			}
			else {
				dstrcpy(savegamestrings[saveSlot], inputString);
				if (savegamestrings[saveSlot][0]) {
					M_DoSave(saveSlot);
				}
			}
			break;

		default:

			if (inputCharIndex >= inputMax) {
				return true;
			}

			if (shiftdown) {
				ch = toupper(ch);
			}

			if (ch != 32) {
				if (ch - ST_FONTSTART < 0 || ch - ST_FONTSTART >= ('z' - ST_FONTSTART + 1)) {
					break;
				}
			}

			if (ch >= 32 && ch <= 127) {
				if (inputCharIndex < (MENUSTRINGSIZE - 1) &&
					M_StringWidth(inputString) < (MENUSTRINGSIZE - 2) * 8) {
					inputString[inputCharIndex++] = ch;
					inputString[inputCharIndex] = 0;
				}
			}
			break;
		}
		return true;
	}

	// Pop-up menu?
	if (!menuactive) {
		if (ch == KEY_ESCAPE && !st_chatOn) {
			M_StartControlPanel(false);
			return true;
		}
		return false;
	}

	// Keys usable within menu
	switch (ch) {
	case KEY_DOWNARROW:
		S_StartSound(NULL, sfx_switch1);
		if (currentMenu == &PasswordDef) {
			itemOn = ((itemOn + 8) & 31);
			return true;
		}
		else {
			do {
				if (itemOn + 1 > currentMenu->numitems - 1) {
					itemOn = 0;
				}
				else {
					itemOn++;
				}
			} while (currentMenu->menuitems[itemOn].status == -1 ||
				currentMenu->menuitems[itemOn].status == -3);
			return true;
		}

	case KEY_UPARROW:
		S_StartSound(NULL, sfx_switch1);
		if (currentMenu == &PasswordDef) {
			itemOn = ((itemOn - 8) & 31);
			return true;
		}
		else {
			do {
				if (!itemOn) {
					itemOn = currentMenu->numitems - 1;
				}
				else {
					itemOn--;
				}
			} while (currentMenu->menuitems[itemOn].status == -1 ||
				currentMenu->menuitems[itemOn].status == -3);
			return true;
		}

	case KEY_LEFTARROW:
		if (currentMenu == &PasswordDef) {
			S_StartSound(NULL, sfx_switch1);
			do {
				if (!itemOn) {
					itemOn = currentMenu->numitems - 1;
				}
				else {
					itemOn--;
				}
			} while (currentMenu->menuitems[itemOn].status == -1);
			return true;
		}
		else {
			if (currentMenu->menuitems[itemOn].routine &&
				currentMenu->menuitems[itemOn].status >= 2) {
				currentMenu->menuitems[itemOn].routine(0);

				if (currentMenu->menuitems[itemOn].status == 3) {
					thermowait = 1;
				}
			}
			return true;
		}

	case KEY_RIGHTARROW:
		if (currentMenu == &PasswordDef) {
			S_StartSound(NULL, sfx_switch1);
			do {
				if (itemOn + 1 > currentMenu->numitems - 1) {
					itemOn = 0;
				}
				else {
					itemOn++;
				}
			} while (currentMenu->menuitems[itemOn].status == -1);
			return true;
		}
		else {
			if (currentMenu->menuitems[itemOn].routine &&
				currentMenu->menuitems[itemOn].status >= 2) {
				currentMenu->menuitems[itemOn].routine(1);

				if (currentMenu->menuitems[itemOn].status == 3) {
					thermowait = -1;
				}
			}
			return true;
		}

	case KEY_ENTER:
		if (itemOn != itemSelected &&
			ev->type == ev_mousedown) {
			return false;
		}

		if (currentMenu == &PasswordDef) {
			M_PasswordSelect();
			return true;
		}
		else {
			if (currentMenu->menuitems[itemOn].routine &&
				currentMenu->menuitems[itemOn].status) {
				if (currentMenu->menuitems[itemOn].routine == M_Return) {
					M_Return(0);
					return true;
				}

				currentMenu->lastOn = itemOn;
				if (currentMenu == &featuresDef) {
					if (currentMenu->menuitems[itemOn].routine == M_DoFeature &&
						itemOn == features_levels) {
						gameaction = ga_warplevel;
						gamemap = nextmap = levelwarp + 1;
						M_ClearMenus();
						dmemset(passwordData, 0xff, 16);
						return true;
					}
				}
				else if (currentMenu->menuitems[itemOn].status >= 2 ||
					currentMenu->menuitems[itemOn].status == -2) {
					currentMenu->menuitems[itemOn].routine(2);
				}
				else {
					if (currentMenu == &ControlsDef) {
						// don't do the fade effect and jump straight to the next screen
						M_ChangeKeyBinding(itemOn);
					}
					else {
						currentMenu->menuitems[itemOn].routine(itemOn);
					}

					S_StartSound(NULL, sfx_pistol);
				}
			}
			return true;
		}

	case KEY_ESCAPE:
		//villsa
		if (gamestate == GS_SKIPPABLE || demoplayback) {
			return false;
		}

		M_ReturnInstant();
		return true;

	case KEY_DEL:
		if (currentMenu == &PasswordDef) {
			M_PasswordDeSelect();
		}
		else if (currentMenu == &ControlsDef) {
			if (currentMenu->menuitems[itemOn].routine) {
				G_UnbindAction(PlayerActions[itemOn].action);
				M_BuildControlMenu();
			}
		}
		return true;

	case KEY_BACKSPACE:
		M_Return(0);
		return true;

	case KEY_MWHEELUP:
		M_Scroll(currentMenu, true);
		return true;

	case KEY_MWHEELDOWN:
		M_Scroll(currentMenu, false);
		return true;

	case KEY_F5:
		// villsa 12292013 - allow screenshots from menu as well
		G_ScreenShot();
		return true;

	default:
		for (i = itemOn + 1; i < currentMenu->numitems; i++) {
			if (currentMenu->menuitems[i].status != -1
				&& currentMenu->menuitems[i].alphaKey == ch) {
				itemOn = i;
				S_StartSound(NULL, sfx_switch1);
				return true;
			}
		}
		for (i = 0; i <= itemOn; i++) {
			if (currentMenu->menuitems[i].status != -1
				&& currentMenu->menuitems[i].alphaKey == ch) {
				itemOn = i;
				S_StartSound(NULL, sfx_switch1);
				return true;
			}
		}
		break;
	}

	return false;
}

// restore mouse position to current cursor position so cursor is not displayed at a random position depending of current in-game mouse position
// cursor start initially centered
static void M_SetCursorPos() {
	if (m_menumouse.value) {
		I_SetMousePos(cursor_x, cursor_y);
		itemSelected = -1; // remove mouse hover item menu
	}
}


//
// M_StartControlPanel
//

void M_StartControlPanel(boolean forcenext) {
	if (!allowmenu) {
		return;
	}

	if (demoplayback) {
		return;
	}

	// intro might call this repeatedly
	if (menuactive) {
		return;
	}

	menuactive = true;
	menufadefunc = NULL;
	nextmenu = NULL;
	newmenu = forcenext;
	currentMenu = !usergame ? &MainDef : &PauseDef;
	itemOn = currentMenu->lastOn;

	S_PauseSound();

	if (!forcenext) {
		M_SetCursorPos();
	}
}

//
// M_StartMainMenu
//

void M_StartMainMenu(void) {
	currentMenu = &MainDef;
	itemOn = 0;
	allowmenu = true;
	menuactive = true;
	mainmenuactive = true;
	M_SetCursorPos();
}

//
// M_DrawMenuSkull
//
// Draws skull icon from the symbols lump
// Pretty straightforward stuff..
//

static void M_DrawMenuSkull(int x, int y) {
	int index = 0;
	float vx1 = 0.0f;
	float vy1 = 0.0f;
	float tx1 = 0.0f;
	float tx2 = 0.0f;
	float ty1 = 0.0f;
	float ty2 = 0.0f;
	float smbwidth;
	float smbheight;
	int pic;
	vtx_t vtx[4];
	const rcolor color = MENUCOLORWHITE;

	pic = GL_BindGfxTexture("SYMBOLS", true);

	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	dglEnable(GL_BLEND);
	dglSetVertex(vtx);

	GL_SetOrtho(0);

	index = (whichSkull & 7) + SM_SKULLS;

	vx1 = (float)x;
	vy1 = (float)y;

	smbwidth = (float)gfxwidth[pic];
	smbheight = (float)gfxheight[pic];

	const float ueps = 0.5f / smbwidth;
	const float veps = 0.5f / smbheight;

	tx1 = ((float)symboldata[index].x / smbwidth) + ueps;
	tx2 = ((float)(symboldata[index].x + symboldata[index].w) / smbwidth) - ueps;
	ty1 = ((float)symboldata[index].y / smbheight) + veps;
	ty2 = ((float)(symboldata[index].y + symboldata[index].h) / smbheight) - veps;

	GL_Set2DQuad(
		vtx,
		vx1,
		vy1,
		symboldata[index].w,
		symboldata[index].h,
		tx1, tx2, ty1, ty2,
		color
	);

	dglTriangle(0, 1, 2);
	dglTriangle(3, 2, 1);
	dglDrawGeometry(4, vtx);

	GL_ResetViewport();
	dglDisable(GL_BLEND);
}

//
// M_DrawCursor
//

static void M_DrawCursor()
{
	if (!m_menumouse.value) return;

	cursor_x = mouse_x;
	cursor_y = mouse_y;

	int gfxIdx;
	float factor;
	float scale;

	scale = ((m_cursorscale.value + 25.0f) / 100.0f);
	gfxIdx = GL_BindGfxTexture("CURSOR", true);
	if (gfxIdx < 0)
		return;

	factor = (((float)SCREENHEIGHT * video_ratio) / (float)video_width) / scale;

	// atsb: fixes cursor alpha
	dglDisable(GL_COLOR_LOGIC_OP);
	dglDisable(GL_DEPTH_TEST);
	dglDepthMask(GL_FALSE);

	GL_SetState(GLSTATE_BLEND, 1);
	dglEnable(GL_BLEND);
	dglDisable(GL_ALPHA_TEST);
	dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GL_SetOrthoScale(scale);

	dglColor4ub(255, 255, 255, 255);

	GL_SetupAndDraw2DQuad((float)cursor_x * factor, (float)cursor_y * factor,
		gfxwidth[gfxIdx], gfxheight[gfxIdx], 0, 1.0f, 0, 1.0f, WHITE, 0);

	GL_SetState(GLSTATE_BLEND, 0);
	dglDepthMask(GL_TRUE);
	GL_SetOrthoScale(1.0f);

}

//
// M_Drawer
//
// Called after the view has been rendered,
// but before it has been blitted.
//

void M_Drawer(void) {
	short x;
	short y;
	short i;
	short max;
	int start;
	int height;

	if (currentMenu != &MainDef) {
		ST_FlashingScreen(0, 0, 0, 96);
	}

	if (MenuBindActive) {
		Draw_BigText(-1, 64, MENUCOLORWHITE, "Press New Key For");
		Draw_BigText(-1, 80, MENUCOLORRED, MenuBindMessage);
		return;
	}

	if (!menuactive) {
		return;
	}

	Draw_BigText(-1, 16, MENUCOLORRED, currentMenu->title);

	if (currentMenu->scale != 1) {
		GL_SetOrthoScale(currentMenu->scale);
	}

	if (currentMenu->routine) {
		currentMenu->routine();    // call Draw routine
	}

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	start = currentMenu->menupageoffset;
	max = (currentMenu->numpageitems == -1) ? currentMenu->numitems : currentMenu->numpageitems;

	if (currentMenu->textonly) {
		height = TEXTLINEHEIGHT;
	}
	else {
		height = LINEHEIGHT;
	}

	if (currentMenu->smallfont) {
		height /= 2;
	}

	//
	// begin drawing all menu items
	//
	for (i = start; i < max + start; i++) {
		//
		// skip hidden items
		//
		if (currentMenu->menuitems[i].status == -3) {
			continue;
		}

		if (currentMenu == &PasswordDef) {
			if (i > 0) {
				if (!(i & 7)) {
					y += height;
					x = currentMenu->x;
				}
				else {
					x += TEXTLINEHEIGHT;
				}
			}
		}

		if (currentMenu->menuitems[i].status != -1) {
			//
			// blinking letter for password menu
			//
			if (currentMenu == &PasswordDef && gametic & 4 && i == itemOn) {
				continue;
			}

			if (!currentMenu->smallfont) {
				rcolor fontcolor = MENUCOLORRED;

				if (itemSelected == i) {
					fontcolor += D_RGBA(0, 128, 8, 0);
				}

				Draw_BigText(x, y, fontcolor, currentMenu->menuitems[i].name);
			}
			else {
				rcolor color = MENUCOLORWHITE;

				if (itemSelected == i) {
					color = D_RGBA(255, 255, 0, menualphacolor);
				}

				//
				// tint the non-bindable key items to a shade of red
				//
				if (currentMenu == &ControlsDef) {
					if (i >= (NUM_CONTROL_ITEMS)) {
						color = D_RGBA(255, 192, 192, menualphacolor);
					}
				}

				Draw_Text(
					x,
					y,
					color,
					currentMenu->scale,
					false,
					currentMenu->menuitems[i].name
				);

				//
				// nasty hack to re-set the scale after a drawtext call
				//
				if (currentMenu->scale != 1) {
					GL_SetOrthoScale(currentMenu->scale);
				}
			}
		}
		//
		// if menu item is static but has text, then display it as gray text
		// used for subcategories
		//
		else if (currentMenu->menuitems[i].name[0] != 0) {
			if (!currentMenu->smallfont) {
				Draw_BigText(
					-1,
					y,
					MENUCOLORWHITE,
					currentMenu->menuitems[i].name
				);
			}
			else {
				int strwidth = M_StringWidth(currentMenu->menuitems[i].name);

				Draw_Text(
					((int)(160.0f / currentMenu->scale) - (strwidth / 2)),
					y,
					D_RGBA(255, 0, 0, menualphacolor),
					currentMenu->scale,
					false,
					currentMenu->menuitems[i].name
				);

				//
				// nasty hack to re-set the scale after a drawtext call
				//
				if (currentMenu->scale != 1) {
					GL_SetOrthoScale(currentMenu->scale);
				}
			}
		}

		if (currentMenu != &PasswordDef) {
			y += height;
		}
	}

	//
	// display indicators that the user can scroll farther up or down
	//
	if (currentMenu->numpageitems != -1) {
		if (currentMenu->menupageoffset) {
			//up arrow
			Draw_BigText(currentMenu->x, currentMenu->y - 24, MENUCOLORWHITE, "/u More...");
		}

		if (currentMenu->menupageoffset + currentMenu->numpageitems < currentMenu->numitems) {
			//down arrow
			Draw_BigText(currentMenu->x, (currentMenu->y - 2 + (currentMenu->numpageitems - 1) * height) + 24,
				MENUCOLORWHITE, "/d More...");
		}
	}

	//
	// draw password cursor
	//
	if (currentMenu == &PasswordDef) {
		Draw_BigText((currentMenu->x + ((itemOn & 7) * height)) - 4,
			currentMenu->y + ((int)(itemOn / 8) * height) + 3, MENUCOLORWHITE, "/b");
	}
	else {
		// DRAW SKULL
		if (!currentMenu->smallfont) {
			int offset = 0;

			if (currentMenu->textonly) {
				x += SKULLXTEXTOFF;
			}
			else {
				x += SKULLXOFF;
			}

			if (itemOn) {
				for (i = itemOn; i > 0; i--) {
					if (currentMenu->menuitems[i].status == -3) {
						offset++;
					}
				}
			}

			M_DrawMenuSkull(x, currentMenu->y - 5 + ((itemOn - currentMenu->menupageoffset) - offset) * height);
		}
		//
		// draw arrow cursor
		//
		else {
			Draw_BigText(x - 12,
				currentMenu->y - 4 + (itemOn - currentMenu->menupageoffset) * height,
				MENUCOLORWHITE, "/l");
		}
	}

	if (currentMenu->scale != 1) {
		GL_SetOrthoScale(1.0f);
	}

	M_DrawCursor();
}

//
// M_ClearMenus
//

void M_ClearMenus(void) {
	if (!allowclearmenu) {
		return;
	}

	// center mouse before clearing menu
	// so the input code won't try to
	// re-center the mouse; which can
	// cause the player's view to warp
	if (gamestate == GS_LEVEL) {
		I_CenterMouse();
	}

	menufadefunc = NULL;
	nextmenu = NULL;
	menualphacolor = 0xff;
	menuactive = 0;

	S_ResumeSound();
}

//
// M_NextMenu
//

static void M_NextMenu(void) {
	currentMenu = nextmenu;
	itemOn = currentMenu->lastOn;
	menualphacolor = 0xff;
	alphaprevmenu = false;
	menufadefunc = NULL;
	nextmenu = NULL;
}

//
// M_SetupNextMenu
//

void M_SetupNextMenu(menu_t* menudef) {
	if (newmenu) {
		menufadefunc = M_NextMenu;
	}
	else {
		menufadefunc = M_MenuFadeOut;
	}

	alphaprevmenu = false;
	nextmenu = menudef;
	newmenu = false;
}

//
// M_MenuFadeIn
//

void M_MenuFadeIn(void) {
	int fadetime = (int)(m_menufadetime.value + 20);

	if ((menualphacolor + fadetime) < 0xff) {
		menualphacolor += fadetime;
	}
	else {
		menualphacolor = 0xff;
		alphaprevmenu = false;
		menufadefunc = NULL;
		nextmenu = NULL;
	}
}

//
// M_MenuFadeOut
//

void M_MenuFadeOut(void) {
	int fadetime = (int)(m_menufadetime.value + 20);

	if (menualphacolor > fadetime) {
		menualphacolor -= fadetime;
	}
	else {
		menualphacolor = 0;

		if (alphaprevmenu == false) {
			currentMenu = nextmenu;
			itemOn = currentMenu->lastOn;
		}
		else {
			currentMenu = currentMenu->prevMenu;
			itemOn = currentMenu->lastOn;
		}

		menufadefunc = M_MenuFadeIn;
	}
}

//
// M_Ticker
//

void M_Ticker(void) {
	mainmenuactive = (currentMenu == &MainDef) ? true : false;

	if ((currentMenu == &MainDef ||
		currentMenu == &PauseDef) && usergame && demoplayback) {
		menuactive = 0;
		return;
	}
	if (!usergame) {
		OptionsDef.prevMenu = &MainDef;
		LoadDef.prevMenu = &MainDef;
		SaveDef.prevMenu = &MainDef;
	}
	else {
		OptionsDef.prevMenu = &PauseDef;
		LoadDef.prevMenu = &PauseDef;
		SaveDef.prevMenu = &PauseDef;
	}

	// auto-adjust itemOn and page offset if the first menu item is being used as a header
	if (currentMenu->menuitems[0].status == -1 &&
		currentMenu->menuitems[0].name[0] != 0) {
		// bump page offset up
		if (itemOn == 1) {
			currentMenu->menupageoffset = 0;
		}

		// bump the cursor down
		if (itemOn <= 0) {
			itemOn = 1;
		}
	}

	if (menufadefunc) {
		menufadefunc();
	}

	// auto adjust page offset for long menu items
	if (currentMenu->numpageitems != -1) {
		if (itemOn >= (currentMenu->numpageitems + currentMenu->menupageoffset)) {
			currentMenu->menupageoffset = (itemOn + 1) - currentMenu->numpageitems;

			if (currentMenu->menupageoffset >= currentMenu->numitems) {
				currentMenu->menupageoffset = currentMenu->numitems;
			}
		}
		else if (itemOn < currentMenu->menupageoffset) {
			currentMenu->menupageoffset = itemOn;

			if (currentMenu->menupageoffset < 0) {
				currentMenu->menupageoffset = 0;
			}
		}
	}

	if (--skullAnimCounter <= 0) {
		whichSkull++;
		skullAnimCounter = 4;
	}

	if (thermowait != 0 && currentMenu->menuitems[itemOn].status == 3 &&
		currentMenu->menuitems[itemOn].routine) {
		currentMenu->menuitems[itemOn].routine(thermowait == -1 ? 1 : 0);
	}
}

void M_InitEpisodes() {
	int episodes = P_GetNumEpisodes();
	if (episodes <= 0) return;
	menuitem_t* menus = NULL;
	menus = Z_Realloc(menus, sizeof(menuitem_t) * episodes, PU_STATIC, 0);
	for (int i = 0; i < episodes; i++)
	{
		episodedef_t* epi = P_GetEpisode(i);
		menuitem_t menu;
		menu.status = 1;
		strcpy(menu.name, epi->name);
		menu.routine = M_ChooseMap;
		menu.alphaKey = epi->key;
		dmemcpy(&menus[i], &menu, sizeof(menuitem_t));
	}
	MapSelectDef.menuitems = menus;
	MapSelectDef.numitems = episodes;
	NewDef.prevMenu = &MapSelectDef;
}

//
// M_Init
//

void M_Init(void) {
	int i = 0;

	currentMenu = &MainDef;
	menuactive = 0;
	itemOn = currentMenu->lastOn;
	itemSelected = -1;
	whichSkull = 0;
	skullAnimCounter = 4;
	quickSaveSlot = -1;
	menufadefunc = NULL;
	nextmenu = NULL;
	newmenu = false;

	// cursor initially centered
	cursor_x = ((float)video_width * SCREENHEIGHT / SCREENWIDTH) / 2;
	cursor_y = video_height / 2.f;

	for (i = 0; i < NUM_CONTROL_ITEMS; i++) {
		ControlsItem[i].alphaKey = 0;
		dmemset(ControlsItem[i].name, 0, 64);
		ControlsItem[i].routine = NULL;
		ControlsItem[i].status = 1;
	}

	// setup password menu

	for (i = 0; i < 32; i++) {
		PasswordMenu[i].status = 1;
		PasswordMenu[i].name[0] = passwordChar[i];
		PasswordMenu[i].routine = NULL;
		PasswordMenu[i].alphaKey = (char)passwordChar[i];
	}

	dmemset(passwordData, 0xff, 16);

	MainDef.y += 8;
	NewDef.prevMenu = &MainDef;

	M_InitEpisodes();
	M_InitShiftXForm();
}

//
// M_RegisterCvars
//

void M_RegisterCvars(void) {
	CON_CvarRegister(&m_menumouse);
	CON_CvarRegister(&m_cursorscale);
	CON_CvarRegister(&m_menufadetime);
}

// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      DOOM selection menu, options, episode etc.
//      Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#include "i_opndir.h"
#else
#include <dirent.h>
#endif

#include <fcntl.h>
#include "doomdef.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "d_englsh.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "d_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "g_actions.h"
#include "g_game.h"
#include "s_sound.h"
#include "doomstat.h"
#include "sounds.h"
#include "m_menu.h"
#include "m_fixed.h"
#include "d_devstat.h"
#include "r_main.h"
#include "r_things.h"
#include "r_lights.h"
#include "m_shift.h"
#include "m_password.h"
#include "r_wipe.h"
#include "st_stuff.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "gl_texture.h"
#include "gl_draw.h"
#include "i_w3swrapper.h"
//
// definitions
//

#define MENUSTRINGSIZE      32
#define SKULLXOFF           -40    //villsa: changed from -32 to -40
#define SKULLXTEXTOFF       -24
#define LINEHEIGHT          18
#define TEXTLINEHEIGHT      18
#define MENUCOLORRED        D_RGBA(255, 0, 0, menualphacolor)
#define MENUCOLORWHITE      D_RGBA(255, 255, 255, menualphacolor)
#define MENUCOLORYELLOW     D_RGBA(194, 174, 28, menualphacolor)

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
static boolean showfullitemvalue[3] = { false, false, false };
static int      levelwarp = 0;
static int      thermowait = 0;
static int      m_aspectRatio = 0;
static int      m_ScreenSize = 1;

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
void M_ClearMenus(void);

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
static void M_DrawSaveGameFrontend(menu_t* def);
static void M_SetInputString(char* string, int len);
static void M_Scroll(menu_t* menu, boolean up);
static void M_DoVideoReset(int choice);

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
	SDL_ShowCursor(cvar->value < 1);
	if (cvar->value <= 0) {
		itemSelected = -1;
		SDL_ShowCursor(cvar->value = 0);
	}
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
quitprompt_e;

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
quit2prompt_e;

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
rlprompt_e;

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
startnewnotify_e;

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
void M_ChooseMapLost(int choice);

enum {
	doom64,
	lostlevels,
	mapg_end
} mapselect_e;

menuitem_t MapMenu[] = {
	{1,"DOOM 64",M_ChooseMap, 'd'},
	{1,"Lost Levels",M_ChooseMapLost, 'l'},
};

menu_t MapSelectDef = {
	mapg_end,
	false,
	&MainDef,
	MapMenu,
	NULL,
	"Choose Campaign...",
	112,80,
	doom64,
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
void M_ChooseSkillLost(int choice);

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

menuitem_t NewGameMenuLost[] = {
	{1,"Be Gentle!",M_ChooseSkillLost, 'b'},
	{1,"Bring It On!",M_ChooseSkillLost, 'r'},
	{1,"I Own Doom!",M_ChooseSkillLost, 'i'},
	{1,"Watch Me Die!",M_ChooseSkillLost, 'w'},
	{1,"Hardcore!",M_ChooseSkillLost, 'h'},
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

menu_t NewDefLost = {
	newg_end,
	false,
	&MainDef,
	NewGameMenuLost,
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
	
	M_SetupNextMenu(&MapSelectDef);
}

void M_ChooseMap(int choice) {
	if (netgame && !demoplayback) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&StartNewNotifyDef);
		return;
	}
    
	P_GetEpisode(choice)->mapid;
	M_SetupNextMenu(&NewDef);
}

void M_ChooseMapLost(int choice) {
	if (netgame && !demoplayback) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&StartNewNotifyDef);
		return;
	}

	M_SetupNextMenu(&NewDefLost);
}

void M_ChooseSkill(int choice) {
	G_DeferedInitNew(choice, map);
	M_ClearMenus();
	memset(passwordData, 0xff, 16);
	allowmenu = false;
}

void M_ChooseSkillLost(int choice) {
	G_DeferedInitNew(choice, 34);
	M_ClearMenus();
	memset(passwordData, 0xff, 16);
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
	options_network,
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
	{1,"Network",M_Network, 'n'},
	{1,"/r Return",M_Return, 0x20}
};

char* OptionHints[opt_end] = {
	"control configuration",
	"adjust sound volume",
	"miscellaneous options for gameplay and other features",
	"settings for the heads-up display",
	"configure video-specific options",
	"enter a password to access a level",
	"setup options for a hosted session",
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
CVAR_EXTERNAL(p_allowjump);
//André: remove autoaim and use the normal aim instead.  CVAR_EXTERNAL(p_autoaim);
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
	network_allowjump,
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
	{2,"Allow Jumping:", M_NetworkChoice, 'j'},
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
	{ &p_allowjump, 0 },
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
	"allow players to jump",
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
	15,
	0,
	0.5f,
	NetworkHints,
	NULL
};

void M_Network(int choice) {
	M_SetupNextMenu(&NetworkDef);
	strcpy(inputString, m_playername.string);
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
	case network_allowjump:
		M_SetOptionValue(choice, 0, 1, 1, &p_allowjump);
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

	static const char* networkscalestrings[3] = {
		"x 1",
		"x 2",
		"x 3"
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
	DRAWNETWORKITEM(network_allowjump, p_allowjump.value, msgNames);
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
CVAR_EXTERNAL(r_skybox);
CVAR_EXTERNAL(p_autorun);
CVAR_EXTERNAL(p_usecontext);
CVAR_EXTERNAL(compat_mobjpass);
CVAR_EXTERNAL(r_wipe);
CVAR_EXTERNAL(hud_disablesecretmessages);
CVAR_EXTERNAL(m_nospawnsound);
CVAR_EXTERNAL(m_obituaries);
CVAR_EXTERNAL(m_brutal);

enum {
	misc_header1,
	misc_menufade,
	misc_empty1,
	misc_menumouse,
	misc_cursorsize,
	misc_empty2,
	misc_header2,
	misc_jump,
	misc_autorun,
	misc_context,
	misc_header3,
	misc_wipe,
	misc_skybox,
	misc_header4,
	misc_showkey,
	misc_showlocks,
	misc_amobjects,
	misc_amoverlay,
	misc_nospawnsound,
	misc_obituaries,
	misc_brutal,
	misc_header5,
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
	{2,"Jumping:",M_MiscChoice, 'j'},
	{2,"Always Run:",M_MiscChoice, 'z' },
	{2,"Use Context:",M_MiscChoice, 'u'},
	{-1,"Misc",0 },
	{2,"Screen Melt:",M_MiscChoice, 's' },
	{2,"Skybox:",M_MiscChoice,'k'},
	{-1,"Automap",0 },
	{2,"Key Pickups:",M_MiscChoice },
	{2,"Locked Doors:",M_MiscChoice },
	{2,"Draw Objects:",M_MiscChoice },
	{2,"Overlay:",M_MiscChoice },
	{2,"No Spawn Sound:",M_MiscChoice },
	{2,"Obituaries:",M_MiscChoice },
	{2,"Brutal Mode:",M_MiscChoice },
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
	"toggle the ability to jump",
	"enable autorun",
	"if enabled interactive objects will highlight when near",
	NULL,
	"enable the melt effect when completing a level",
	"toggle skies to render either normally or as skyboxes",
	NULL,
	"display key pickups in automap",
	"colorize locked doors accordingly to the key in automap",
	"set how objects are rendered in automap",
	"render the automap into the player hud",
	"spawn sound toggle",
	"death messages",
	"get knee deep in the gibs",
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
	{ &p_allowjump, 0 },
	{ &p_autorun, 1 },
	{ &p_usecontext, 0 },
	{ &r_wipe, 1 },
	{ &r_skybox, 0 },
	{ &hud_disablesecretmessages, 0 },
	{ &am_showkeymarkers, 0 },
	{ &am_showkeycolors, 0 },
	{ &am_drawobjects, 0 },
	{ &am_overlay, 0 },
	{ &m_nospawnsound, 0 },
	{ &m_obituaries, 0 },
	{ &m_brutal, 0 },
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

	case misc_jump:
		M_SetOptionValue(choice, 0, 1, 1, &p_allowjump);
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

	case misc_skybox:
		M_SetOptionValue(choice, 0, 1, 1, &r_skybox);
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
	DRAWMISCITEM(misc_jump, p_allowjump.value, msgNames);
	DRAWMISCITEM(misc_context, p_usecontext.value, mapdisplaytype);
	DRAWMISCITEM(misc_wipe, r_wipe.value, msgNames);
	DRAWMISCITEM(misc_autorun, p_autorun.value, autoruntype);
	DRAWMISCITEM(misc_skybox, r_skybox.value, msgNames);
	DRAWMISCITEM(misc_showkey, am_showkeymarkers.value, mapdisplaytype);
	DRAWMISCITEM(misc_showlocks, am_showkeycolors.value, mapdisplaytype);
	DRAWMISCITEM(misc_amobjects, am_drawobjects.value, objectdrawtype);
	DRAWMISCITEM(misc_amoverlay, am_overlay.value, msgNames);
	DRAWMISCITEM(misc_nospawnsound, m_nospawnsound.value, disablesecretmessages);
	DRAWMISCITEM(misc_obituaries, m_obituaries.value, autoruntype);
	DRAWMISCITEM(misc_brutal, m_brutal.value, autoruntype);
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
void M_ChangeMouseLook(int choice);
void M_ChangeMouseInvert(int choice);
void M_ChangeYAxisMove(int choice);
void M_ChangeXAxisMove(int choice);
void M_DrawMouse(void);

CVAR_EXTERNAL(v_msensitivityx);
CVAR_EXTERNAL(v_msensitivityy);
CVAR_EXTERNAL(v_mlook);
CVAR_EXTERNAL(v_mlookinvert);
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
	mouse_look,
	mouse_invert,
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
	{2,"Mouse Look:",M_ChangeMouseLook,'l'},
	{2,"Invert Look:",M_ChangeMouseInvert, 'i'},
	{2,"Y-Axis Move:",M_ChangeYAxisMove, 'y'},
        {2, "X-Axis Move:", M_ChangeXAxisMove, 'x'},
	{-2,"Default",M_DoDefaults,'d'},
	{1,"/r Return",M_Return, 0x20}
};

menudefault_t MouseDefault[] = {
	{ &v_msensitivityx, 5 },
	{ &v_msensitivityy, 5 },
	{ &v_macceleration, 0 },
	{ &v_mlook, 0 },
	{ &v_mlookinvert, 0 },
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
	0.925f,
	NULL,
	MouseBars
};

void M_DrawMouse(void) {
	M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * (mouse_sensx + 1), MAXSENSITIVITY, v_msensitivityx.value);
	M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * (mouse_sensy + 1), MAXSENSITIVITY, v_msensitivityy.value);

	M_DrawThermo(MouseDef.x, MouseDef.y + LINEHEIGHT * (mouse_accel + 1), 20, v_macceleration.value);

	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_look, MENUCOLORRED,
		msgNames[(int)v_mlook.value]);
	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_invert, MENUCOLORRED,
		msgNames[(int)v_mlookinvert.value]);
	Draw_BigText(MouseDef.x + 144, MouseDef.y + LINEHEIGHT * mouse_yaxismove, MENUCOLORRED,
		msgNames[(int)v_yaxismove.value]);
	Draw_BigText(144, MouseDef.x + LINEHEIGHT * mouse_xaxismove, MENUCOLORRED,
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

void M_ChangeMouseLook(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_mlook);
}

void M_ChangeMouseInvert(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_mlookinvert);
}

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
void M_ChangeCrosshair(int choice);
void M_ChangeOpacity(int choice);
void M_DrawDisplay(void);
void M_ChangeHUDColor(int choice);

CVAR_EXTERNAL(st_drawhud);
CVAR_EXTERNAL(st_crosshair);
CVAR_EXTERNAL(st_crosshairopacity);
CVAR_EXTERNAL(st_flashoverlay);
CVAR_EXTERNAL(st_showpendingweapon);
CVAR_EXTERNAL(st_showstats);
CVAR_EXTERNAL(m_messages);
CVAR_EXTERNAL(p_damageindicator);
CVAR_EXTERNAL(st_hud_color);

enum {
	messages,
	statusbar,
	display_flash,
	display_damage,
	display_weapon,
	display_stats,
	display_hud_color,
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
	{3,"HUD Colour",M_ChangeHUDColor, 'o'},
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
	"change the hud text colour",
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
	{ &st_hud_color, 0 },
	{ &st_crosshair, 0 },
	{ &st_crosshairopacity, 80 },
	{ NULL, -1 }
};

menuthermobar_t DisplayBars[] = {
	{ display_empty1, 255, &st_crosshairopacity },
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
	0.715f,
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
	Draw_BigText(DisplayDef.x + 140, DisplayDef.y + LINEHEIGHT * display_hud_color, MENUCOLORRED,
		hud_color[(int)st_hud_color.value]);

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
}

void M_ToggleFlashOverlay(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &st_flashoverlay);
}

void M_ChangeCrosshair(int choice) {
	M_SetOptionValue(choice, 0, (float)st_crosshairs, 1, &st_crosshair);
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
void M_ChangeFilter(int choice);
void M_ChangeWindowed(int choice);
void M_ChangeRatio(int choice);
void M_ChangeResolution(int choice);
void M_ChangeAnisotropic(int choice);
void M_ChangeInterpolateFrames(int choice);
void M_ChangeVerticalSynchronisation(int choice);
void M_ChangeAccessibility(int choice);
void M_DrawVideo(void);

CVAR_EXTERNAL(v_width);
CVAR_EXTERNAL(v_height);
CVAR_EXTERNAL(v_windowed);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(i_gamma);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(r_filter);
CVAR_EXTERNAL(r_anisotropic);
CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(v_vsync);
CVAR_EXTERNAL(v_accessibility);

enum {
	video_dbrightness,
	video_empty1,
	video_dgamma,
	video_empty2,
	filter,
	anisotropic,
	windowed,
	ratio,
	resolution,
	interpolate_frames,
	vsync,
	accessibility,
	v_default,
	v_videoreset,
	video_return,
	video_end
} video_e;

menuitem_t VideoMenu[] = {
	{3,"Brightness",M_ChangeBrightness, 'b'},
	{-1,"",0},
	{3,"Gamma Correction",M_ChangeGammaLevel, 'g'},
	{-1,"",0},
	{2,"Filter:",M_ChangeFilter, 'f'},
	{2,"Anisotropy:",M_ChangeAnisotropic, 'a'},
	{2,"Windowed:",M_ChangeWindowed, 'w'},
	{2,"Aspect Ratio:",M_ChangeRatio, 'a'},
	{2,"Resolution:",M_ChangeResolution, 'r'},
	{2,"Interpolation:",M_ChangeInterpolateFrames, 'i'},
	{2,"Vsync:",M_ChangeVerticalSynchronisation, 'v'},
	{2,"Accessibility:",M_ChangeAccessibility, 'y'},
	{2,"Apply Settings",M_DoVideoReset, 's'},
	{-2,"Default",M_DoDefaults, 'e'},
	{1,"/r Return",M_Return, 0x20}
};

char* VideoHints[video_end] = {
	"change light color intensity",
	NULL,
	"adjust screen gamma",
	NULL,
	"toggle texture filtering",
	"toggle blur reduction on textures",
	"toggle windowed mode",
	"select aspect ratio",
	"resolution changes will take effect\n after restarting",
	"toggle frame interpolation to\n achieve smooth framerates",
	"toggle vertical synchronisation to \n reduce screen tear",
	"toggle accessibility to \n remove flashing lights",
	"apply video settings"
};

menudefault_t VideoDefault[] = {
	{ &i_brightness, 0 },
	{ &i_gamma, 0 },
	{ &r_filter, 0 },
	{ &r_anisotropic, 1 },
	{ &v_windowed, 0 },
	{ &i_interpolateframes, 1 },
	{ &v_vsync, 1 },
	{ &v_accessibility, 0 },
	{ NULL, -1 }
};

menuthermobar_t VideoBars[] = {
	{ video_empty1, 20, &i_gamma },
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

#define MAX_RES4_3  14
static const int Resolution4_3[MAX_RES4_3][2] = {
	{   256,    192     },	
	{   320,    240     },
	{   640,    480     },
	{   768,    576     },
	{   800,    600     },
	{   1024,   768     },
	{   1152,   864     },
	{   1280,   960     },
	{   1400,   1050    },
	{   1600,   1200    },
	{   2048,   1536    },
	{   3200,   2400    },
	{   4096,   3072    },
	{   6400,   4800    }
};

#define MAX_RES5_4  3
static const int Resolution5_4[MAX_RES5_4][2] = {
	{   1280,   1024     },
	{   2560,   2048     },
	{   5120,   4096     }
};
#define MAX_RES16_9  12
static const int Resolution16_9[MAX_RES16_9][2] = {
	{   640,    360     },
	{   854,    480     },
	{   1024,   576     },
	{   1024,   600     },
	{   1280,   720     },
	{   1366,   768     },
	{   1600,   900     },
	{   1920,   1080    },
	{   2048,   1152    },
	{   2560,   1440    },
	{   2880,   1620    },
	{   3840,   2160    }
};

#define MAX_RES16_10  10
static const int Resolution16_10[MAX_RES16_10][2] = {
	{   320,    200     },
	{   1024,   640     },
	{	1280,   600		},
	{   1280,   800     },
	{   1440,   900     },
	{   1680,   1050    },
	{   1920,   1200    },
	{   2560,   1600    },
	{   3840,   2400    },
	{   7680,   4800    }
};

#define MAX_RES21_09  2
static const int Resolution21_09[MAX_RES21_09][2] = {
	{   2560,    1080     },
	{   3840,    2160     }
}; //For the samsung ultrawide monitors lol.

static const float ratioVal[5] = {
	4.0f / 3.0f,
	16.0f / 9.0f,
	16.0f / 10.0f,
	5.0f / 4.0f,
	21.0f / 9.0f
};
static int8_t gammamsg[21][28] = {
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
	float checkratio;
	int i;

	M_SetupNextMenu(&VideoDef);

	checkratio = v_width.value / v_height.value;

	if (fcmp(checkratio, ratioVal[2])) {
		m_aspectRatio = 2;
	}
	else if (fcmp(checkratio, ratioVal[1])) {
		m_aspectRatio = 1;
	}
	else if (fcmp(checkratio, ratioVal[3])) {
		m_aspectRatio = 3;
	}
	else if (fcmp(checkratio, ratioVal[4])) {
		m_aspectRatio = 4;
	}
	else if (fcmp(checkratio, ratioVal[5])) {
		m_aspectRatio = 5;
	}
	else {
		m_aspectRatio = 0;
	}

	switch (m_aspectRatio) {
	case 0:
		for (i = 0; i < MAX_RES4_3; i++) {
			if ((int)v_width.value == Resolution4_3[i][0]) {
				m_ScreenSize = i;
				return;
			}
		}
		break;
	case 1:
		for (i = 0; i < MAX_RES16_9; i++) {
			if ((int)v_width.value == Resolution16_9[i][0]) {
				m_ScreenSize = i;
				return;
			}
		}
		break;
	case 2:
		for (i = 0; i < MAX_RES16_10; i++) {
			if ((int)v_width.value == Resolution16_10[i][0]) {
				m_ScreenSize = i;
				return;
			}
		}
		break;
	case 3:
		for (i = 0; i < MAX_RES5_4; i++) {
			if ((int)v_width.value == Resolution5_4[i][0]) {
				m_ScreenSize = i;
				return;
			}
		}
		break;
	case 4:
		for (i = 0; i < MAX_RES21_09; i++) {
			if ((int)v_width.value == Resolution21_09[i][0]) {
				m_ScreenSize = i;
				return;
			}
		}
		break;
	}

	m_ScreenSize = 1;
}

void M_DrawVideo(void) {
	static const char* constfilterType[2] = { "Linear", "Nearest" };
	static const char* ratioName[5] = { "4 : 3", "16 : 9", "16 : 10", "5 : 4", "21 : 09" };
	static const char* frametype[2] = { "Off", "On" };
#ifdef VITA	 
    static const char* vsyncType[3] = { "Unlimited", "60 Fps", "30 Fps" }; //Add this later for debug
	static char bitValue[8];
#endif

	char res[16];
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

	DRAWVIDEOITEM2(filter, r_filter.value, constfilterType);
	DRAWVIDEOITEM2(anisotropic, r_anisotropic.value, msgNames);
	DRAWVIDEOITEM2(windowed, v_windowed.value, msgNames);
	DRAWVIDEOITEM2(ratio, m_aspectRatio, ratioName);

	sprintf(res, "%ix%i", (int)v_width.value, (int)v_height.value);
	DRAWVIDEOITEM(resolution, res);
	DRAWVIDEOITEM2(interpolate_frames, i_interpolateframes.value, frametype);
	DRAWVIDEOITEM2(vsync, v_vsync.value, frametype);
	DRAWVIDEOITEM2(accessibility, v_accessibility.value, frametype);

#undef DRAWVIDEOITEM
#undef DRAWVIDEOITEM2
	/*
	Draw_Text(120, 308, MENUCOLORWHITE, VideoDef.scale, false,
			  "Resolution changes will take effect\nafter restarting the game..");
	GL_SetOrthoScale(VideoDef.scale);
	*/
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

void M_ChangeFilter(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &r_filter);
}

void M_ChangeAnisotropic(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &r_anisotropic);
}

void M_ChangeWindowed(int choice) {
	M_SetOptionValue(choice, 0, 1, 1, &v_windowed);
}

static void M_SetResolution(void) {
	int width = SCREENWIDTH;
	int height = SCREENHEIGHT;

	switch (m_aspectRatio) {
	case 0:
		width = Resolution4_3[m_ScreenSize][0];
		height = Resolution4_3[m_ScreenSize][1];
		break;
	case 1:
		width = Resolution16_9[m_ScreenSize][0];
		height = Resolution16_9[m_ScreenSize][1];
		break;
	case 2:
		width = Resolution16_10[m_ScreenSize][0];
		height = Resolution16_10[m_ScreenSize][1];
		break;
	case 3:
		width = Resolution5_4[m_ScreenSize][0];
		height = Resolution5_4[m_ScreenSize][1];
		break;
	case 4:
		width = Resolution21_09[m_ScreenSize][0];
		height = Resolution21_09[m_ScreenSize][1];
		break;
	}

	M_SetCvar(&v_width, (float)width);
	M_SetCvar(&v_height, (float)height);
}

void M_ChangeRatio(int choice) {
	int dmax = 0;

	if (choice) {
		if (++m_aspectRatio > 4) {
			if (choice == 3) {
				m_aspectRatio = 0;
			}
			else {
				m_aspectRatio = 4;
			}
		}
	}
	else {
		m_aspectRatio = max(m_aspectRatio--, 0);
	}

	switch (m_aspectRatio) {
	case 0:
		dmax = MAX_RES4_3;
		break;
	case 1:
		dmax = MAX_RES16_9;
		break;
	case 2:
		dmax = MAX_RES16_10;
		break;
	case 3:
		dmax = MAX_RES5_4;
		break;
	case 4:
		dmax = MAX_RES21_09;
		break;
	}
	m_ScreenSize = min(m_ScreenSize, dmax - 1);
	M_SetResolution();
}

void M_ChangeResolution(int choice) {
	int dmax = 0;

	switch (m_aspectRatio) {
	case 0:
		dmax = MAX_RES4_3;
		break;
	case 1:
		dmax = MAX_RES16_9;
		break;
	case 2:
		dmax = MAX_RES16_10;
		break;
	case 3:
		dmax = MAX_RES5_4;
		break;
	case 4:
		dmax = MAX_RES21_09;
		break;
	}

	if (choice) {
		if (++m_ScreenSize > dmax - 1) {
			if (choice == 2) {
				m_ScreenSize = 0;
			}
			else {
				m_ScreenSize = dmax - 1;
			}
		}
	}
	else {
		m_ScreenSize = max(m_ScreenSize--, 0);
	}
	M_SetResolution();
}

void M_ChangeInterpolateFrames(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &i_interpolateframes);
}

void M_ChangeVerticalSynchronisation(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &v_vsync);
}

void M_ChangeAccessibility(int choice)
{
	M_SetOptionValue(choice, 0, 1, 1, &v_accessibility);
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
	unsigned char* passData;
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

	memset(password, 0, 2);
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
	passwordData[curPasswordSlot++] = (unsigned char)itemOn;
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
	{ &s_sfxvol, 80 },
	{ &s_musvol, 80 },
	{ NULL, -1 }
};

menuthermobar_t SoundBars[] = {
	{ sfx_empty1, 100, &s_sfxvol },
	{ sfx_empty2, 100, &s_musvol },
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
	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 100, s_sfxvol.value);
	M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), 100, s_musvol.value);
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
		if (s_sfxvol.value < 100.0f) {
			M_SetCvar(&s_sfxvol, s_sfxvol.value + 1);
		}
		else {
			CON_CvarSetValue(s_sfxvol.name, 100);
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
		if (s_musvol.value < 100.0f) {
			M_SetCvar(&s_musvol, s_musvol.value + 1);
		}
		else {
			CON_CvarSetValue(s_musvol.name, 100);
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

#include "g_controls.h"

//------------------------------------------------------------------------
//
// GAMEPAD CONTROLLER MENU
//
//------------------------------------------------------------------------
void M_XGamePadChoice(int choice);
void M_DrawXGamePad(void);

CVAR_EXTERNAL(i_rsticksensitivityy);
CVAR_EXTERNAL(i_rsticksensitivityx);
CVAR_EXTERNAL(i_xinputscheme);
enum {
	xgp_sensitivityx,
	xgp_empty1,
	xgp_sensitivityy,
	xgp_empty2,
	xgp_look,
	xgp_invert,
	xgp_default,
	xgp_return,
	xgp_end
} xgp_e;

menuitem_t XGamePadMenu[] = {
	{3,"Look Sensitivity x",M_XGamePadChoice,'s'},
	{-1,"",0},
	{3,"Look Sensitivity y",M_XGamePadChoice,'t'},
	{-1,"",0},
	{2,"Y Axis Look:",M_ChangeMouseLook,'l'},
	{2,"Invert Look:",M_ChangeMouseInvert, 'i'},
	{-2,"Default",M_DoDefaults,'d'},
	{1,"/r Return",M_Return, 0x20}
};

menudefault_t XGamePadDefault[] = {
	{ &i_rsticksensitivityx, 2.0f },
	{ &i_rsticksensitivityy, 1.5f},
	{ &v_mlook, 0 },
	{ &v_mlookinvert, 0 },
	{ NULL, -1 }
};

menu_t XGamePadDef = {
	xgp_end,
	false,
	&ControlMenuDef,
	XGamePadMenu,
	M_DrawXGamePad,
	"Gamepad Menu",
	88,48,
	0,
	false,
	XGamePadDefault,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_XGamePadChoice(int choice) {
#ifdef VITA
	float slope1 = 10.0f / 100.0f;
	float slope2 = 10.0f / 100.0f;
#else
	float slope1 = 0.0125f / 100.0f;
	float slope2 = 100.0f / 50.0f;
#endif

	switch (itemOn) {
	case xgp_sensitivityx:
		if (choice) {
			if (i_rsticksensitivityx.value < 0.0125f) {
				M_SetCvar(&i_rsticksensitivityx, i_rsticksensitivityx.value + slope1);
			}
			else {
				CON_CvarSetValue(&i_rsticksensitivityx, 0.0125f);
			}
		}
		else {
			if (i_rsticksensitivityy.value > 0.001f) {
				M_SetCvar(&i_rsticksensitivityx, i_rsticksensitivityy.value - slope1);
			}
			else {
				CON_CvarSetValue(&i_rsticksensitivityx, 0.001f);
			}
		}
		break;

	case xgp_sensitivityy:
		if (choice) {
			if (i_rsticksensitivityy.value < 10.0f) {
				M_SetCvar(&i_rsticksensitivityy, i_rsticksensitivityy.value + slope2);
			}
			else {
				CON_CvarSetValue(&i_rsticksensitivityy, 100);
			}
		}
		else {
			if (i_rsticksensitivityy.value > 1) {
				M_SetCvar(&i_rsticksensitivityy, i_rsticksensitivityy.value - slope2);
			}
			else {
				CON_CvarSetValue(&i_rsticksensitivityy, 1);
			}
		}
		break;
	}
}

void M_DrawXGamePad(void) {
	M_DrawThermo(XGamePadDef.x, XGamePadDef.y + LINEHEIGHT * (xgp_sensitivityx + 1),
		100, i_rsticksensitivityx.value * 10.0f);

	M_DrawThermo(XGamePadDef.x, XGamePadDef.y + LINEHEIGHT * (xgp_sensitivityy + 1),
		50, i_rsticksensitivityy.value * 0.5f);

	Draw_BigText(XGamePadDef.x + 128, XGamePadDef.y + LINEHEIGHT * xgp_look, MENUCOLORRED,
		msgNames[(int)v_mlook.value]);

	Draw_BigText(XGamePadDef.x + 128, XGamePadDef.y + LINEHEIGHT * xgp_invert, MENUCOLORRED,
		msgNames[(int)v_mlookinvert.value]);
}
//------------------------------------------------------------------------
//
// CONTROLS MENU
//
//------------------------------------------------------------------------

void M_ChangeKeyBinding(int choice);
void M_BuildControlMenu(void);
void M_DrawControls(void);

#define NUM_NONBINDABLE_ITEMS   9
#define NUM_CONTROL_ACTIONS     42
#define NUM_CONTROL_ITEMS        NUM_CONTROL_ACTIONS + NUM_NONBINDABLE_ITEMS

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
	{"Jump", "+jump"},
	{"Look Up", "+lookup"},
	{"Look Down", "+lookdown"},
	{"Center View", "+center"},
	{"Weapons", NULL},
	{"Next Weapon", "nextweap"},
	{"Previous Weapon", "prevweap"},
	{"Fist", "weapon 2"},
	{"Pistol", "weapon 3"},
	{"Shotgun(s)", "weapon 4"},
	{"Chaingun", "weapon 6"},
	{"Rocket Launcher", "weapon 7"},
	{"Plasma Rifle", "weapon 8"},
	{"BFG 9000", "weapon 9"},
	{"Unmaker", "weapon 10"},
	{"Chainsaw", "weapon 1"},
	{"Automap", NULL},
	{"Toggle", "automap"},
	{"Zoom In", "+automap_in"},
	{"Zoom Out", "+automap_out"},
	{"Pan Left", "+automap_left"},
	{"Pan Right", "+automap_right"},
	{"Pan Up", "+automap_up"},
	{"Pan Down", "+automap_down"},
	{"Mouse Pan", "+automap_freepan"},
	{"Follow Mode", "automap_follow"},
	{"Other", NULL},
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
	menu->numitems = actions + NUM_NONBINDABLE_ITEMS;
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
		strcpy(menu->menuitems[item].name, PlayerActions[item].name);
		if (PlayerActions[item].action) {
			for (i = strlen(PlayerActions[item].name); i < 17; i++) {
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

#define ADD_NONBINDABLE_ITEM(i, str, s)                 \
    strcpy(menu->menuitems[actions + i].name, str);    \
    menu->menuitems[actions + i].status = s;            \
    menu->menuitems[actions + i].routine = NULL

	ADD_NONBINDABLE_ITEM(0, "Non-Bindable Keys", -1);
	ADD_NONBINDABLE_ITEM(1, "Save Game        F2", 1);
	ADD_NONBINDABLE_ITEM(2, "Load Game        F3", 1);
	ADD_NONBINDABLE_ITEM(3, "Screenshot       F5", 1);
	ADD_NONBINDABLE_ITEM(4, "Quicksave        F6", 1);
	ADD_NONBINDABLE_ITEM(5, "Quickload        F7", 1);
	ADD_NONBINDABLE_ITEM(6, "Change Gamma     F11", 1);
	ADD_NONBINDABLE_ITEM(7, "Chat             t", 1);
	ADD_NONBINDABLE_ITEM(8, "Console          TILDE and BACKQUOTE", 1);
}

void M_ChangeKeyBinding(int choice) {
	char action[128];
	sprintf(action, "%s %d", PlayerActions[choice].action, 1);
	strcpy(MenuBindBuff, action);
	messageBindCommand = MenuBindBuff;
	sprintf(MenuBindMessage, "%s", PlayerActions[choice].name);
	MenuBindActive = true;
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
	controls_gamepad,
	controls_return,
	controls_end
} controls_e;

menuitem_t ControlsMenu[] = {
	{1,"Bindings",M_ControlChoice, 'k'},
	{1,"Mouse",M_ControlChoice, 'm'},
	{1,"Gamepad",M_ControlChoice, 'g'},
	{1,"/r Return",M_Return, 0x20}
};

char* ControlsHints[controls_end] = {
	"configure bindings",
	"configure mouse functionality",
	"configure gamecontroller functionality",
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
	case controls_gamepad:
		M_SetupNextMenu(&XGamePadDef);
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
// QUICKSAVE CONFIRMATION
//
//------------------------------------------------------------------------

void M_DrawQuickSaveConfirm(void);

enum {
	QS_Ok = 0,
	QS_End
};
qsconfirm_e;

menuitem_t QuickSaveConfirm[] = {
	{1,"Ok",M_ReturnToOptions,'o'}
};

menu_t QuickSaveConfirmDef = {
	QS_End,
	false,
	&PauseDef,
	QuickSaveConfirm,
	M_DrawQuickSaveConfirm,
	" ",
	144,112,
	QS_Ok,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_DrawQuickSaveConfirm(void) {
	Draw_BigText(-1, 16, MENUCOLORRED, "You Need To Pick");
	Draw_BigText(-1, 32, MENUCOLORRED, "A Quicksave Slot!");
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
netloadnotify_e;

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
savedeadnotify_e;

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
	strcpy(inputString, savegamestrings[choice]);
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
	G_LoadGame(P_GetSaveGameName(choice));
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
		handle = w3sopen(P_GetSaveGameName(i), O_RDONLY | 0, 0666); //This macro needs to be fixed...
		if (handle == -1) {
			strcpy(&savegamestrings[i][0], EMPTYSTRING);
			DoomLoadMenu[i].status = 0;
			continue;
		}

		w3sread(handle, &savegamestrings[i], MENUSTRINGSIZE);
		w3sclose(handle);
		DoomLoadMenu[i].status = 1;
	}
}

//------------------------------------------------------------------------
//
// QUICKSAVE PROMPT
//
//------------------------------------------------------------------------

void M_QuickSaveResponse(int ch);

enum {
	QSP_Yes = 0,
	QSP_No,
	QSP_End
};
quicksaveprompt_e;

menuitem_t QuickSavePrompt[] = {
	{1,"Yes",M_QuickSaveResponse,'y'},
	{1,"No",M_ReturnToOptions,'n'}
};

menu_t QuickSavePromptDef = {
	QSP_End,
	false,
	&PauseDef,
	QuickSavePrompt,
	NULL,
	"Overwrite Quicksave?",
	144,112,
	QSP_Yes,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_QuickSaveResponse(int ch) {
	M_DoSave(quickSaveSlot);
}

//------------------------------------------------------------------------
//
// QUICKLOAD PROMPT
//
//------------------------------------------------------------------------

void M_QuickLoadResponse(int ch);

enum {
	QLP_Yes = 0,
	QLP_No,
	QLP_End
};
quickloadprompt_e;

menuitem_t QuickLoadPrompt[] = {
	{1,"Yes",M_QuickLoadResponse,'y'},
	{1,"No",M_ReturnToOptions,'n'}
};

menu_t QuickLoadPromptDef = {
	QLP_End,
	false,
	&PauseDef,
	QuickLoadPrompt,
	NULL,
	"Load Quicksave?",
	144,112,
	QLP_Yes,
	false,
	NULL,
	-1,
	0,
	1.0f,
	NULL,
	NULL
};

void M_QuickLoadResponse(int ch) {
	M_LoadSelect(quickSaveSlot);
}

//------------------------------------------------------------------------
//
// COMMON MENU FUNCTIONS
//
//------------------------------------------------------------------------

//
// M_SetCvar
//

static int prevtic = 0; // hack - check for overlapping sounds
static void M_SetCvar(cvar_t* cvar, float value) {
	if (cvar->value == value) {
		return;
	}

	if (prevtic != gametic) {
		S_StartSound(NULL,
			currentMenu->menuitems[itemOn].status == 3 ? sfx_secmove : sfx_switch2);

		prevtic = gametic;
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
		CON_CvarSetValue(v_width.name, 640);
		CON_CvarSetValue(v_height.name, 480);

		GL_DumpTextures();
		GL_SetTextureFilter();
	}

	S_StartSound(NULL, sfx_switch2);
}

//
// M_DoVideoReset
//
static void M_DoVideoReset(int choice) {
	GL_Init();
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
	strcpy(oldInputString, string);

	// hack
	if (!strcmp(string, EMPTYSTRING)) {
#ifdef VITA
        // autoname the save
        snprintf(inputString, SAVESTRINGSIZE-1, "SAVEGAME%d", saveSlot);
#else
        inputString[0] = 0;
#endif
	}

	inputCharIndex = strlen(inputString);
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

//
// M_StringWidth
// Find string width from hu_font chars
//

static int M_StringWidth(const char* string) {
	int i;
	int w = 0;
	int c;

	for (i = 0; i < strlen(string); i++) {
		c = toupper(string[i]) - ST_FONTSTART;
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

	len = strlen(string);

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
	int x;
	int y;
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
	if (!(ev->data1 & 1) || !(ev->data2 | ev->data3)) {
		return;
	}

	bar = menu->thermobars;
	x = menu->x;
	y = menu->y;
	mx = (float)mouse_x;
	my = (float)mouse_y;
	scalex = ((float)video_width /
		((float)SCREENHEIGHT * video_ratio)) * menu->scale;
	scaley = ((float)video_height /
		(float)SCREENHEIGHT) * menu->scale;
	startx = (float)x * scalex;
	width = startx + (100.0f * scalex);

	// check if cursor is within range
	for (i = 0; bar[i].item != -1; i++) {
		lineheight = (float)(LINEHEIGHT * bar[i].item);
		scrny = ((float)y + lineheight + 10) * scaley;

		if (my < scrny) {
			scrny = ((float)y + lineheight + 2) * scaley;
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
	mx = (float)mouse_x;
	my = (float)mouse_y;
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

void M_QuickSave(void) {
	if (!usergame) {
		S_StartSound(NULL, sfx_oof);
		return;
	}

	if (gamestate != GS_LEVEL) {
		return;
	}

	if (quickSaveSlot < 0) {
		M_StartControlPanel(true);
		M_ReadSaveStrings();
		M_SetupNextMenu(&SaveDef);
		quickSaveSlot = -2;     // means to pick a slot now
		return;
	}

	M_StartControlPanel(true);
	M_SetupNextMenu(&QuickSavePromptDef);
}

void M_QuickLoad(void) {
	if (netgame) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&NetLoadNotifyDef);
		return;
	}

	if (quickSaveSlot < 0) {
		M_StartControlPanel(true);
		M_SetupNextMenu(&QuickSaveConfirmDef);
		return;
	}

	M_StartControlPanel(true);
	M_SetupNextMenu(&QuickLoadPromptDef);
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
	unsigned char* data;

	data = Z_Malloc(SAVEGAMETBSIZE, PU_STATIC, 0);

	//
	// poke into savegame file and fetch
	// date and stats
	//
	if (!P_QuickReadSaveHeader(P_GetSaveGameName(which), thumbnail_date, (int*)data,
		&thumbnail_skill, &thumbnail_map)) {
		Z_Free(data);
		return 0;
	}

	Z_Free(data);

	return 1;
}

//
// M_DrawSaveGameFrontend
//

static void M_DrawSaveGameFrontend(menu_t* def) {
	GL_SetState(GLSTATE_BLEND, 1);
	GL_SetOrtho(0);

	glDisable(GL_TEXTURE_2D);

	//
	// draw back panels
	//
	glColor4ub(4, 4, 4, menualphacolor);
	//
	// save game panel
	//
	glRecti(
		def->x - 48,
		def->y - 12,
		def->x + 256,
		def->y + 156
	);
	//
	// stats panel
	//
	glRecti(
		def->x + 272,
		def->y - 12,
		def->x + 464,
		def->y + 116
	);

	//
	// draw outline for panels
	//
	glColor4ub(240, 86, 84, menualphacolor);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//
	// save game panel
	//
	glRecti(
		def->x - 48,
		def->y - 12,
		def->x + 256,
		def->y + 156
	);
	//
	// stats panel
	//
	glRecti(
		def->x + 272,
		def->y - 12,
		def->x + 464,
		def->y + 116
	);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);

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

const symboldata_t xinputbutons[12] = {
	{ 0, 0, 15, 16 },   // B
	{ 15, 0, 15, 16 },  // A
	{ 30, 0, 15, 16 },  // Y
	{ 45, 0, 15, 16 },  // X
	{ 60, 0, 19, 16 },  // LB
	{ 79, 0, 19, 16 },  // RB
	{ 98, 0, 15, 16 },  // LEFT
	{ 113, 0, 15, 16 }, // RIGHT
	{ 128, 0, 15, 16 }, // UP
	{ 143, 0, 15, 16 }, // DOWN
	{ 158, 0, 12, 16 }, // START
	{ 170, 0, 12, 16 }  // SELECT
};

//
// M_DrawXInputButton
//

void M_DrawXInputButton(int x, int y, int button) {
	int index = 0;
	float vx1 = 0.0f;
	float vy1 = 0.0f;
	float tx1 = 0.0f;
	float tx2 = 0.0f;
	float ty1 = 0.0f;
	float ty2 = 0.0f;
	float width;
	float height;
	int pic;
	vtx_t vtx[4];
	const unsigned int color = MENUCOLORWHITE;

	switch (button) {
	case GAMEPAD_A:
		index = 0;
		break;
	case GAMEPAD_B:
		index = 1;
		break;
	case GAMEPAD_X:
		index = 2;
		break;
	case GAMEPAD_Y:
		index = 3;
		break;
	case GAMEPAD_BACK:
		index = 4;
		break;
	case GAMEPAD_GUIDE:
		index = 5;
		break;
	case GAMEPAD_START:
		index = 6;
		break;
	case GAMEPAD_LSTICK:
		index = 7;
		break;
	case GAMEPAD_RSTICK:
		index = 8;
		break;
	case GAMEPAD_LSHOULDER:
		index = 9;
		break;
	case GAMEPAD_RSHOULDER:
		index = 10;
		break;
	case GAMEPAD_DPAD_UP:
		index = 11;
		break;
	case GAMEPAD_DPAD_DOWN:
		index = 12;
		break;
	case GAMEPAD_DPAD_LEFT:
		index = 13;
		break;
	case GAMEPAD_DPAD_RIGHT:
		index = 14;
		break;
	case GAMEPAD_BUTTON_MISC1:
		index = 15; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
		break;
	case GAMEPAD_BUTTON_PADDLE1:
		index = 16; /* Xbox Elite paddle P1 */
		break;
	case GAMEPAD_BUTTON_PADDLE2:
		index = 17; /* Xbox Elite paddle P3 */
		break;
	case GAMEPAD_BUTTON_PADDLE3:  /* Xbox Elite paddle P2 */
		index = 18;
		break;
	case GAMEPAD_BUTTON_PADDLE4: /* Xbox Elite paddle P4 */
		index = 19;
		break;
	case GAMEPAD_BUTTON_TOUCHPAD: /* PS4/PS5 touchpad button */
		index = 20;
		break;
	case GAMEPAD_LTRIGGER:
		index = 21;
		break;
	case GAMEPAD_RTRIGGER:
		index = 18;
		break;	

	default:
		return;
	}

	pic = GL_BindGfxTexture("BUTTONS", true);

	width = (float)gfxwidth[pic];
	height = (float)gfxheight[pic];

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glEnable(GL_BLEND);
	glSetVertex(vtx);

	GL_SetOrtho(0);

	vx1 = (float)x;
	vy1 = (float)y;

	tx1 = ((float)xinputbutons[index].x / width) + 0.001f;
	tx2 = (tx1 + (float)xinputbutons[index].w / width) - 0.001f;
	ty1 = ((float)xinputbutons[index].y / height) + 0.005f;
	ty2 = (ty1 + (((float)xinputbutons[index].h / height))) - 0.008f;

	GL_Set2DQuad(
		vtx,
		vx1,
		vy1,
		xinputbutons[index].w,
		xinputbutons[index].h,
		tx1,
		tx2,
		ty1,
		ty2,
		color
	);

	glTriangle(0, 1, 2);
	glTriangle(3, 2, 1);
	glDrawGeometry(4, vtx);

	GL_ResetViewport();
	glDisable(GL_BLEND);
}
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
	else if (ev->type == ev_mouse && (ev->data2 | ev->data3)) {
		// handle mouse-over selection
		if (m_menumouse.value) {
			M_CheckDragThermoBar(ev, currentMenu);
			if (M_CursorHighlightItem(currentMenu)) {
				itemOn = itemSelected;
			}
		}
	}

	if (ch == -1) {
		return false;
	}

	if (MenuBindActive == true) { //key Bindings
		if (ch == KEY_ESCAPE) {
			MenuBindActive = false;
			M_BuildControlMenu();
		}
		else if (G_BindActionByEvent(ev, messageBindCommand)) {
			MenuBindActive = false;
			M_BuildControlMenu();
		}
		return true;
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
			strcpy(inputString, oldInputString);
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
				strcpy(savegamestrings[saveSlot], inputString);
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

	// F-Keys
	if (!menuactive) {
		switch (ch) {
		case KEY_F2:            // Save
			M_StartControlPanel(true);
			M_SaveGame(0);
			return true;

		case KEY_F3:            // Load
			M_StartControlPanel(true);
			M_LoadGame(0);
			return true;

		case KEY_F5:
			G_ScreenShot();
			return true;

		case KEY_F6:            // Quicksave
			M_QuickSave();
			return true;

		case KEY_F7:            // Quickload
			M_QuickLoad();
			return true;

		case KEY_F11:           // gamma toggle
			M_ChangeGammaLevel(2);
			return true;
		}
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
				if (currentMenu->menuitems[itemOn].status >= 2 ||
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
	M_StartControlPanel(false);
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
	const unsigned int color = MENUCOLORWHITE;

	pic = GL_BindGfxTexture("SYMBOLS", true);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glEnable(GL_BLEND);
	glSetVertex(vtx);

	GL_SetOrtho(0);

	index = (whichSkull & 7) + SM_SKULLS;

	vx1 = (float)x;
	vy1 = (float)y;

	smbwidth = (float)gfxwidth[pic];
	smbheight = (float)gfxheight[pic];

	tx1 = ((float)symboldata[index].x / smbwidth) + 0.001f;
	tx2 = (tx1 + (float)symboldata[index].w / smbwidth) - 0.001f;
	ty1 = ((float)symboldata[index].y / smbheight) + 0.005f;
	ty2 = (ty1 + (((float)symboldata[index].h / smbheight))) - 0.008f;

	GL_Set2DQuad(
		vtx,
		vx1,
		vy1,
		symboldata[index].w,
		symboldata[index].h,
		tx1,
		tx2,
		ty1,
		ty2,
		color
	);

	glTriangle(0, 1, 2);
	glTriangle(3, 2, 1);
	glDrawGeometry(4, vtx);

	GL_ResetViewport();
	glDisable(GL_BLEND);
}

//
// M_DrawCursor
//

static void M_DrawCursor(int x, int y) {
	if (m_menumouse.value) {
		int gfxIdx;
		float factor;
		float scale;

		scale = ((m_cursorscale.value + 25.0f) / 100.0f);
		gfxIdx = GL_BindGfxTexture("CURSOR", true);
		factor = (((float)SCREENHEIGHT * video_ratio) / (float)video_width) / scale;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		GL_SetOrthoScale(scale);
		GL_SetState(GLSTATE_BLEND, 1);
		GL_SetupAndDraw2DQuad((float)x * factor, (float)y * factor,
			gfxwidth[gfxIdx], gfxheight[gfxIdx], 0, 1.0f, 0, 1.0f, WHITE, 0);

		GL_SetState(GLSTATE_BLEND, 0);
		GL_SetOrthoScale(1.0f);
	}
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
			    unsigned int fontcolor = MENUCOLORRED;

				if (itemSelected == i) {
					fontcolor += D_RGBA(0, 128, 8, 0);
				}

				Draw_BigText(x, y, fontcolor, currentMenu->menuitems[i].name);
			}
			else {
				unsigned int color = MENUCOLORWHITE;

				if (itemSelected == i) {
					color = D_RGBA(255, 255, 0, menualphacolor);
				}

				//
				// tint the non-bindable key items to a shade of red
				//
				if (currentMenu == &ControlsDef) {
					if (i >= (NUM_CONTROL_ITEMS - NUM_NONBINDABLE_ITEMS)) {
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
		else if (currentMenu->menuitems[i].name != NULL) {
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

	if (currentMenu != &MainDef) {
		GL_SetOrthoScale(0.75f);
		if (currentMenu == &PasswordDef) {
			M_DrawXInputButton(4, 271, GAMEPAD_B);
			Draw_Text(22, 276, MENUCOLORWHITE, 0.75f, false, "Change");
		}

		GL_SetOrthoScale(0.75f);
		M_DrawXInputButton(4, 287, GAMEPAD_A);
		Draw_Text(22, 292, MENUCOLORWHITE, 0.75f, false, "Select");

		if (currentMenu != &PauseDef) {
			GL_SetOrthoScale(0.75f);
			M_DrawXInputButton(5, 303, GAMEPAD_START);
			Draw_Text(22, 308, MENUCOLORWHITE, 0.75f, false, "Return");
		}

		GL_SetOrthoScale(1);
	}

	M_DrawCursor(mouse_x, mouse_y);
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
		currentMenu->menuitems[0].name != NULL) {
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
	int i;
	if (episodes <= 0) return;
	menuitem_t* menus = NULL;
	menus = Z_Realloc(menus, sizeof(menuitem_t) * episodes, PU_STATIC, 0);
	for(i = 0; i < episodes; i++)
	{
		episodedef_t* epi = P_GetEpisode(i);
		menuitem_t menu;
		menu.status = 1;
		strcpy(menu.name, epi->name);
		menu.routine = M_ChooseMap;
		menu.alphaKey = epi->key;
		memcpy(&menus[i], &menu, sizeof(menuitem_t));
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

	for (i = 0; i < NUM_CONTROL_ITEMS; i++) {
		ControlsItem[i].alphaKey = 0;
		memset(ControlsItem[i].name, 0, 64);
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

	memset(passwordData, 0xff, 16);

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

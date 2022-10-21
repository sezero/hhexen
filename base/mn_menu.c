//**************************************************************************
//**
//** mn_menu.c : Heretic 2 : Raven Software, Corp.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <ctype.h>
#include "h2def.h"
#include "p_local.h"
#include "r_local.h"
#include "i_cdmus.h"
#include "soundst.h"
#include "v_compat.h"

// MACROS ------------------------------------------------------------------

#define LEFT_DIR		0
#define RIGHT_DIR		1
#define ITEM_HEIGHT		20
#define SMALL_ITEM_HEIGHT	9
#define MENU_MAX_MOUSE_SENS	50

#define SELECTOR_XOFFSET	(-28)
#define SELECTOR_YOFFSET	(-1)
#define SLOTTEXTLEN		16
#define ASCII_CURSOR		'['

#ifdef RENDER3D
#define V_DrawPatch(x,y,p)		OGL_DrawPatch((x),(y),(p))
#define V_DrawRawScreen(a)		OGL_DrawRawScreen((a))
#endif

// TYPES -------------------------------------------------------------------

typedef enum
{
	ITT_EMPTY,
	ITT_EFUNC,
	ITT_LRFUNC,
	ITT_SETMENU,
	ITT_INERT,
	ITT_SETKEY
} ItemType_t;

typedef enum
{
	MENU_MAIN,
	MENU_CLASS,
	MENU_SKILL,
	MENU_OPTIONS,
	MENU_OPTIONS2,
	MENU_OPTIONS3,
	MENU_FILES,
	MENU_LOAD,
	MENU_SAVE,
	MENU_NONE
} MenuType_t;

typedef struct
{
	ItemType_t type;
	const char *text;
	void (*func)(int option);
	int option;
	MenuType_t menu;
} MenuItem_t;

typedef struct
{
	int x;
	int y;
	void (*drawFunc)(void);
	int itemCount;
	MenuItem_t *items;
	int oldItPos;
	int step;
	MenuType_t prevMenu;
} Menu_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static const char *Key2String(int key);
static void ClearControls(int key);
static void InitFonts(void);
static void SetTheMenu(MenuType_t menu);
static void SCQuitGame(int option);
static void SCClass(int option);
static void SCSkill(int option);
static void SCMouseSensi(int option);
static void SCSfxVolume(int option);
static void SCMusicVolume(int option);
static void SCScreenSize(int option);
static boolean SCNetCheck(int option);
static void SCNetCheck2(int option);
static void SCLoadGame(int option);
static void SCSaveGame(int option);
static void SCMessages(int option);
static void SCEndGame(int option);
static void SCInfo(int option);
static void SCSetKey(int option);
static void SCMouselook(int option);
static void SCAlwaysRun(int option);
static void SCCDAudio(int option);
static void DrawMainMenu(void);
static void DrawClassMenu(void);
static void DrawSkillMenu(void);
static void DrawOptionsMenu(void);
static void DrawOptions2Menu(void);
static void DrawOptions3Menu(void);
static void DrawFileSlots(Menu_t *menu);
static void DrawFilesMenu(void);
static void MN_DrawInfo(void);
static void DrawLoadMenu(void);
static void DrawSaveMenu(void);
static void DrawSlider(Menu_t *menu, int item, int width, int slot);
static void MN_LoadSlotText(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern default_t defaults[];

extern int detailLevel;
extern int screenblocks;
extern boolean gamekeydown[MAXKEYS];

extern int alwaysrun;
extern int mouselook;

extern int key_right,key_left,key_up,key_down;
extern int key_straferight,key_strafeleft,key_jump;
extern int key_fire, key_use, key_strafe, key_speed;
extern int key_flyup, key_flydown, key_flycenter;
extern int key_lookup, key_lookdown, key_lookcenter;
extern int key_invleft, key_invright, key_useartifact;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

boolean MenuActive;
int InfoType;
int messageson;	/* boolean */
boolean mn_SuicideConsole;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int FontABaseLump;
static int FontAYellowBaseLump;
static int FontBBaseLump;
static int MauloBaseLump;
static Menu_t *CurrentMenu;
static int CurrentItPos;
static int MenuPClass;
static int MenuTime;
static boolean soundchanged;

#ifdef RENDER3D
static float bgAlpha = 0;
static float outFade = 0;
static boolean fadingOut = false;
static int menuDarkTicks = 15;
static int slamInTicks = 9;
#endif

boolean askforquit;
static boolean FileMenuKeySteal;
static boolean slottextloaded;
static char SlotText[6][SLOTTEXTLEN+2];
static char oldSlotText[SLOTTEXTLEN+2];
static int SlotStatus[6];
static int slotptr;
static int currentSlot;
static int quicksave;
static int quickload;
static int typeofask;

static int FirstKey = 0;
static boolean askforkey = false;
static int keyaskedfor;

static MenuItem_t MainItems[] =
{
	{ ITT_SETMENU, "NEW GAME", SCNetCheck2, 1, MENU_CLASS },
	{ ITT_SETMENU, "OPTIONS", NULL, 0, MENU_OPTIONS },
	{ ITT_SETMENU, "GAME FILES", NULL, 0, MENU_FILES },
	{ ITT_EFUNC, "INFO", SCInfo, 0, MENU_NONE },
	{ ITT_EFUNC, "QUIT GAME", SCQuitGame, 0, MENU_NONE }
};

static Menu_t MainMenu =
{
	110, 56,
	DrawMainMenu,
	5, MainItems,
	0,
	ITEM_HEIGHT,
	MENU_NONE
};

static MenuItem_t ClassItems[] =
{
	{ ITT_EFUNC, "FIGHTER", SCClass, 0, MENU_NONE },
	{ ITT_EFUNC, "CLERIC", SCClass, 1, MENU_NONE },
	{ ITT_EFUNC, "MAGE", SCClass, 2, MENU_NONE }
};

static Menu_t ClassMenu =
{
	66, 66,
	DrawClassMenu,
	NUMCLASSES_HUMAN,
	ClassItems,
	0,
	ITEM_HEIGHT,
	MENU_MAIN
};

static MenuItem_t FilesItems[] =
{
	{ ITT_SETMENU, "LOAD GAME", SCNetCheck2, 2, MENU_LOAD },
	{ ITT_SETMENU, "SAVE GAME", NULL, 0, MENU_SAVE }
};

static Menu_t FilesMenu =
{
	110, 60,
	DrawFilesMenu,
	2, FilesItems,
	0,
	ITEM_HEIGHT,
	MENU_MAIN
};

static MenuItem_t LoadItems[] =
{
	{ ITT_EFUNC, NULL, SCLoadGame, 0, MENU_NONE },
	{ ITT_EFUNC, NULL, SCLoadGame, 1, MENU_NONE },
	{ ITT_EFUNC, NULL, SCLoadGame, 2, MENU_NONE },
	{ ITT_EFUNC, NULL, SCLoadGame, 3, MENU_NONE },
	{ ITT_EFUNC, NULL, SCLoadGame, 4, MENU_NONE },
	{ ITT_EFUNC, NULL, SCLoadGame, 5, MENU_NONE }
};

static Menu_t LoadgameMenu =
{
	70, 30,
	DrawLoadMenu,
	6, LoadItems,
	0,
	ITEM_HEIGHT,
	MENU_FILES
};

static MenuItem_t SaveItems[] =
{
	{ ITT_EFUNC, NULL, SCSaveGame, 0, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSaveGame, 1, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSaveGame, 2, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSaveGame, 3, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSaveGame, 4, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSaveGame, 5, MENU_NONE }
};

static Menu_t SavegameMenu =
{
	70, 30,
	DrawSaveMenu,
	6, SaveItems,
	0,
	ITEM_HEIGHT,
	MENU_FILES
};

static MenuItem_t SkillItems[] =
{
	{ ITT_EFUNC, NULL, SCSkill, sk_baby, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSkill, sk_easy, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSkill, sk_medium, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSkill, sk_hard, MENU_NONE },
	{ ITT_EFUNC, NULL, SCSkill, sk_nightmare, MENU_NONE }
};

static Menu_t SkillMenu =
{
	120, 44,
	DrawSkillMenu,
	5, SkillItems,
	2,
	ITEM_HEIGHT,
	MENU_CLASS
};

static MenuItem_t OptionsItems[] =
{
	{ ITT_EFUNC, "END GAME", SCEndGame, 0, MENU_NONE },
	{ ITT_EFUNC, "MESSAGES : ", SCMessages, 0, MENU_NONE },
	{ ITT_LRFUNC, "MOUSE SENSITIVITY :", SCMouseSensi, 0, MENU_NONE },
	{ ITT_SETMENU, "CONTROL SETUP", NULL, 0, MENU_OPTIONS3 },
	{ ITT_LRFUNC, "MOUSELOOK : ", SCMouselook, 0, MENU_NONE },
	{ ITT_EFUNC, "ALWAYS RUN : ", SCAlwaysRun, 0, MENU_NONE },
	{ ITT_SETMENU, "MORE...", NULL, 0, MENU_OPTIONS2 }
};

static Menu_t OptionsMenu =
{
	88, 30,
	DrawOptionsMenu,
	7,
	OptionsItems,
	0,
	ITEM_HEIGHT,
	MENU_MAIN
};

static MenuItem_t Options2Items[] =
{
	{ ITT_LRFUNC, "SCREEN SIZE", SCScreenSize, 0, MENU_NONE },
	{ ITT_EMPTY, NULL, NULL, 0, MENU_NONE },
	{ ITT_LRFUNC, "SFX VOLUME", SCSfxVolume, 0, MENU_NONE },
	{ ITT_EMPTY, NULL, NULL, 0, MENU_NONE },
	{ ITT_LRFUNC, "MUSIC VOLUME", SCMusicVolume, 0, MENU_NONE },
	{ ITT_EMPTY, NULL, NULL, 0, MENU_NONE },
	{ ITT_EFUNC, "CD AUDIO :", SCCDAudio, 0, MENU_NONE }
};

static Menu_t Options2Menu =
{
	90, 20,
	DrawOptions2Menu,
	7,
	Options2Items,
	0,
	ITEM_HEIGHT,
	MENU_OPTIONS
};

static MenuItem_t Options3Items[] =
{
/* see defaults[] in m_misc.c for the correct option number:
 * key_right corresponds to defaults[3], which means that we
 * are using the (index_number - 3) here.
 */
	{ ITT_SETKEY, "TURN RIGHT :", SCSetKey, 0, MENU_NONE },
	{ ITT_SETKEY, "TURN LEFT :", SCSetKey, 1, MENU_NONE },
	{ ITT_SETKEY, "MOVE FORWARD :", SCSetKey, 2, MENU_NONE },
	{ ITT_SETKEY, "MOVE BACK :" , SCSetKey, 3, MENU_NONE },
	{ ITT_SETKEY, "STRAFE LEFT :", SCSetKey, 4, MENU_NONE },
	{ ITT_SETKEY, "STRAFE RIGHT :", SCSetKey, 5, MENU_NONE },
	{ ITT_SETKEY, "FLY UP :", SCSetKey, 6, MENU_NONE },
	{ ITT_SETKEY, "FLY DOWN :", SCSetKey, 7, MENU_NONE },
	{ ITT_SETKEY, "FLY CENTER :", SCSetKey, 8, MENU_NONE },
	{ ITT_SETKEY, "LOOK UP :", SCSetKey, 9, MENU_NONE },
	{ ITT_SETKEY, "LOOK DOWN :", SCSetKey, 10, MENU_NONE },
	{ ITT_SETKEY, "LOOK CENTER :", SCSetKey, 11, MENU_NONE },
	{ ITT_SETKEY, "INVENTORY LEFT :", SCSetKey, 12, MENU_NONE },
	{ ITT_SETKEY, "INVENTORY RIGHT :", SCSetKey, 13, MENU_NONE },
	{ ITT_SETKEY, "USE ARTIFACT :", SCSetKey, 14, MENU_NONE },
	{ ITT_SETKEY, "FIRE :", SCSetKey, 15, MENU_NONE },
	{ ITT_SETKEY, "USE :", SCSetKey, 16, MENU_NONE },
	{ ITT_SETKEY, "STRAFE :", SCSetKey, 17, MENU_NONE },
	{ ITT_SETKEY, "SPEED :", SCSetKey, 18, MENU_NONE },
	{ ITT_SETKEY, "JUMP :", SCSetKey, 19, MENU_NONE }
};

/* Many items in Options3Items[], only 15 can be drawn on a page:
 * So, FirstKey changes between 0 and FIRSTKEY_MAX. This menu is
 * way too fragile. Should we adapt from Quake's M_Menu_Keys and
 * bindnames?? */
#define FIRSTKEY_MAX	5
static Menu_t Options3Menu =
{
	70, 20,
	DrawOptions3Menu,
	15, /* actually 20 */
	Options3Items,
	0,
	SMALL_ITEM_HEIGHT,
	MENU_OPTIONS
};

static Menu_t *Menus[] =
{
	&MainMenu,
	&ClassMenu,
	&SkillMenu,
	&OptionsMenu,
	&Options2Menu,
	&Options3Menu,
	&FilesMenu,
	&LoadgameMenu,
	&SavegameMenu
};

static const char *mlooktext[] =
{
	"OFF",
	"NORMAL",
	"INVERSE"
};

static const char *stupidtable[] =
{
	"A","B","C","D","E",
	"F","G","H","I","J",
	"K","L","M","N","O",
	"P","Q","R","S","T",
	"U","V","W","X","Y",
	"Z"
};

static const char *GammaText[] =
{
	TXT_GAMMA_LEVEL_OFF,
	TXT_GAMMA_LEVEL_1,
	TXT_GAMMA_LEVEL_2,
	TXT_GAMMA_LEVEL_3,
	TXT_GAMMA_LEVEL_4
};

// CODE --------------------------------------------------------------------

static const char *Key2String (int key)
{
/* S.A.: return "[" or "]" or "\"" doesn't work
 * because there are no lumps for these chars,
 * therefore we have to go with "RIGHT BRACKET"
 * and similar for much punctuation.  Probably
 * won't work with international keyboards and
 * dead keys, either.
 */
	switch (key)
	{
	case KEY_LEFTBRACKET:	return "LEFT BRACK";
	case KEY_RIGHTBRACKET:	return "RIGHT BRACK";
	case KEY_BACKQUOTE:	return "BACK QUOTE";
	case KEY_QUOTE:		return "'";
	case KEY_QUOTEDBL:	return "DOUBLE QUOTE";
	case KEY_SEMICOLON:	return ";";
	case KEY_MINUS:		return "-";
	case KEY_PERIOD:	return ".";
	case KEY_COMMA:		return ",";
	case KEY_SLASH:		return "/";
	case KEY_BACKSLASH:	return "BACKSLASH";
	case KEY_TAB:		return "TAB";
	case KEY_EQUALS:	return "=";

	case KEY_RIGHTARROW:	return "RIGHT ARROW";
	case KEY_LEFTARROW:	return "LEFT ARROW";
	case KEY_DOWNARROW:	return "DOWN ARROW";
	case KEY_UPARROW:	return "UP ARROW";
	case KEY_ENTER:		return "ENTER";
	case KEY_PGUP:		return "PAGE UP";
	case KEY_PGDN:		return "PAGE DOWN";
	case KEY_INS:		return "INSERT";
	case KEY_HOME:		return "HOME";
	case KEY_END:		return "END";
	case KEY_DEL:		return "DELETE";
	case ' ':		return "SPACE";
	case KEY_RSHIFT:	return "SHIFT";
	case KEY_RALT:		return "ALT";
	case KEY_RCTRL:		return "CTRL";
	}
	/* Handle letter keys */
	/* S.A.: could also be done with toupper */
	if (key >= 'a' && key <= 'z')
		return stupidtable[(key - 'a')];

	return "?";		/* Everything else */
}

static void ClearControls (int key)
{
	int i;

	for (i = 3; i < 24; i++)
	{
		if (*defaults[i].location == key)
			*defaults[i].location = 0;
	}
}

//---------------------------------------------------------------------------
//
// PROC MN_Init
//
//---------------------------------------------------------------------------

void MN_Init(void)
{
	InitFonts();
	MenuActive = false;
//	messageson = 1;	// Set by defaults in .CFG
	MauloBaseLump = W_GetNumForName("FBULA0"); // ("M_SKL00");
}

//---------------------------------------------------------------------------
//
// PROC InitFonts
//
//---------------------------------------------------------------------------

static void InitFonts(void)
{
	FontABaseLump = W_GetNumForName("FONTA_S") + 1;
	FontAYellowBaseLump = W_GetNumForName("FONTAY_S") + 1;
	FontBBaseLump = W_GetNumForName("FONTB_S") + 1;
}

//---------------------------------------------------------------------------
//
// PROC MN_DrTextA
//
// Draw text using font A.
//
//---------------------------------------------------------------------------

void MN_DrTextA(const char *text, int x, int y)
{
	char c;
	patch_t *p;

#ifdef RENDER3D
	OGL_SetColorAndAlpha(1, 1, 1, 1);
#endif

	while ((c = *text++) != 0)
	{
		if (c < 33)
		{
			x += 5;
		}
		else
		{
			p = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
#ifdef RENDER3D
			OGL_DrawPatch_CS(x, y, FontABaseLump + c - 33);
#else
			V_DrawPatch(x, y, p);
#endif
			x += SHORT(p->width) - 1;
		}
	}
}

//==========================================================================
//
// MN_DrTextAYellow
//
//==========================================================================

void MN_DrTextAYellow(const char *text, int x, int y)
{
	char c;
	patch_t *p;

#ifdef RENDER3D
	OGL_SetColorAndAlpha(1, 1, 1, 1);
#endif

	while ((c = *text++) != 0)
	{
		if (c < 33)
		{
			x += 5;
		}
		else
		{
			p = (patch_t *) W_CacheLumpNum(FontAYellowBaseLump + c - 33, PU_CACHE);
#ifdef RENDER3D
			OGL_DrawPatch_CS(x, y, FontAYellowBaseLump + c - 33);
#else
			V_DrawPatch(x, y, p);
#endif
			x += SHORT(p->width) - 1;
		}
	}
}

//---------------------------------------------------------------------------
//
// FUNC MN_TextAWidth
//
// Returns the pixel width of a string using font A.
//
//---------------------------------------------------------------------------

int MN_TextAWidth(const char *text)
{
	char c;
	int width;
	patch_t *p;

	width = 0;
	while ((c = *text++) != 0)
	{
		if (c < 33)
		{
			width += 5;
		}
		else
		{
			p = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
			width += SHORT(p->width) - 1;
		}
	}
	return (width);
}

//---------------------------------------------------------------------------
//
// PROC MN_DrTextB
//
// Draw text using font B.
//
//---------------------------------------------------------------------------

void MN_DrTextB(const char *text, int x, int y)
{
	char c;
	patch_t *p;

#ifdef RENDER3D
	OGL_SetColorAndAlpha(1, 1, 1, 1);
#endif

	while ((c = *text++) != 0)
	{
		if (c < 33)
		{
			x += 8;
		}
		else
		{
			p = (patch_t *) W_CacheLumpNum(FontBBaseLump + c - 33, PU_CACHE);
#ifdef RENDER3D
			OGL_DrawPatch_CS(x, y, FontBBaseLump + c - 33);
#else
			V_DrawPatch(x, y, p);
#endif
			x += SHORT(p->width) - 1;
		}
	}
}

//---------------------------------------------------------------------------
//
// FUNC MN_TextBWidth
//
// Returns the pixel width of a string using font B.
//
//---------------------------------------------------------------------------

int MN_TextBWidth(const char *text)
{
	char c;
	int width;
	patch_t *p;

	width = 0;
	while ((c = *text++) != 0)
	{
		if (c < 33)
		{
			width += 5;
		}
		else
		{
			p = (patch_t *) W_CacheLumpNum(FontBBaseLump + c - 33, PU_CACHE);
			width += SHORT(p->width) - 1;
		}
	}
	return (width);
}

//---------------------------------------------------------------------------
//
// PROC MN_Ticker
//
//---------------------------------------------------------------------------

void MN_Ticker(void)
{
	if (MenuActive == false)
	{
#ifdef RENDER3D
		if (bgAlpha > 0)
		{
			bgAlpha -= .5 / (float)menuDarkTicks;
			if (bgAlpha < 0)
				bgAlpha = 0;
		}
		if (fadingOut)
		{
			outFade += 1 / (float)slamInTicks;
			if (outFade > 1)
				fadingOut = false;
		}
#endif
		return;
	}
	MenuTime++;
}


#ifdef RENDER3D
static void MN_OGL_SetupState(float time)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (time > 1 && time <= 2)
	{
		time = 2 - time;
		glTranslatef(160, 100, 0);
		glScalef(.9 + time*.1, .9 + time*.1, 1);
		glTranslatef(-160, -100, 0);
		glColor4f(1, 1, 1, time);
		return;
	}

	glTranslatef(160, 100, 0);
	glScalef(2-time, 2-time, 1);
	glTranslatef(-160, -100, 0);
	glColor4f(1, 1, 1, time*time);
}

static void MN_OGL_RestoreState(void)
{
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
#endif


//---------------------------------------------------------------------------
//
// PROC MN_Drawer
//
//---------------------------------------------------------------------------

static const char *QuitEndMsg[] =
{
	"ARE YOU SURE YOU WANT TO QUIT?",
	"ARE YOU SURE YOU WANT TO END THE GAME?",
	"DO YOU WANT TO QUICKSAVE THE GAME NAMED",
	"DO YOU WANT TO QUICKLOAD THE GAME NAMED",
	"ARE YOU SURE YOU WANT TO SUICIDE?"
};


void MN_Drawer(void)
{
	int i;
	int x;
	int y;
	MenuItem_t *item;
	const char *selName;

	if (MenuActive == false)
	{
#ifdef RENDER3D
		if (bgAlpha > 0)
		{
			UpdateState |= I_FULLSCRN;
			BorderNeedRefresh = true;
		//	OGL_SetNoTexture();
			glDisable(GL_TEXTURE_2D);
			OGL_DrawRect(0, 0, 320, 200, 0, 0, 0, bgAlpha);
			glEnable(GL_TEXTURE_2D);
		}
#endif
		if (askforquit)
		{
			MN_DrTextA(QuitEndMsg[typeofask-1], 160 - MN_TextAWidth(QuitEndMsg[typeofask-1])/2, 80);
			if (typeofask == 3)
			{
				MN_DrTextA(SlotText[quicksave-1], 160 - MN_TextAWidth(SlotText[quicksave-1])/2, 90);
				MN_DrTextA("?", 160 + MN_TextAWidth(SlotText[quicksave-1])/2, 90);
			}
			if (typeofask == 4)
			{
				MN_DrTextA(SlotText[quickload-1], 160 - MN_TextAWidth(SlotText[quickload-1])/2, 90);
				MN_DrTextA("?", 160 + MN_TextAWidth(SlotText[quicksave-1])/2, 90);
			}
			UpdateState |= I_FULLSCRN;
		}
	}
#ifdef RENDER3D
	if (MenuActive || fadingOut)
	{
		int effTime = (MenuTime > menuDarkTicks) ? menuDarkTicks : MenuTime;
		float temp = .5 * effTime / (float)menuDarkTicks;

		UpdateState |= I_FULLSCRN;

		if (!fadingOut)
		{
			if (temp > bgAlpha)
				bgAlpha = temp;
			effTime = (MenuTime>slamInTicks) ? slamInTicks : MenuTime;
			temp = effTime / (float)slamInTicks;

			// Draw a dark background. It makes it easier to read the menus.
			//OGL_SetNoTexture();
			glDisable(GL_TEXTURE_2D);
			OGL_DrawRect(0, 0, 320, 200, 0, 0, 0, bgAlpha);
			glEnable(GL_TEXTURE_2D);
		}
		else
			temp = outFade + 1;
		MN_OGL_SetupState(temp);

		if (InfoType)
		{
			MN_DrawInfo();
			MN_OGL_RestoreState();
			return;
		}
	//	if (screenblocks < 10)
	//	{
			BorderNeedRefresh = true;
	//	}
		if (CurrentMenu->drawFunc != NULL)
		{
			CurrentMenu->drawFunc();
		}
		x = CurrentMenu->x;
		y = CurrentMenu->y;
		item = CurrentMenu->items;
		if (item->type == ITT_SETKEY)
			item += FirstKey;
		for (i = 0; i < CurrentMenu->itemCount; i++)
		{
			switch (item->type)
			{
			case (ITT_EMPTY):
				break;
			case (ITT_SETKEY):
				if (item->text)
					MN_DrTextA(item->text, x, y+6);
				break;
			default:
				if (item->text)
					MN_DrTextB(item->text, x, y);
			}
			y += CurrentMenu->step;
			item++;
		}
		y = CurrentMenu->y + (CurrentItPos * CurrentMenu->step) + SELECTOR_YOFFSET;
		selName = (MenuTime & 16) ? "M_SLCTR1" : "M_SLCTR2";
		OGL_DrawPatch_CS(x + SELECTOR_XOFFSET, y, W_GetNumForName(selName));

		MN_OGL_RestoreState();
	}
#else
	else
	{
		UpdateState |= I_FULLSCRN;
		if (InfoType)
		{
			MN_DrawInfo();
			return;
		}
		if (screenblocks < 10)
		{
			BorderNeedRefresh = true;
		}
		if (CurrentMenu->drawFunc != NULL)
		{
			CurrentMenu->drawFunc();
		}
		x = CurrentMenu->x;
		y = CurrentMenu->y;
		item = CurrentMenu->items;
		if (item->type == ITT_SETKEY)
			item += FirstKey;
		for (i = 0; i < CurrentMenu->itemCount; i++)
		{
			switch (item->type)
			{
			case (ITT_EMPTY):
				break;
			case (ITT_SETKEY):
				if (item->text)
					MN_DrTextA(item->text, x, y+6);
				break;
			default:
				if (item->text)
					MN_DrTextB(item->text, x, y);
			}
			y += CurrentMenu->step;
			item++;
		}
		y = CurrentMenu->y + (CurrentItPos * CurrentMenu->step) + SELECTOR_YOFFSET;
		selName = (MenuTime & 16) ? "M_SLCTR1" : "M_SLCTR2";
		V_DrawPatch(x + SELECTOR_XOFFSET, y, (patch_t *)W_CacheLumpName(selName, PU_CACHE));
	}
#endif
}

//---------------------------------------------------------------------------
//
// PROC DrawMainMenu
//
//---------------------------------------------------------------------------

static void DrawMainMenu(void)
{
	int frame;

	frame = (MenuTime / 5) % 7;

#ifdef RENDER3D
	OGL_DrawPatch_CS(88, 0, W_GetNumForName("M_HTIC") );
	OGL_DrawPatch_CS(37, 80, MauloBaseLump + (frame + 2) % 7);
	OGL_DrawPatch_CS(278, 80, MauloBaseLump + frame);
#else
	V_DrawPatch(88, 0, (patch_t *)W_CacheLumpName("M_HTIC", PU_CACHE));
// Old Gold skull positions: (40, 10) and (232, 10)
	V_DrawPatch(37, 80, (patch_t *)W_CacheLumpNum(MauloBaseLump + (frame + 2) % 7, PU_CACHE));
	V_DrawPatch(278, 80, (patch_t *)W_CacheLumpNum(MauloBaseLump + frame, PU_CACHE));
#endif
}

//==========================================================================
//
// DrawClassMenu
//
//==========================================================================

static void DrawClassMenu(void)
{
	pclass_t pClass;
	static const char *boxLumpName[4] =
	{
		"m_fbox",
		"m_cbox",
		"m_mbox",
		"m_abox"
	};
	static const char *walkLumpName[4] =
	{
		"m_fwalk1",
		"m_cwalk1",
		"m_mwalk1",
		"m_awalk1"
	};

	MN_DrTextB("CHOOSE CLASS:", 34, 24);
	pClass = (pclass_t)CurrentMenu->items[CurrentItPos].option;
#ifdef RENDER3D
	OGL_DrawPatch_CS(174, 8, W_GetNumForName(boxLumpName[pClass]));
	OGL_DrawPatch_CS(174+24, 8+12,
			 W_GetNumForName(walkLumpName[pClass]) + ((MenuTime>>3) & 3));
#else
	V_DrawPatch(174, 8, (patch_t *)W_CacheLumpName(boxLumpName[pClass], PU_CACHE));
	V_DrawPatch(174+24, 8+12,
		(patch_t *)W_CacheLumpNum(W_GetNumForName(walkLumpName[pClass]) + ((MenuTime>>3) & 3), PU_CACHE));
#endif
}

//---------------------------------------------------------------------------
//
// PROC DrawSkillMenu
//
//---------------------------------------------------------------------------

static void DrawSkillMenu(void)
{
	MN_DrTextB("CHOOSE SKILL LEVEL:", 74, 16);
}

//---------------------------------------------------------------------------
//
// PROC DrawFilesMenu
//
//---------------------------------------------------------------------------

static void DrawFilesMenu(void)
{
// clear out the quicksave/quickload stuff
	quicksave = 0;
	quickload = 0;
	P_ClearMessage(&players[consoleplayer]);
}

//---------------------------------------------------------------------------
//
// PROC DrawLoadMenu
//
//---------------------------------------------------------------------------

static void DrawLoadMenu(void)
{
	MN_DrTextB("LOAD GAME", 160 - MN_TextBWidth("LOAD GAME")/2, 10);
	if (!slottextloaded)
	{
		MN_LoadSlotText();
	}
	DrawFileSlots(&LoadgameMenu);
}

//---------------------------------------------------------------------------
//
// PROC DrawSaveMenu
//
//---------------------------------------------------------------------------

static void DrawSaveMenu(void)
{
	MN_DrTextB("SAVE GAME", 160 - MN_TextBWidth("SAVE GAME")/2, 10);
	if (!slottextloaded)
	{
		MN_LoadSlotText();
	}
	DrawFileSlots(&SavegameMenu);
}

//===========================================================================
//
// MN_LoadSlotText
//
// For each slot, looks for save games and reads the description field.
//
//===========================================================================

static void MN_LoadSlotText(void)
{
	int slot;
	FILE *fp;
	char name[MAX_OSPATH];
	char versionText[HXS_VERSION_TEXT_LENGTH];
	char description[HXS_DESCRIPTION_LENGTH];
	boolean found;

	for (slot = 0; slot < 6; slot++)
	{
		found = false;
		snprintf(name, sizeof(name), "%shex%d.hxs", basePath, slot);
		fp = fopen(name, "rb");
		if (fp)
		{
			fread(description, HXS_DESCRIPTION_LENGTH, 1, fp);
			fread(versionText, HXS_VERSION_TEXT_LENGTH, 1, fp);
			fclose(fp);
			if (!strcmp(versionText, HXS_VERSION_TEXT))
			{
				found = true;
			}
		}
		if (found)
		{
			memcpy(SlotText[slot], description, SLOTTEXTLEN);
			SlotStatus[slot] = 1;
		}
		else
		{
			memset(SlotText[slot], 0, SLOTTEXTLEN);
			SlotStatus[slot] = 0;
		}
	}
	slottextloaded = true;
}

//---------------------------------------------------------------------------
//
// PROC DrawFileSlots
//
//---------------------------------------------------------------------------

static void DrawFileSlots(Menu_t *menu)
{
	int i;
	int x;
	int y;

	x = menu->x;
	y = menu->y;
	for (i = 0; i < 6; i++)
	{
#ifdef RENDER3D
		OGL_DrawPatch_CS(x, y, W_GetNumForName("M_FSLOT"));
#else
		V_DrawPatch(x, y, (patch_t *)W_CacheLumpName("M_FSLOT", PU_CACHE));
#endif
		if (SlotStatus[i])
		{
			MN_DrTextA(SlotText[i], x+5, y+5);
		}
		y += ITEM_HEIGHT;
	}
}

//---------------------------------------------------------------------------
//
// PROC DrawOptionsMenu
//
//---------------------------------------------------------------------------

static void DrawOptionsMenu(void)
{
	char num[5];

	if (messageson)
	{
		MN_DrTextB("ON", 196, 50);
	}
	else
	{
		MN_DrTextB("OFF", 196, 50);
	}

	MN_DrTextB(mlooktext[mouselook], 208, 110);

	snprintf(num, sizeof(num), "%d", mouseSensitivity);
	MN_DrTextB(num, 265, 71);

	if (alwaysrun)
	{
		MN_DrTextB("ON", 208, 130);
	}
	else
	{
		MN_DrTextB("OFF", 208,130);
	}
}

//---------------------------------------------------------------------------
//
// PROC DrawOptions2Menu
//
//---------------------------------------------------------------------------

static void DrawOptions2Menu(void)
{
	DrawSlider(&Options2Menu, 1, 9, screenblocks-3);
	DrawSlider(&Options2Menu, 3, 16, snd_MaxVolume);
	DrawSlider(&Options2Menu, 5, 16, snd_MusicVolume);
	if (cdaudio)
	{
		MN_DrTextB("ON", 196, 140);
	}
	else
	{
		MN_DrTextB("OFF", 196, 140);
	}
}

static void DrawOptions3Menu(void)
{
	int i;

	for (i = 0; i < 15; i++)
	{
		if (askforkey && keyaskedfor == i)
		{
			MN_DrTextAYellow("???", 195, (i*SMALL_ITEM_HEIGHT+26));
		}
		else
		{
			MN_DrTextA(Key2String(*(defaults[i+FirstKey+3].location)),
				195, (i*SMALL_ITEM_HEIGHT+26));
		}
	}
}

//---------------------------------------------------------------------------
//
// PROC SCQuitGame
//
//---------------------------------------------------------------------------

static void SCQuitGame(int option)
{
	MenuActive = false;
	askforquit = true;
	typeofask = 1; //quit game
	if (!netgame && !demoplayback)
	{
		paused = true;
	}
}

//---------------------------------------------------------------------------
//
// PROC SCEndGame
//
//---------------------------------------------------------------------------

static void SCEndGame(int option)
{
	if (demoplayback)
	{
		return;
	}
	if (SCNetCheck(3))
	{
		MenuActive = false;
		askforquit = true;
		typeofask = 2; //endgame
		if (!netgame && !demoplayback)
		{
			paused = true;
		}
	}
}

//---------------------------------------------------------------------------
//
// PROC SCMessages
//
//---------------------------------------------------------------------------

static void SCMessages(int option)
{
	if (messageson)
	{
		messageson = 0;
		P_SetMessage(&players[consoleplayer], "MESSAGES OFF", true);
	}
	else
	{
		messageson = 1;
		P_SetMessage(&players[consoleplayer], "MESSAGES ON", true);
	}
	S_StartSound(NULL, SFX_CHAT);
}

//===========================================================================
//
// SCNetCheck
//
//===========================================================================

static boolean SCNetCheck(int option)
{
	if (!netgame)
	{
		return true;
	}
	switch (option)
	{
	case 1: // new game
		P_SetMessage(&players[consoleplayer],
			"YOU CAN'T START A NEW GAME IN NETPLAY!", true);
		break;
	case 2: // load game
		P_SetMessage(&players[consoleplayer],
			"YOU CAN'T LOAD A GAME IN NETPLAY!", true);
		break;
	case 3: // end game
		P_SetMessage(&players[consoleplayer],
			"YOU CAN'T END A GAME IN NETPLAY!", true);
		break;
	}
	MenuActive = false;
	S_StartSound(NULL, SFX_CHAT);
	return false;
}

//===========================================================================
//
// SCNetCheck2
//
//===========================================================================

static void SCNetCheck2(int option)
{
	SCNetCheck(option);
	return;
}

//---------------------------------------------------------------------------
//
// PROC SCLoadGame
//
//---------------------------------------------------------------------------

static void SCLoadGame(int option)
{
	if (!SlotStatus[option])
	{ // Don't try to load from an empty slot
		return;
	}
	G_LoadGame(option);
	MN_DeactivateMenu();
	BorderNeedRefresh = true;
	if (quickload == -1)
	{
		quickload = option+1;
		P_ClearMessage(&players[consoleplayer]);
	}
}

//---------------------------------------------------------------------------
//
// PROC SCSaveGame
//
//---------------------------------------------------------------------------

static void SCSaveGame(int option)
{
	char *ptr;
	player_t *player = &players[consoleplayer];

	if (gamestate != GS_LEVEL || demoplayback
		|| player->playerstate == PST_DEAD)
	{
		FileMenuKeySteal = false;
		return;
	}

	if (!FileMenuKeySteal)
	{
		FileMenuKeySteal = true;
		strcpy(oldSlotText, SlotText[option]);
		ptr = SlotText[option];
		while (*ptr)
		{
			ptr++;
		}
		*ptr = '[';
		*(ptr+1) = 0;
		SlotStatus[option]++;
		currentSlot = option;
		slotptr = ptr-SlotText[option];
		return;
	}
	else
	{
		G_SaveGame(option, SlotText[option]);
		FileMenuKeySteal = false;
		MN_DeactivateMenu();
	}
	BorderNeedRefresh = true;
	if (quicksave == -1)
	{
		quicksave = option+1;
		P_ClearMessage(&players[consoleplayer]);
	}
}

//==========================================================================
//
// SCClass
//
//==========================================================================

static void SCClass(int option)
{
	if (netgame)
	{
		P_SetMessage(&players[consoleplayer],
			"YOU CAN'T START A NEW GAME FROM WITHIN A NETGAME!", true);
		return;
	}
	MenuPClass = option;
	switch (MenuPClass)
	{
	case PCLASS_FIGHTER:
		SkillMenu.x = 120;
		SkillItems[0].text = "SQUIRE";
		SkillItems[1].text = "KNIGHT";
		SkillItems[2].text = "WARRIOR";
		SkillItems[3].text = "BERSERKER";
		SkillItems[4].text = "TITAN";
		break;
	case PCLASS_CLERIC:
		SkillMenu.x = 116;
		SkillItems[0].text = "ALTAR BOY";
		SkillItems[1].text = "ACOLYTE";
		SkillItems[2].text = "PRIEST";
		SkillItems[3].text = "CARDINAL";
		SkillItems[4].text = "POPE";
		break;
	case PCLASS_MAGE:
		SkillMenu.x = 112;
		SkillItems[0].text = "APPRENTICE";
		SkillItems[1].text = "ENCHANTER";
		SkillItems[2].text = "SORCERER";
		SkillItems[3].text = "WARLOCK";
		SkillItems[4].text = "ARCHIMAGE";
		break;
	}
	SetTheMenu(MENU_SKILL);
}

//---------------------------------------------------------------------------
//
// PROC SCSkill
//
//---------------------------------------------------------------------------

static void SCSkill(int option)
{
	extern int SB_state;

	PlayerClasses[consoleplayer] = MenuPClass;
	G_DeferredNewGame(option);
	SB_SetClassData();
	SB_state = -1;
	MN_DeactivateMenu();
}

//---------------------------------------------------------------------------
//
// PROC SCMouseSensi
//
//---------------------------------------------------------------------------

static void SCMouseSensi(int option)
{
	if (option == RIGHT_DIR)
	{
		if (mouseSensitivity < MENU_MAX_MOUSE_SENS)
		{
			mouseSensitivity++;
		}
	}
	else if (mouseSensitivity)
	{
		mouseSensitivity--;
	}
}

//---------------------------------------------------------------------------
//
// PROC SCSfxVolume
//
//---------------------------------------------------------------------------

static void SCSfxVolume(int option)
{
	if (option == RIGHT_DIR)
	{
		if (snd_MaxVolume < 15)
		{
			snd_MaxVolume++;
		}
	}
	else if (snd_MaxVolume)
	{
		snd_MaxVolume--;
	}
	soundchanged = true; // we'll set it when we leave the menu
}

//---------------------------------------------------------------------------
//
// PROC SCMusicVolume
//
//---------------------------------------------------------------------------

static void SCMusicVolume(int option)
{
	if (option == RIGHT_DIR)
	{
		if (snd_MusicVolume < 15)
		{
			snd_MusicVolume++;
		}
	}
	else if (snd_MusicVolume)
	{
		snd_MusicVolume--;
	}
	S_SetMusicVolume();
}

//---------------------------------------------------------------------------
//
// PROC SCScreenSize
//
//---------------------------------------------------------------------------

static void SCScreenSize(int option)
{
	if (option == RIGHT_DIR)
	{
		if (screenblocks < 11)
		{
			screenblocks++;
		}
	}
	else if (screenblocks > 3)
	{
		screenblocks--;
	}
	R_SetViewSize(screenblocks, detailLevel);
}

//---------------------------------------------------------------------------
//
// PROC SCInfo
//
//---------------------------------------------------------------------------

static void SCInfo(int option)
{
	InfoType = 1;
	S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
	if (!netgame && !demoplayback)
	{
		paused = true;
	}
}

//---------------------------------------------------------------------------
//
// PROC SCSetKey
//
//---------------------------------------------------------------------------

static void SCSetKey(int option)
{
	askforkey = true;
	keyaskedfor = option;
	if (!netgame && !demoplayback)
	{
		paused = true;
	}
}

//---------------------------------------------------------------------------
//
// PROC SCMouslook(int option)
//
//---------------------------------------------------------------------------

static void SCMouselook(int option)
{
	if (option == RIGHT_DIR)
	{
		if (mouselook < 2)
			mouselook++;
	}
	else if (mouselook)
		mouselook--;
}

static void SCAlwaysRun(int option)
{
	if (alwaysrun)
		alwaysrun = 0;
	else	alwaysrun = 1;
}

static void SCCDAudio(int option)
{
	if (cdaudio)
	{
		cdaudio = 0;
		I_CDMusStop();
	// FIXME: start ordinary music here
	}
	else
	{
		cdaudio = 1;
		if (i_CDMusic)
		{
		// FIXME: start cdaudio here
		}
	}
}

//---------------------------------------------------------------------------
//
// FUNC MN_Responder
//
//---------------------------------------------------------------------------

boolean MN_Responder(event_t *event)
{
	int key;
	int i;
	MenuItem_t *item;
	static boolean shiftdown;
	extern void H2_StartTitle(void);
	extern void G_CheckDemoStatus(void);
	char *textBuffer;

	if (askforkey && event->type == ev_keydown)
	{
		ClearControls(event->data1);
		*defaults[keyaskedfor + 3 + FirstKey].location = event->data1;
		askforkey = false;
		return true;
	}
	if (askforkey && event->type == ev_mouse)
	{
		if (event->data1 & 1)
			return true;
		if (event->data1 & 2)
			return true;
		if (event->data1 & 4)
			return true;
		return false;
	}
	if (event->data1 == KEY_RSHIFT)
	{
		shiftdown = (event->type == ev_keydown);
	}
	if (event->type != ev_keydown)
	{
		return false;
	}
	key = event->data1;
	if (InfoType)
	{
		InfoType = (InfoType + 1) % 4;
		if (key == KEY_ESCAPE)
		{
			InfoType = 0;
		}
		if (!InfoType)
		{
			if (!netgame && !demoplayback)
			{
				paused = false;
			}
			MN_DeactivateMenu();
			SB_state = -1; //refresh the statbar
			BorderNeedRefresh = true;
		}
		S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
		return true; //make the info screen eat the keypress
	}

	if (ravpic && key == KEY_F1)
	{
	// F12 is screenshot now. This
	// is here for reference, only.
		G_ScreenShot();
		return true;
	}

	if (askforquit)
	{
		switch (key)
		{
		case KEY_ENTER:
		case 'y':
			if (askforquit)
			{
				switch (typeofask)
				{
				case 1:
					G_CheckDemoStatus(); 
					I_Quit();
					break;
				case 2:
					P_ClearMessage(&players[consoleplayer]);
					typeofask = 0;
					askforquit = false;
					paused = false;
					V_SetPaletteBase();
					H2_StartTitle(); // go to intro/demo mode.
					break;
				case 3:
					P_SetMessage(&players[consoleplayer], 
						"QUICKSAVING....", false);
					FileMenuKeySteal = true;
					SCSaveGame(quicksave-1);
					askforquit = false;
					typeofask = 0;
					BorderNeedRefresh = true;
					return true;
				case 4:
					P_SetMessage(&players[consoleplayer], 
						"QUICKLOADING....", false);
					SCLoadGame(quickload-1);
					askforquit = false;
					typeofask = 0;
					BorderNeedRefresh = true;
					return true;
				case 5:
					askforquit = false;
					typeofask = 0;
					BorderNeedRefresh = true;
					mn_SuicideConsole = true;
					return true;
				default:
					return true; // eat the 'y' keypress
				}
			}
			return false;

		case 'n':
		case KEY_ESCAPE:
			if (askforquit)
			{
				players[consoleplayer].messageTics = 0;
				askforquit = false;
				typeofask = 0;
				paused = false;
				UpdateState |= I_FULLSCRN;
				BorderNeedRefresh = true;
				return true;
			}
			return false;
		}
		return false; // don't let the keys filter thru
	}
	if (MenuActive == false && !chatmodeon)
	{
		switch (key)
		{
		case KEY_MINUS:
			if (automapactive)
			{ // Don't screen size in automap
				return false;
			}
			SCScreenSize(LEFT_DIR);
			S_StartSound(NULL, SFX_PICKUP_KEY);
			BorderNeedRefresh = true;
			UpdateState |= I_FULLSCRN;
			return true;
		case KEY_EQUALS:
			if (automapactive)
			{ // Don't screen size in automap
				return false;
			}
			SCScreenSize(RIGHT_DIR);
			S_StartSound(NULL, SFX_PICKUP_KEY);
			BorderNeedRefresh = true;
			UpdateState |= I_FULLSCRN;
			return true;
#ifdef __NeXT__
		case 'q':
			MenuActive = false;
			askforquit = true;
			typeofask = 5; // suicide
			return true;
#endif
		case KEY_F1: // help screen
			SCInfo(0); // start up info screens
			MenuActive = true;
			return true;
		case KEY_F2: // save game
			if (gamestate == GS_LEVEL && !demoplayback)
			{
				MenuActive = true;
				FileMenuKeySteal = false;
				MenuTime = 0;
				CurrentMenu = &SavegameMenu;
				CurrentItPos = CurrentMenu->oldItPos;
				if (!netgame && !demoplayback)
				{
					paused = true;
				}
				S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
				slottextloaded = false; //reload the slot text, when needed
			}
			return true;
		case KEY_F3: // load game
			if (SCNetCheck(2))
			{
				MenuActive = true;
				FileMenuKeySteal = false;
				MenuTime = 0;
				CurrentMenu = &LoadgameMenu;
				CurrentItPos = CurrentMenu->oldItPos;
				if (!netgame && !demoplayback)
				{
					paused = true;
				}
				S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
				slottextloaded = false; //reload the slot text, when needed
			}
			return true;
		case KEY_F4: // S.A.: made F4 Controls. was Volume, before.
			MenuActive = true;
			FileMenuKeySteal = false;
			MenuTime = 0;
			CurrentMenu = &OptionsMenu;
			CurrentItPos = CurrentMenu->oldItPos;
			if (!netgame && !demoplayback)
			{
				paused = true;
			}
			S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
			slottextloaded = false; //reload the slot text, when needed
			return true;
		case KEY_F5: // volume
			MenuActive = true;
			FileMenuKeySteal = false;
			MenuTime = 0;
			CurrentMenu = &Options2Menu;
			CurrentItPos = CurrentMenu->oldItPos;
			if (!netgame && !demoplayback)
			{
				paused = true;
			}
			S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
			slottextloaded = false; //reload the slot text, when needed
			return true;
		case KEY_F6: // quicksave
			if (gamestate == GS_LEVEL && !demoplayback)
			{
				if (!quicksave || quicksave == -1)
				{
					MenuActive = true;
					FileMenuKeySteal = false;
					MenuTime = 0;
					CurrentMenu = &SavegameMenu;
					CurrentItPos = CurrentMenu->oldItPos;
					if (!netgame && !demoplayback)
					{
						paused = true;
					}
					S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
					slottextloaded = false; //reload the slot text
					quicksave = -1;
					P_SetMessage(&players[consoleplayer],
						"CHOOSE A QUICKSAVE SLOT", true);
				}
				else
				{
					askforquit = true;
					typeofask = 3;
					if (!netgame && !demoplayback)
					{
						paused = true;
					}
					S_StartSound(NULL, SFX_CHAT);
				}
			}
			return true;
		case KEY_F7: // endgame
			if (SCNetCheck(3))
			{
				if (gamestate == GS_LEVEL && !demoplayback)
				{
					S_StartSound(NULL, SFX_CHAT);
					SCEndGame(0);
				}
			}
			return true;
		case KEY_F8: // toggle messages
			SCMessages(0);
			return true;
		case KEY_F9: // quickload
			if (SCNetCheck(2))
			{
				if (!quickload || quickload == -1)
				{
					MenuActive = true;
					FileMenuKeySteal = false;
					MenuTime = 0;
					CurrentMenu = &LoadgameMenu;
					CurrentItPos = CurrentMenu->oldItPos;
					if (!netgame && !demoplayback)
					{
						paused = true;
					}
					S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
					slottextloaded = false; // reload the slot text
					quickload = -1;
					P_SetMessage(&players[consoleplayer],
						"CHOOSE A QUICKLOAD SLOT", true);
				}
				else
				{
					askforquit = true;
					if (!netgame && !demoplayback)
					{
						paused = true;
					}
					typeofask = 4;
					S_StartSound(NULL, SFX_CHAT);
				}
			}
			return true;
		case KEY_F10: // quit
		// S.A.: allowed quit to work when not in a level
		//	if (!(gamestate == GS_LEVEL || gamestate == GS_FINALE))
		//		return true;
			SCQuitGame(0);
			S_StartSound(NULL, SFX_CHAT);
			return true;
		case KEY_F11: // F11 - gamma mode correction
			usegamma++;
			if (usegamma > 4)
			{
				usegamma = 0;
			}
			SB_PaletteFlash(true); // force change
			P_SetMessage(&players[consoleplayer], GammaText[usegamma], false);
			return true;
		/*
		case KEY_F12: // F12 - reload current map (devmaps mode)
			if (netgame || DevMaps == false)
			{
				return false;
			}
			if (gamekeydown[key_speed])
			{ // Monsters ON
				nomonsters = false;
			}
			if (gamekeydown[key_strafe])
			{ // Monsters OFF
				nomonsters = true;
			}
			G_DeferedInitNew(gameskill, gameepisode, gamemap);
			P_SetMessage(&players[consoleplayer], TXT_CHEATWARP,
				false);
			return true;
		*/
		case KEY_F12: // S.A.: made F12 Screenshot
			G_ScreenShot();
			return true;
		}
	}

	if (MenuActive == false)
	{
		if (key == KEY_ESCAPE || gamestate == GS_DEMOSCREEN || demoplayback)
		{
			MN_ActivateMenu();
			return true;
		}
		return false;
	}
	if (!FileMenuKeySteal)
	{
		item = &CurrentMenu->items[CurrentItPos];
		switch (key)
		{
		case KEY_DOWNARROW:
			do
			{
				if (CurrentMenu->items[CurrentItPos].type == ITT_SETKEY
					&& CurrentItPos+1 > CurrentMenu->itemCount-1)
				{
					if (FirstKey == FIRSTKEY_MAX)
					{
						CurrentItPos = 0; // End of Key menu
						FirstKey = 0;
					}
					else
					{
						FirstKey++;
					}
				}
				else if (CurrentItPos+1 > CurrentMenu->itemCount-1)
				{
					CurrentItPos = 0;
				}
				else
				{
					CurrentItPos++;
				}
			}
			while (CurrentMenu->items[CurrentItPos].type == ITT_EMPTY);
			S_StartSound(NULL, SFX_FIGHTER_HAMMER_HITWALL);
			return true;
		case KEY_UPARROW:
			do
			{
				if (CurrentMenu->items[CurrentItPos].type == ITT_SETKEY && CurrentItPos == 0)
				{
					if (FirstKey == 0)
					{
						CurrentItPos = 14; // End of Key menu
								   // 14 == 15 (max lines on a page) - 1
						FirstKey = FIRSTKEY_MAX;
					}
					else
					{
						FirstKey--;
					}
				}
				else if (CurrentItPos == 0)
				{
					CurrentItPos = CurrentMenu->itemCount-1;
				}
				else
				{
					CurrentItPos--;
				}
			}
			while (CurrentMenu->items[CurrentItPos].type == ITT_EMPTY);
			S_StartSound(NULL, SFX_FIGHTER_HAMMER_HITWALL);
			return true;
		case KEY_LEFTARROW:
			if (item->type == ITT_LRFUNC && item->func != NULL)
			{
				item->func(LEFT_DIR);
				S_StartSound(NULL, SFX_PICKUP_KEY);
			}
			return true;
		case KEY_RIGHTARROW:
			if (item->type == ITT_LRFUNC && item->func != NULL)
			{
				item->func(RIGHT_DIR);
				S_StartSound(NULL, SFX_PICKUP_KEY);
			}
			return true;
		case KEY_ENTER:
			if (item->type == ITT_SETMENU)
			{
				if (item->func != NULL)	
				{
					item->func(item->option);
				}
				SetTheMenu(item->menu);
			}
			else if (item->func != NULL)
			{
				CurrentMenu->oldItPos = CurrentItPos;
				if (item->type == ITT_LRFUNC)
				{
					item->func(RIGHT_DIR);
				}
				else if (item->type == ITT_EFUNC)
				{
					item->func(item->option);
				}
				else if (item->type == ITT_SETKEY)
				{
					item->func(item->option);
				}
			}
			S_StartSound(NULL, SFX_DOOR_LIGHT_CLOSE);
			return true;
		case KEY_ESCAPE:
			MN_DeactivateMenu();
			return true;
		case KEY_BACKSPACE:
			S_StartSound(NULL, SFX_PICKUP_KEY);
			if (CurrentMenu->prevMenu == MENU_NONE)
			{
				MN_DeactivateMenu();
			}
			else
			{
				SetTheMenu(CurrentMenu->prevMenu);
			}
			return true;
		default:
			for (i = 0; i < CurrentMenu->itemCount; i++)
			{
				if (CurrentMenu->items[i].text)
				{
					if (toupper(key)
						== toupper(CurrentMenu->items[i].text[0]))
					{
						CurrentItPos = i;
						return true;
					}
				}
			}
			break;
		}
		return false;
	}
	else
	{ // Editing file names
		textBuffer = &SlotText[currentSlot][slotptr];
		if (key == KEY_BACKSPACE)
		{
			if (slotptr)
			{
				*textBuffer-- = 0;
				*textBuffer = ASCII_CURSOR;
				slotptr--;
			}
			return true;
		}
		if (key == KEY_ESCAPE)
		{
			memset(SlotText[currentSlot], 0, SLOTTEXTLEN+2);
			strcpy(SlotText[currentSlot], oldSlotText);
			SlotStatus[currentSlot]--;
			MN_DeactivateMenu();
			return true;
		}
		if (key == KEY_ENTER)
		{
			SlotText[currentSlot][slotptr] = 0; // clear the cursor
			item = &CurrentMenu->items[CurrentItPos];
			CurrentMenu->oldItPos = CurrentItPos;
			if (item->type == ITT_EFUNC)
			{
				item->func(item->option);
				if (item->menu != MENU_NONE)
				{
					SetTheMenu(item->menu);
				}
			}
			return true;
		}
		if (slotptr < SLOTTEXTLEN && key != KEY_BACKSPACE)
		{
			if ((key >= 'a' && key <= 'z'))
			{
				*textBuffer++ = key-32;
				*textBuffer = ASCII_CURSOR;
				slotptr++;
				return true;
			}
			if (((key >= '0' && key <= '9') || key == ' '
				|| key == ',' || key == '.' || key == '-')
				&& !shiftdown)
			{
				*textBuffer++ = key;
				*textBuffer = ASCII_CURSOR;
				slotptr++;
				return true;
			}
			if (shiftdown && key == '1')
			{
				*textBuffer++ = '!';
				*textBuffer = ASCII_CURSOR;
				slotptr++;
				return true;
			}
		}
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// PROC MN_ActivateMenu
//
//---------------------------------------------------------------------------

void MN_ActivateMenu(void)
{
	if (MenuActive)
	{
		return;
	}
	if (paused)
	{
		S_ResumeSound();
	}
	MenuActive = true;
	FileMenuKeySteal = false;
	MenuTime = 0;
	CurrentMenu = &MainMenu;
	CurrentItPos = CurrentMenu->oldItPos;
	if (!netgame && !demoplayback)
	{
		paused = true;
	}
	S_StartSound(NULL, SFX_PLATFORM_STOP);
	slottextloaded = false; //reload the slot text, when needed
}

//---------------------------------------------------------------------------
//
// PROC MN_DeactivateMenu
//
//---------------------------------------------------------------------------

void MN_DeactivateMenu(void)
{
	if (CurrentMenu)
		CurrentMenu->oldItPos = CurrentItPos;
	MenuActive = false;
	if (!netgame)
	{
		paused = false;
	}
	S_StartSound(NULL, SFX_PLATFORM_STOP);
	P_ClearMessage(&players[consoleplayer]);
}

//---------------------------------------------------------------------------
//
// PROC MN_DrawInfo
//
//---------------------------------------------------------------------------

void MN_DrawInfo(void)
{
	V_SetPaletteBase();
	V_DrawRawScreen((BYTE_REF) WR_CacheLumpNum(W_GetNumForName("TITLE")+InfoType, PU_CACHE));
}


//---------------------------------------------------------------------------
//
// PROC SetMenu
//
//---------------------------------------------------------------------------

static void SetTheMenu(MenuType_t menu)
{
	CurrentMenu->oldItPos = CurrentItPos;
	CurrentMenu = Menus[menu];
	CurrentItPos = CurrentMenu->oldItPos;
}

//---------------------------------------------------------------------------
//
// PROC DrawSlider
//
//---------------------------------------------------------------------------

static void DrawSlider(Menu_t *menu, int item, int width, int slot)
{
	int x;
	int y;
	int x2;
	int count;

	x = menu->x + 24;
	y = menu->y + 2 + (item*ITEM_HEIGHT);

#ifdef RENDER3D
	OGL_DrawPatch_CS(x-32, y, W_GetNumForName("M_SLDLT"));
	for (x2 = x, count = width; count--; x2 += 8)
	{
		OGL_DrawPatch_CS(x2, y, W_GetNumForName((count & 1) ? "M_SLDMD1" : "M_SLDMD2"));
	}
	OGL_DrawPatch_CS(x2, y, W_GetNumForName("M_SLDRT"));
	OGL_DrawPatch_CS(x + 4 + slot*8, y + 7, W_GetNumForName("M_SLDKB"));
#else
	V_DrawPatch(x-32, y, (patch_t *)W_CacheLumpName("M_SLDLT", PU_CACHE));
	for (x2 = x, count = width; count--; x2 += 8)
	{
		V_DrawPatch(x2, y, (patch_t *)W_CacheLumpName((count & 1) ? "M_SLDMD1" : "M_SLDMD2", PU_CACHE));
	}
	V_DrawPatch(x2, y, (patch_t *)W_CacheLumpName("M_SLDRT", PU_CACHE));
	V_DrawPatch(x + 4 + slot*8, y + 7, (patch_t *)W_CacheLumpName("M_SLDKB", PU_CACHE));
#endif
}


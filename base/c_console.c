//**************************************************************************
//**
//** c_console.c : HHexen : Dan Olson
//**
//**************************************************************************
// HEADER FILES ------------------------------------------------------------
#include "h2def.h"

// MACROS ------------------------------------------------------------------

#define CONBUFFERSIZE 16384

// TYPES -------------------------------------------------------------------

typedef struct cmd_s
{
	char *name;
	int (*func)(int argc, char **argv);
	struct cmd_s *next;
} cmd_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

int CFListCvars(int argc, char **argv);
int CFListCmds( int argc, char **argv);
int CFQuit(int argc, char **argv);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern boolean demorecording;
extern boolean demoplayback;
extern boolean MenuActive;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

boolean ConsoleActive;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static cmd_t *Commands;
static int ConsoleTime;
static int FontLump;
static int BottomLine;
static byte *ConBack;
static byte *ConBackSection;
static char *ConsoleBuffer;
static int ShowCursor = 1;
// FIXME!!!
static int LineWidth = SCREENWIDTH / 10;
static int ConPos = 0;

// CODE --------------------------------------------------------------------

int CFListCvars(int argc, char **argv)
{
	return 0;
}

int CFListCmds(int argc, char **argv)
{
	return 0;
}

int CFQuit(int argc, char **argv)
{
	return 0;
}

void CON_BufInsertLine (const char *line)
{
	// FIXME: Handle long strings.
	// FIXME: LineWidth is unitialized
	memcpy (&ConsoleBuffer[ConPos*LineWidth],line,strlen(line));
	ConPos++;
}	
		
void CON_SetFont (int x)
{
	switch (x) {
		case 0:
			FontLump = W_GetNumForName("FONTA_S")+1;
			break;
		case 1:
			FontLump = W_GetNumForName ("FONTAY_S")+1;
			break;
		case 2:
			FontLump = W_GetNumForName ("FONTB_S")+1;
			break;
	}
}	

void CON_DrawMessage (char *text, int x, int y)
{
	char c;
	patch_t *p;

	if (ConsoleActive == false)
		return;

	while ((c = *text++) != 0)
	{
		if (c < 33)
		{
			x += 5;
		}
		else
		{
			p = W_CacheLumpNum(FontLump+c-33, PU_CACHE);
			V_DrawPatch (x, y, p);
			x += p->width-1;
		}
	}
}

void CON_Show ()
{
}

void CON_Init(void)
{
	ConsoleActive = false;
	CON_SetFont (0);	// Don't want this to be uninitialized
	ConsoleBuffer = (byte *) malloc (sizeof(byte)*CONBUFFERSIZE);
	memset ((byte *)ConsoleBuffer, ' ', CONBUFFERSIZE);
	CON_BufInsertLine ("Console initialized. Welcome to HHexen 1.4");
}

void CON_UnInit(void)
{
	free (ConsoleBuffer);
}

void CON_Activate(void)
{
	// Allocate and fill the conback
	ConBack = (byte *) malloc (SCREENWIDTH*SCREENHEIGHT);
	memcpy(ConBack, (byte *)W_CacheLumpName("INTERPIC", PU_CACHE),SCREENWIDTH*SCREENHEIGHT);

	ConsoleActive = true;
	paused = true;
	ConsoleTime = 0;
}

void CON_DeActivate(void)
{
	ConsoleActive = false;
	paused = false;
}

void CON_Ticker(void)
{
	if (ConsoleActive == false)
		return;
	ConsoleTime++;
	if (!(ConsoleTime % 15)) 
		ShowCursor = !ShowCursor;
}

void CON_Drawer(void)
{
	if (ConsoleActive == false && BottomLine == 0)
		return;

	// Update the console position
	if (ConsoleActive == true && BottomLine < 100) {
		BottomLine += 15;
		if (BottomLine > 100) BottomLine = 100;
	}
	else if (ConsoleActive == false && BottomLine != 0) {
		BottomLine -= 15;
		if (BottomLine < 0) {
			BottomLine = 0;
			free (ConBack);
		}
	}

	// Draw the console background
	ConBackSection = ConBack + SCREENWIDTH*(SCREENHEIGHT-BottomLine);
	V_BlitToScreen(0,0,ConBackSection,SCREENWIDTH, BottomLine);
	CON_SetFont(1);
	CON_DrawMessage ("HHEXEN V1.4", SCREENWIDTH-85, (BottomLine-10));
	if (BottomLine > 20) {
		// Fix these once you have a real font
		CON_DrawMessage (":", 0, (BottomLine - 20));
		if (ShowCursor) {
			CON_SetFont(0);
			CON_DrawMessage (":", 10, (BottomLine - 20));
		}
	}
	CON_Show ();
}

boolean CON_Responder (event_t *event)
{
	int key;
	static boolean shiftdown;
	
	if (event->data1 == KEY_RSHIFT) {
		shiftdown = (event->type == ev_keydown);
		return (false);
	}
	if (event->type != ev_keydown)
		return (false);
	key = event->data1;

	// Tilde key enables/disables Console
	if ((key == '`') && !(demorecording || demoplayback)
			&& (MenuActive == false))
	{
		if (ConsoleActive == true) {
			if (!netgame && !demoplayback)
			{
				CON_DeActivate();	
			}
		} else {
			if (!netgame && !demoplayback)
			{
				CON_Activate();	
			}
		}
		return (true);
	}

	if (ConsoleActive == true) {
		// Do nothing for now :/
		return (true);
	}
	return (false);
}


//**************************************************************************
//**
//** template.h : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: st_start.h,v $
//** $Revision: 1.1.1.1 $
//** $Date: 2000-04-11 17:38:18 $
//** $Author: theoddone33 $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------
extern void ST_Init(void);
extern void ST_Done(void);
extern void ST_Message(char *message, ...);
extern void ST_RealMessage(char *message, ...);
extern void ST_Progress(void);
extern void ST_NetProgress(void);
extern void ST_NetDone(void);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

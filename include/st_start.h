
//**************************************************************************
//**
//** template.h : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: st_start.h,v $
//** $Revision: 1.2 $
//** $Date: 2008-06-17 09:20:20 $
//** $Author: sezero $
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


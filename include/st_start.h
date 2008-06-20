
//**************************************************************************
//**
//** template.h : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: st_start.h,v $
//** $Revision: 1.3 $
//** $Date: 2008-06-20 07:04:27 $
//** $Author: sezero $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------
extern void ST_Init(void);
extern void ST_Done(void);
extern void ST_Message(const char *message, ...) __attribute__((format(printf,1,2)));
extern void ST_RealMessage(const char *message, ...) __attribute__((format(printf,1,2)));
extern void ST_Progress(void);
extern void ST_NetProgress(void);
extern void ST_NetDone(void);

// PUBLIC DATA DECLARATIONS ------------------------------------------------


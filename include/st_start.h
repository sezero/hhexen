
//**************************************************************************
//**
//** st_start.h : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: st_start.h,v $
//** $Revision: 1.4 $
//** $Date: 2008-06-22 16:20:46 $
//** $Author: sezero $
//**
//**************************************************************************

#ifndef __ST_START__
#define __ST_START__

extern void ST_Init(void);
extern void ST_Done(void);
extern void ST_Message(const char *message, ...) __attribute__((format(printf,1,2)));
extern void ST_RealMessage(const char *message, ...) __attribute__((format(printf,1,2)));
extern void ST_Progress(void);
extern void ST_NetProgress(void);
extern void ST_NetDone(void);

#endif	/* __ST_START__ */


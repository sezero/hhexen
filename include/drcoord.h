
//**************************************************************************
//**
//** DRCoord.h : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: drcoord.h,v $
//** $Revision: 1.1.1.1 $
//** $Date: 2000-04-11 17:38:16 $
//** $Author: theoddone33 $
//**
//**************************************************************************

#import <appkit/appkit.h>

@interface DRCoord:Object
{
	id	players_i;
	id	console_i;
	id	skill_i;
	id	episode_i;
	id	map_i;
}

- newGame: sender;
- scale1: sender;
- scale2: sender;
- scale4: sender;

@end

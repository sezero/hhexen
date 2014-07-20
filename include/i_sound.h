//**************************************************************************
//**
//** i_sound.h : Heretic 2 : Raven Software, Corp.
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

#ifndef __SOUND__
#define __SOUND__

#define SND_TICRATE		140	/* tic rate for updating sound */
#define SND_MAXSONGS		40	/* max number of songs in game */
#define SND_SAMPLERATE		11025	/* sample rate of sound effects */

typedef enum
{
	snd_none,
	snd_PC,
	snd_Adlib,
	snd_SB,
	snd_PAS,
	snd_GUS,
	snd_MPU,
	snd_MPU2,
	snd_MPU3,
	snd_AWE,
	snd_CDMUSIC,
	NUM_SCARDS
} cardenum_t;

void I_PauseSong(int handle);
void I_ResumeSong(int handle);
void I_SetMusicVolume(int volume);
void I_SetSfxVolume(int volume);
int I_RegisterSong(void *data, int siz);
int I_RegisterExternalSong(const char *name);	/* External music file support */
void I_UnRegisterSong(int handle);
int I_QrySongPlaying(int handle);
void I_StopSong(int handle);
void I_PlaySong(int handle, boolean looping);
int I_GetSfxLumpNum(sfxinfo_t *sound);
int I_StartSound (int id, void *data, int vol, int sep, int pitch, int priority);
void I_StopSound(int handle);
int I_SoundIsPlaying(int handle);
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch);
void I_sndArbitrateCards(void);
void I_StartupSound (void);
void I_ShutdownSound (void);
void I_SetChannels(int channels);

#endif	/* __SOUND__ */


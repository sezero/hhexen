/* $Id: i_sdlmixer.c,v 1.4 2008-12-11 16:55:33 sezero Exp $
 *
 *  Ripped && Adapted from the PrBoom project:
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA.
 *
 * DESCRIPTION:
 *  System interface for sound, using SDL_mixer.
 *
 */


#include "h2stdinc.h"
#include <unistd.h>
#include <math.h>	/* pow() */
#include "SDL.h"
#include "SDL_mixer.h"
#include "h2def.h"
#include "sounds.h"
#include "i_sound.h"
#include "mmus2mid.h"


#define SAMPLE_FORMAT	AUDIO_S16SYS
#define SAMPLE_ZERO	0
#define SAMPLE_RATE	11025	/* Hz */
#define SAMPLE_CHANNELS	2

#if 0
#define SAMPLE_TYPE	char
#else
#define SAMPLE_TYPE	short
#endif


/*
 *	SOUND HEADER & DATA
 */

int tsm_ID = -1;

int snd_Channels;
int snd_DesiredMusicDevice, snd_DesiredSfxDevice;
int snd_MusicDevice,		/* current music card # (index to dmxCodes) */
	snd_SfxDevice,		/* current sfx card # (index to dmxCodes) */
	snd_MaxVolume,		/* maximum volume for sound */
	snd_MusicVolume;	/* maximum volume for music */

int snd_SBport, snd_SBirq, snd_SBdma;	/* sound blaster variables */
int snd_Mport;				/* midi variables */

boolean snd_MusicAvail,		/* whether music is available */
	snd_SfxAvail;		/* whether sfx are available */

/*
 *	SOUND FX API
 */

typedef struct
{
	unsigned char	*begin;		/* pointers into Sample.firstSample */
	unsigned char	*end;

	SAMPLE_TYPE	*lvol_table;	/* point into vol_lookup */
	SAMPLE_TYPE	*rvol_table;

	unsigned int	pitch_step;
	unsigned int	step_remainder;	/* 0.16 bit remainder of last step. */

	int		pri;
	unsigned int	time;
} Channel;

typedef struct
{
/* Sample data is a lump from a wad: byteswap the a, freq
 * and the length fields before using them		*/
	short		a;		/* always 3	*/
	short		freq;		/* always 11025	*/
	int32_t		length;		/* sample length */
	unsigned char	firstSample;
} Sample;
COMPILE_TIME_ASSERT(Sample, offsetof(Sample,firstSample) == 8);


#define CHAN_COUNT	8
static Channel	channel[CHAN_COUNT];

#define MAX_VOL		64	/* 64 keeps our table down to 16Kb */
static SAMPLE_TYPE	vol_lookup[MAX_VOL * 256];

static int	steptable[256];		/* Pitch to stepping lookup */

static boolean	snd_initialized;
static int	SAMPLECOUNT = 512;
int	snd_samplerate = SAMPLE_RATE;


static void audio_loop (void *unused, Uint8 *stream, int len)
{
	Channel* chan;
	Channel* cend;
	SAMPLE_TYPE *begin;
	SAMPLE_TYPE *end;
	unsigned int sample;
	register int dl;
	register int dr;

	end = (SAMPLE_TYPE *) (stream + len);
	cend = channel + CHAN_COUNT;

		begin = (SAMPLE_TYPE *) stream;
		while (begin < end)
		{
		// Mix all the channels together.
			// Do not zero: SDL_mixer writes the
			// music before sending the stream!!
			dl = begin[0];	//	dl = SAMPLE_ZERO;
			dr = begin[1];	//	dr = SAMPLE_ZERO;

			chan = channel;
			for ( ; chan < cend; chan++)
			{
				// Check channel, if active.
				if (chan->begin)
				{
					// Get the sample from the channel.
					sample = *chan->begin;

					// Adjust volume accordingly.
					dl += chan->lvol_table[sample];
					dr += chan->rvol_table[sample];

					// Increment sample pointer with pitch adjustment.
					chan->step_remainder += chan->pitch_step;
					chan->begin += chan->step_remainder >> 16;
					chan->step_remainder &= 65535;

					// Check whether we are done.
					if (chan->begin >= chan->end)
					{
						chan->begin = NULL;
					//	printf ("  channel done %d\n", chan);
					}
				}
			}
#if 0	/* SAMPLE_FORMAT */
			if (dl > 127)
				dl = 127;
			else if (dl < -128)
				dl = -128;

			if (dr > 127)
				dr = 127;
			else if (dr < -128)
				dr = -128;
#else
			if (dl > 0x7fff)
				dl = 0x7fff;
			else if (dl < -0x8000)
				dl = -0x8000;

			if (dr > 0x7fff)
				dr = 0x7fff;
			else if (dr < -0x8000)
				dr = -0x8000;
#endif

			*begin++ = dl;
			*begin++ = dr;
		}
}


void I_SetSfxVolume(int volume)
{
}

// Gets lump nums of the named sound.  Returns pointer which will be
// passed to I_StartSound() when you want to start an SFX.  Must be
// sure to pass this to UngetSoundEffect() so that they can be
// freed!

int I_GetSfxLumpNum(sfxinfo_t *sound)
{
	return W_GetNumForName(sound->lumpname);
}


// Id is unused.
// Data is a pointer to a Sample structure.
// Volume ranges from 0 to 127.
// Separation (orientation/stereo) ranges from 0 to 255.  128 is balanced.
// Pitch ranges from 0 to 255.  Normal is 128.
// Priority looks to be unused (always 0).

int I_StartSound(int id, void *data, int vol, int sep, int pitch, int priority)
{
	// Relative time order to find oldest sound.
	static unsigned int soundTime = 0;
	int chanId;
	Sample *sample;
	Channel *chan;
	int oldest;
	int i;

	// Find an empty channel, the oldest playing channel, or default to 0.
	// Currently ignoring priority.

	chanId = 0;
	oldest = soundTime;
	for (i = 0; i < CHAN_COUNT; i++)
	{
		if (! channel[ i ].begin)
		{
			chanId = i;
			break;
		}
		if (channel[ i ].time < oldest)
		{
			chanId = i;
			oldest = channel[ i ].time;
		}
	}

	sample = (Sample *) data;
	chan = &channel[chanId];

	I_UpdateSoundParams(chanId + 1, vol, sep, pitch);

	// begin must be set last because the audio thread will access the channel
	// once it is non-zero.  Perhaps this should be protected by a mutex.
	chan->pri = priority;
	chan->time = soundTime;
	chan->end = &sample->firstSample + LONG(sample->length);
	chan->begin = &sample->firstSample;

	soundTime++;

#if 0
	printf ("I_StartSound %d: v:%d s:%d p:%d pri:%d | %d %d %d %d\n",
		id, vol, sep, pitch, priority,
		chanId, chan->pitch_step, SHORT(sample->a), SHORT(sample->freq));
#endif

	return chanId + 1;
}

void I_StopSound(int handle)
{
	handle--;
	handle &= 7;
	channel[handle].begin = NULL;
}

int I_SoundIsPlaying(int handle)
{
	handle--;
	handle &= 7;
	return (channel[ handle ].begin != NULL);
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
	int lvol, rvol;
	Channel *chan;

	if (!snd_initialized)
		return;
	SDL_LockAudio();
	// Set left/right channel volume based on seperation.
	sep += 1;	// range 1 - 256
	lvol = vol - ((vol * sep * sep) >> 16);	// (256*256);
	sep = sep - 257;
	rvol = vol - ((vol * sep * sep) >> 16);

	// Sanity check, clamp volume.
	if (rvol < 0)
	{
	//	printf ("rvol out of bounds %d, id %d\n", rvol, handle);
		rvol = 0;
	}
	else if (rvol > 127)
	{
	//	printf ("rvol out of bounds %d, id %d\n", rvol, handle);
		rvol = 127;
	}

	if (lvol < 0)
	{
	//	printf ("lvol out of bounds %d, id %d\n", lvol, handle);
		lvol = 0;
	}
	else if (lvol > 127)
	{
	//	printf ("lvol out of bounds %d, id %d\n", lvol, handle);
		lvol = 127;
	}

	// Limit to MAX_VOL (64)
	lvol >>= 1;
	rvol >>= 1;

	handle--;
	handle &= 7;
	chan = &channel[handle];
	chan->pitch_step = steptable[pitch];
	chan->step_remainder = 0;
	chan->lvol_table = &vol_lookup[lvol * 256];
	chan->rvol_table = &vol_lookup[rvol * 256];

	SDL_UnlockAudio();
}


/*
 *	SOUND STARTUP STUFF
 */

// inits all sound stuff
void I_StartupSound (void)
{
	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	int audio_buffers;

	if (snd_initialized)
		return;

	/* Initialize variables */
	snd_SfxAvail = snd_MusicAvail = false;
	snd_MusicDevice = snd_SfxDevice = 0;
	audio_rate = snd_samplerate;
	audio_format = SAMPLE_FORMAT;
	audio_channels = SAMPLE_CHANNELS;
	SAMPLECOUNT = 512;
	audio_buffers = SAMPLECOUNT*snd_samplerate/11025;

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0)
	{
		fprintf(stderr, "Couldn't open audio with desired format\n");
		return;
	}
	snd_initialized = true;
	SAMPLECOUNT = audio_buffers;
	Mix_SetPostMix(audio_loop, NULL);
	fprintf(stdout, "Configured audio device with %d samples/slice\n", SAMPLECOUNT);
	snd_SfxAvail = snd_MusicAvail = true;
//	SDL_PauseAudio(0);

	if (snd_MusicVolume < 0 || snd_MusicVolume > 15)
		snd_MusicVolume = 10;
	Mix_VolumeMusic(snd_MusicVolume*8);
}

// shuts down all sound stuff
void I_ShutdownSound (void)
{
	if (snd_initialized)
	{
		snd_initialized = false;
		snd_SfxAvail = false;
		snd_MusicAvail = false;
		Mix_CloseAudio();
	}
}

void I_SetChannels(int channels)
{
	int v, j;
	int *steptablemid;

	// We always have CHAN_COUNT channels.

	for (j = 0; j < CHAN_COUNT; j++)
	{
		channel[j].begin = NULL;
		channel[j].end   = NULL;
		channel[j].time = 0;
	}

	// This table provides step widths for pitch parameters.
	steptablemid = steptable + 128;
	for (j = -128; j < 128; j++)
	{
		steptablemid[j] = (int) (pow(2.0, (j/64.0)) * 65536.0);
	}

	// Generate the volume lookup tables.
	for (v = 0; v < MAX_VOL; v++)
	{
		for (j = 0; j < 256; j++)
		{
		//	vol_lookup[v*256+j] = 128 + ((v * (j-128)) / (MAX_VOL-1));

		// Turn the unsigned samples into signed samples.

#if 0	/* SAMPLE_FORMAT */
			vol_lookup[v*256+j] = (v * (j-128)) / (MAX_VOL-1);
#else
			vol_lookup[v*256+j] = (v * (j-128) * 256) / (MAX_VOL-1);
#endif
		//	printf ("vol_lookup[%d*256+%d] = %d\n", v, j, vol_lookup[v*256+j]);
		}
	}
}


/*
 *	SONG API
 */

static Mix_Music *CurrentSong = NULL;

int I_RegisterSong(void *data)
{
	MIDI *mididata;
	byte *mid;
	int midlen;
	char tmpfile[MAX_OSPATH];
	int err;

	if (!snd_initialized)
		return 0;
	if (memcmp(data, "MUS", 3) != 0)
		return 0;
	mididata = (MIDI *)malloc(sizeof(MIDI));
	if (!mididata)
		return 0;
	err = mmus2mid(data, mididata, 89, 0);
	if (err)
	{
		free(mididata);
		return 0;
	}
	err = MIDIToMidi(mididata, &mid, &midlen);
	if (err)
	{
		free(mididata);
		return 0;
	}
	snprintf(tmpfile, sizeof(tmpfile), "%stmpmusic.mid", basePath);
	M_WriteFile(tmpfile, mid, midlen);
	free(mid);
	free_mididata(mididata);
	free(mididata);
	CurrentSong = Mix_LoadMUS(tmpfile);
	if (!CurrentSong)
	{
		fprintf(stderr, "Mix_LoadMUS failed: %s\n", Mix_GetError());
		return 0;
	}
	return 1;
}

/* External music file support : */
static const struct
{
	Mix_MusicType	type;		/* as enum'ed in SDL_mixer.h	*/
	const char	*ext;		/* the file extension		*/
} MusicFile[] =
{
	{ MUS_OGG, "ogg" },
	{ MUS_MP3, "mp3" },
	{ MUS_MID, "mid" },	/* midi must be the last before NULL	*/
	{ MUS_NONE, NULL }	/* the last entry must be NULL		*/
};

int I_RegisterExternalSong(const char *name)
{
	int i = 0, ret = -1;
	char path[MAX_OSPATH];

	if (!snd_initialized)
		return 0;
	if (!name || ! *name)
		return 0;

	while (MusicFile[i].ext != NULL)
	{
		/* first try from <userdir>/music */
		snprintf (path, sizeof(path), "%smusic/%s.%s",
				basePath, name, MusicFile[i].ext);
		ret = access(path, R_OK);
		if (ret == -1)
		{
			/* try from <CWD>/music */
			snprintf (path, sizeof(path), "music/%s.%s",
					name, MusicFile[i].ext);
			ret = access(path, R_OK);
		}
		if (ret != -1)
			break;
		i++;
	}
	if (ret == -1)
		return 0;
	CurrentSong = Mix_LoadMUS(path);
	if (!CurrentSong)
	{
		fprintf(stderr, "Mix_LoadMUS failed for %s.%s: %s\n",
				name, MusicFile[i].ext, Mix_GetError());
		return 0;
	}
	return 1;
}

void I_UnRegisterSong(int handle)
{
	if (handle != 1)
		return;
	if (CurrentSong)
	{
		Mix_FreeMusic(CurrentSong);
		CurrentSong = NULL;
	}
}

void I_PauseSong(int handle)
{
	if (handle != 1)
		return;
	if (CurrentSong)
		Mix_PauseMusic();
}

void I_ResumeSong(int handle)
{
	if (handle != 1)
		return;
	if (CurrentSong)
		Mix_ResumeMusic();
}

void I_SetMusicVolume(int volume)
{
	if (snd_initialized)
		Mix_VolumeMusic(volume*8);
}

int I_QrySongPlaying(int handle)
{
	if (snd_initialized)
		return Mix_PlayingMusic();
	return 0;
}

// Stops a song.  MUST be called before I_UnregisterSong().
void I_StopSong(int handle)
{
	if (handle != 1)
		return;
	if (CurrentSong)
		Mix_HaltMusic();
}

void I_PlaySong(int handle, boolean looping)
{
	if (handle != 1)
		return;
	if (CurrentSong)
		Mix_FadeInMusic(CurrentSong, looping ? -1 : 0, 500);
}


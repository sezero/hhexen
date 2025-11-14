/* alsa.c -- ALSA output for Linux, based on XMP player:
 *
 * Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include "config.h"

#include <stdio.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

#include "audio_plugin.h"

static snd_pcm_t *pcm_handle;

static void alsa_init(void)
{
}

static int alsa_open(AFormat format, int srate, int nch)
{
	snd_pcm_hw_params_t *hwparams;
	int ret;
	snd_pcm_format_t fmt;
	unsigned int rate, channels;
	unsigned int btime = 125000;	/* 125ms */
	unsigned int ptime =  25000;	/*  25ms */
	const char *card_name = "default";

	if (srate <= 0) return 0;
	if (nch != 1 && nch != 2)
		return 0;
	rate = srate;
	channels = nch;
	switch (format) {
	case FMT_U8:
		fmt = SND_PCM_FORMAT_U8;
		break;
	case FMT_S16:
		fmt = SND_PCM_FORMAT_S16;
		break;
	default:
		return 0;
	}

	if ((ret = snd_pcm_open(&pcm_handle, card_name,
		SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Unable to initialize ALSA pcm device %s: %s\n",
						card_name, snd_strerror(ret));
		return -1;
	}

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_hw_params_any(pcm_handle, hwparams);
	snd_pcm_hw_params_set_access(pcm_handle, hwparams,
						SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, hwparams, fmt);
	snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL);
	snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &channels);
	snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &btime, NULL);
	snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &ptime, NULL);
	snd_pcm_nonblock(pcm_handle, 0);

	if ((ret = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
		fprintf(stderr, "Unable to set ALSA output parameters: %s\n",
							snd_strerror(ret));
		return -1;
	}

	if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
		fprintf(stderr, "Unable to prepare ALSA: %s\n",
					snd_strerror(ret));
		return -1;
	}

	return 0;
}

static void alsa_write(void *ptr, int len)
{
	int frames = snd_pcm_bytes_to_frames(pcm_handle, len);

	if (snd_pcm_writei(pcm_handle, ptr, frames) < 0) {
		snd_pcm_prepare(pcm_handle);
	}
}

static void alsa_close(void)
{
	snd_pcm_close(pcm_handle);
}

static const char *alsa_about(void)
{
	return "ALSA PCM audio";
}

static OutputPlugin alsa_op =
{
	alsa_about,
	alsa_init,
	alsa_open,
	alsa_write,
	alsa_close,
};

OutputPlugin *get_oplugin_info(void)
{
	return &alsa_op;
}

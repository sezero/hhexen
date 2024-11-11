/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-1999  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "audio_plugin.h"

#ifndef AFMT_S16_NE
# ifdef WORDS_BIGENDIAN
#  define AFMT_S16_NE AFMT_S16_BE
# else
#  define AFMT_S16_NE AFMT_S16_LE
# endif
#endif


/* All that remains of glib usage... */
typedef unsigned char	guchar;
typedef unsigned short	gushort;
typedef unsigned int	guint;
typedef unsigned long	gulong;

#define NFRAGS		32

static void oss_set_audio_params(void);

static int audio_fd = 0;
static void *buffer;
static int going = 0;
static int blk_size;
static int bps;
static int fragsize, oss_format, channels;
static int frequency, efrequency;
static char device_name[32];

static struct OSSConfig
{
	int audio_device;
	int mixer_device;
	int buffer_size;
	int prebuffer;
	int fragment_count;
} oss_cfg;


static const char *oss_about(void)
{
	return "XMMS OSS Driver 0.9.5.1";
}

static int oss_setup_format(AFormat fmt,int rate, int nch)
{
	frequency = rate;
	channels = nch;
	bps = rate * nch;
	switch (fmt)
	{
	case FMT_U8:
		oss_format = AFMT_U8;
		break;
	case FMT_S16:
		bps *= 2;
		oss_format = AFMT_S16_NE;
		break;
	default:
		return -1;
	}

	fragsize = 0;
	while ((1L << fragsize) < bps / 25)
		fragsize++;
	fragsize--;

	return 0;
}

static int oss_downsample(guchar * ob, guint length, guint speed, guint espeed)
{
	guint nlen, i, off, d, w;

	if (oss_format == AFMT_S16_NE && channels == 2)
	{
		gulong *nbuffer, *obuffer, *ptr;

		obuffer = (gulong *) ob;
		length >>= 2;

		nlen = (length * espeed) / speed;
		d = (speed << 8) / espeed;

		nbuffer = (gulong *) malloc(nlen << 2);
		for (i = 0, off = 0, ptr = nbuffer; i < nlen; i++)
		{
			*ptr++ = obuffer[off >> 8];
			off += d;
		}
		w = write(audio_fd, nbuffer, nlen << 2);
		free(nbuffer);
	}
	else if ((oss_format == AFMT_S16_NE && channels == 1)
	      || (oss_format == AFMT_U8 && channels == 2))
	{
		gushort *nbuffer, *obuffer, *ptr;

		obuffer = (gushort *) ob;
		length >>= 1;

		nlen = (length * espeed) / speed;
		d = (speed << 8) / espeed;

		nbuffer = (gushort *) malloc(nlen << 1);
		for (i = 0, off = 0, ptr = nbuffer; i < nlen; i++)
		{
			*ptr++ = obuffer[off >> 8];
			off += d;
		}
		w = write(audio_fd, nbuffer, nlen << 1);
		free(nbuffer);
	}
	else
	{
		guchar *nbuffer, *obuffer, *ptr;

		obuffer = ob;

		nlen = (length * espeed) / speed;
		d = (speed << 8) / espeed;

		nbuffer = (guchar *) malloc(nlen);
		for (i = 0, off = 0, ptr = nbuffer; i < nlen; i++)
		{
			*ptr++ = obuffer[off >> 8];
			off += d;
		}
		w = write(audio_fd, nbuffer, nlen);
		free(nbuffer);
	}
	return w;
}

static void oss_write_audio(void *data, int length)
{
	audio_buf_info abuf_info;

	if (!ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &abuf_info))
	{
		while (abuf_info.bytes < length)
		{
			usleep(10000);
			ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &abuf_info);
		}
	}
	if (frequency == efrequency)
		write(audio_fd, data, length);
	else
		oss_downsample(data, length, frequency, efrequency);
}

static void oss_write(void *ptr, int length)
{
	oss_write_audio(ptr, length);
}

static void oss_close(void)
{
	going = 0;
	ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
	close(audio_fd);
}

static void oss_set_audio_params(void)
{
	int frag, stereo;

	ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
//	frag = (NFRAGS << 16) | fragsize;
	frag = (oss_cfg.fragment_count << 16) | fragsize;
	ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag);
	ioctl(audio_fd, SNDCTL_DSP_SETFMT, &oss_format);
	ioctl(audio_fd, SNDCTL_DSP_SETFMT, &oss_format);
	stereo = channels - 1;
	ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo);
	efrequency = frequency;
	ioctl(audio_fd, SNDCTL_DSP_SPEED, &efrequency);
	ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &blk_size);
	if (abs(((efrequency * 100) / frequency) - 100) < 10)
		efrequency = frequency;
	if (ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &blk_size) == -1)
		blk_size = 1L << fragsize;
}

static int oss_open(AFormat fmt, int rate, int nch)
{
	if (oss_setup_format(fmt, rate, nch) < 0)
		return -1;

	if (oss_cfg.audio_device > 0)
		snprintf(device_name, sizeof(device_name), "/dev/dsp%d", oss_cfg.audio_device);
	else
	{
		strncpy(device_name, "/dev/dsp", sizeof(device_name) - 1);
		device_name[sizeof(device_name) - 1] = '\0';
	}

	audio_fd = open(device_name, O_WRONLY);
	if (audio_fd < 0)
	{
		if (buffer)
			free(buffer);
		buffer = NULL;
		return -1;
	}

	oss_set_audio_params();

	going = 1;

	return 0;
}

static void oss_init(void)
{
	memset(&oss_cfg, 0, sizeof (struct OSSConfig));

	oss_cfg.audio_device = 0;
	oss_cfg.mixer_device = 0;
	oss_cfg.buffer_size = 3000;
//	oss_cfg.fragment_count = NFRAGS;
	oss_cfg.fragment_count = 3;
}


static OutputPlugin oss_op =
{
	oss_about,
	oss_init,
	oss_open,
	oss_write,
	oss_close,
};

OutputPlugin *get_oplugin_info(void)
{
	return &oss_op;
}


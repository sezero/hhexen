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


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/soundcard.h>
#include "oss.h"

#ifndef AFMT_U16_NE
# ifdef WORDS_BIGENDIAN
#  define AFMT_U16_NE AFMT_U16_BE
# else
#  define AFMT_U16_NE AFMT_U16_LE
# endif
#endif
#ifndef AFMT_S16_NE
# ifdef WORDS_BIGENDIAN
#  define AFMT_S16_NE AFMT_S16_BE
# else
#  define AFMT_S16_NE AFMT_S16_LE
# endif
#endif


/* All that remains of glib usage... */
#define FALSE			0
#define TRUE			1
typedef int		gboolean;
typedef unsigned char	guchar;
typedef unsigned short	gushort;
typedef unsigned int	guint;
typedef unsigned long	gulong;

#define NFRAGS		32

#define min(x,y)	((x) < (y) ? (x) : (y))

static void oss_set_audio_params(void);

static int audio_fd = 0;
static void *buffer;
static gboolean going = FALSE, prebuffer = FALSE, remove_prebuffer = FALSE;
static gboolean paused = FALSE, unpause = FALSE, do_pause = FALSE;
static int buffer_size, prebuffer_size, blk_size;
static int rd_index = 0, wr_index = 0;
static int output_time_offset = 0, written = 0, output_bytes = 0;
static int bps, ebps;
static int flush;
static int fragsize, format, oss_format, channels;
static int frequency, efrequency, device_buffer_size;
static int input_bps, input_format, input_frequency, input_channels;
static char device_name[16];
static pthread_t buffer_thread;
static gboolean realtime = FALSE;


static OSSConfig oss_cfg;


static void oss_about(void)
{
	printf ("XMMS OSS Driver 0.9.5.1\n");
}

static void oss_setup_format(AFormat fmt,int rate, int nch)
{
	format = fmt;
	frequency = rate;
	channels = nch;
	switch (fmt)
	{
	case FMT_U8:
		oss_format = AFMT_U8;
		break;
	case FMT_S8:
		oss_format = AFMT_S8;
		break;
	case FMT_U16_LE:
		oss_format = AFMT_U16_LE;
		break;
	case FMT_U16_BE:
		oss_format = AFMT_U16_BE;
		break;
	case FMT_U16_NE:
		oss_format = AFMT_U16_NE;
		break;
	case FMT_S16_LE:
		oss_format = AFMT_S16_LE;
		break;
	case FMT_S16_BE:
		oss_format = AFMT_S16_BE;
		break;
	case FMT_S16_NE:
		oss_format = AFMT_S16_NE;
		break;
	}

	bps = rate * nch;
	if (oss_format == AFMT_U16_BE || oss_format == AFMT_U16_LE || oss_format == AFMT_S16_BE || oss_format == AFMT_S16_LE)
		bps *= 2;
	fragsize = 0;
	while ((1L << fragsize) < bps / 25)
		fragsize++;
	fragsize--;

//	device_buffer_size = ((1L << fragsize) * (NFRAGS + 1));
	device_buffer_size = ((1L << fragsize) * (oss_cfg.fragment_count + 1));
}

static int oss_get_written_time(void)
{
	if (!going)
		return 0;
	return (int) (((float) written * 1000) / (float) (input_bps));
}

static int oss_get_output_time(void)
{
	audio_buf_info buf_info;
	int bytes;

	if (!audio_fd || !going)
		return 0;

	if (!paused)
	{
		if (!ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &buf_info))
			bytes = output_bytes - ((buf_info.fragstotal - buf_info.fragments) * buf_info.fragsize);
		else
			bytes = output_bytes;
	}
	else
		bytes = output_bytes;

	if (bytes < 0)
		bytes = 0;
	return output_time_offset + (int) ((float) ((bytes) * 1000.0) / (float) ebps);
}

static int oss_used(void)
{
	if (realtime)
		return 0;
	else
	{
		if (wr_index >= rd_index)
			return wr_index - rd_index;
		return buffer_size - (rd_index - wr_index);
	}
}

static int oss_playing(void)
{
	audio_buf_info buf_info;
	int bytes;

	if (!ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &buf_info))
		bytes = ((buf_info.fragstotal - buf_info.fragments - 3) * buf_info.fragsize);
	else
		bytes = 0;

	if (!oss_used() && bytes <= 0)
		return FALSE;

	return TRUE;
}

static int oss_free(void)
{
	if (!realtime)
	{
		if (remove_prebuffer && prebuffer)
		{
			prebuffer = FALSE;
			remove_prebuffer = FALSE;
		}
		if (prebuffer)
			remove_prebuffer = TRUE;

		if (rd_index > wr_index)
			return (rd_index - wr_index) - device_buffer_size - 1;
		return (buffer_size - (wr_index - rd_index)) - device_buffer_size - 1;
	}
	else
	{
		if (paused)
			return 0;
		else
			return 1000000;
	}
}

static int oss_downsample(guchar * ob, guint length, guint speed, guint espeed)
{
	guint nlen, i, off, d, w;

	if ((oss_format == AFMT_U16_BE || oss_format == AFMT_U16_LE || oss_format == AFMT_S16_BE || oss_format == AFMT_S16_LE) && channels == 2)
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
	else if (((oss_format == AFMT_U16_BE || oss_format == AFMT_U16_LE || oss_format == AFMT_S16_BE || oss_format == AFMT_S16_LE) && channels == 1)
	      || ((oss_format == AFMT_U8 || oss_format == AFMT_S8) && channels == 2))
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
		output_bytes += write(audio_fd, data, length);
	else
		output_bytes += oss_downsample(data, length, frequency, efrequency);
}

static void oss_write(void *ptr, int length)
{
	int cnt, off = 0;

	if (!realtime)
	{
		remove_prebuffer = FALSE;
		written += length;
		while (length > 0)
		{
			cnt = min(length, buffer_size - wr_index);
			memcpy((guchar *)buffer + wr_index, (guchar *)ptr + off, cnt);
			wr_index = (wr_index + cnt) % buffer_size;
			length -= cnt;
			off = +cnt;
		}
	}
	else
	{
		if (paused)
			return;

		oss_write_audio(ptr, length);
		written += length;
	}
}

static void oss_close(void)
{
	going = 0;
	if (!realtime)
		pthread_join(buffer_thread, NULL);
	else
	{
		ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
		close(audio_fd);
	}
	wr_index = 0;
	rd_index = 0;
}

static void oss_flush(int flushtime)
{
	if (!realtime)
	{
		flush = flushtime;
		while (flush != -1)
			usleep(10000);
	}
	else
	{
		ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
		close(audio_fd);
		audio_fd = open(device_name, O_WRONLY);
		oss_set_audio_params();
		output_time_offset = flushtime;
		written = (flushtime / 10) * (input_bps / 100);
		output_bytes = 0;
	}
}

static void oss_pause(short p)
{
	if (!realtime)
	{
		if (p == TRUE)
			do_pause = TRUE;
		else
			unpause = TRUE;
	}
	else
		paused = p;
}

static void *oss_loop(void *arg)
{
	int length, cnt;
	audio_buf_info abuf_info;

	while (going)
	{
		if (oss_used() > prebuffer_size)
			prebuffer = FALSE;
		if (oss_used() > 0 && !paused && !prebuffer)
		{
			length = min(blk_size, oss_used());
			while (length > 0)
			{
				cnt = min(length, buffer_size - rd_index);

				oss_write_audio(buffer + rd_index, cnt);
				rd_index = (rd_index + cnt) % buffer_size;
				length -= cnt;
			}
			if (!oss_used())
				ioctl(audio_fd, SNDCTL_DSP_POST, 0);
		}
		else
			usleep(10000);

		if (do_pause && !paused)
		{
			do_pause = FALSE;
			paused = TRUE;
			if (!ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &abuf_info))
			{
				rd_index -= (abuf_info.fragstotal - abuf_info.fragments) * abuf_info.fragsize;
				output_bytes -= (abuf_info.fragstotal - abuf_info.fragments) * abuf_info.fragsize;
			}
			if (rd_index < 0)
				rd_index += buffer_size;
			ioctl(audio_fd, SNDCTL_DSP_RESET, 0);

		}

		if (unpause && paused)
		{
			unpause = FALSE;
			close(audio_fd);
			audio_fd = open(device_name, O_WRONLY);
			oss_set_audio_params();
			paused = FALSE;
		}

		if (flush != -1)
		{
			/*
			 * This close and open is a work around of a bug that exists in some drivers which 
			 * cause the driver to get fucked up by a reset
			 */
			ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
			close(audio_fd);
			audio_fd = open(device_name, O_WRONLY);
			oss_set_audio_params();
			output_time_offset = flush;
			written = (flush / 10) * (input_bps / 100);
			rd_index = wr_index = output_bytes = 0;
			flush = -1;
			prebuffer = TRUE;
		}
	}

	ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
	close(audio_fd);
	free(buffer);
	pthread_exit(NULL);
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

	ebps = efrequency * channels;
	if (oss_format == AFMT_U16_BE || oss_format == AFMT_U16_LE || oss_format == AFMT_S16_BE || oss_format == AFMT_S16_LE)
		ebps *= 2;
}

static int oss_open(AFormat fmt, int rate, int nch)
{
	oss_setup_format(fmt, rate, nch);

	input_format = format;
	input_channels = channels;
	input_frequency = frequency;
	input_bps = bps;

//	realtime = xmms_check_realtime_priority();
	realtime = TRUE;

	if (!realtime)
	{
		buffer_size = (oss_cfg.buffer_size * input_bps) / 1000;
		if (buffer_size < 8192)
			buffer_size = 8192;
		prebuffer_size = (buffer_size * oss_cfg.prebuffer) / 100;
		if (buffer_size - prebuffer_size < 4096)
			prebuffer_size = buffer_size - 4096;

		buffer_size += device_buffer_size;
		buffer = calloc(1, buffer_size);
		if (buffer == NULL)
			return 0;
	}

	if (oss_cfg.audio_device > 0)
		snprintf(device_name, sizeof(device_name), "/dev/dsp%d", oss_cfg.audio_device);
	else
	{
		strncpy(device_name, "/dev/dsp", sizeof(device_name) - 1);
		device_name[sizeof(device_name) - 1] = '\0';
	}

	audio_fd = open(device_name, O_WRONLY);
	if (audio_fd == -1)
	{
		if (buffer)
			free(buffer);
		buffer = NULL;
		return 0;
	}

	oss_set_audio_params();

	flush = -1;
	prebuffer = 1;
	wr_index = rd_index = output_time_offset = written = output_bytes = 0;
	paused = FALSE;
	do_pause = FALSE;
	unpause = FALSE;
	remove_prebuffer = FALSE;
	going = 1;

	if (!realtime)
		pthread_create(&buffer_thread, NULL, oss_loop, NULL);
	return 1;
}


static void scan_devices(const char* type)
{
	FILE* file;
	char buf[256];
	char* tmp2;
	int found = 0;
	int idx = 0;

	printf("%s\n", type);

	file = fopen("/dev/sndstat", "r");
	if (file)
	{
		while (fgets(buf, 255, file))
		{
			if (found && buf[0] == '\n')
				break;
			if (buf[strlen(buf) - 1] == '\n')
				buf[strlen(buf) - 1] = '\0';
			if (found)
			{
				if (idx == 0)
				{
					tmp2 = strchr(buf, ':');
					if (tmp2)
					{
						tmp2++;
						while (*tmp2 == ' ')
							tmp2++;
					}
					else
						tmp2 = buf;

					printf("  %s (default)\n", tmp2);
				}
				else
				{
					printf("  %s\n", buf);
				}
			}
			if (! strcasecmp(buf, type))
				found = 1;
		}
		fclose(file);
	}
}

static void oss_configure(void)
{
	printf("OSS configure not implemented - here are the current devices:\n");
	scan_devices("Audio devices:");
	scan_devices("Mixers:");

	/*
	oss_cfg.audio_device = audio_device;
	oss_cfg.mixer_device = mixer_device;
	oss_cfg.buffer_size = (gint) GTK_ADJUSTMENT(buffer_size_adj)->value;
	oss_cfg.prebuffer = (gint) GTK_ADJUSTMENT(buffer_pre_adj)->value;
	oss_cfg.fragment_count = NFRAGS;
	*/
}

static void oss_init(void)
{
	memset(&oss_cfg, 0, sizeof (OSSConfig));

	oss_cfg.audio_device = 0;
	oss_cfg.mixer_device = 0;
	oss_cfg.buffer_size = 3000;
	oss_cfg.prebuffer = 25;
//	oss_cfg.fragment_count = NFRAGS;
	oss_cfg.fragment_count = 3;
}

static void oss_get_volume(int *l, int *r)
{
	int fd, v, cmd, devs;
	char devname[20];

	if (oss_cfg.mixer_device > 0)
		snprintf(devname, sizeof(devname), "/dev/mixer%d", oss_cfg.mixer_device);
	else
	{
		strncpy(devname, "/dev/mixer", sizeof(devname) - 1);
		devname[sizeof(devname) - 1] = '\0';
	}

	fd = open(devname, O_RDONLY);
	if (fd != -1)
	{
		ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
		if (devs & SOUND_MASK_PCM)
			cmd = SOUND_MIXER_READ_PCM;
		else if (devs & SOUND_MASK_VOLUME)
			cmd = SOUND_MIXER_READ_VOLUME;
		else
		{
			close(fd);
			return;
		}
		ioctl(fd, cmd, &v);
		*r = (v & 0xFF00) >> 8;
		*l = (v & 0x00FF);
		close(fd);
	}
}

static void oss_set_volume(int l, int r)
{
	int fd, v, cmd, devs;
	char devname[20];

	if (oss_cfg.mixer_device > 0)
		snprintf(devname, sizeof(devname), "/dev/mixer%d", oss_cfg.mixer_device);
	else
	{
		strncpy(devname, "/dev/mixer", sizeof(devname) - 1);
		devname[sizeof(devname) - 1] = '\0';
	}

	fd = open(devname, O_RDONLY);
	if (fd != -1)
	{
		ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
		if (devs & SOUND_MASK_PCM)
			cmd = SOUND_MIXER_WRITE_PCM;
		else if (devs & SOUND_MASK_VOLUME)
			cmd = SOUND_MIXER_WRITE_VOLUME;
		else
		{
			close(fd);
			return;
		}
		v = (r << 8) | l;
		ioctl(fd, cmd, &v);
		close(fd);
	}
}


static OutputPlugin oss_op =
{
	NULL,
	NULL,
	"XMMS OSS Driver 0.9.5.1",
	oss_init,
	oss_about,
	oss_configure,
	oss_get_volume,
	oss_set_volume,
	oss_open,
	oss_write,
	oss_close,
	oss_flush,
	oss_pause,
	oss_free,
	oss_playing,
	oss_get_output_time,
	oss_get_written_time,
};

OutputPlugin *get_oplugin_info(void)
{
	return &oss_op;
}


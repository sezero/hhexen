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

#ifndef AUDIO_PLUGIN_H
#define AUDIO_PLUGIN_H


typedef enum
{
	FMT_U8, FMT_S8, FMT_U16_LE, FMT_U16_BE, FMT_U16_NE, FMT_S16_LE, FMT_S16_BE, FMT_S16_NE
}
AFormat;

typedef struct
{
	void *handle;			/* Filled in by xmms */
	char *filename;			/* Filled in by xmms */
	const char *description;	/* The description that is shown in the preferences box */
	void (*init) (void);
	void (*about) (void);		/* Show the about box */
	void (*configure) (void);	/* Show the configuration dialog */
	void (*get_volume) (int *l, int *r);
	void (*set_volume) (int l, int r);	/* Set the volume */

	int (*open_audio) (AFormat fmt, int rate, int nch);
					/* Open the device, if the device can't handle the given 
					   parameters the plugin is responsible for downmixing
					   the data to the right format before outputting it */

	void (*write_audio) (void *ptr, int length);
					/* The input plugin calls this to write data to the output buffer */

	void (*close_audio) (void);	/* No comment... */
	void (*flush) (int flushtime);	/* Flush the buffer and set the plugins internal timers to time */
	void (*pause) (short paused);	/* Pause or unpause the output */
	int (*buffer_free) (void);	/* Return the amount of data that can be written to the buffer,
					   two calls to this without a call to write_audio should make
					   the plugin output audio directly */
	int (*buffer_playing) (void);	/* Returns TRUE if the plugin currently is playing some audio,
					   otherwise return FALSE */
	int (*output_time) (void);	/* Return the current playing time */
	int (*written_time) (void);	/* Return the length of all the data that has been written to
					   the buffer */
}
OutputPlugin;

#endif	/* AUDIO_PLUGIN_H */


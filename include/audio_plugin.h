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


typedef enum {
  FMT_U8,
  FMT_S8,
  FMT_S16
} AFormat;

typedef struct
{
	const char* (*about) (void);	/* description. */

	void (*init) (void);

	int (*open_audio) (AFormat fmt, int rate, int nch);
					/* Open the device, if the device can't handle the given 
					   parameters the plugin is responsible for downmixing
					   the data to the right format before outputting it */

	void (*write_audio) (void *ptr, int length);
					/* The input plugin calls this to write data to the output buffer */

	void (*close_audio) (void);
}
OutputPlugin;

#endif	/* AUDIO_PLUGIN_H */

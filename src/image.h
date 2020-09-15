/**
 * Copyright (c) 2013, Raphaël.Droz + floss @ Gmail dot COM
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Image display support for rxvt-unicode
 * @see doc/images.txt
 *
 **/

/*
   # tested: ok
   - click, mouse select, scrollbar, page-up/down, newlines
   - reset, background

   # TODO:
   - on_resize_term() #bug: term_start goes to 1000 and up
   - update pictures on_clear_screen()
   - don't base coords on row, but on nrow as empty lines can exist after resizing()
   - scrollback flushing after X pictures or X MB
   - drag & drop (gdk || motif)
*/


#pragma once

#include "../config.h"
#include "rxvt.h"

enum image_flags {
	IMG_NOFLAG	= 0,
	IMG_HPAD	= 1,
	IMG_VPAD	= 2,
	IMG_CVPAD	= 4,

	IMG_SCALED	= 8,
	IMG_SIZED	= 16,
	IMG_RATIO	= 32,

	IMG_FLOW	= 64,
	IMG_RPOS	= 128,
	IMG_POS		= 256,
	IMG_EOL		= 512,

	IMG_WRAP	= 1024,
	IMG_FLUSH	= 2048
};

struct rxvt_embed_imge {
	GdkPixbuf *pictures;
	char *filename;
	row_col_t pos;
	unsigned int flags; // image_flags
	unsigned int height, width; // dimension

	rxvt_term *t;

	// (de)initialization
	~rxvt_embed_imge();
	rxvt_embed_imge(rxvt_term *term, char *filename,
	          GdkPixbuf *new_image, unsigned int flags);

	// image coordinates determination and cursor manipulation
	void set_pos(int x, int y);
	void set_cursor();

	// visibility helpers
	int ceil_width();
	int ceil_height();
	int bottom_lineno();
	bool bottom_visible();
	bool bottom_fully_visible();
	bool visible();

	// set whether or not one window area needs refresh (pictures_need_expose)
	// and set pictures_disp_[yhw] values accordingly
	void request_reexpose(int view_start, int new_view_start);
	// attempt to recompute images position after the guide value (term_start)
	// changed (terminal window resizing)
	void recompute_pos(int prev_total_rows);

	// parse the initialization string
	static void pictures_parse_params(const char *string, char * &file,
	                                  unsigned int &flags,
	                                  int &sw, int &sh, int &x, int &y);
	// gdkpixbuf creation happens in command.C soon before the
	// rxvt_embed_imge is actually instanciated.
	static GdkPixbuf* getGdkPixbuf(char *file, unsigned int flags,
	                                          int sw, int sh);

	// debugging purpose. TODO: #ifdef
	static void print_flags(unsigned int flags);
	void print_info();
};

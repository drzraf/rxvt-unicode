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

#include "../config.h"
#include "rxvt.h"


#define IS(x) (flags & (x))
#define SET(x) (flags |= x)
#define UNSET(x) (flags &= ~x)
#define SETUNSET(x, y) ({ SET(x); UNSET(y); })


rxvt_embed_imge::~rxvt_embed_imge() {
	//print_info();

	g_object_unref(pictures);
	free(filename);
	pictures = NULL; filename = NULL;
}

rxvt_embed_imge::rxvt_embed_imge(rxvt_term *term, char *filename,
		     GdkPixbuf *new_image, unsigned int flags)
	: t(term), filename(filename), pictures(new_image), flags(flags)
{
	this->height = gdk_pixbuf_get_height(this->pictures);
	this->width = gdk_pixbuf_get_width(this->pictures);
}

GdkPixbuf* rxvt_embed_imge::getGdkPixbuf(char *file, unsigned int flags, int sw, int sh) {
	// see http://developer.gnome.org/gdk-pixbuf/stable/gdk-pixbuf-File-Loading.html
	if(IS(IMG_SIZED))
		return gdk_pixbuf_new_from_file_at_size (file, sw, sh, NULL);

	if(IS(IMG_SCALED))
		return gdk_pixbuf_new_from_file_at_scale (file, sw, sh, IS(IMG_RATIO), NULL);

	return gdk_pixbuf_new_from_file (file, NULL);
}




// width of an image converted in rows
// => ceil ( Image-width [px] / Font-width [px] )
int rxvt_embed_imge::ceil_width() {
	return 1 + (width - 1) / t->fwidth ;
}

// height of an image converted in rows
// => ceil ( Image-height [px] / Font-height [px] )
int rxvt_embed_imge::ceil_height() {
	return 1 + (height- 1) / t->fheight;
}

int rxvt_embed_imge::bottom_lineno() {
	return pos.row + ceil_height();
}

bool rxvt_embed_imge::bottom_fully_visible() {
	return bottom_lineno() < t->bottom_no();
}

// image bottom row is greater than the upper-bound
// of the visible area of the terminal [called "display" below]
bool rxvt_embed_imge::bottom_visible() {
	return bottom_lineno() >= t->term_start + t->view_start;
}

// true if, at least partly, visible
bool rxvt_embed_imge::visible() {
	return pos.row <= t->bottom_no() && bottom_visible();
}



void rxvt_embed_imge::set_pos(int x, int y) {
	// default
	pos.col = t->screen.cur.col;
	// position since the begining of the scrollback + the current
	// position relative to the visible part.
	pos.row = t->term_start + t->screen.cur.row;

	// align right
	if(IS(IMG_EOL)) {
		pos.col = t->ncol - ceil_width();
	}
	else if(IS(IMG_POS)) {
		pos.col = x;
		pos.row = y;
	}
	else if(IS(IMG_RPOS)) {
		pos.col = t->screen.cur.col + x;
		pos.row = t->term_start + t->view_start + y;
	}
	// default positionning is over width and WRAP is active
	else if(IS(IMG_WRAP) && t->fwidth < t->col2pixel(pos.col) + width) {
		// should trigger a refresh including this picture
		// before our final set_cursor() happens, but that won't
		// hurt
		while (t->pictures_next_vpad--) t->scr_index(UP);
		t->pictures_next_vpad = 0;
		pos.col = 0;
		// set picture as "inlined"... (on a new line)
		SET(IMG_CVPAD|IMG_HPAD); UNSET(IMG_VPAD);
	}

	//print_info();
	//rxvt_embed_imge::print_flags(flags);
}


void rxvt_embed_imge::set_cursor() {
	int newlines = ceil_height();
	// increment the number of NL we will need to add... (later)
	if(IS(IMG_CVPAD))
		t->pictures_next_vpad = max(t->pictures_next_vpad, newlines);

	// HPAD implies no VPAD
	if(IS(IMG_HPAD)) {
		t->scr_gotorc(0, ceil_width(), RELATIVE);
		return;
	}

	if(IS(IMG_VPAD)) {
		// Add newlines
		// This happens before the new picture is actually displayed,
		// so we avoid one more round-trip (display images, add lines, refresh images)
		if(t->pictures_next_vpad) newlines = t->pictures_next_vpad;
		while (newlines--) t->scr_index(UP);
		t->screen.cur.col = 0;
		t->pictures_next_vpad = 0;
	}
}

void rxvt_embed_imge::request_reexpose(int old_view_start, int new_view_start) {
	if(! visible()) return;

	// scrolling up: top lines displayed risk garbling
	if(new_view_start < old_view_start) {
		unsigned int curstart = t->row2pixel(pos.row - t->top_no());
		unsigned int nextstart = t->row2pixel(pos.row - ( t->term_start + new_view_start ) );

		t->pictures_need_expose = 1;

		t->pictures_disp_y = 0;
		// useless, because rxvt_term::scr_expose() use FAST_REFRESH
		/* if(!pictures_disp_y) pictures_disp_y = curstart;
		   else pictures_disp_y = min(pictures_disp_y, curstart);*/


		// t->pictures_disp_h = max(t->pictures_disp_h, t->row2pixel(old_view_start - new_view_start));
		t->pictures_disp_h = t->height;
	}

	// scrolling down, pict is pushed up, text appears below and some text
	// now covers the pixels that used to be from an image.
	// This is the part that needs refresh.
	// note: t->height == window's height in pixels
	else if(new_view_start > old_view_start) {
		unsigned int curend = t->row2pixel(pos.row - t->top_no()) + height;
		unsigned int nextend = t->row2pixel(pos.row - ( t->term_start + new_view_start ) ) + height;

		// even after scrolling down, picture bottom continues is even
		// more below the visible area of the terminal window. No bottom
		// text line needs refresh
		if(nextend > t->height) {
			return;
		}

		t->pictures_need_expose = 1;
		if(! bottom_fully_visible()) {
			t->pictures_disp_y = min(t->pictures_disp_y, min(nextend, t->height));
			t->pictures_disp_h = max(t->pictures_disp_h, t->height - t->pictures_disp_y);
		}

		/*
		  because we probably added 1 more row than what height
		  exactly represents
		  and because the last row covered by the pict should not drag garbage,
		  we round it back thus the "1" more line
		*/
		else {
			t->pictures_disp_y = max(0, nextend - t->row2pixel(1));
			t->pictures_disp_h = max(t->pictures_disp_h, curend - nextend);
		}
	}
}


void rxvt_embed_imge::recompute_pos(int prev_total_rows) {
	// recompute when column number changes
	if(IS(IMG_WRAP)) {
		if(IS(IMG_EOL)) {
			// already fit but could move to the right
			if(t->fwidth < t->col2pixel(pos.col) + width &&
			   pos.col != t->ncol - ceil_width()) {
				pos.col = t->ncol - ceil_width();
				t->pictures_need_expose = 2;
			}
			// TODO:
			// if pos.col > t->ncol - ceil_width(), we should wrap properly
			// and pictures_need_expose is not needed
		}
		else {
			// TODO
		}
	}

	// printf("[prev image pos] = %d\n", pos.row);

	// circular buffer already initialized, term_start didn't jump
	if(t->term_start_jam) {
		pos.row += (t->total_rows - prev_total_rows);
	}
	else {
		int prev_index_from_bottom = t->prev_nrow - pos.row - t->top_row;
		pos.row = t->total_rows - prev_index_from_bottom - (t->nrow - t->prev_nrow);
	}

	// printf("[cur image pos] = %d\n", pos.row);
}




/*
  sw and sh are the requested width and height to scale too, in case the
  "scale" or "size" parameter was passed.
  The "ratio" flag ask for ratio perservation:
  @see doc/images.txt
*/
void rxvt_embed_imge::pictures_parse_params(const char *string,
				      char * &file,
				      unsigned int &flags,
				      int &sw, int &sh, int &x, int &y) {

	char *opt, *val, *sopt, *sval;

	flags = IMG_HPAD|IMG_CVPAD|IMG_VPAD;
	sw = 0, sh = -2, x = 0, y = 0;
	file = strtok_r(const_cast<char *>(string), ";", &sopt);

	while((opt = strtok_r(NULL, ";", &sopt))) {
		if(!strncmp(opt, "pad", 3)) {
			val = strtok_r(opt, ":", &sval);
			while( (val = strtok_r(NULL, ":", &sval)) ) {
				if(!strcmp(val, "horiz")) SET(IMG_HPAD);
				else if(!strcmp(val, "vert")) SET(IMG_VPAD);
				else if(!strcmp(val, "nohoriz")) UNSET( (IMG_HPAD|IMG_CVPAD) );
				else if(!strcmp(val, "novert")) UNSET(IMG_VPAD);
				else if(!strcmp(val, "nocum")) UNSET(IMG_CVPAD);
			}
		}

		else if(!strncmp(opt, "dim", 3)) {
			val = strtok_r(opt, ":", &sval);
			while( (val = strtok_r(NULL, ":", &sval)) ) {
				if(!strcmp(val, "scale")) SETUNSET(IMG_SCALED, IMG_SIZED);
				else if(!strcmp(val, "size")) SETUNSET(IMG_SIZED, IMG_SCALED);
				else if(!strcmp(val, "ratio")) SET(IMG_RATIO);
				else if( sscanf(val, "%dx%d", &sw, &sh) != 2 ) {
					fprintf(stderr, "can't parse a \"size\" parameter\n");
					return;
				}
			}
		}

		else if(!strncmp(opt, "pos", 3)) {
			val = strtok_r(opt, ":", &sval);
			while( (val = strtok_r(NULL, ":", &sval)) ) {
				if(!strcmp(val, "rel")) SET(IMG_RPOS);
				else if(!strcmp(val, "abs")) SET(IMG_POS);
				else if(!strcmp(val, "eol")) SET(IMG_EOL);
				else if( sscanf(val, "%dx%d", &x, &y) != 2 ) {
					fprintf(stderr, "can't parse a \"position\" parameter\n");
					return;
				}
			}
		}

		else if(!strcmp(opt, "wrap")) {
			SET(IMG_WRAP);
		}

		else if(!strcmp(opt, "flush")) {
			SET(IMG_FLUSH);
		}
	}

	//rxvt_embed_imge::print_flags(flags);

	// default positionning when only coords were given:
	// x < 0 || y < 0 => relative requested, absolute otherwise
	if(!IS(IMG_POS) && !IS(IMG_RPOS) && (x|y)) SET( min(x, y) < 0 ? IMG_RPOS : IMG_POS );
	// scaling or sizing makes no sense without dimensions or if both are negative
	if(min(sh, sw) < -1) UNSET( (IMG_SIZED|IMG_SCALED) ); // will unset both
	// size without other precision => default to "scale" if one dim == -1, "size" otherwise
	else if(! IS(IMG_SIZED) && !IS(IMG_SCALED)) { SET( (sw < 0 || sh < 0) ? IMG_SCALED : IMG_SIZED ); }

	// simple end-of-line takes priority over specific (rel or abs) position
	if(IS(IMG_EOL)) UNSET( (IMG_POS|IMG_RPOS) );
	// IMG_RATIO is only a handul internal flag
	if(IS(IMG_SCALED) && (sw == -1 ^ sh == -1)) SET(IMG_RATIO);
	// if precise position is requested, padding & co are up to the user
	if(IS(IMG_POS) || IS(IMG_RPOS)) UNSET( (IMG_HPAD|IMG_VPAD|IMG_CVPAD) );
	// if we pad with newlines, tabulations are senseless
	if(IS(IMG_VPAD)) UNSET(IMG_HPAD);

	//rxvt_embed_imge::print_flags(flags);
}

void rxvt_embed_imge::print_flags(unsigned int flags) {
	fprintf(stderr,
	        "pad\t"  "pos\t" "eol\t" "dim\n"
	        "%s%s\t" "%s\t"  "%s\t"  "%s%s\n",
	        IS(IMG_VPAD) ? "ve" : IS(IMG_HPAD) ? "ho" : "",
	        IS(IMG_CVPAD) ? "|c" : "",

	        IS(IMG_RPOS) ? "rel" : IS(IMG_POS) ? "abs" :
	        IS(IMG_EOL) ? "eol" : "flow",

	        IS(IMG_WRAP) ? "wrap" : "fix",

	        IS(IMG_SCALED) ? "scale" : IS(IMG_SIZED) ? "size" : "orig",
	        IS(IMG_RATIO) ? " (%)" : ""
	        );
}

void rxvt_embed_imge::print_info() {
	fprintf(stderr,
		"filename: %s, flags = %d, (%dx%d @ %dx%d)\n",
		filename, flags, width, height, pos.col, pos.row);
}

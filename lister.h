#ifndef lister_h__
#define lister_h__
/*
 * lister.h - list things.
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
 * @(#)lister.h 1.1
 *
 */

typedef struct array ARRAY;

struct array {
    void *data;
};

int display_list_cl(ARRAY * list, const char *(*fcn) (void *), xformat fmt, int count, int longest);
int display_list(ARRAY * list, const char *(*fcn) (void *), xformat fmt);
int display_list_c(ARRAY * list, const char *(*fcn) (void *), xformat fmt, int count);
int display_list_l(ARRAY * list, const char *(*fcn) (void *), xformat fmt, int longest);

#endif				/* lister_h__ */

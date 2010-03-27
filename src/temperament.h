/* vim: set sts=2 sw=2 et: */
/*
 * Copyright (C) 2010 Jari Tenhunen <jari.tenhunen@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __TEMPERAMENT_H__
#define __TEMPERAMENT_H__

#include <gtk/gtk.h>

typedef struct
{
  const gchar *name;
  gfloat frequency;
} Note;

enum
{
  NUM_NOTES = 96
};

gfloat interval2cent (gfloat freq, gfloat note);
void recalculate_scale (double a4);
Note * get_scale (void);
Note * get_nearest_note (gfloat frequency);

#endif /* __TEMPERAMENT_H__ */

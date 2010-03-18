/* vim: set sts=2 sw=2 et: */
/*
 * Copyright (C) 2008 Jari Tenhunen <jari.tenhunen@iki.fi>
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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <gtk/gtkwindow.h>
#include <gconf/gconf-client.h>

#define GCONF_ROOT "/apps/tuner"
#define GCONF_KEY_ALGORITHM GCONF_ROOT "/algorithm"
#define GCONF_KEY_CALIBRATION GCONF_ROOT "/calibration"
#define GCONF_KEY_DISPLAY_KEEPALIVE GCONF_ROOT "/display_keepalive"

#define DEFAULT_ALGORITHM (0) /* GST_PITCH_ALGORITHM_FFT */
#define DEFAULT_DISPLAY_KEEPALIVE TRUE

enum
{
  CALIB_MIN = 430,
  CALIB_MAX = 450,
  CALIB_DEFAULT = 440
};

gint settings_get_algorithm (gint default_value);
gint settings_get_calibration (gint default_value);
gboolean settings_get_display_keepalive (gboolean default_value);

gboolean settings_set_algorithm (gint value);
gboolean settings_set_calibration (gint value);
gboolean settings_set_display_keepalive (gboolean val);
gboolean settings_init (GConfClientNotifyFunc func, gpointer user_data);
#if HILDON == 1
void settings_dialog_show (GtkWindow * parent);
GtkWidget * calibration_editor_new (gint min, gint max);
#endif

#endif /* __SETTINGS_H__ */


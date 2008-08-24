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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "settings.h"

static GConfClient * client = NULL;

static GConfClient *
get_client ()
{
  if (!client)
    client = gconf_client_get_default ();

  return client;

}

gboolean
settings_get_display_keepalive (gboolean default_value)
{
  GConfValue * value = NULL;
  GError * err = NULL;
  gboolean val = default_value;

  value = gconf_client_get (get_client (), GCONF_KEY_DISPLAY_KEEPALIVE, &err);
  if (err) {
    g_error_free (err);
  }

  if (value) {
    if (value->type == GCONF_VALUE_BOOL)
      val = gconf_value_get_bool (value);

    gconf_value_free (value);
  }

  return val;
}

gboolean
settings_set_disp_keepalive (gboolean val)
{
  return gconf_client_set_bool (get_client (), GCONF_KEY_DISPLAY_KEEPALIVE, val, NULL);
}

gint
settings_get_algorithm (gint default_value)
{
  GConfValue * value = NULL;
  GError * err = NULL;
  gint val = default_value;

  value = gconf_client_get (get_client (), GCONF_KEY_ALGORITHM, &err);
  if (err) {
    g_error_free (err);
  }

  if (value) {
    if (value->type == GCONF_VALUE_INT)
      val = gconf_value_get_int (value);

    gconf_value_free (value);
  }

  return val;
}

gboolean
settings_set_algorithm (gint val)
{
  return gconf_client_set_int (get_client (), GCONF_KEY_ALGORITHM, val, NULL);
}

gint
settings_get_calibration (gint default_value)
{
  GError * err = NULL;
  gint val;

  val = gconf_client_get_int (get_client (), GCONF_KEY_CALIBRATION, &err);
  if (err) {
    val = default_value;
    g_error_free (err);
  }
  if (val == 0)
    val = default_value;

  return val;
}

gboolean
settings_set_calibration (gint value)
{
  return gconf_client_set_int (get_client (), GCONF_KEY_CALIBRATION, value, NULL);
}

gboolean
settings_init (GConfClientNotifyFunc func, gpointer user_data)
{
  gconf_client_add_dir (get_client (), GCONF_ROOT, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
  gconf_client_notify_add (get_client (), GCONF_ROOT, func, user_data, NULL, NULL);

  return TRUE;
}

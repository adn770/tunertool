/* vim: set sts=2 sw=2 et: */
/* 
 * Copyright (C) 2008-2009 Jari Tenhunen <jari.tenhunen@iki.fi>
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

#include <gtk/gtk.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-number-editor.h>
#include <hildon/hildon.h>

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
settings_set_display_keepalive (gboolean val)
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

static void
fix_hildon_number_editor (GtkWidget * widget, gpointer data)
{
  if (GTK_IS_EDITABLE (widget)) {
    gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
    g_object_set (G_OBJECT (widget), "can-focus", FALSE, NULL);
  }
}

GtkWidget *
calibration_editor_new (gint min, gint max)
{
  GtkWidget *control;
 
  control = hildon_number_editor_new (min, max);
  /* we don't want that ugly cursor there */
  gtk_container_forall (GTK_CONTAINER (control),
      (GtkCallback) fix_hildon_number_editor, NULL);

  return control;
}

void
settings_dialog_show (GtkWindow * parent)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *editor = NULL;
  GtkWidget *control;
  gint res;

  dialog = gtk_dialog_new_with_buttons("Settings",
      parent,
      GTK_DIALOG_MODAL | 
      GTK_DIALOG_DESTROY_WITH_PARENT | 
      GTK_DIALOG_NO_SEPARATOR,
      "Save", GTK_RESPONSE_OK,
      "Cancel",
      GTK_RESPONSE_CANCEL,
      NULL, NULL);

  g_signal_connect (G_OBJECT (dialog), "delete_event", G_CALLBACK (gtk_widget_destroy), NULL);

  vbox = gtk_vbox_new (FALSE, HILDON_MARGIN_DEFAULT);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), HILDON_MARGIN_DEFAULT);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);

  HildonPickerButton *picker = HILDON_PICKER_BUTTON (hildon_picker_button_new (
        HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_HALFSCREEN_WIDTH, 
        HILDON_BUTTON_ARRANGEMENT_VERTICAL));
  hildon_button_set_title (HILDON_BUTTON (picker), "Pitch detection algorithm");
  HildonTouchSelector *selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
  hildon_picker_button_set_selector (picker, selector);
  hildon_touch_selector_append_text (selector, "Simple FFT");
  hildon_touch_selector_append_text (selector, "Harmonic Product Spectrum");
  hildon_picker_button_set_active (picker, settings_get_algorithm (DEFAULT_ALGORITHM));
  
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (picker), FALSE, FALSE, 0);
#if 0
  editor = calibration_editor_new (CALIB_MIN, CALIB_MAX);
  hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (editor), CALIB_DEFAULT);
  caption = hildon_caption_new (group, "Calibration:",
      editor, NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, 0);
#endif

  control = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_HALFSCREEN_WIDTH | HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  gtk_button_set_label (GTK_BUTTON (control), "Keep display on");
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (control), 
      settings_get_display_keepalive (DEFAULT_DISPLAY_KEEPALIVE));
  gtk_box_pack_start (GTK_BOX (vbox), control, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog);
  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res == GTK_RESPONSE_OK) {
    /* save settings */
    g_debug ("algorithm: %d", hildon_picker_button_get_active (picker));
    settings_set_algorithm (hildon_picker_button_get_active (picker));
    if (editor) {
      g_debug ("calib: %d", hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (editor)));
      settings_set_calibration (hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (editor)));
    }
    g_debug ("keepalive: %d", hildon_check_button_get_active (HILDON_CHECK_BUTTON (control)));
    settings_set_display_keepalive (hildon_check_button_get_active (HILDON_CHECK_BUTTON (control)));
  }

  gtk_widget_destroy (dialog);
}


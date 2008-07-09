/* GStreamer
 * Copyright (C) 2006 Josep Torra <j.torra@telefonica.net>
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

#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

#define DEFAULT_AUDIOSINK "alsasink"

GtkWidget *lblFrequency;

static void
on_window_destroy (GtkObject * object, gpointer user_data)
{
  gtk_main_quit ();
}

/* control audiotestsrc frequency */
static void
on_frequency_changed (GtkRange * range, gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  gdouble value = gtk_range_get_value (range);

  g_object_set (machine, "freq", value, NULL);
}

/* control audiotestsrc frequency */
static void
update_frequency (gint frequency)
{
  gchar *buffer;
  gchar freq[5];

  g_snprintf (freq, 5, "%d", frequency);
  buffer = g_strconcat ("Frequency is ", freq, NULL);
  gtk_label_set_text (GTK_LABEL (lblFrequency), buffer);
  g_free (buffer);
}

/* receive spectral data from element message */
gboolean
message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);

    if (strcmp (name, "kissfft") == 0) {
      gint frequency;

      frequency = g_value_get_int (gst_structure_get_value (s, "frequency"));
      update_frequency (frequency);
    }
  }
  /* we handled the message we want, and ignored the ones we didn't want.
   * so the core can unref the message for us */
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GstElement *bin;
  GstElement *src, *kissfft, *sink;
  GstBus *bus;
  GtkWidget *appwindow, *vbox, *widget;

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  bin = gst_pipeline_new ("bin");

  src = gst_element_factory_make ("audiotestsrc", "src");

  kissfft = gst_element_factory_make ("kissfft", "kissfft");
  g_object_set (G_OBJECT (kissfft), "nfft", 1024, "message", TRUE, "minfreq",
      220, "maxfreq", 1500, NULL);

  sink = gst_element_factory_make (DEFAULT_AUDIOSINK, "sink");

  gst_bin_add_many (GST_BIN (bin), src, kissfft, sink, NULL);
  if (!gst_element_link_many (src, kissfft, sink, NULL)) {
    fprintf (stderr, "cant link elements\n");
    exit (1);
  }

  bus = gst_element_get_bus (bin);
  gst_bus_add_watch (bus, message_handler, NULL);
  gst_object_unref (bus);

  appwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (appwindow), "destroy",
      G_CALLBACK (on_window_destroy), NULL);
  vbox = gtk_vbox_new (FALSE, 6);

  widget = gtk_hscale_new_with_range (50.0, 2000.0, 10);
  gtk_widget_set_size_request (widget, 300, -1);
  gtk_scale_set_draw_value (GTK_SCALE (widget), TRUE);
  gtk_scale_set_value_pos (GTK_SCALE (widget), GTK_POS_TOP);
  gtk_range_set_value (GTK_RANGE (widget), 440.0);
  g_signal_connect (G_OBJECT (widget), "value-changed",
      G_CALLBACK (on_frequency_changed), (gpointer) src);
  gtk_container_add (GTK_CONTAINER (vbox), widget);

  lblFrequency = gtk_label_new ("Frequency is ");
  gtk_container_add (GTK_CONTAINER (vbox), lblFrequency);

  gtk_container_add (GTK_CONTAINER (appwindow), vbox);
  gtk_widget_show_all (appwindow);

  gst_element_set_state (bin, GST_STATE_PLAYING);
  gtk_main ();
  gst_element_set_state (bin, GST_STATE_NULL);

  gst_object_unref (bin);

  return 0;
}

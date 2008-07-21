/* vim: set sts=2 sw=2 et: */
/* Tuner
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

#define TUNER_VERSION "0.4"

#ifdef HILDON
#  if HILDON==1
#    include <hildon/hildon-program.h>
#    include <hildon/hildon-number-editor.h>
#  elif defined(MAEMO1)
#    include <hildon-widgets/hildon-app.h>
#    include <hildon-widgets/hildon-appview.h>
#  else
#    include <hildon-widgets/hildon-program.h>
#  endif

#include <libosso.h>

#define OSSO_PACKAGE "tuner-tool"
#define OSSO_VERSION TUNER_VERSION

#endif /* ifdef HILDON */

#ifdef MAEMO
# define DEFAULT_AUDIOSRC "dsppcmsrc"
# define DEFAULT_AUDIOSINK "dsppcmsink"
#else
# define DEFAULT_AUDIOSRC "alsasrc"
# define DEFAULT_AUDIOSINK "alsasink"
#endif


#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

#define between(x,a,b) (((x)>=(a)) && ((x)<=(b)))

#define MAGIC (1.059463094359f) /* 2^(1/2) */

extern gboolean plugin_pitch_init (GstPlugin * plugin);
extern gboolean plugin_tonesrc_init (GstPlugin * plugin);

typedef struct
{
  const gchar *name;
  gfloat frequency;
} Note;

enum
{
  NUM_NOTES = 96
};

enum
{
  CALIB_MIN = 430,
  CALIB_MAX = 450,
  CALIB_DEFAULT = 440
};

#define NUM_LEDS (66)

static Note equal_tempered_scale[] = {
  {"C0", 16.35},
  {"C#0/Db0", 17.32},
  {"D0", 18.35},
  {"D#0/Eb0", 19.45},
  {"E0", 20.60},
  {"F0", 21.83},
  {"F#0/Gb0", 23.12},
  {"G0", 24.50},
  {"G#0/Ab0", 25.96},
  {"A0", 27.50},
  {"A#0/Bb0", 29.14},
  {"B0", 30.87},
  {"C1", 32.70},
  {"C#1/Db1", 34.65},
  {"D1", 36.71},
  {"D#1/Eb1", 38.89},
  {"E1", 41.20},
  {"F1", 43.65},
  {"F#1/Gb1", 46.25},
  {"G1", 49.00},
  {"G#1/Ab1", 51.91},
  {"A1", 55.00},
  {"A#1/Bb1", 58.27},
  {"B1", 61.74},
  {"C2", 65.41},
  {"C#2/Db2", 69.30},
  {"D2", 73.42},
  {"D#2/Eb2", 77.78},
  {"E2", 82.41},
  {"F2", 87.31},
  {"F#2/Gb2", 92.50},
  {"G2", 98.00},
  {"G#2/Ab2", 103.83},
  {"A2", 110.00},
  {"A#2/Bb2", 116.54},
  {"B2", 123.47},
  {"C3", 130.81},
  {"C#3/Db3", 138.59},
  {"D3", 146.83},
  {"D#3/Eb3", 155.56},
  {"E3", 164.81},
  {"F3", 174.61},
  {"F#3/Gb3", 185.00},
  {"G3", 196.00},
  {"G#3/Ab3", 207.65},
  {"A3", 220.00},
  {"A#3/Bb3", 233.08},
  {"B3", 246.94},
  {"C4", 261.63},
  {"C#4/Db4", 277.18},
  {"D4", 293.66},
  {"D#4/Eb4", 311.13},
  {"E4", 329.63},
  {"F4", 349.23},
  {"F#4/Gb4", 369.99},
  {"G4", 392.00},
  {"G#4/Ab4", 415.30},
  {"A4", 440.00},
  {"A#4/Bb4", 466.16},
  {"B4", 493.88},
  {"C5", 523.25},
  {"C#5/Db5", 554.37},
  {"D5", 587.33},
  {"D#5/Eb5", 622.25},
  {"E5", 659.26},
  {"F5", 698.46},
  {"F#5/Gb5", 739.99},
  {"G5", 783.99},
  {"G#5/Ab5", 830.61},
  {"A5", 880.00},
  {"A#5/Bb5", 932.33},
  {"B5", 987.77},
  {"C6", 1046.50},
  {"C#6/Db6", 1108.73},
  {"D6", 1174.66},
  {"D#6/Eb6", 1244.51},
  {"E6", 1318.51},
  {"F6", 1396.91},
  {"F#6/Gb6", 1479.98},
  {"G6", 1567.98},
  {"G#6/Ab6", 1661.22},
  {"A6", 1760.00},
  {"A#6/Bb6", 1864.66},
  {"B6", 1975.53},
  {"C7", 2093.00},
  {"C#7/Db7", 2217.46},
  {"D7", 2349.32},
  {"D#7/Eb7", 2489.02},
  {"E7", 2637.02},
  {"F7", 2793.83},
  {"F#7/Gb7", 2959.96},
  {"G7", 3135.96},
  {"G#7/Ab7", 3322.44},
  {"A7", 3520.00},
  {"A#7/Bb7", 3729.31},
  {"B7", 3951.07},
};

static GdkColor ledOnColor = { 0, 0 * 255, 180 * 255, 95 * 255 };
static GdkColor ledOnColor2 = { 0, 180 * 255, 180 * 255, 0 * 255 };
static GdkColor ledOffColor = { 0, 80 * 255, 80 * 255, 80 * 255 };

GtkWidget *mainWin;
GtkWidget *targetFrequency;
GtkWidget *currentFrequency;
GtkWidget *drawingarea1;
GtkWidget *drawingarea2;

static void
recalculate_scale (double a4)
{
  int i;

  for (i = 0; i < NUM_NOTES; i++) {
    equal_tempered_scale[i].frequency = a4 * pow (MAGIC, i - 57);
    /* fprintf(stdout, "%s: %.2f\n", equal_tempered_scale[i].name, equal_tempered_scale[i].frequency); */
  }
}

#ifdef HILDON
static void
fix_hildon_number_editor (GtkWidget * widget, gpointer data)
{
  if (GTK_IS_EDITABLE (widget)) {
    gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
    g_object_set (G_OBJECT (widget), "can-focus", FALSE, NULL);
  }
}
#endif

static void
calibration_changed (GObject * object, GParamSpec * pspec, gpointer user_data)
{
  gint value;

#ifdef HILDON
  value = hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (object));
#else
  value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (object));
#endif

  if (value >= CALIB_MIN && value <= CALIB_MAX) {
    recalculate_scale (value);
    g_debug ("Calibration changed to %d Hz", value);
  }
}

static void
on_window_destroy (GtkObject * object, gpointer user_data)
{
  gtk_main_quit ();
}

static void
draw_leds (gint n)
{
  gint i, j, k;
  static GdkGC *gc = NULL;

  if (!gc) {
    gc = gdk_gc_new (drawingarea1->window);
  }
  gdk_gc_set_rgb_fg_color (gc, &drawingarea1->style->fg[0]);

  gdk_draw_rectangle (drawingarea1->window, gc, TRUE, 0, 0,
      drawingarea1->allocation.width, drawingarea1->allocation.height);

  if (abs (n) > (NUM_LEDS / 2))
    n = n / n * (NUM_LEDS / 2);

  if (n > 0) {
    j = NUM_LEDS / 2 + 1;
    k = NUM_LEDS / 2 + n;
  } else {
    j = NUM_LEDS / 2 + n;
    k = NUM_LEDS / 2 - 1;
  }

  // Draw all leds
  for (i = 0; i < NUM_LEDS; i++) {
    if (i == NUM_LEDS / 2) {
      if (n == 0)
        gdk_gc_set_rgb_fg_color (gc, &ledOnColor2);
      else
        gdk_gc_set_rgb_fg_color (gc, &ledOffColor);

      gdk_draw_rectangle (drawingarea1->window, gc, TRUE, (i * 10) + 8, 2, 4,
          36);
    } else {
      if ((i >= j) && (i <= k))
        gdk_gc_set_rgb_fg_color (gc, &ledOnColor);
      else
        gdk_gc_set_rgb_fg_color (gc, &ledOffColor);

      gdk_draw_rectangle (drawingarea1->window, gc, TRUE, (i * 10) + 6, 10, 8,
          20);
    }
  }
}

/* update frequency info */
static void
update_frequency (gint frequency)
{
  gchar *buffer;
  gint i, j;
  gfloat diff, min_diff;

  min_diff = fabs (frequency - (equal_tempered_scale[0].frequency - 10));
  for (i = j = 0; i < NUM_NOTES; i++) {
    diff = fabs (frequency - equal_tempered_scale[i].frequency);
    if (diff <= min_diff) {
      min_diff = diff;
      j = i;
    } else {
      break;
    }
  }

  buffer =
      g_strdup_printf ("Nearest note is %s with %.2f Hz frequency",
      equal_tempered_scale[j].name, equal_tempered_scale[j].frequency);
  gtk_label_set_text (GTK_LABEL (targetFrequency), buffer);
  g_free (buffer);

  buffer = g_strdup_printf ("Played frequency is %d Hz", frequency);
  gtk_label_set_text (GTK_LABEL (currentFrequency), buffer);
  g_free (buffer);

  draw_leds ((gint) roundf (min_diff));
}

/* receive spectral data from element message */
gboolean
message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);

    if (strcmp (name, "pitch") == 0) {
      gint frequency;

      frequency = g_value_get_int (gst_structure_get_value (s, "frequency"));
      update_frequency (frequency);
    }
  }
  /* we handled the message we want, and ignored the ones we didn't want.
   * so the core can unref the message for us */
  return TRUE;
}

gfloat
keynote2freq (gint x, gint y)
{
  gint i, j, height, found;
  gfloat frequency = 0;

  height = drawingarea2->allocation.height;

  j = 0;
  found = 0;
  for (i = 0; i < 15; i++) {
    // Test for a white key  
    j++;
    if (between (x, i * 45, i * 45 + 44) && between (y, 0, height))
      found = j;
    // Test for a black key
    if (((i % 7) != 2) && ((i % 7) != 6) && (i != 14)) {
      j++;
      if (between (x, 24 + i * 45, 24 + i * 45 + 42)
          && between (y, 0, height / 2))
        found = j;
    }
    if (found) {
      frequency = equal_tempered_scale[48 + found - 1].frequency;
      break;
    }
  }
  return frequency;
}

static gboolean
expose_event (GtkWidget * widget, GdkEventExpose * event)
{
  gint i;
  static GdkGC *gc = NULL;

  if (!gc) {
    gc = gdk_gc_new (drawingarea2->window);
  }
  gdk_gc_set_rgb_fg_color (gc, &drawingarea2->style->fg[0]);

  gdk_draw_rectangle (drawingarea2->window, gc, FALSE, 0, 0,
      drawingarea2->allocation.width - 1, drawingarea2->allocation.height - 1);

  for (i = 0; i < 14; i++)
    gdk_draw_rectangle (drawingarea2->window, gc, FALSE, i * 45, 0,
        45, drawingarea2->allocation.height - 1);

  for (i = 0; i < 14; i++) {
    if (((i % 7) != 2) && ((i % 7) != 6))
      gdk_draw_rectangle (drawingarea2->window, gc, TRUE, 24 + i * 45, 0,
          42, drawingarea2->allocation.height / 2);
  }
  return FALSE;
}

static gboolean
key_press_event (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  GstElement *piano = GST_ELEMENT (user_data);

  if (event->button == 1) {
    g_object_set (piano, "freq", (gdouble) keynote2freq (event->x, event->y),
        "volume", 0.8, NULL);
  }

  return TRUE;
}

static gboolean
key_release_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  GstElement *piano = GST_ELEMENT (user_data);

  if (event->button == 1) {
    g_object_set (piano, "volume", 0.0, NULL);
  }

  return TRUE;
}

int
main (int argc, char *argv[])
{
#ifdef HILDON
#if defined(MAEMO1)
  HildonApp *app = NULL;
  HildonAppView *view = NULL;
#else
  HildonProgram *app = NULL;
  HildonWindow *view = NULL;
#endif
  osso_context_t *osso_context = NULL;  /* handle to osso */
#endif

  GstElement *bin1, *bin2;
  GstElement *src1, *pitch, *sink1;
  GstElement *src2, *sink2;
  GstBus *bus;

  GtkWidget *mainBox;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *alignment;
  GtkWidget *calibrate;
  GtkWidget *sep;

#ifndef HILDON
  GdkPixbuf *icon = NULL;
  GError *error = NULL;
#endif
  gboolean piano_enabled = TRUE;

  /* Init GStreamer */
  gst_init (&argc, &argv);
  /* Register the GStreamer plugins */
  plugin_pitch_init (NULL);
  plugin_tonesrc_init (NULL);

  recalculate_scale (CALIB_DEFAULT);

  /* Init the gtk - must be called before any hildon stuff */
  gtk_init (&argc, &argv);

#ifdef HILDON
#if defined(MAEMO1)
  /* Create the hildon application and setup the title */
  app = HILDON_APP (hildon_app_new ());
  hildon_app_set_title (app, "Tuner Tool");
  hildon_app_set_two_part_title (app, TRUE);
#else
  app = HILDON_PROGRAM (hildon_program_get_instance ());
  g_set_application_name ("Tuner Tool");
#endif

  /* Initialize maemo application */
  osso_context = osso_initialize (OSSO_PACKAGE, OSSO_VERSION, TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL) {
    g_print ("Bummer, osso failed\n");
  }
  g_assert (osso_context);

  mainBox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (mainBox), 0);
#if defined(MAEMO1)
  view = HILDON_APPVIEW (hildon_appview_new ("Tuner"));
  hildon_appview_set_fullscreen_key_allowed (view, TRUE);
  mainWin = GTK_WIDGET (app);
#else
  view = HILDON_WINDOW (hildon_window_new ());
  mainWin = GTK_WIDGET (view);
#endif
#else
  mainWin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (mainWin), "Tuner " TUNER_VERSION);
  icon = gdk_pixbuf_new_from_file ("tuner64.png", &error);
  if (icon != NULL) {
    g_print ("Setting icon\n");
    gtk_window_set_icon (GTK_WINDOW (mainWin), icon);
  }
  mainBox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (mainBox), 0);
#endif

  /* Bin for tuner functionality */
  bin1 = gst_pipeline_new ("bin1");

  src1 = gst_element_factory_make (DEFAULT_AUDIOSRC, "src1");
  pitch = gst_element_factory_make ("pitch", "pitch");
  g_object_set (G_OBJECT (pitch), "nfft", 8000, "message", TRUE, "minfreq", 10,
      "maxfreq", 4000, "interval", GST_SECOND, NULL);

  sink1 = gst_element_factory_make ("fakesink", "sink1");
  g_object_set (G_OBJECT (sink1), "silent", 1, NULL);

  gst_bin_add_many (GST_BIN (bin1), src1, pitch, sink1, NULL);
  if (!gst_element_link_many (src1, pitch, sink1, NULL)) {
    fprintf (stderr, "cant link elements\n");
    exit (1);
  }

  bus = gst_element_get_bus (bin1);
  gst_bus_add_watch (bus, message_handler, NULL);
  gst_object_unref (bus);

  /* Bin for piano functionality */
  bin2 = gst_pipeline_new ("bin2");

  //src2 = gst_element_factory_make ("audiotestsrc", "src2");
  //g_object_set (G_OBJECT (src2), "volume", 0.0, "wave", 7, NULL);
  src2 = gst_element_factory_make ("tonesrc", "src2");
  g_object_set (G_OBJECT (src2), "volume", 0.0, NULL);
  sink2 = gst_element_factory_make (DEFAULT_AUDIOSINK, "sink2");

  gst_bin_add_many (GST_BIN (bin2), src2, sink2, NULL);
  if (!gst_element_link_many (src2, sink2, NULL)) {
    piano_enabled = FALSE;
  }

  /* GUI */
  g_signal_connect (G_OBJECT (mainWin), "destroy",
      G_CALLBACK (on_window_destroy), NULL);

  /* Note label */
  targetFrequency = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (mainBox), targetFrequency, FALSE, FALSE, 5);

  /* Leds */
  drawingarea1 = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawingarea1, 636, 40);
  gtk_box_pack_start (GTK_BOX (mainBox), drawingarea1, FALSE, FALSE, 5);

  /* Current frequency lable */
  currentFrequency = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (mainBox), currentFrequency, FALSE, FALSE, 5);

  /* Calibration spinner */
  box = gtk_hbox_new (FALSE, 0);
  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  label = gtk_label_new ("Calibration");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 5);

#ifdef HILDON
  calibrate = hildon_number_editor_new (CALIB_MIN, CALIB_MAX);
  hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (calibrate),
      CALIB_DEFAULT);
  /* we don't want that ugly cursor there */
  gtk_container_forall (GTK_CONTAINER (calibrate),
      (GtkCallback) fix_hildon_number_editor, NULL);
  g_signal_connect (G_OBJECT (calibrate), "notify::value",
      G_CALLBACK (calibration_changed), NULL);
#else
  calibrate = gtk_spin_button_new_with_range (CALIB_MIN, CALIB_MAX, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (calibrate), CALIB_DEFAULT);
  g_signal_connect (G_OBJECT (calibrate), "value_changed",
      G_CALLBACK (calibration_changed), NULL);
#endif
  gtk_box_pack_start (GTK_BOX (box), calibrate, FALSE, FALSE, 5);
  gtk_container_add (GTK_CONTAINER (alignment), box);
  gtk_box_pack_start (GTK_BOX (mainBox), alignment, FALSE, FALSE, 5);

  /* Separator */
  sep = gtk_hseparator_new ();

  /* Credits */
  gtk_box_pack_start (GTK_BOX (mainBox), sep, FALSE, FALSE, 5);

  label = gtk_label_new ("Tuner Tool developed by Josep Torra.\n"
      "http://n770galaxy.blogspot.com/");
  gtk_box_pack_start (GTK_BOX (mainBox), label, FALSE, FALSE, 5);

  /* Piano keyboard */
  drawingarea2 = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawingarea2, 636, 130);
  gtk_box_pack_start (GTK_BOX (mainBox), drawingarea2, FALSE, FALSE, 5);

  g_signal_connect (G_OBJECT (drawingarea2), "expose_event",
      G_CALLBACK (expose_event), NULL);
  if (piano_enabled) {
    g_signal_connect (G_OBJECT (drawingarea2), "button_press_event",
        G_CALLBACK (key_press_event), (gpointer) src2);

    g_signal_connect (G_OBJECT (drawingarea2), "button_release_event",
        G_CALLBACK (key_release_event), (gpointer) src2);

    gtk_widget_set_events (drawingarea2, GDK_EXPOSURE_MASK
        | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  } else {
    gtk_widget_set_events (drawingarea2, GDK_EXPOSURE_MASK);
  }
#ifdef HILDON
  gtk_container_add (GTK_CONTAINER (view), mainBox);
#if defined(MAEMO1)
  hildon_app_set_appview (app, view);
  gtk_widget_show_all (GTK_WIDGET (app));
#else
  hildon_program_add_window (app, view);
  gtk_widget_show_all (GTK_WIDGET (view));
#endif
#else
  gtk_container_add (GTK_CONTAINER (mainWin), mainBox);
  gtk_widget_show_all (GTK_WIDGET (mainWin));
#endif

  gst_element_set_state (bin1, GST_STATE_PLAYING);
  gst_element_set_state (bin2, GST_STATE_PLAYING);
  gtk_main ();
  gst_element_set_state (bin2, GST_STATE_NULL);
  gst_element_set_state (bin1, GST_STATE_NULL);

  gst_object_unref (bin1);
  gst_object_unref (bin2);

  return 0;
}
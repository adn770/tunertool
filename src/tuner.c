/* vim: set sts=2 sw=2 et: */
/* Tuner
 * Copyright (C) 2006 Josep Torra <j.torra@telefonica.net>
 *               2008-2009 Jari Tenhunen <jari.tenhunen@iki.fi>
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

#ifdef HILDON
#include <hildon/hildon-defines.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-number-editor.h>

#include <libosso.h>

#define OSSO_PACKAGE "tuner-tool"
#define OSSO_VERSION VERSION

#endif /* ifdef HILDON */

#ifdef MAEMO
# define DEFAULT_AUDIOSRC "pulsesrc"
# define DEFAULT_AUDIOSINK "pulsesink"
#else
# define DEFAULT_AUDIOSRC "alsasrc"
# define DEFAULT_AUDIOSINK "alsasink"
#endif


#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "gstpitch.h"
#include "settings.h"

#define between(x,a,b) (((x)>=(a)) && ((x)<=(b)))

#define MAGIC (1.059463094359f) /* 2^(1/2) */
#define CENT (1.0005777895f) /* 1/100th of a half-tone */
#define LOG_CENT (0.00057762265046662109f) /* ln (CENT) */

extern gboolean plugin_pitch_init (GstPlugin * plugin);
extern gboolean plugin_tonesrc_init (GstPlugin * plugin);

typedef struct
{
  const gchar *name;
  gfloat frequency;
} Note;

struct app_data
{
  GtkWidget *targetFrequency;
  GtkWidget *currentFrequency;
  GtkWidget *drawingarea1;
  GtkWidget *drawingarea2;

  GstElement *bin1;
  GstElement *bin2;
  GstElement *tonesrc;
  GstElement *pitch;
  guint stop_timer_id;
  guint vol_timer_id;
  gdouble target_vol;

  gboolean display_keepalive;
#ifdef MAEMO
  osso_context_t *osso_context;
  gpointer app;
  guint display_timer_id;
#endif
};

typedef struct app_data AppData;

enum
{
  NUM_NOTES = 96
};



#define NUM_LEDS (50)
#define NUM_WKEYS (15) /* # of white keys in the piano keyboard */
#define WKEY_WIDTH (45)

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
static GdkColor whiteColor = { 0, 65535, 65535, 65535 };
static GdkColor blackColor = { 0, 0, 0, 0 };

static void
recalculate_scale (double a4)
{
  int i;

  for (i = 0; i < NUM_NOTES; i++) {
    equal_tempered_scale[i].frequency = a4 * pow (MAGIC, i - 57);
    /* fprintf(stdout, "%s: %.2f\n", equal_tempered_scale[i].name, equal_tempered_scale[i].frequency); */
  }
}

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
    settings_set_calibration (value);
  }
}

static void
on_window_destroy (GtkObject * object, gpointer user_data)
{
  gtk_main_quit ();
}

static void
toggle_fullscreen (GtkWindow * window)
{
  static gboolean fullscreen = FALSE;

  fullscreen = !fullscreen;
  if (fullscreen)
    gtk_window_fullscreen (GTK_WINDOW (window));
  else
    gtk_window_unfullscreen (GTK_WINDOW (window));
}

static gboolean 
key_press_event (GtkWidget * widget, GdkEventKey * event, GtkWindow * window)
{
  switch (event->keyval) {
#ifdef HILDON
    case HILDON_HARDKEY_FULLSCREEN:
      toggle_fullscreen (window);
      break;
#endif
    default:
      break;
  }

  return FALSE;
}

static void
draw_leds (AppData * appdata, gint n)
{
  gint i, j, k;
  static GdkGC *gc = NULL;
  gint width = appdata->drawingarea1->allocation.width;
  gint led_width = ((gfloat) width / (gfloat) (NUM_LEDS)) * 0.8;
  gint led_space = ((gfloat) width / (gfloat) (NUM_LEDS)) * 0.2;
  gint led_total = led_width + led_space;
  gint padding = (width - NUM_LEDS * led_total) / 2;

  if (!gc) {
    gc = gdk_gc_new (appdata->drawingarea1->window);
  }
  gdk_gc_set_rgb_fg_color (gc, &appdata->drawingarea1->style->fg[0]);

  gdk_draw_rectangle (appdata->drawingarea1->window, gc, TRUE, 0, 0,
      appdata->drawingarea1->allocation.width, appdata->drawingarea1->allocation.height);

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

      gdk_draw_rectangle (appdata->drawingarea1->window, gc, TRUE, padding + (i * led_total) + ((led_total - 4) / 2), 2, 4,
          36);
    } else {
      if ((i >= j) && (i <= k))
        gdk_gc_set_rgb_fg_color (gc, &ledOnColor);
      else
        gdk_gc_set_rgb_fg_color (gc, &ledOffColor);

      gdk_draw_rectangle (appdata->drawingarea1->window, gc, TRUE, padding + (i * led_total), 10, led_width,
          20);
    }
  }
}

/* translate the interval (ratio of two freqs) into cents */
gfloat
interval2cent (gfloat freq, gfloat note)
{
  //return (gfloat) (log (freq / note) / log (CENT));
  return (gfloat) (log (freq / note) / LOG_CENT);
}

/* update frequency info */
static void
update_frequency (AppData * appdata, gfloat frequency)
{
  gchar *buffer;
  gint i, j;
  gfloat diff, min_diff;

  min_diff = frequency - (equal_tempered_scale[0].frequency - 10);
  for (i = j = 0; i < NUM_NOTES; i++) {
    diff = frequency - equal_tempered_scale[i].frequency;
    if (fabs (diff) <= fabs (min_diff)) {
      min_diff = diff;
      j = i;
    } else {
      break;
    }
  }

  buffer =
      g_strdup_printf ("Nearest note is %s with %.2f Hz frequency",
      equal_tempered_scale[j].name, equal_tempered_scale[j].frequency);
  gtk_label_set_text (GTK_LABEL (appdata->targetFrequency), buffer);
  g_free (buffer);

  buffer = g_strdup_printf ("Played frequency is %.2f Hz", frequency);
  gtk_label_set_text (GTK_LABEL (appdata->currentFrequency), buffer);
  g_free (buffer);

  /* make leds display the difference in steps of two cents */
  diff = interval2cent (frequency, equal_tempered_scale[j].frequency);
  draw_leds (appdata, (gint) roundf (diff / 2.0));
}

/* receive spectral data from element message */
gboolean
message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);

    if (strcmp (name, "pitch") == 0) {
      gfloat frequency;

      frequency = g_value_get_float (gst_structure_get_value (s, "frequency"));
      if (frequency != 0)
        update_frequency (data, frequency);
    }
  }
  /* we handled the message we want, and ignored the ones we didn't want.
   * so the core can unref the message for us */
  return TRUE;
}

gfloat
keynote2freq (AppData * appdata, gint x, gint y)
{
  gint i, j, height, found;
  gfloat frequency = 0;

  height = appdata->drawingarea2->allocation.height;

  j = 0;
  found = 0;
  for (i = 0; i < NUM_WKEYS; i++) {
    // Test for a white key  
    j++;
    if (between (x, i * WKEY_WIDTH, i * WKEY_WIDTH + (WKEY_WIDTH - 1)) && between (y, 0, height))
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
expose_event (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;
  gint i;
  static GdkGC *gc = NULL;

  if (!gc) {
    gc = gdk_gc_new (appdata->drawingarea2->window);
  }
  gdk_gc_set_rgb_fg_color (gc, &whiteColor);
  gdk_draw_rectangle (appdata->drawingarea2->window, gc, TRUE, 0, 0,
      NUM_WKEYS * WKEY_WIDTH, appdata->drawingarea2->allocation.height - 1);

  gdk_gc_set_rgb_fg_color (gc, &blackColor);
  gdk_draw_rectangle (appdata->drawingarea2->window, gc, FALSE, 0, 0,
      NUM_WKEYS * WKEY_WIDTH, appdata->drawingarea2->allocation.height - 1);

  for (i = 0; i < NUM_WKEYS - 1; i++)
    gdk_draw_rectangle (appdata->drawingarea2->window, gc, FALSE, i * WKEY_WIDTH, 0,
        WKEY_WIDTH, appdata->drawingarea2->allocation.height - 1);

  for (i = 0; i < NUM_WKEYS - 1; i++) {
    if (((i % 7) != 2) && ((i % 7) != 6))
      gdk_draw_rectangle (appdata->drawingarea2->window, gc, TRUE, 24 + i * WKEY_WIDTH, 0,
          42, appdata->drawingarea2->allocation.height / 2);
  }
  return FALSE;
}

static gboolean
button_press_event (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  if (event->button == 1) {
    g_object_set (appdata->tonesrc, "freq", (gdouble) keynote2freq (appdata, event->x, event->y),
        "volume", 0.8, NULL);
  }

  return TRUE;
}

static gboolean
button_release_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  if (event->button == 1) {
    g_object_set (appdata->tonesrc, "volume", 0.0, NULL);
  }

  return TRUE;
}

static void
set_pipeline_states (AppData * appdata, GstState state)
{
    if (appdata->bin1)
      gst_element_set_state (appdata->bin1, state);

    if (appdata->bin2)
      gst_element_set_state (appdata->bin2, state);
}

static gboolean
stop_pipelines (gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  /* dsppcmsrc needs to go to READY or NULL state to make 
   * the DSP sleep and OMAP reach retention mode */
  set_pipeline_states (appdata, GST_STATE_READY); 
  appdata->stop_timer_id = 0;

  return FALSE;
}

#ifdef FAKE_FREQUENCY
static gboolean
fake_frequency (gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  update_frequency (appdata, 440.0);

  return TRUE;
}
#endif

#ifdef MAEMO
static void
osso_hw_state_cb (osso_hw_state_t *state, gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  if (state->shutdown_ind) {
    gtk_main_quit ();
    return;
  }

  if (state->system_inactivity_ind) {
    /* do not stop pipelines if the app is on foreground 
     * and display is kept on */
    if (appdata->display_timer_id == 0) {
      if (appdata->stop_timer_id != 0)
        g_source_remove (appdata->stop_timer_id);

      appdata->stop_timer_id = g_timeout_add (5000, (GSourceFunc) stop_pipelines, user_data);
    }
  }
  else {
#if HILDON == 1
    if (hildon_program_get_is_topmost (HILDON_PROGRAM (appdata->app))) {
      if (appdata->stop_timer_id != 0) {
        g_source_remove (appdata->stop_timer_id);
        appdata->stop_timer_id = 0;
      }

      set_pipeline_states (appdata, GST_STATE_PLAYING);
    }
    /* not topmost => topmost_notify will set pipelines to PLAYING 
     * when the application is on the foreground again */
#else
    if (appdata->stop_timer_id != 0) {
      g_source_remove (appdata->stop_timer_id);
      appdata->stop_timer_id = 0;
    }

    set_pipeline_states (appdata, GST_STATE_PLAYING);
#endif

  }
}
#endif /* MAEMO */

#if HILDON == 1
static gboolean
display_keepalive (gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  /* first (direct) call: call blanking_pause and set up timer */
  if (appdata->display_timer_id == 0) {
    osso_display_blanking_pause (appdata->osso_context);
    appdata->display_timer_id = g_timeout_add (55000, (GSourceFunc) display_keepalive, user_data);
    return TRUE; /* does not really matter */
  }

  /* callback from main loop */
  if (hildon_program_get_is_topmost (HILDON_PROGRAM (appdata->app))) {
    osso_display_blanking_pause (appdata->osso_context);
    return TRUE;
  }
  /* else */
  appdata->display_timer_id = 0;
  return FALSE;
}

static void
display_keepalive_stop (AppData * appdata)
{
  if (appdata->display_timer_id) {
    g_source_remove (appdata->display_timer_id);
    appdata->display_timer_id = 0;
  }
}

static gboolean
topmost_notify (GObject * object, GParamSpec * pspec, gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  if (hildon_program_get_is_topmost (HILDON_PROGRAM (object))) {
    /* cancel pipeline stop timer if it is ticking */
    if (appdata->stop_timer_id != 0) {
      g_source_remove (appdata->stop_timer_id);
      appdata->stop_timer_id = 0;
    }

    set_pipeline_states (appdata, GST_STATE_PLAYING);

    /* keep display on */
    if (appdata->display_keepalive && appdata->display_timer_id == 0)
      display_keepalive (user_data);
  }
  else {
    /* pause pipelines so that we don't update the UI needlessly */
    set_pipeline_states (appdata, GST_STATE_PAUSED);
    /* stop pipelines fully if the app stays in the background for 30 seconds */
    appdata->stop_timer_id = g_timeout_add (30000, (GSourceFunc) stop_pipelines, user_data);
    /* let display dim and switch off */
    display_keepalive_stop (appdata);
  }

  return FALSE;
}
#endif

static void
settings_notify (GConfClient * client, guint cnxn_id, GConfEntry * entry, gpointer user_data)
{
  AppData * appdata = (AppData *) user_data;

  g_debug ("%s changed", gconf_entry_get_key (entry));

  if (strcmp (gconf_entry_get_key (entry), GCONF_KEY_ALGORITHM) == 0) {
    if (gconf_entry_get_value (entry) != NULL && gconf_entry_get_value (entry)->type == GCONF_VALUE_INT) {
      g_object_set (G_OBJECT (appdata->pitch), 
          "algorithm", gconf_value_get_int (gconf_entry_get_value (entry)),
          NULL);
    }
  }
  else if (strcmp (gconf_entry_get_key (entry), GCONF_KEY_CALIBRATION) == 0) {
    /* TODO */
  }
  else if (strcmp (gconf_entry_get_key (entry), GCONF_KEY_DISPLAY_KEEPALIVE) == 0) {
    if (gconf_entry_get_value (entry) != NULL && gconf_entry_get_value (entry)->type == GCONF_VALUE_BOOL) {
      appdata->display_keepalive = gconf_value_get_bool (gconf_entry_get_value (entry));

      if (appdata->display_keepalive && appdata->display_timer_id == 0)
        display_keepalive (user_data);
      else
        display_keepalive_stop (appdata);
    }
  }
  else {
    g_warning ("unknown GConf key `%s'", gconf_entry_get_key (entry));
  }
}

static void 
settings_activate (GtkWidget * widget, GtkWidget * main_win)
{
  settings_dialog_show (GTK_WINDOW (main_win));
}

static void 
about_activate (GtkWidget * widget, GtkWindow * main_win)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *dialog;
 
  dialog = gtk_dialog_new_with_buttons("About tuner", main_win,
      GTK_DIALOG_MODAL | 
      GTK_DIALOG_DESTROY_WITH_PARENT |
      GTK_DIALOG_NO_SEPARATOR,
      NULL, NULL);

  g_signal_connect (G_OBJECT (dialog), "delete_event", G_CALLBACK (gtk_widget_destroy), NULL);

  vbox = gtk_vbox_new (FALSE, HILDON_MARGIN_DEFAULT);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), HILDON_MARGIN_DEFAULT);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  label = gtk_label_new ("Tuner Tool is developed by Josep Torra and Jari Tenhunen.\n"
      "http://n770galaxy.blogspot.com/\n");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 5);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}

static HildonAppMenu *
create_menu (GtkWidget *parent)
{
  HildonSizeType button_size = HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH;
  HildonAppMenu *menu = HILDON_APP_MENU (hildon_app_menu_new ());
  GtkButton *button;

  button = GTK_BUTTON (hildon_gtk_button_new (button_size));
  gtk_button_set_label (button, "Settings");
  g_signal_connect_after (G_OBJECT (button), "clicked",
      G_CALLBACK (settings_activate), parent);
  hildon_app_menu_append (menu, button);

  button = GTK_BUTTON (hildon_gtk_button_new (button_size));
  gtk_button_set_label (button, "About");
  g_signal_connect_after (G_OBJECT (button), "clicked",
      G_CALLBACK (about_activate), parent);
  hildon_app_menu_append (menu, button);

  gtk_widget_show_all (GTK_WIDGET (menu));

  return menu;
}

int
main (int argc, char *argv[])
{
  AppData * appdata = NULL;
#ifdef HILDON
  HildonProgram *app = NULL;
  osso_hw_state_t hw_state_mask = { TRUE, FALSE, FALSE, TRUE, 0 };
#endif
  gint calib;

  GstElement *src1, *src2, *sink1;
  GstElement *sink2;
  GstBus *bus;

  GtkWidget *mainWin;
  GtkWidget *mainBox;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *alignment;
  GtkWidget *calibrate;
  GtkWidget *sep;
  HildonAppMenu *menu;

#ifndef HILDON
  GdkPixbuf *icon = NULL;
  GError *error = NULL;
#endif
  gboolean piano_enabled = TRUE;

  appdata = g_new0(AppData, 1);

  /* Init GStreamer */
  gst_init (&argc, &argv);
  /* Register the GStreamer plugins */
  plugin_pitch_init (NULL);
  plugin_tonesrc_init (NULL);


  /* Init the gtk - must be called before any hildon stuff */
  gtk_init (&argc, &argv);

  app = HILDON_PROGRAM (hildon_program_get_instance ());
  g_set_application_name ("Tuner");

  appdata->app = app;

  /* Initialize maemo application */
  appdata->osso_context = osso_initialize (OSSO_PACKAGE, OSSO_VERSION, TRUE, NULL);

  /* Check that initialization was ok */
  if (appdata->osso_context == NULL) {
    g_print ("Bummer, osso failed\n");
  }
  g_assert (appdata->osso_context);

  /* could use also display_event_cb but it is available only from chinook onwards */
  if (osso_hw_set_event_cb (appdata->osso_context, &hw_state_mask, osso_hw_state_cb, appdata) != OSSO_OK)
    g_warning ("setting osso_hw_state_cb failed!");

  settings_init (&settings_notify, appdata);

  calib = settings_get_calibration (CALIB_DEFAULT);
  recalculate_scale (calib);

  mainBox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (mainBox), 0);
  mainWin = hildon_stackable_window_new ();
  g_signal_connect (G_OBJECT (app), "notify::is-topmost", G_CALLBACK (topmost_notify), appdata);

  menu = create_menu (mainWin);
  hildon_program_set_common_app_menu (app, menu);

  /* Bin for tuner functionality */
  appdata->bin1 = gst_pipeline_new ("bin1");

  src1 = gst_element_factory_make (DEFAULT_AUDIOSRC, "src1");
  g_object_set (G_OBJECT (src1), "device", "source.voice.raw", NULL);

  appdata->pitch = gst_element_factory_make ("pitch", "pitch");

  g_object_set (G_OBJECT (appdata->pitch), "message", TRUE, "minfreq", 10,
      "maxfreq", 4000, 
      "algorithm", settings_get_algorithm (DEFAULT_ALGORITHM),
      NULL);

  sink1 = gst_element_factory_make ("fakesink", "sink1");
  g_object_set (G_OBJECT (sink1), "silent", 1, NULL);

  gst_bin_add_many (GST_BIN (appdata->bin1), src1, appdata->pitch, sink1, NULL);
  if (!gst_element_link_many (src1, appdata->pitch, sink1, NULL)) {
    fprintf (stderr, "cant link elements\n");
    exit (1);
  }

  bus = gst_element_get_bus (appdata->bin1);
  gst_bus_add_watch (bus, message_handler, appdata);
  gst_object_unref (bus);

  /* Bin for piano functionality */
  appdata->bin2 = gst_pipeline_new ("bin2");

  //src2 = gst_element_factory_make ("audiotestsrc", "src2");
  //g_object_set (G_OBJECT (src2), "volume", 0.0, "wave", 7, NULL);
  src2 = gst_element_factory_make ("tonesrc", "src2");
  g_object_set (G_OBJECT (src2), "volume", 0.0, NULL);
  sink2 = gst_element_factory_make (DEFAULT_AUDIOSINK, "sink2");

  gst_bin_add_many (GST_BIN (appdata->bin2), src2, sink2, NULL);
  if (!gst_element_link_many (src2, sink2, NULL)) {
    piano_enabled = FALSE;
  }

  appdata->tonesrc = src2;

  /* GUI */
  g_signal_connect (G_OBJECT (mainWin), "destroy",
      G_CALLBACK (on_window_destroy), NULL);
  g_signal_connect (G_OBJECT(mainWin), "key_press_event", 
      G_CALLBACK (key_press_event), mainWin);

  /* Note label */
  appdata->targetFrequency = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (mainBox), appdata->targetFrequency, FALSE, FALSE, 5);

  /* Leds */
  appdata->drawingarea1 = gtk_drawing_area_new ();
  gtk_widget_set_size_request (appdata->drawingarea1, 636, 40);
  gtk_box_pack_start (GTK_BOX (mainBox), appdata->drawingarea1, FALSE, FALSE, 5);

  /* Current frequency lable */
  appdata->currentFrequency = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (mainBox), appdata->currentFrequency, FALSE, FALSE, 5);

  /* Calibration spinner */
  box = gtk_hbox_new (FALSE, 0);
  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  label = gtk_label_new ("Calibration");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 5);

#ifdef HILDON
  calibrate = calibration_editor_new (CALIB_MIN, CALIB_MAX);
  hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (calibrate),
      calib);
  g_signal_connect (G_OBJECT (calibrate), "notify::value",
      G_CALLBACK (calibration_changed), NULL);
#else
  calibrate = gtk_spin_button_new_with_range (CALIB_MIN, CALIB_MAX, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (calibrate), calib);
  g_signal_connect (G_OBJECT (calibrate), "value_changed",
      G_CALLBACK (calibration_changed), NULL);
#endif
  gtk_box_pack_start (GTK_BOX (box), calibrate, FALSE, FALSE, 5);
  gtk_container_add (GTK_CONTAINER (alignment), box);
  gtk_box_pack_start (GTK_BOX (mainBox), alignment, FALSE, FALSE, 5);

  /* Separator */
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (mainBox), sep, FALSE, FALSE, 5);

  /* Piano keyboard */
  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  appdata->drawingarea2 = gtk_drawing_area_new ();
  gtk_widget_set_size_request (appdata->drawingarea2, NUM_WKEYS * WKEY_WIDTH + 1, 130);
  gtk_container_add (GTK_CONTAINER (alignment), appdata->drawingarea2);
  gtk_box_pack_start (GTK_BOX (mainBox), alignment, FALSE, FALSE, 5);

  g_signal_connect (G_OBJECT (appdata->drawingarea2), "expose_event",
      G_CALLBACK (expose_event), appdata);
  if (piano_enabled) {
    g_signal_connect (G_OBJECT (appdata->drawingarea2), "button_press_event",
        G_CALLBACK (button_press_event), (gpointer) appdata);

    g_signal_connect (G_OBJECT (appdata->drawingarea2), "button_release_event",
        G_CALLBACK (button_release_event), (gpointer) appdata);

    gtk_widget_set_events (appdata->drawingarea2, GDK_EXPOSURE_MASK
        | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  } else {
    gtk_widget_set_events (appdata->drawingarea2, GDK_EXPOSURE_MASK);
  }

  gtk_container_add (GTK_CONTAINER (mainWin), mainBox);
  hildon_program_add_window (app, HILDON_WINDOW (mainWin));
  gtk_widget_show_all (GTK_WIDGET (mainWin));

  appdata->display_keepalive = settings_get_display_keepalive (DEFAULT_DISPLAY_KEEPALIVE);

  if (appdata->display_keepalive)
    display_keepalive (appdata);

  draw_leds (appdata, 0);

  set_pipeline_states (appdata, GST_STATE_PLAYING);

#ifdef FAKE_FREQUENCY
  g_timeout_add (2000, (GSourceFunc) fake_frequency, appdata);
#endif

  gtk_main ();

  set_pipeline_states (appdata, GST_STATE_NULL);

  gst_object_unref (appdata->bin1);
  gst_object_unref (appdata->bin2);

  return 0;
}

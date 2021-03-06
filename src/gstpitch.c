/* vim: set sts=2 sw=2 et: */
/*
 * GStreamer
 * Copyright (C) 2006 Josep Torra <j.torra@telefonica.net>
 *               2008-2010 Jari Tenhunen <jari.tenhunen@iki.fi>
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

#include <gst/audio/audio.h>

#include "gstpitch.h"

GST_DEBUG_CATEGORY_STATIC (gst_pitch_debug);
#define GST_CAT_DEFAULT gst_pitch_debug

#define RATE    32000
#define RATESTR "32000"
#define WANTED  RATE * 2
#define ZERO_PADDING_FACTOR 2
#define FFT_LEN (RATE * ZERO_PADDING_FACTOR)

/* Filter signals and args */
enum
{
  PROP_0,
  PROP_SIGNAL_FFREQ,
  PROP_MINFREQ,
  PROP_MAXFREQ,
  PROP_NFFT,
  PROP_ALGORITHM
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) " RATESTR ", "
        "channels = (int) 1, "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) 16, " "depth = (int) 16, " "signed = (boolean) true")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [1, MAX], "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) 16, " "depth = (int) 16, " "signed = (boolean) true")
    );

#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_pitch_debug, "Pitch", 0, "fundamental frequency plugin");

GST_BOILERPLATE_FULL (GstPitch, gst_pitch, GstBaseTransform,
    GST_TYPE_BASE_TRANSFORM, DEBUG_INIT);

static void gst_pitch_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_pitch_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_pitch_dispose (GObject * object);

static gboolean gst_pitch_set_caps (GstBaseTransform * trans, GstCaps * in,
    GstCaps * out);
static gboolean gst_pitch_start (GstBaseTransform * trans);

static GstFlowReturn gst_pitch_transform_ip (GstBaseTransform * trans,
    GstBuffer * in);

#define DEFAULT_PROP_ALGORITHM GST_PITCH_ALGORITHM_FFT

#define GST_TYPE_PITCH_ALGORITHM (gst_pitch_algorithm_get_type())
static GType
gst_pitch_algorithm_get_type (void)
{
  static GType pitch_algorithm_type = 0;
  static const GEnumValue pitch_algorithm[] = {
    {GST_PITCH_ALGORITHM_FFT, "fft", "fft"},
    {GST_PITCH_ALGORITHM_HPS, "hps", "hps"},
    {0, NULL, NULL},
  };

  if (!pitch_algorithm_type) {
    pitch_algorithm_type =
        g_enum_register_static ("GstPitchAlgorithm",
        pitch_algorithm);
  }
  return pitch_algorithm_type;
}

/* GObject vmethod implementations */

static void
gst_pitch_base_init (gpointer klass)
{
  static GstElementDetails element_details = {
    "Pitch analyzer",
    "Filter/Analyzer/Audio",
    "Run an FFT on the audio signal, output fundamental frequency",
    "Josep Torra <j.torra@telefonica.net>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_set_details (element_class, &element_details);
}

static void
gst_pitch_class_init (GstPitchClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class = (GObjectClass *) klass;
  gobject_class->set_property = gst_pitch_set_property;
  gobject_class->get_property = gst_pitch_get_property;
  gobject_class->dispose = gst_pitch_dispose;

  trans_class->set_caps = GST_DEBUG_FUNCPTR (gst_pitch_set_caps);
  trans_class->start = GST_DEBUG_FUNCPTR (gst_pitch_start);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_pitch_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  g_object_class_install_property (gobject_class, PROP_SIGNAL_FFREQ,
      g_param_spec_boolean ("message", "Message",
          "Post a fundamental frequency message for each passed interval",
          TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MINFREQ,
      g_param_spec_int ("minfreq", "MinFreq",
          "Initial scan frequency, default 30 Hz",
          1, G_MAXINT, 30, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAXFREQ,
      g_param_spec_int ("maxfreq", "MaxFreq",
          "Final scan frequency, default 1500 Hz",
          1, G_MAXINT, 1500, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ALGORITHM,
      g_param_spec_enum ("algorithm", "Algorithm",
          "Pitch detection algorithm to use",
          GST_TYPE_PITCH_ALGORITHM, DEFAULT_PROP_ALGORITHM,
          G_PARAM_READWRITE));


  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_pitch_transform_ip);
}

static void
gst_pitch_setup_algorithm (GstPitch * filter, GstPitchAlgorithm algorithm)
{
  g_mutex_lock (filter->mutex);
  if (algorithm == GST_PITCH_ALGORITHM_HPS) {
    if (NULL == filter->module)
      filter->module = (gint *) g_malloc (FFT_LEN * sizeof (gint));
  }
  else {
    if (filter->module) 
      g_free (filter->module);

    filter->module = NULL;
  }
  filter->algorithm = algorithm;
  g_mutex_unlock (filter->mutex);
}

static void
gst_pitch_init (GstPitch * filter, GstPitchClass * klass)
{
  filter->adapter = gst_adapter_new ();

  filter->minfreq = 30;
  filter->maxfreq = 1500;
  filter->message = TRUE;
  filter->mutex = g_mutex_new ();
    
  gst_pitch_setup_algorithm (filter, DEFAULT_PROP_ALGORITHM);
}

static void
gst_pitch_dispose (GObject * object)
{
  GstPitch *filter = GST_PITCH (object);

  if (filter->adapter) {
    g_object_unref (filter->adapter);
    filter->adapter = NULL;
  }

  g_free (filter->fft_cfg);
  g_free (filter->signal);
  g_free (filter->spectrum);
  if (filter->module)
    g_free (filter->module);

  g_mutex_free (filter->mutex);

  kiss_fft_cleanup ();

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_pitch_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstPitch *filter = GST_PITCH (object);

  switch (prop_id) {
    case PROP_SIGNAL_FFREQ:
      filter->message = g_value_get_boolean (value);
      break;
    case PROP_MINFREQ:
      filter->minfreq = g_value_get_int (value);
      break;
    case PROP_MAXFREQ:
      filter->maxfreq = g_value_get_int (value);
      break;
    case PROP_ALGORITHM:
      gst_pitch_setup_algorithm (filter, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_pitch_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstPitch *filter = GST_PITCH (object);

  switch (prop_id) {
    case PROP_SIGNAL_FFREQ:
      g_value_set_boolean (value, filter->message);
      break;
    case PROP_MINFREQ:
      g_value_set_int (value, filter->minfreq);
      break;
    case PROP_MAXFREQ:
      g_value_set_int (value, filter->maxfreq);
      break;
    case PROP_ALGORITHM:
      g_value_set_enum (value, filter->algorithm);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_pitch_set_caps (GstBaseTransform * trans, GstCaps * in, GstCaps * out)
{
  GstPitch *filter = GST_PITCH (trans);

  filter->fft_cfg = kiss_fft_alloc (FFT_LEN, 0, NULL, NULL);
  filter->signal =
      (kiss_fft_cpx *) g_malloc0 (FFT_LEN * sizeof (kiss_fft_cpx));
  filter->spectrum =
      (kiss_fft_cpx *) g_malloc (FFT_LEN * sizeof (kiss_fft_cpx));

  return TRUE;
}

static gboolean
gst_pitch_start (GstBaseTransform * trans)
{
  GstPitch *filter = GST_PITCH (trans);

  gst_adapter_clear (filter->adapter);

  return TRUE;
}

static GstMessage *
gst_pitch_message_new (GstPitch * filter)
{
  GstStructure *s;
  gint i, min_i, max_i;
  gint freq_index, frequency_module;
  gfloat frequency;

  /* Extract fundamental frequency */
  freq_index = 0;
  frequency_module = 0;
  frequency = 0.0;
  min_i = filter->minfreq * ZERO_PADDING_FACTOR;
  max_i = filter->maxfreq * ZERO_PADDING_FACTOR;

  GST_DEBUG_OBJECT (filter, "min_freq = %d, max_freq = %d", filter->minfreq,
      filter->maxfreq);
  /*GST_DEBUG_OBJECT (filter, "min_i = %d, max_i = %d", min_i, max_i); */

  g_mutex_lock (filter->mutex);
  switch (filter->algorithm) {

    case GST_PITCH_ALGORITHM_FFT:
      {
        gint module = 0;

        for (i = min_i; i < max_i; i++) {
          module = (filter->spectrum[i].r * filter->spectrum[i].r);
          module += (filter->spectrum[i].i * filter->spectrum[i].i);

          if (module > 0)
            GST_LOG_OBJECT (filter, "module[%d] = %d", i, module);

          /* find strongest peak */
          if (module > frequency_module) {
            frequency_module = module;
            freq_index = i;
          }
        }
      }
      break;

    case GST_PITCH_ALGORITHM_HPS:
      {
        gint prev_frequency = 0;
        gint j, t;

        for (i = min_i; i < FFT_LEN; i++) {
          filter->module[i] = (filter->spectrum[i].r * filter->spectrum[i].r);
          filter->module[i] += (filter->spectrum[i].i * filter->spectrum[i].i);

          if (filter->module[i] > 0)
            GST_LOG_OBJECT (filter, "module[%d] = %d", i, filter->module[i]);

        }
        /* Harmonic Product Spectrum algorithm */
#define MAX_DS_FACTOR (6)
        for (i = min_i; (i <= max_i) && (i < FFT_LEN); i++) {
          for (j = 2; j <= MAX_DS_FACTOR; j++) {
            t = i * j * ZERO_PADDING_FACTOR;
            if (t > FFT_LEN)
              break;

            /* this is not part of the HPS but it seems
             * there are lots of zeroes in the spectrum ... 
             */
            if (filter->module[t] != 0) 
              filter->module[i] *= filter->module[t];
          }

          /* find strongest peak */
          if (filter->module[i] > frequency_module) {
            prev_frequency = freq_index;
            frequency_module = filter->module[i];
            freq_index = i;
          }
        }

        /* try to correct octave error */
#if 0
        if (freq_index != 0 && prev_frequency != 0) {
          float ratio = (float) frequency / (float) prev_frequency;
          if (ratio >= 1.9 && ratio < 2.1 && (float) filter->module[prev_frequency] >= 0.2 * (float) frequency_module ) {
            g_debug("Chose freq %d[%d] over %d[%d]\n", prev_frequency, filter->module[prev_frequency], frequency, filter->module[frequency]);
            frequency = prev_frequency;
            frequency_module = filter->module[prev_frequency];
          } 
        }
#endif
      }
      break;
    default:
      break;
  }
  g_mutex_unlock (filter->mutex);

  frequency = (gfloat) freq_index / ZERO_PADDING_FACTOR;
  /*
  g_debug("freq %d[%d]\n", frequency, frequency_module);
  */
  GST_DEBUG_OBJECT (filter, "preparing message, frequency = %.2f", frequency);

  s = gst_structure_new ("pitch", "frequency", G_TYPE_FLOAT, frequency, NULL);

  return gst_message_new_element (GST_OBJECT (filter), s);
}

/* GstBaseTransform vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn
gst_pitch_transform_ip (GstBaseTransform * trans, GstBuffer * in)
{
  GstPitch *filter = GST_PITCH (trans);
  gint16 *samples;
  gint i;
  guint avail;

  GST_DEBUG_OBJECT (filter, "transform : %ld bytes", GST_BUFFER_SIZE (in));
  gst_adapter_push (filter->adapter, gst_buffer_ref (in));

  /* required number of bytes */
  avail = gst_adapter_available (filter->adapter);
  GST_DEBUG_OBJECT (filter, "avail: %d wanted: %d", avail, WANTED);

  if (avail > WANTED) {

    /* copy sample data in the complex vector */
    samples = (gint16 *) gst_adapter_peek (filter->adapter, WANTED);

    for (i = 0; i < RATE; i++) {
      filter->signal[i].r = (kiss_fft_scalar) (samples[i]);
    }

    /* flush half second of data to implement sliding window */
    gst_adapter_flush (filter->adapter, WANTED >> 1);

    GST_DEBUG ("perform fft");
    kiss_fft (filter->fft_cfg, filter->signal, filter->spectrum);

    if (filter->message) {
      GstMessage *m = gst_pitch_message_new (filter);
      gst_element_post_message (GST_ELEMENT (filter), m);
    }
  }

  return GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and pad templates
 * register the features
 *
 * exchange the string 'plugin' with your elemnt name
 */
/* static */ gboolean
plugin_pitch_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "pitch", GST_RANK_NONE, GST_TYPE_PITCH);
}

/* this is the structure that gstreamer looks for to register plugins
 *
 * exchange the strings 'plugin' and 'Template plugin' with you plugin name and
 * description
 */
#if 0
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "pitch",
    "Run an FFT on the audio signal, output fundamental frequency",
    plugin_init, VERSION, "LGPL", "GStreamer", "http://gstreamer.net/")
#endif

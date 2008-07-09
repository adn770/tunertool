/*
 * GStreamer
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

#include <gst/audio/audio.h>

#include "gstpitch.h"

GST_DEBUG_CATEGORY_STATIC (gst_pitch_debug);
#define GST_CAT_DEFAULT gst_pitch_debug

/* Filter signals and args */
enum
{
  PROP_0,
  PROP_SIGNAL_FFREQ,
  PROP_SIGNAL_INTERVAL,
  PROP_SIGNAL_MINFREQ,
  PROP_SIGNAL_MAXFREQ,
  PROP_NFFT
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [1, MAX], "
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

  g_object_class_install_property (gobject_class, PROP_SIGNAL_INTERVAL,
      g_param_spec_uint64 ("interval", "Interval",
          "Interval of time between message posts (in nanoseconds)",
          1, G_MAXUINT64, GST_SECOND / 10, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SIGNAL_MINFREQ,
      g_param_spec_int ("minfreq", "MinFreq",
          "Initial scan frequency, default 30 Hz",
          1, G_MAXINT, 30, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SIGNAL_MAXFREQ,
      g_param_spec_int ("maxfreq", "MaxFreq",
          "Final scan frequency, default 1500 Hz",
          1, G_MAXINT, 1500, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NFFT,
      g_param_spec_int ("nfft", "NFFT",
          "Number of samples taken for FFT",
          1, G_MAXINT, 1024, G_PARAM_READWRITE));

  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_pitch_transform_ip);
}

static void
gst_pitch_init (GstPitch * filter, GstPitchClass * klass)
{
  filter->adapter = gst_adapter_new ();

  filter->minfreq = 30;
  filter->maxfreq = 1500;
  filter->nfft = 1024;
  filter->message = TRUE;
  filter->interval = GST_SECOND / 10;

  filter->fft_cfg = kiss_fft_alloc (filter->nfft, 0, NULL, NULL);
  filter->signal =
      (kiss_fft_cpx *) g_malloc (filter->nfft * sizeof (kiss_fft_cpx));
  filter->spectrum =
      (kiss_fft_cpx *) g_malloc (filter->nfft * sizeof (kiss_fft_cpx));
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
    case PROP_SIGNAL_INTERVAL:
      filter->interval = gst_guint64_to_gdouble (g_value_get_uint64 (value));
      break;
    case PROP_SIGNAL_MINFREQ:
      filter->minfreq = g_value_get_int (value);
      break;
    case PROP_SIGNAL_MAXFREQ:
      filter->maxfreq = g_value_get_int (value);
      break;
    case PROP_NFFT:
      filter->nfft = g_value_get_int (value);
      g_free (filter->fft_cfg);
      g_free (filter->signal);
      g_free (filter->spectrum);
      filter->fft_cfg = kiss_fft_alloc (filter->nfft, 0, NULL, NULL);
      filter->signal =
          (kiss_fft_cpx *) g_malloc (filter->nfft * sizeof (kiss_fft_cpx));
      filter->spectrum =
          (kiss_fft_cpx *) g_malloc (filter->nfft * sizeof (kiss_fft_cpx));
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
    case PROP_SIGNAL_INTERVAL:
      g_value_set_uint64 (value, filter->interval);
      break;
    case PROP_SIGNAL_MINFREQ:
      g_value_set_int (value, filter->minfreq);
      break;
    case PROP_SIGNAL_MAXFREQ:
      g_value_set_int (value, filter->maxfreq);
      break;
    case PROP_NFFT:
      g_value_set_int (value, filter->nfft);
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
  GstStructure *structure;

  structure = gst_caps_get_structure (in, 0);
  gst_structure_get_int (structure, "rate", &filter->rate);
  gst_structure_get_int (structure, "width", &filter->width);
  gst_structure_get_int (structure, "channels", &filter->channels);

  return TRUE;
}

static gboolean
gst_pitch_start (GstBaseTransform * trans)
{
  GstPitch *filter = GST_PITCH (trans);

  gst_adapter_clear (filter->adapter);
  filter->num_frames = 0;

  return TRUE;
}

static GstMessage *
gst_pitch_message_new (GstPitch * filter)
{
  GstStructure *s;
  gint i, min_i, max_i;
  gint frequency, frequency_module, module;

  /* Extract fundamental frequency */
  frequency = 0;
  frequency_module = 0;
  min_i = filter->minfreq * filter->nfft / filter->rate;
  max_i = filter->maxfreq * filter->nfft / filter->rate;
  GST_DEBUG_OBJECT (filter, "min_freq = %d, max_freq = %d", filter->minfreq,
      filter->maxfreq);
  GST_DEBUG_OBJECT (filter, "min_i = %d, max_i = %d", min_i, max_i);
  for (i = min_i; (i <= max_i) && (i < filter->nfft); i++) {
    module = (filter->spectrum[i].r * filter->spectrum[i].r);
    module += (filter->spectrum[i].i * filter->spectrum[i].i);

    if (module > 0)
      GST_DEBUG_OBJECT (filter, "module[%d] = %d", i, module);

    if (module > frequency_module) {
      frequency_module = module;
      frequency = i;
    }
  }

  frequency = frequency * filter->rate / filter->nfft;

  GST_DEBUG_OBJECT (filter, "preparing message, frequency = %d ", frequency);

  s = gst_structure_new ("pitch", "frequency", G_TYPE_INT, frequency, NULL);

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
  gint wanted;
  gint i, j, k;
  gint32 acc;

  GST_DEBUG ("transform : %ld bytes", GST_BUFFER_SIZE (in));

  gst_adapter_push (filter->adapter, gst_buffer_ref (in));
  /* required number of bytes */
  wanted = filter->channels * filter->nfft * 2;

  while (gst_adapter_available (filter->adapter) > wanted) {

    GST_DEBUG ("  adapter loop");
    samples = (gint16 *) gst_adapter_take (filter->adapter, wanted);

    for (i = 0, j = 0; i < filter->nfft; i++) {
      for (k = 0, acc = 0; k < filter->channels; k++)
        acc += samples[j++];
      filter->signal[i].r = (kiss_fft_scalar) (acc / filter->channels);
    }

    GST_DEBUG ("  fft");

    kiss_fft (filter->fft_cfg, filter->signal, filter->spectrum);

    GST_DEBUG ("  send message? %d", filter->num_frames);
    filter->num_frames += filter->nfft;
    /* do we need to message ? */
    if (filter->num_frames >=
        GST_CLOCK_TIME_TO_FRAMES (filter->interval, filter->rate)) {
      if (filter->message) {
        GstMessage *m = gst_pitch_message_new (filter);

        GST_DEBUG ("  sending message");
        gst_element_post_message (GST_ELEMENT (filter), m);
      }
      filter->num_frames = 0;
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

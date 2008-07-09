/* Derived code of GStreamer - AudioTestSrc element
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2008 Josep Torra <j.torra@telefonica.net>
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

#include "gsttonesrc.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923
#endif

#define M_PI_M2 ( M_PI + M_PI )

GST_DEBUG_CATEGORY_STATIC (tone_src_debug);
#define GST_CAT_DEFAULT tone_src_debug

static const GstElementDetails gst_tone_src_details =
GST_ELEMENT_DETAILS ("Tone source",
    "Source/Audio",
    "Creates audio test signals of given frequency and volume",
    "Stefan Kost <ensonic@users.sf.net>");


enum
{
  PROP_0,
  PROP_SAMPLES_PER_BUFFER,
  PROP_FREQ,
  PROP_VOLUME,
};


static GstStaticPadTemplate gst_tone_src_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1; "
        "audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 32, "
        "depth = (int) 32,"
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1; "
        "audio/x-raw-float, "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) { 32, 64 }, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 1")
    );


GST_BOILERPLATE (GstToneSrc, gst_tone_src, GstBaseSrc, GST_TYPE_BASE_SRC);

static void gst_tone_src_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_tone_src_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_tone_src_setcaps (GstBaseSrc * basesrc, GstCaps * caps);
static void gst_tone_src_src_fixate (GstPad * pad, GstCaps * caps);

static GstFlowReturn gst_tone_src_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);

#define DEFINE_OSCILATOR(type,scale) \
static void \
gst_tone_src_create_oscilator_##type (GstToneSrc * src, g##type * samples) \
{ \
  gint i; \
  gdouble step, amp; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  amp = src->volume * scale; \
  \
  for (i = 0; i < src->generate_samples_per_buffer; i++) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    samples[i] = (g##type) (sin (src->accumulator) * amp); \
  } \
}

DEFINE_OSCILATOR (int16, 32767.0);
DEFINE_OSCILATOR (int32, 2147483647.0);
DEFINE_OSCILATOR (float, 1.0);
DEFINE_OSCILATOR (double, 1.0);

static ProcessFunc oscilator_funcs[] = {
  (ProcessFunc) gst_tone_src_create_oscilator_int16,
  (ProcessFunc) gst_tone_src_create_oscilator_int32,
  (ProcessFunc) gst_tone_src_create_oscilator_float,
  (ProcessFunc) gst_tone_src_create_oscilator_double
};

static void
gst_tone_src_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_tone_src_src_template));
  gst_element_class_set_details (element_class, &gst_tone_src_details);
}

static void
gst_tone_src_class_init (GstToneSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_tone_src_set_property;
  gobject_class->get_property = gst_tone_src_get_property;

  g_object_class_install_property (gobject_class, PROP_SAMPLES_PER_BUFFER,
      g_param_spec_int ("samplesperbuffer", "Samples per buffer",
          "Number of samples in each outgoing buffer",
          1, G_MAXINT, 1024, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_FREQ,
      g_param_spec_double ("freq", "Frequency", "Frequency of test signal",
          0.0, 20000.0, 440.0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of test signal", 0.0,
          1.0, 0.8, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_tone_src_setcaps);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_tone_src_create);
}

static void
gst_tone_src_init (GstToneSrc * src, GstToneSrcClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (src);

  gst_pad_set_fixatecaps_function (pad, gst_tone_src_src_fixate);

  src->samplerate = 44100;
  src->format = GST_TONE_SRC_FORMAT_NONE;
  src->volume = 0.8;
  src->freq = 440.0;

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

  src->samples_per_buffer = 1024;
  src->generate_samples_per_buffer = src->samples_per_buffer;
}

static void
gst_tone_src_src_fixate (GstPad * pad, GstCaps * caps)
{
  GstToneSrc *src = GST_TONE_SRC (GST_PAD_PARENT (pad));
  const gchar *name;
  GstStructure *structure;

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "rate", src->samplerate);

  name = gst_structure_get_name (structure);
  if (strcmp (name, "audio/x-raw-int") == 0)
    gst_structure_fixate_field_nearest_int (structure, "width", 32);
  else if (strcmp (name, "audio/x-raw-float") == 0)
    gst_structure_fixate_field_nearest_int (structure, "width", 64);
}

static gboolean
gst_tone_src_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstToneSrc *src = GST_TONE_SRC (basesrc);
  const GstStructure *structure;
  const gchar *name;
  gint width;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "rate", &src->samplerate);

  name = gst_structure_get_name (structure);
  if (strcmp (name, "audio/x-raw-int") == 0) {
    ret &= gst_structure_get_int (structure, "width", &width);
    src->format = (width == 32) ? GST_TONE_SRC_FORMAT_S32 :
        GST_TONE_SRC_FORMAT_S16;
  } else {
    ret &= gst_structure_get_int (structure, "width", &width);
    src->format = (width == 32) ? GST_TONE_SRC_FORMAT_F32 :
        GST_TONE_SRC_FORMAT_F64;
  }

  src->process = oscilator_funcs[src->format];

  return ret;
}



static GstFlowReturn
gst_tone_src_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstFlowReturn res;
  GstToneSrc *src;
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 n_samples;
  gint sample_size;

  src = GST_TONE_SRC (basesrc);

  /* allocate a new buffer suitable for this pad */
  switch (src->format) {
    case GST_TONE_SRC_FORMAT_S16:
      sample_size = sizeof (gint16);
      break;
    case GST_TONE_SRC_FORMAT_S32:
      sample_size = sizeof (gint32);
      break;
    case GST_TONE_SRC_FORMAT_F32:
      sample_size = sizeof (gfloat);
      break;
    case GST_TONE_SRC_FORMAT_F64:
      sample_size = sizeof (gdouble);
      break;
    default:
      sample_size = -1;
      GST_ELEMENT_ERROR (src, CORE, NEGOTIATION, (NULL),
          ("format wasn't negotiated before get function"));
      return GST_FLOW_NOT_NEGOTIATED;
      break;
  }

  n_samples = src->n_samples + src->samples_per_buffer;
  next_time = gst_util_uint64_scale (n_samples, GST_SECOND,
      (guint64) src->samplerate);

  if ((res = gst_pad_alloc_buffer (basesrc->srcpad, src->n_samples,
              src->generate_samples_per_buffer * sample_size,
              GST_PAD_CAPS (basesrc->srcpad), &buf)) != GST_FLOW_OK) {
    return res;
  }

  GST_BUFFER_TIMESTAMP (buf) = src->timestamp_offset + src->running_time;
  GST_BUFFER_OFFSET_END (buf) = n_samples;
  GST_BUFFER_DURATION (buf) = next_time - src->running_time;

  gst_object_sync_values (G_OBJECT (src), src->running_time);

  src->running_time = next_time;
  src->n_samples = n_samples;

  GST_LOG_OBJECT (src, "generating %u samples at ts %" GST_TIME_FORMAT,
      length, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

  src->process (src, GST_BUFFER_DATA (buf));

  if (G_UNLIKELY ((src->volume == 0.0))) {
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_GAP);
  }

  *buffer = buf;

  return GST_FLOW_OK;
}

static void
gst_tone_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstToneSrc *src = GST_TONE_SRC (object);

  switch (prop_id) {
    case PROP_SAMPLES_PER_BUFFER:
      src->samples_per_buffer = g_value_get_int (value);
      break;
    case PROP_FREQ:
      src->freq = g_value_get_double (value);
      break;
    case PROP_VOLUME:
      src->volume = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_tone_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstToneSrc *src = GST_TONE_SRC (object);

  switch (prop_id) {
    case PROP_SAMPLES_PER_BUFFER:
      g_value_set_int (value, src->samples_per_buffer);
      break;
    case PROP_FREQ:
      g_value_set_double (value, src->freq);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, src->volume);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*static*/ gboolean
plugin_tonesrc_init (GstPlugin * plugin)
{
  /* initialize gst controller library */
  gst_controller_init (NULL, NULL);

  GST_DEBUG_CATEGORY_INIT (tone_src_debug, "tonesrc", 0, "Audio Test Source");

  return gst_element_register (plugin, "tonesrc",
      GST_RANK_NONE, GST_TYPE_TONE_SRC);
}

/*
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "tonesrc",
    "Creates audio test signals of given frequency and volume",
    plugin_init, VERSION, "LGPL", NULL, NULL);
*/

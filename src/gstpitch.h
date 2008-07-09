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

#ifndef __GST_PITCH_H__
#define __GST_PITCH_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstbasetransform.h>

#include "kiss_fft.h"

G_BEGIN_DECLS
#define GST_TYPE_PITCH \
  (gst_pitch_get_type())
#define GST_PITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PITCH,GstPitch))
#define GST_PITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PITCH,GstPitchClass))
#define GST_IS_PLUGIN_TEMPLATE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PITCH))
#define GST_IS_PLUGIN_TEMPLATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PITCH))
typedef struct _GstPitch GstPitch;
typedef struct _GstPitchClass GstPitchClass;

struct _GstPitch
{
  GstBaseTransform element;

  GstPad *sinkpad, *srcpad;
  GstAdapter *adapter;

  /* properties */
  gboolean message;             /* whether or not to post messages */
  gdouble interval;             /* how many seconds between emits */
  gint minfreq;                 /* initial frequency on scan for fundamental frequency */
  gint maxfreq;                 /* final frequency on scan for fundamental frequency */
  gint nfft;                    /* number of samples taken for FFT */

  /* <private> */
  gint rate;                    /* caps variables */
  gint width;
  gint channels;

  gint num_frames;              /* frame count (1 sample per channel) */
  /* since last emit */

  kiss_fft_cfg fft_cfg;
  kiss_fft_cpx *signal;
  kiss_fft_cpx *spectrum;
};

struct _GstPitchClass
{
  GstBaseTransformClass parent_class;
};

GType gst_pithc_get_type (void);

G_END_DECLS
#endif /* __GST_PITCH_H__ */

/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
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

#ifndef __GST_TONE_SRC_H__
#define __GST_TONE_SRC_H__


#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS
#define GST_TYPE_TONE_SRC \
  (gst_tone_src_get_type())
#define GST_TONE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TONE_SRC,GstToneSrc))
#define GST_TONE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TONE_SRC,GstToneSrcClass))
#define GST_IS_TONE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TONE_SRC))
#define GST_IS_TONE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TONE_SRC))
    typedef enum
{
  GST_TONE_SRC_FORMAT_NONE = -1,
  GST_TONE_SRC_FORMAT_S16 = 0,
  GST_TONE_SRC_FORMAT_S32,
  GST_TONE_SRC_FORMAT_F32,
  GST_TONE_SRC_FORMAT_F64
} GstToneSrcFormat;

typedef struct _GstToneSrc GstToneSrc;
typedef struct _GstToneSrcClass GstToneSrcClass;

typedef void (*ProcessFunc) (GstToneSrc *, guint8 *);

/**
 * GstToneSrc:
 *
 * audiotestsrc object structure.
 */
struct _GstToneSrc
{
  GstBaseSrc parent;

  ProcessFunc process;

  /* parameters */
  gdouble volume;
  gdouble freq;

  /* audio parameters */
  gint samplerate;
  gint samples_per_buffer;
  GstToneSrcFormat format;

  /*< private > */
  gboolean tags_pushed;         /* send tags just once ? */

  GstClockTimeDiff timestamp_offset;    /* base offset */
  GstClockTime running_time;    /* total running time */
  gint64 n_samples;             /* total samples sent */
  gint64 n_samples_stop;

  gboolean eos_reached;
  gint generate_samples_per_buffer;     /* used to generate a partial buffer */

  /* waveform specific context data */
  gdouble accumulator;          /* phase angle */
};

struct _GstToneSrcClass
{
  GstBaseSrcClass parent_class;
};

GType gst_tone_src_get_type (void);

G_END_DECLS
#endif /* __GST_TONE_SRC_H__ */

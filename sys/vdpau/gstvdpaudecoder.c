/*
 * GStreamer
 * Copyright (C) 2009 Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
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

#include <gst/gst.h>
#include <gst/video/video.h>

#include "gstvdpaudecoder.h"

GST_DEBUG_CATEGORY_STATIC (gst_vdpau_decoder_debug);
#define GST_CAT_DEFAULT gst_vdpau_decoder_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_DISPLAY,
  PROP_SILENT
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-yuv, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ]"));

#define DEBUG_INIT(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_vdpau_decoder_debug, "vdpaudecoder", 0, "vdpaudecoder base class");

GST_BOILERPLATE_FULL (GstVdpauDecoder, gst_vdpau_decoder, GstElement,
    GST_TYPE_ELEMENT, DEBUG_INIT);

static void gst_vdpau_decoder_finalize (GObject * object);
static void gst_vdpau_decoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_vdpau_decoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

GstFlowReturn
gst_vdpau_decoder_push_video_surface (GstVdpauDecoder * dec,
    VdpVideoSurface surface)
{
  GstVdpauDevice *device;
  GstBuffer *buffer;

  device = dec->device;

  switch (dec->format) {
    case GST_MAKE_FOURCC ('Y', 'V', '1', '2'):
    {
      gint size;
      GstFlowReturn result;
      VdpStatus status;
      guint8 *data[3];
      guint32 stride[3];

      size =
          gst_video_format_get_size (GST_VIDEO_FORMAT_YV12, dec->width,
          dec->height);
      result =
          gst_pad_alloc_buffer_and_set_caps (dec->src, GST_BUFFER_OFFSET_NONE,
          size, GST_PAD_CAPS (dec->src), &buffer);
      if (G_UNLIKELY (result != GST_FLOW_OK))
        return result;


      data[0] = GST_BUFFER_DATA (buffer) +
          gst_video_format_get_component_offset (GST_VIDEO_FORMAT_YV12,
          0, dec->width, dec->height);
      data[1] = data[0] +
          gst_video_format_get_component_offset (GST_VIDEO_FORMAT_YV12,
          2, dec->width, dec->height);
      data[2] = data[0] +
          gst_video_format_get_component_offset (GST_VIDEO_FORMAT_YV12,
          1, dec->width, dec->height);

      stride[0] = gst_video_format_get_row_stride (GST_VIDEO_FORMAT_YV12,
          0, dec->width);
      stride[1] = gst_video_format_get_row_stride (GST_VIDEO_FORMAT_YV12,
          2, dec->width);
      stride[2] = gst_video_format_get_row_stride (GST_VIDEO_FORMAT_YV12,
          1, dec->width);

      status =
          device->vdp_video_surface_get_bits_ycbcr (surface,
          VDP_YCBCR_FORMAT_YV12, (void *) data, stride);
      if (G_UNLIKELY (status != VDP_STATUS_OK)) {
        GST_ELEMENT_ERROR (dec, RESOURCE, READ,
            ("Couldn't get data from vdpau"),
            ("Error returned from vdpau was: %s",
                device->vdp_get_error_string (status)));
        return GST_FLOW_ERROR;
      }
      break;
    }
    case GST_MAKE_FOURCC ('N', 'V', '1', '2'):
    {
      gint size;
      GstFlowReturn result;
      VdpStatus status;
      guint8 *data[2];
      guint32 stride[2];

      size = dec->width * dec->height + dec->width * dec->height / 2;
      result =
          gst_pad_alloc_buffer_and_set_caps (dec->src, GST_BUFFER_OFFSET_NONE,
          size, GST_PAD_CAPS (dec->src), &buffer);
      if (G_UNLIKELY (result != GST_FLOW_OK))
        return result;


      data[0] = GST_BUFFER_DATA (buffer);
      data[1] = data[0] + dec->width * dec->height;

      stride[0] = dec->width;
      stride[1] = dec->width;

      status =
          device->vdp_video_surface_get_bits_ycbcr (surface,
          VDP_YCBCR_FORMAT_NV12, (void *) data, stride);
      if (G_UNLIKELY (status != VDP_STATUS_OK)) {
        GST_ELEMENT_ERROR (dec, RESOURCE, READ,
            ("Couldn't get data from vdpau"),
            ("Error returned from vdpau was: %s",
                device->vdp_get_error_string (status)));
        return GST_FLOW_ERROR;
      }
      break;
    }
    default:
      break;
  }

  GST_BUFFER_TIMESTAMP (buffer) =
      gst_util_uint64_scale_int (GST_SECOND * dec->frame_nr,
      dec->framerate_denominator, dec->framerate_numerator);
  GST_BUFFER_DURATION (buffer) =
      gst_util_uint64_scale_int (GST_SECOND, dec->framerate_denominator,
      dec->framerate_numerator);
  GST_BUFFER_OFFSET (buffer) = dec->frame_nr;
  dec->frame_nr++;
  GST_BUFFER_OFFSET_END (buffer) = dec->frame_nr;

  return gst_pad_push (dec->src, buffer);
}

typedef struct
{
  VdpChromaType chroma_type;
  VdpYCbCrFormat format;
  guint32 fourcc;
} VdpauFormats;

static VdpChromaType chroma_types[3] =
    { VDP_CHROMA_TYPE_420, VDP_CHROMA_TYPE_422, VDP_CHROMA_TYPE_444 };
static VdpauFormats formats[6] = {
  {
        VDP_CHROMA_TYPE_420,
        VDP_YCBCR_FORMAT_NV12,
        GST_MAKE_FOURCC ('N', 'V', '1', '2')
      },
  {
        VDP_CHROMA_TYPE_422,
        VDP_YCBCR_FORMAT_UYVY,
        GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y')
      },
  {
        VDP_CHROMA_TYPE_444,
        VDP_YCBCR_FORMAT_V8U8Y8A8,
        GST_MAKE_FOURCC ('A', 'Y', 'U', 'V')
      },
  {
        VDP_CHROMA_TYPE_444,
        VDP_YCBCR_FORMAT_Y8U8V8A8,
        GST_MAKE_FOURCC ('A', 'V', 'U', 'Y')
      },
  {
        VDP_CHROMA_TYPE_422,
        VDP_YCBCR_FORMAT_YUYV,
        GST_MAKE_FOURCC ('Y', 'U', 'Y', 'V')
      },
  {
        VDP_CHROMA_TYPE_420,
        VDP_YCBCR_FORMAT_YV12,
        GST_MAKE_FOURCC ('Y', 'V', '1', '2')
      }
};

VdpVideoSurface
gst_vdpau_decoder_create_video_surface (GstVdpauDecoder * dec)
{
  GstVdpauDevice *device;
  VdpChromaType chroma_type;
  gint i;
  VdpStatus status;
  VdpVideoSurface surface;

  device = dec->device;

  chroma_type = VDP_CHROMA_TYPE_422;
  for (i = 0; i < 6; i++) {
    if (formats[i].fourcc == dec->format) {
      chroma_type = formats[i].chroma_type;
      break;
    }
  }

  status =
      device->vdp_video_surface_create (device->device, chroma_type, dec->width,
      dec->height, &surface);
  if (status != VDP_STATUS_OK) {
    GST_ELEMENT_ERROR (dec, RESOURCE, READ,
        ("Couldn't create a VdpVideoSurface"),
        ("Error returned from vdpau was: %s",
            device->vdp_get_error_string (status)));
    return VDP_INVALID_HANDLE;
  }

  return surface;
}

static GstCaps *
gst_vdpau_decoder_get_vdpau_support (GstVdpauDecoder * dec)
{
  GstVdpauDevice *device;
  GstCaps *caps;
  gint i;

  device = dec->device;

  caps = gst_caps_new_empty ();

  for (i = 0; i < 3; i++) {
    VdpStatus status;
    VdpBool is_supported;
    guint32 max_w, max_h;

    status =
        device->vdp_video_surface_query_capabilities (device->device,
        chroma_types[i], &is_supported, &max_w, &max_h);

    if (status != VDP_STATUS_OK && status != VDP_STATUS_INVALID_CHROMA_TYPE) {
      GST_ELEMENT_ERROR (dec, RESOURCE, READ,
          ("Could not get query VDPAU video surface capabilites"),
          ("Error returned from vdpau was: %s",
              device->vdp_get_error_string (status)));

      return NULL;
    }
    if (is_supported) {
      gint j;

      for (j = 0; j < 6; j++) {
        if (formats[j].chroma_type != chroma_types[i])
          continue;

        status =
            device->vdp_video_surface_query_ycbcr_capabilities (device->device,
            formats[j].chroma_type, formats[j].format, &is_supported);
        if (status != VDP_STATUS_OK
            && status != VDP_STATUS_INVALID_Y_CB_CR_FORMAT) {
          GST_ELEMENT_ERROR (dec, RESOURCE, READ,
              ("Could not query VDPAU YCbCr capabilites"),
              ("Error returned from vdpau was: %s",
                  device->vdp_get_error_string (status)));

          return NULL;
        }
        if (is_supported) {
          GstCaps *format_caps;

          format_caps = gst_caps_new_simple ("video/x-raw-yuv",
              "format", GST_TYPE_FOURCC, formats[j].fourcc,
              "width", GST_TYPE_INT_RANGE, 1, max_w,
              "height", GST_TYPE_INT_RANGE, 1, max_h,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
          gst_caps_append (caps, format_caps);
        }
      }
    }
  }
  if (gst_caps_is_empty (caps)) {
    gst_caps_unref (caps);
    return NULL;
  }

  return caps;
}

static gboolean
gst_vdpau_decoder_init_vdpau (GstVdpauDecoder * dec)
{
  GstCaps *caps;

  dec->device = gst_vdpau_get_device (dec->display_name);

  caps = gst_vdpau_decoder_get_vdpau_support (dec);
  if (!caps)
    return FALSE;

  dec->src_caps = caps;

  return TRUE;
}

static GstStateChangeReturn
gst_vdpau_decoder_change_state (GstElement * element, GstStateChange transition)
{
  GstVdpauDecoder *dec;

  dec = GST_VDPAU_DECODER (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_vdpau_decoder_init_vdpau (dec))
        return GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      g_object_unref (dec->device);
      dec->device = NULL;
      break;
    default:
      break;
  }

  return GST_STATE_CHANGE_SUCCESS;
}

static gboolean
gst_vdpau_decoder_sink_set_caps (GstPad * pad, GstCaps * caps)
{
  GstVdpauDecoder *dec = GST_VDPAU_DECODER (GST_OBJECT_PARENT (pad));
  GstVdpauDecoderClass *dec_class = GST_VDPAU_DECODER_GET_CLASS (dec);

  GstCaps *src_caps, *new_caps;
  GstStructure *structure;
  gint width, height;
  gint framerate_numerator, framerate_denominator;
  gint par_numerator, par_denominator;
  guint32 fourcc_format;
  gboolean res;

  structure = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (structure, "width", &width);
  gst_structure_get_int (structure, "height", &height);
  gst_structure_get_fraction (structure, "framerate",
      &framerate_numerator, &framerate_denominator);
  gst_structure_get_fraction (structure, "pixel-aspect-ratio",
      &par_numerator, &par_denominator);

  src_caps = gst_pad_get_allowed_caps (dec->src);
  if (G_UNLIKELY (!src_caps))
    return FALSE;

  new_caps = gst_caps_copy_nth (src_caps, 0);
  gst_caps_unref (src_caps);
  structure = gst_caps_get_structure (new_caps, 0);
  gst_structure_get_fourcc (structure, "format", &fourcc_format);
  gst_structure_set (structure,
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      "framerate", GST_TYPE_FRACTION, framerate_numerator,
      framerate_denominator,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, par_numerator,
      par_denominator, NULL);

  gst_pad_fixate_caps (dec->src, new_caps);
  res = gst_pad_set_caps (dec->src, new_caps);

  gst_caps_unref (new_caps);

  if (G_UNLIKELY (!res))
    return FALSE;

  dec->width = width;
  dec->height = height;
  dec->framerate_numerator = framerate_numerator;
  dec->framerate_denominator = framerate_denominator;
  dec->format = fourcc_format;

  if (dec_class->set_caps && !dec_class->set_caps (dec, caps))
    return FALSE;

  return TRUE;
}

static GstCaps *
gst_vdpau_decoder_src_getcaps (GstPad * pad)
{
  GstVdpauDecoder *dec;

  dec = GST_VDPAU_DECODER (GST_OBJECT_PARENT (pad));

  if (GST_PAD_CAPS (dec->src))
    return gst_caps_ref (GST_PAD_CAPS (dec->src));

  if (dec->src_caps)
    return gst_caps_ref (dec->src_caps);

  return gst_caps_copy (gst_pad_get_pad_template_caps (dec->src));
}

/* GObject vmethod implementations */

static void
gst_vdpau_decoder_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_set_details_simple (element_class,
      "VdpauDecoder",
      "Codec/Decoder/Video",
      "VDPAU decoder base class",
      "Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
}

/* initialize the vdpaudecoder's class */
static void
gst_vdpau_decoder_class_init (GstVdpauDecoderClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_vdpau_decoder_finalize;
  gobject_class->set_property = gst_vdpau_decoder_set_property;
  gobject_class->get_property = gst_vdpau_decoder_get_property;

  g_object_class_install_property (gobject_class, PROP_DISPLAY,
      g_param_spec_string ("display", "Display", "X Display name",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  gstelement_class->change_state = gst_vdpau_decoder_change_state;
}

static void
gst_vdpau_decoder_init (GstVdpauDecoder * dec, GstVdpauDecoderClass * klass)
{
  dec->display_name = NULL;
  dec->device = NULL;
  dec->silent = FALSE;
  dec->src_caps = NULL;

  dec->height = 0;
  dec->width = 0;
  dec->framerate_numerator = 0;
  dec->framerate_denominator = 0;
  dec->format = 0;

  dec->frame_nr = 0;

  dec->src = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_set_getcaps_function (dec->src, gst_vdpau_decoder_src_getcaps);
  gst_element_add_pad (GST_ELEMENT (dec), dec->src);

  dec->sink = gst_pad_new_from_template (gst_element_class_get_pad_template
      (GST_ELEMENT_CLASS (klass), "sink"), "sink");
  gst_pad_set_setcaps_function (dec->sink, gst_vdpau_decoder_sink_set_caps);
  gst_element_add_pad (GST_ELEMENT (dec), dec->sink);
  gst_pad_set_active (dec->sink, TRUE);
}

static void
gst_vdpau_decoder_finalize (GObject * object)
{
  GstVdpauDecoder *dec = (GstVdpauDecoder *) object;

  if (dec->src_caps)
    g_object_unref (dec->src_caps);
  if (dec->device)
    g_object_unref (dec->device);

  g_free (dec->display_name);
}

static void
gst_vdpau_decoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVdpauDecoder *dec = GST_VDPAU_DECODER (object);

  switch (prop_id) {
    case PROP_DISPLAY:
      g_free (dec->display_name);
      dec->display_name = g_value_dup_string (value);
      break;
    case PROP_SILENT:
      dec->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_vdpau_decoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstVdpauDecoder *dec = GST_VDPAU_DECODER (object);

  switch (prop_id) {
    case PROP_DISPLAY:
      g_value_set_string (value, dec->display_name);
      break;
    case PROP_SILENT:
      g_value_set_boolean (value, dec->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
/* GStreamer RTP DTMF source
 *
 * gstrtpdtmfsrc.c:
 *
 * Copyright (C) <2007> Nokia Corporation.
 *   Contact: Zeeshan Ali <zeeshan.ali@nokia.com>
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *               2000,2005 Wim Taymans <wim@fluendo.com>
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

/**
 * SECTION:element-rtpdtmfsrc
 * @short_description: Generates RTP DTMF packets
 *
 * <refsect2>
 *
 * <para>
 * The RTPDTMFSrc element generates RTP DTMF (RFC 2833) event packets on request
 * from application. The application communicates the beginning and end of a
 * DTMF event using custom upstream gstreamer events. To report a DTMF event, an
 * application must send an event of type GST_EVENT_CUSTOM_UPSTREAM, having a
 * structure of name "dtmf-event" with fields set according to the following
 * table:
 * </para>
 *
 * <para>
 * <informaltable>
 * <tgroup cols='4'>
 * <colspec colname='Name' />
 * <colspec colname='Type' />
 * <colspec colname='Possible values' />
 * <colspec colname='Purpose' />
 * 
 * <thead>
 * <row>
 * <entry>Name</entry>
 * <entry>GType</entry>
 * <entry>Possible values</entry>
 * <entry>Purpose</entry>
 * </row>
 * </thead>
 * 
 * <tbody>
 * <row>
 * <entry>type</entry>
 * <entry>G_TYPE_INT</entry>
 * <entry>0-1</entry>
 * <entry>The application uses this field to specify which of the two methods
 * specified in RFC 2833 to use. The value should be 0 for tones and 1 for
 * named events. This element is only capable of generating named events.
 * </entry>
 * </row>
 * <row>
 * <entry>number</entry>
 * <entry>G_TYPE_INT</entry>
 * <entry>0-16</entry>
 * <entry>The event number.</entry>
 * </row>
 * <row>
 * <entry>volume</entry>
 * <entry>G_TYPE_INT</entry>
 * <entry>0-36</entry>
 * <entry>This field describes the power level of the tone, expressed in dBm0
 * after dropping the sign. Power levels range from 0 to -63 dBm0. The range of
 * valid DTMF is from 0 to -36 dBm0. Can be omitted if start is set to FALSE.
 * </entry>
 * </row>
 * <row>
 * <entry>start</entry>
 * <entry>G_TYPE_BOOLEAN</entry>
 * <entry>True or False</entry>
 * <entry>Whether the event is starting or ending.</entry>
 * </row>
 * <row>
 * <entry>method</entry>
 * <entry>G_TYPE_INT</entry>
 * <entry>1</entry>
 * <entry>The method used for sending event, this element will react if this field
 * is absent or 1.
 * </entry>
 * </row>
 * </tbody>
 * </tgroup>
 * </informaltable>
 * </para>
 * 
 * <para>For example, the following code informs the pipeline (and in turn, the
 * RTPDTMFSrc element inside the pipeline) about the start of an RTP DTMF named
 * event '1' of volume -25 dBm0:
 * </para>
 * 
 * <para>
 * <programlisting>
 * structure = gst_structure_new ("dtmf-event",
 *                    "type", G_TYPE_INT, 1,
 *                    "number", G_TYPE_INT, 1,
 *                    "volume", G_TYPE_INT, 25,
 *                    "start", G_TYPE_BOOLEAN, TRUE, NULL);
 * 
 * event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
 * gst_element_send_event (pipeline, event);
 * </programlisting>
 * </para>
 *
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "gstrtpdtmfsrc.h"

#define GST_RTP_DTMF_TYPE_EVENT  1
#define DEFAULT_PACKET_INTERVAL  50 /* ms */
#define MIN_PACKET_INTERVAL      10 /* ms */
#define MAX_PACKET_INTERVAL      50 /* ms */
#define DEFAULT_SSRC             -1
#define DEFAULT_PT               96
#define DEFAULT_TIMESTAMP_OFFSET -1
#define DEFAULT_SEQNUM_OFFSET    -1
#define DEFAULT_CLOCK_RATE       8000
#define MIN_EVENT                0
#define MAX_EVENT                16
#define MIN_EVENT_STRING         "0"
#define MAX_EVENT_STRING         "16"
#define MIN_VOLUME               0
#define MAX_VOLUME               36

#define DEFAULT_PACKET_REDUNDANCY 1
#define MIN_PACKET_REDUNDANCY 1
#define MAX_PACKET_REDUNDANCY 5

/* elementfactory information */
static const GstElementDetails gst_rtp_dtmf_src_details =
GST_ELEMENT_DETAILS ("RTP DTMF packet generator",
    "Source/Network",
    "Generates RTP DTMF packets",
    "Zeeshan Ali <zeeshan.ali@nokia.com>");

GST_DEBUG_CATEGORY_STATIC (gst_rtp_dtmf_src_debug);
#define GST_CAT_DEFAULT gst_rtp_dtmf_src_debug

/* signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SSRC,
  PROP_TIMESTAMP_OFFSET,
  PROP_SEQNUM_OFFSET,
  PROP_PT,
  PROP_CLOCK_RATE,
  PROP_TIMESTAMP,
  PROP_SEQNUM,
  PROP_INTERVAL,
  PROP_REDUNDANCY
};

static GstStaticPadTemplate gst_rtp_dtmf_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) [ 96, 127 ], "
        "clock-rate = (int) [ 0, MAX ], " 
        "ssrc = (int) [ 0, MAX ], " 
        "events = (int) [ " MIN_EVENT_STRING ", " MAX_EVENT_STRING " ], "
        "encoding-name = (string) \"telephone-event\"")
    );

static GstElementClass *parent_class = NULL;

static void gst_rtp_dtmf_src_base_init (gpointer g_class);
static void gst_rtp_dtmf_src_class_init (GstRTPDTMFSrcClass * klass);
static void gst_rtp_dtmf_src_init (GstRTPDTMFSrc * dtmfsrc, gpointer g_class);
static void gst_rtp_dtmf_src_finalize (GObject * object);

GType
gst_rtp_dtmf_src_get_type (void)
{
  static GType base_src_type = 0;

  if (G_UNLIKELY (base_src_type == 0)) {
    static const GTypeInfo base_src_info = {
      sizeof (GstRTPDTMFSrcClass),
      (GBaseInitFunc) gst_rtp_dtmf_src_base_init,
      NULL,
      (GClassInitFunc) gst_rtp_dtmf_src_class_init,
      NULL,
      NULL,
      sizeof (GstRTPDTMFSrc),
      0,
      (GInstanceInitFunc) gst_rtp_dtmf_src_init,
    };

    base_src_type = g_type_register_static (GST_TYPE_ELEMENT,
        "GstRTPDTMFSrc", &base_src_info, 0);
  }
  return base_src_type;
}

static void gst_rtp_dtmf_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_dtmf_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_rtp_dtmf_src_handle_event (GstPad * pad, GstEvent * event);
static GstStateChangeReturn gst_rtp_dtmf_src_change_state (GstElement * element,
    GstStateChange transition);
static void gst_rtp_dtmf_src_push_next_rtp_packet (GstRTPDTMFSrc *dtmfsrc);
static void gst_rtp_dtmf_src_start (GstRTPDTMFSrc *dtmfsrc, gint event_number,
    gint event_volume);
static void gst_rtp_dtmf_src_stop (GstRTPDTMFSrc *dtmfsrc);
static void gst_rtp_dtmf_src_set_caps (GstRTPDTMFSrc *dtmfsrc);

static void
gst_rtp_dtmf_src_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  
  GST_DEBUG_CATEGORY_INIT (gst_rtp_dtmf_src_debug,
          "dtmfsrc", 0, "dtmfsrc element");
  
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_rtp_dtmf_src_template));
  
  gst_element_class_set_details (element_class, &gst_rtp_dtmf_src_details);
}

static void
gst_rtp_dtmf_src_class_init (GstRTPDTMFSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_finalize);
  gobject_class->set_property = 
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_get_property);

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMESTAMP,
      g_param_spec_uint ("timestamp", "Timestamp",
          "The RTP timestamp of the last processed packet",
          0, G_MAXUINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SEQNUM,
      g_param_spec_uint ("seqnum", "Sequence number",
          "The RTP sequence number of the last processed packet",
          0, G_MAXUINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_TIMESTAMP_OFFSET, g_param_spec_int ("timestamp-offset",
          "Timestamp Offset",
          "Offset to add to all outgoing timestamps (-1 = random)", -1,
          G_MAXINT, DEFAULT_TIMESTAMP_OFFSET, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SEQNUM_OFFSET,
      g_param_spec_int ("seqnum-offset", "Sequence number Offset",
          "Offset to add to all outgoing seqnum (-1 = random)", -1, G_MAXINT,
          DEFAULT_SEQNUM_OFFSET, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CLOCK_RATE,
      g_param_spec_uint ("clock-rate", "clockrate",
          "The clock-rate at which to generate the dtmf packets",
          0, G_MAXUINT, DEFAULT_CLOCK_RATE, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SSRC,
      g_param_spec_uint ("ssrc", "SSRC",
          "The SSRC of the packets (-1 == random)",
          0, G_MAXUINT, DEFAULT_SSRC, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PT,
      g_param_spec_uint ("pt", "payload type",
          "The payload type of the packets",
          0, 0x80, DEFAULT_PT, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INTERVAL,
      g_param_spec_int ("interval", "Interval between rtp packets",
          "Interval in ms between two rtp packets", MIN_PACKET_INTERVAL,
          MAX_PACKET_INTERVAL, DEFAULT_PACKET_INTERVAL, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_REDUNDANCY,
      g_param_spec_int ("packet-redundancy", "Packet Redundancy",
          "Number of packets to send to indicate start and stop dtmf events",
          MIN_PACKET_REDUNDANCY, MAX_PACKET_REDUNDANCY,
          DEFAULT_PACKET_REDUNDANCY, G_PARAM_READWRITE));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_change_state);
}

static void
gst_rtp_dtmf_src_init (GstRTPDTMFSrc * dtmfsrc, gpointer g_class)
{
  dtmfsrc->srcpad =
      gst_pad_new_from_static_template (&gst_rtp_dtmf_src_template, "src");
  GST_DEBUG_OBJECT (dtmfsrc, "adding src pad");
  gst_element_add_pad (GST_ELEMENT (dtmfsrc), dtmfsrc->srcpad);

  gst_pad_set_event_function (dtmfsrc->srcpad, gst_rtp_dtmf_src_handle_event);
  
  dtmfsrc->ssrc = DEFAULT_SSRC;
  dtmfsrc->seqnum_offset = DEFAULT_SEQNUM_OFFSET;
  dtmfsrc->ts_offset = DEFAULT_TIMESTAMP_OFFSET;
  dtmfsrc->pt = DEFAULT_PT;
  dtmfsrc->clock_rate = DEFAULT_CLOCK_RATE;
  dtmfsrc->payload = NULL;
  dtmfsrc->interval = DEFAULT_PACKET_INTERVAL;
  dtmfsrc->packet_redundancy = DEFAULT_PACKET_REDUNDANCY;
  
  GST_DEBUG_OBJECT (dtmfsrc, "init done");
}

static void
gst_rtp_dtmf_src_finalize (GObject * object)
{
  GstRTPDTMFSrc *dtmfsrc;

  dtmfsrc = GST_RTP_DTMF_SRC (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_dtmf_src_handle_dtmf_event (GstRTPDTMFSrc *dtmfsrc,
        const GstStructure * event_structure)
{
  gint event_type;
  gboolean start;
  gint method;

  if (!gst_structure_get_int (event_structure, "type", &event_type) ||
          !gst_structure_get_boolean (event_structure, "start", &start) ||
          event_type != GST_RTP_DTMF_TYPE_EVENT)
    goto failure;

  if (gst_structure_get_int (event_structure, "method", &method)) {
    if (method != 1) {
      goto failure;
    }
  }

  if (start) {
    gint event_number;
    gint event_volume;

    if (!gst_structure_get_int (event_structure, "number", &event_number) ||
            !gst_structure_get_int (event_structure, "volume", &event_volume))
      goto failure;

    GST_DEBUG_OBJECT (dtmfsrc, "Received start event %d with volume %d",
            event_number, event_volume);
    gst_rtp_dtmf_src_start (dtmfsrc, event_number, event_volume);
  }

  else {
    GST_DEBUG_OBJECT (dtmfsrc, "Received stop event");
    gst_rtp_dtmf_src_stop (dtmfsrc);
  }

  return TRUE;
failure:
  return FALSE;
}

static gboolean
gst_rtp_dtmf_src_handle_custom_upstream (GstRTPDTMFSrc *dtmfsrc,
    GstEvent * event)
{
  gboolean result = FALSE;
  const GstStructure *structure;

  if (GST_STATE (dtmfsrc) != GST_STATE_PLAYING) {
    GST_DEBUG_OBJECT (dtmfsrc, "Received event while not in PLAYING state");
    goto ret;
  }

  GST_DEBUG_OBJECT (dtmfsrc, "Received event is of our interest");
  structure = gst_event_get_structure (event);
  if (structure && gst_structure_has_name (structure, "dtmf-event"))
    result = gst_rtp_dtmf_src_handle_dtmf_event (dtmfsrc, structure);

ret:
  return result;
}

static gboolean
gst_rtp_dtmf_src_handle_event (GstPad * pad, GstEvent * event)
{
  GstRTPDTMFSrc *dtmfsrc;
  gboolean result = FALSE;

  dtmfsrc = GST_RTP_DTMF_SRC (GST_PAD_PARENT (pad));

  GST_DEBUG_OBJECT (dtmfsrc, "Received an event on the src pad");
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      result = gst_rtp_dtmf_src_handle_custom_upstream (dtmfsrc, event);
      break;
    }
    /* Ideally this element should not be flushed but let's handle the event
     * just in case it is */
    case GST_EVENT_FLUSH_START:
      gst_rtp_dtmf_src_stop (dtmfsrc);
      result = TRUE;
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_segment_init (&dtmfsrc->segment, GST_FORMAT_UNDEFINED);
      break;
    case GST_EVENT_NEWSEGMENT:
      {
        gboolean update;
        gdouble rate;
        GstFormat fmt;
        gint64 start, stop, position;

        gst_event_parse_new_segment (event, &update, &rate, &fmt, &start,
            &stop, &position);
        gst_segment_set_newsegment (&dtmfsrc->segment, update, rate, fmt,
            start, stop, position);
      }
      /* fallthrough */
    default:
      result = gst_pad_event_default (pad, event);
      break;
  }

  gst_event_unref (event);
  return result;
}

static void
gst_rtp_dtmf_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRTPDTMFSrc *dtmfsrc;

  dtmfsrc = GST_RTP_DTMF_SRC (object);

  switch (prop_id) {
    case PROP_TIMESTAMP_OFFSET:
      dtmfsrc->ts_offset = g_value_get_int (value);
      break;
    case PROP_SEQNUM_OFFSET:
      dtmfsrc->seqnum_offset = g_value_get_int (value);
      break;
    case PROP_CLOCK_RATE:
      dtmfsrc->clock_rate = g_value_get_uint (value);
      gst_rtp_dtmf_src_set_caps (dtmfsrc);
      break;
    case PROP_SSRC:
      dtmfsrc->ssrc = g_value_get_uint (value);
      break;
    case PROP_PT:
      dtmfsrc->pt = g_value_get_uint (value);
      gst_rtp_dtmf_src_set_caps (dtmfsrc);
      break;
    case PROP_INTERVAL:
      dtmfsrc->interval = g_value_get_int (value);
      break;
    case PROP_REDUNDANCY:
      dtmfsrc->packet_redundancy = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_dtmf_src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstRTPDTMFSrc *dtmfsrc;

  dtmfsrc = GST_RTP_DTMF_SRC (object);

  switch (prop_id) {
    case PROP_TIMESTAMP_OFFSET:
      g_value_set_int (value, dtmfsrc->ts_offset);
      break;
    case PROP_SEQNUM_OFFSET:
      g_value_set_int (value, dtmfsrc->seqnum_offset);
      break;
    case PROP_CLOCK_RATE:
      g_value_set_uint (value, dtmfsrc->clock_rate);
      break;
    case PROP_SSRC:
      g_value_set_uint (value, dtmfsrc->ssrc);
      break;
    case PROP_PT:
      g_value_set_uint (value, dtmfsrc->pt);
      break;
    case PROP_TIMESTAMP:
      g_value_set_uint (value, dtmfsrc->rtp_timestamp);
      break;
    case PROP_SEQNUM:
      g_value_set_uint (value, dtmfsrc->seqnum);
      break;
    case PROP_INTERVAL:
      g_value_set_uint (value, dtmfsrc->interval);
      break;
    case PROP_REDUNDANCY:
      g_value_set_uint (value, dtmfsrc->packet_redundancy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_dtmf_src_set_stream_lock (GstRTPDTMFSrc *dtmfsrc, gboolean lock)
{
   GstEvent *event;
   GstStructure *structure;

   structure = gst_structure_new ("stream-lock",
                      "lock", G_TYPE_BOOLEAN, lock, NULL);

   event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM_OOB, structure);
   gst_pad_push_event (dtmfsrc->srcpad, event);
}

static void
gst_rtp_dtmf_prepare_timestamps (GstRTPDTMFSrc *dtmfsrc)
{
  GstClock *clock;

  clock = GST_ELEMENT_CLOCK (dtmfsrc);
  if (clock != NULL)
    dtmfsrc->timestamp = gst_clock_get_time (GST_ELEMENT_CLOCK (dtmfsrc));

  else {
    GST_ERROR_OBJECT (dtmfsrc, "No clock set for element %s",
        GST_ELEMENT_NAME (dtmfsrc));
    dtmfsrc->timestamp = GST_CLOCK_TIME_NONE;
  }

  dtmfsrc->rtp_timestamp = dtmfsrc->ts_base +
      gst_util_uint64_scale_int (
          gst_segment_to_running_time (&dtmfsrc->segment, GST_FORMAT_TIME,
              dtmfsrc->timestamp -
              gst_element_get_base_time (GST_ELEMENT (dtmfsrc))),
          dtmfsrc->clock_rate, GST_SECOND);
}

static void
gst_rtp_dtmf_src_start (GstRTPDTMFSrc *dtmfsrc,
        gint event_number, gint event_volume)
{
  g_return_if_fail (dtmfsrc->payload == NULL);

  dtmfsrc->payload = g_new0 (GstRTPDTMFPayload, 1);
  dtmfsrc->payload->event = CLAMP (event_number, MIN_EVENT, MAX_EVENT);
  dtmfsrc->payload->volume = CLAMP (event_volume, MIN_VOLUME, MAX_VOLUME);
  dtmfsrc->first_packet = TRUE;

  gst_rtp_dtmf_prepare_timestamps (dtmfsrc);
  gst_rtp_dtmf_src_set_caps (dtmfsrc);

  /* Don't forget to get exclusive access to the stream */
  gst_rtp_dtmf_src_set_stream_lock (dtmfsrc, TRUE);
  
  if (!gst_pad_start_task (dtmfsrc->srcpad,
      (GstTaskFunction) gst_rtp_dtmf_src_push_next_rtp_packet, dtmfsrc)) {
    GST_ERROR_OBJECT (dtmfsrc, "Failed to start task on src pad");
  }
}

static void
gst_rtp_dtmf_src_stop (GstRTPDTMFSrc *dtmfsrc)
{
  g_return_if_fail (dtmfsrc->payload != NULL);

  if (!gst_pad_pause_task (dtmfsrc->srcpad)) {
    GST_ERROR_OBJECT (dtmfsrc, "Failed to pause task on src pad");
    return;
  }

  /* Push the last packet with e-bit set */
  dtmfsrc->payload->e = 1;
  gst_rtp_dtmf_src_push_next_rtp_packet (dtmfsrc);

  /* Don't forget to release the stream lock */
  gst_rtp_dtmf_src_set_stream_lock (dtmfsrc, FALSE);
  
  g_free (dtmfsrc->payload);
  dtmfsrc->payload = NULL;
}

static void
gst_rtp_dtmf_src_wait_for_buffer_ts (GstRTPDTMFSrc *dtmfsrc, GstBuffer * buf)
{
  GstClock *clock;
  
  clock = GST_ELEMENT_CLOCK (dtmfsrc);
  if (clock != NULL) {
    GstClockID clock_id;
    GstClockReturn clock_ret;

    clock_id = gst_clock_new_single_shot_id (clock, dtmfsrc->timestamp);
    clock_ret = gst_clock_id_wait (clock_id, NULL);
    if (clock_ret != GST_CLOCK_OK && clock_ret != GST_CLOCK_EARLY) {
      GST_ERROR_OBJECT (dtmfsrc, "Failed to wait on clock %s",
              GST_ELEMENT_NAME (clock));
    }
    gst_clock_id_unref (clock_id);
  }

  else {
    GST_ERROR_OBJECT (dtmfsrc, "No clock set for element %s",
        GST_ELEMENT_NAME (dtmfsrc));
  }
}

static void
gst_rtp_dtmf_prepare_rtp_headers (GstRTPDTMFSrc *dtmfsrc, GstBuffer *buf)
{
  gst_rtp_buffer_set_ssrc (buf, dtmfsrc->current_ssrc);
  gst_rtp_buffer_set_payload_type (buf, dtmfsrc->pt);
  if (dtmfsrc->first_packet) {
    gst_rtp_buffer_set_marker (buf, TRUE);
    dtmfsrc->first_packet = FALSE;
  }
  dtmfsrc->seqnum++;
  gst_rtp_buffer_set_seq (buf, dtmfsrc->seqnum);
  
  /* timestamp of RTP header */
  gst_rtp_buffer_set_timestamp (buf, dtmfsrc->rtp_timestamp);
}

static void
gst_rtp_dtmf_prepare_buffer_data (GstRTPDTMFSrc *dtmfsrc, GstBuffer *buf)
{
  GstRTPDTMFPayload *payload;
  
  gst_rtp_dtmf_prepare_rtp_headers (dtmfsrc, buf);

  /* duration of DTMF payload */
  dtmfsrc->payload->duration +=
      dtmfsrc->interval * dtmfsrc->clock_rate / 1000;
  
  /* timestamp and duration of GstBuffer */ 
  GST_BUFFER_DURATION (buf) = dtmfsrc->interval * GST_MSECOND;
  GST_BUFFER_TIMESTAMP (buf) = dtmfsrc->timestamp;
  dtmfsrc->timestamp += GST_BUFFER_DURATION (buf);
  
  payload = (GstRTPDTMFPayload *) gst_rtp_buffer_get_payload (buf);
  
  /* copy payload and convert to network-byte order */
  g_memmove (payload, dtmfsrc->payload, sizeof (GstRTPDTMFPayload));
  payload->duration = g_htons (payload->duration);
}

static GstBuffer *
gst_rtp_dtmf_src_create_next_rtp_packet (GstRTPDTMFSrc *dtmfsrc)
{
  GstBuffer *buf = NULL;
  
  /* create buffer to hold the payload */
  buf = gst_rtp_buffer_new_allocate (sizeof (GstRTPDTMFPayload), 0, 0);

  gst_rtp_dtmf_prepare_buffer_data (dtmfsrc, buf);

  /* FIXME: Should we sync to clock ourselves or leave it to sink */
  gst_rtp_dtmf_src_wait_for_buffer_ts (dtmfsrc, buf);

  /* Set caps on the buffer before pushing it */
  gst_buffer_set_caps (buf, GST_PAD_CAPS (dtmfsrc->srcpad));

  return buf;
}

static void
gst_rtp_dtmf_src_push_next_rtp_packet (GstRTPDTMFSrc *dtmfsrc)
{
  GstBuffer *buf = NULL;
  GstFlowReturn ret;
  gint redundancy_count = 1;

  if (dtmfsrc->first_packet == TRUE || dtmfsrc->payload->e) {
    redundancy_count = dtmfsrc->packet_redundancy;

    if(dtmfsrc->first_packet == TRUE) {
      GST_DEBUG_OBJECT (dtmfsrc,
          "redundancy count set to %d due to dtmf start",
          redundancy_count);
    }
    if(dtmfsrc->payload->e) {
      GST_DEBUG_OBJECT (dtmfsrc,
          "redundancy count set to %d due to dtmf stop",
          redundancy_count);
    }

  }

  /* create buffer to hold the payload */
  buf = gst_rtp_dtmf_src_create_next_rtp_packet (dtmfsrc);

  while ( redundancy_count-- ) {
    gst_buffer_ref(buf);

    GST_DEBUG_OBJECT (dtmfsrc,
        "pushing buffer on src pad of size %d with redundancy count %d",
        GST_BUFFER_SIZE (buf), redundancy_count);
    ret = gst_pad_push (dtmfsrc->srcpad, buf);
    if (ret != GST_FLOW_OK)
      GST_ERROR_OBJECT (dtmfsrc,
          "Failed to push buffer on src pad", GST_BUFFER_SIZE (buf));
  }

  gst_buffer_unref(buf);
  GST_DEBUG_OBJECT (dtmfsrc,
          "pushed DTMF event '%d' on src pad", dtmfsrc->payload->event);
}

static void
gst_rtp_dtmf_src_set_caps (GstRTPDTMFSrc *dtmfsrc)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "payload", G_TYPE_INT, dtmfsrc->pt,
      "clock-rate", G_TYPE_INT, dtmfsrc->clock_rate,
      "encoding-name", G_TYPE_STRING, "telephone-event",
      "ssrc", G_TYPE_UINT, dtmfsrc->current_ssrc,
      "clock-base", G_TYPE_UINT, dtmfsrc->ts_base,
      "seqnum-base", G_TYPE_UINT, dtmfsrc->seqnum_base, NULL);

  if (!gst_pad_set_caps (dtmfsrc->srcpad, caps))
    GST_ERROR_OBJECT (dtmfsrc,
            "Failed to set caps %" GST_PTR_FORMAT " on src pad", caps);
  else
    GST_DEBUG_OBJECT (dtmfsrc,
            "caps %" GST_PTR_FORMAT " set on src pad", caps);

  gst_caps_unref (caps);
}

static void
gst_rtp_dtmf_src_ready_to_paused (GstRTPDTMFSrc *dtmfsrc)
{
  gst_segment_init (&dtmfsrc->segment, GST_FORMAT_UNDEFINED);
  
  if (dtmfsrc->ssrc == -1)
    dtmfsrc->current_ssrc = g_random_int ();
  else
    dtmfsrc->current_ssrc = dtmfsrc->ssrc;

  if (dtmfsrc->seqnum_offset == -1)
    dtmfsrc->seqnum_base = g_random_int_range (0, G_MAXUINT16);
  else
    dtmfsrc->seqnum_base = dtmfsrc->seqnum_offset;
  dtmfsrc->seqnum = dtmfsrc->seqnum_base;
  
  if (dtmfsrc->ts_offset == -1)
    dtmfsrc->ts_base = g_random_int ();
  else
    dtmfsrc->ts_base = dtmfsrc->ts_offset;
}

static GstStateChangeReturn
gst_rtp_dtmf_src_change_state (GstElement * element, GstStateChange transition)
{
  GstRTPDTMFSrc *dtmfsrc;
  GstStateChangeReturn result;
  gboolean no_preroll = FALSE;

  dtmfsrc = GST_RTP_DTMF_SRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_dtmf_src_ready_to_paused (dtmfsrc);
      /* Indicate that we don't do PRE_ROLL */
      no_preroll = TRUE;
      break;
    default:
      break;
  }

  if ((result =
          GST_ELEMENT_CLASS (parent_class)->change_state (element,
              transition)) == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      /* Indicate that we don't do PRE_ROLL */
      no_preroll = TRUE;
      break;
    default:
      break;
  }

  if (no_preroll && result == GST_STATE_CHANGE_SUCCESS)
    result = GST_STATE_CHANGE_NO_PREROLL;

  return result;

  /* ERRORS */
failure:
  {
    GST_ERROR_OBJECT (dtmfsrc, "parent failed state change");
    return result;
  }
}

static gboolean
gst_rtp_dtmf_src_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpdtmfsrc",
      GST_RANK_NONE, GST_TYPE_RTP_DTMF_SRC);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_rtp_dtmf_src_plugin_init (plugin))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "dtmf",
    "DTMF plugins",
    plugin_init, "0.1" , "LGPL", "DTMF", "");
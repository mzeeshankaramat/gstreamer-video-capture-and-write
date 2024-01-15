#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include "gstnvdsmeta.h"
#include <map>
#include <iostream>
#include <unordered_set>
#include <chrono>  

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
#define MUXER_OUTPUT_WIDTH 1920
#define MUXER_OUTPUT_HEIGHT 1080

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 40000
#define IGNORE_THRESHOLD_SEC 5  // 5 seconds threshold

std::unordered_set<int> initialObjectIds;
guint64 frameCount = 0;
// Declare these variables globally or in class scope
std::chrono::steady_clock::time_point start_time;
int frameCounter = 0;
const int FRAME_INTERVAL = 30;  // compute FPS every 30 frames for instance

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("ERROR from element %s: %s\n",
                       GST_OBJECT_NAME(msg->src), error->message);
            if (debug)
                g_printerr("Error details: %s\n", debug);
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

static GstPadProbeReturn osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info, gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *) info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

    for (NvDsMetaList * l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        frameCount++;
        if (frameCounter == 0) {
            start_time = std::chrono::steady_clock::now();
        }

        frameCounter++;

        if (frameCounter == FRAME_INTERVAL) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            double fps = 1000.0 * FRAME_INTERVAL / duration;  // converting duration to seconds and then computing FPS
            
            printf("Computed FPS: %f\n", fps);

            frameCounter = 0;  // reset counter
        }

        for (GList *elem = frame_meta->obj_meta_list; elem != NULL; elem = elem->next) {
            NvDsObjectMeta *obj = (NvDsObjectMeta *) elem->data;
            obj->text_params.display_text = NULL;  // Remove the display text.

            // In the first 400 frames (which is approx. 13.3 seconds for 30fps videos), record the object IDs.
            if (frameCount <= 30) {
                initialObjectIds.insert(obj->object_id);
            }

            // If the object was detected in the first 400 frames or it's not class 7, then hide the bounding box.
            if (initialObjectIds.count(obj->object_id) > 0 || obj->class_id != 7) {
                obj->rect_params.border_color.alpha = 0;
                continue;
            } 

            // Otherwise, for objects of class_id 7 detected after the first 400 frames:
            obj->rect_params.border_color.red = 1.0;
            obj->rect_params.border_color.green = 1.0;
            obj->rect_params.border_color.blue = 0.0;
            obj->rect_params.border_color.alpha = 1.0; // Make sure the box is visible
        }
    }
    return GST_PAD_PROBE_OK;
}

void cb_new_pad (GstElement *src, GstPad *new_pad, gpointer data) {
  GstElement *depay = (GstElement *)data;
  GstPad *sink_pad = gst_element_get_static_pad(depay, "sink");
  GstPadLinkReturn ret;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

  if (gst_pad_is_linked(sink_pad)) {
    g_print ("Sink pad from %s already linked. Ignoring.\n", GST_PAD_NAME(new_pad));
    goto exit;
  }

  ret = gst_pad_link(new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED(ret)) {
    g_printerr ("Failed to link pad '%s' to rtph264depay's sink pad.\n", GST_PAD_NAME(new_pad));
  } else {
    g_print ("Link succeeded with pad '%s'.\n", GST_PAD_NAME(new_pad));
  }

exit:
  gst_object_unref(sink_pad);
}


int main (int argc, char *argv[]){

  GMainLoop *loop = NULL;
  GstElement *pipeline = NULL, *source = NULL, *rtph264depay = NULL, *h264parser = NULL, *nvv4l2h264enc = NULL, *qtdemux = NULL,
             *nvv4l2decoder = NULL, *streammux = NULL, *sink = NULL, *nvvidconv = NULL, *qtmux = NULL,
             *pgie = NULL, *tracker = NULL, *nvvidconv2 = NULL, *nvosd = NULL, *h264parser2 = NULL, *fpsdisplaysink = NULL;

  GstElement *transform = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;

  /* Standard GStreamer initialization */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer elements */
  /* Create Pipeline element that will form a connection of other elements */
  pipeline = gst_pipeline_new ("deepstream_tutorial_app1");

  /* Input File source element */
  // source = gst_element_factory_make ("filesrc", "file-source");
  source = gst_element_factory_make ("rtspsrc", "rtsp-source");

  rtph264depay = gst_element_factory_make ("rtph264depay", "rtph264depay");

  /* Since the data format in the input file is elementary h264 stream,
   * we need a h264parser */
  h264parser = gst_element_factory_make ("h264parse", "h264-parser");

  /* Use nvdec_h264 for hardware accelerated decode on GPU */
  nvv4l2decoder = gst_element_factory_make ("nvv4l2decoder", "nvv4l2-decoder");

  /* Create nvstreammux instance to form batches from one or more sources. */
  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file */
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  /* Assigns track ids to detected bounding boxes*/
  tracker = gst_element_factory_make ("nvtracker", "tracker");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv2 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter2");

  fpsdisplaysink = gst_element_factory_make ("fpsdisplaysink", "fpsdisplaysink");

  /* Use convertor to convert from NV12 to H264 as required */
  nvv4l2h264enc = gst_element_factory_make ("nvv4l2h264enc", "nvv4l2h264enc");

  /* Since the data format for the output file is elementary h264 stream, * we need a h264parser */
  h264parser2 = gst_element_factory_make ("h264parse", "h264parser2");

  qtmux = gst_element_factory_make ("qtmux", "qtmux");

  sink = gst_element_factory_make ("filesink", "filesink");


    if (!pipeline || !source || !h264parser || !rtph264depay ||
      !nvv4l2decoder || !streammux || !pgie || !tracker || 
      !nvvidconv || !nvosd || !nvvidconv2 || !fpsdisplaysink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }


  /* we set the input filename to the source element */
  g_object_set (
    G_OBJECT (source), 
    "location", 
    "rtsp://admin:BTS123456@192.168.100.99:554", 
    NULL
  );

  g_object_set (
    G_OBJECT (streammux), 
    "batch-size", 
    1, 
    "width", 
    MUXER_OUTPUT_WIDTH, 
    "height",
    MUXER_OUTPUT_HEIGHT,
    "batched-push-timeout", 
    MUXER_BATCH_TIMEOUT_USEC, NULL
  );

  g_object_set (
      G_OBJECT (pgie),
      "config-file-path", 
      "/home/zeeshan/Angler/deepstream_yolo_nvdcf/config_infer_primary_yoloV7.txt",
      NULL
    );
  g_object_set(G_OBJECT(pgie), "process-mode", 1, NULL);

  g_object_set(G_OBJECT(tracker), "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so", NULL);

  /* Set output file location */
  g_object_set (
      G_OBJECT (sink),
      "location", 
      "output.mp4", 
      NULL
    );

  g_object_set (
      G_OBJECT (fpsdisplaysink),
      "sync", 
      false, 
      NULL
    );

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);


  /* Set up the pipeline */
  /* we add all elements into the pipeline */
  gst_bin_add_many (
    GST_BIN (pipeline),
    source,
    rtph264depay,
    h264parser,
    nvv4l2decoder,
    streammux, 
    pgie,
    tracker,
    nvvidconv, 
    nvosd,
    nvvidconv2,
    fpsdisplaysink,
    NULL
  );

  /* Connect the pad-added signal for the rtspsrc */
  g_signal_connect (source, "pad-added", G_CALLBACK (cb_new_pad), rtph264depay);

  /* Link rtph264depay to h264parser */
  if (!gst_element_link (rtph264depay, h264parser)) {
  g_printerr ("Failed to link rtph264depay to h264parser.\n");
  return -1;
  }

  GstPad *osd_sink_pad = NULL;
  osd_sink_pad = gst_element_get_static_pad (nvosd, "sink");
  if (!osd_sink_pad)
    g_print ("Unable to get sink pad\n");
  else
    gst_pad_add_probe (osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_sink_pad_buffer_probe, NULL, NULL);

  GstPad *sinkpad, *srcpad;
  gchar pad_name_sink[16] = "sink_0";
  gchar pad_name_src[16] = "src";

  /* Dynamically created pad */
  sinkpad = gst_element_get_request_pad (streammux, pad_name_sink);
  if (!sinkpad) {
    g_printerr ("Streammux request sink pad failed. Exiting.\n");
    return -1;
  }

  /* Statically created pad */
  srcpad = gst_element_get_static_pad (nvv4l2decoder, pad_name_src);
  if (!srcpad) {
    g_printerr ("Decoder request src pad failed. Exiting.\n");
    return -1;
  }

  /* Linking the pads */
  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link decoder to stream muxer. Exiting.\n");
      return -1;
  }

  /* Unreference the object */
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);
  gst_object_unref (osd_sink_pad);



  if (!gst_element_link_many (h264parser, nvv4l2decoder, NULL)) {
    g_printerr ("H264Parse and NvV4l2-Decoder could not be linked: 2. Exiting.\n");
    return -1;
  }

  if (!gst_element_link_many (streammux, pgie, tracker, nvvidconv, nvosd, nvvidconv2, fpsdisplaysink, NULL)) {
    g_printerr ("Rest of the pipeline elements could not be linked: 3. Exiting.\n");
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Using file: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait till pipeline encounters an error or EOS */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;
}

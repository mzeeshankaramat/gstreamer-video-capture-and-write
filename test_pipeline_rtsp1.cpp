#include <gst/gst.h>
#include <glib.h>

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

void cb_new_pad (GstElement *rtph264depay, GstPad* pad, gpointer data) {
  GstElement* h264parser = (GstElement*) data;
  gchar *name = gst_pad_get_name (pad);
  if (strcmp (name, "video_0") == 0 && 
      !gst_element_link_pads(rtph264depay, name, h264parser, "sink")){
    g_printerr ("Could not link %s pad of rtph264depay to sink pad of h264parser", name);
  }
}

/* Handler for the pad-added signal */
static void pad_added_handler(GstElement *src, GstPad *new_pad, GstElement *sink) {
    GstPad *sink_pad = gst_element_get_static_pad(sink, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* Check if the sink pad is already linked */
    if (gst_pad_is_linked(sink_pad)) {
        g_print("Pad already linked. Ignoring.\n");
        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);

    /* Check if it's the right type */
    if (!g_str_has_prefix(new_pad_type, "application/x-rtp")) {
        g_print("It's not an RTP pad. Ignoring.\n");
        goto exit;
    }

    /* Attempt to link */
    ret = gst_pad_link(new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret)) {
        g_print("Type is '%s' but link failed.\n", new_pad_type);
    } else {
        g_print("Link succeeded (type '%s').\n", new_pad_type);
    }

    exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
    gst_caps_unref(new_pad_caps);
}

int main(int argc, char *argv[]) {
    GMainLoop *loop;

    GstElement *pipeline, *source, *rtph264depay, *h264parse, *decoder, *streammux, *nvinfer, *nvvidconv1, *nvosd, *nvvidconv2, *tee, *queue1, *queue2, *fpsdisplaysink, *encoder, *h264parser, *splitmuxsink;
    GstBus *bus;
    GstCaps *caps;
    guint bus_watch_id;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    // Create GStreamer elements
    pipeline = gst_pipeline_new("rtsp-pipeline");
    source = gst_element_factory_make("rtspsrc", "source");
    rtph264depay = gst_element_factory_make("rtph264depay", "rtph264depay");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    decoder = gst_element_factory_make("nvv4l2decoder", "decoder");
    streammux = gst_element_factory_make("nvstreammux", "streammux");
    nvinfer = gst_element_factory_make("nvinfer", "nvinfer");
    nvvidconv1 = gst_element_factory_make("nvvideoconvert", "nvvidconv1");
    nvosd = gst_element_factory_make("nvdsosd", "nvosd");
    nvvidconv2 = gst_element_factory_make("nvvideoconvert", "nvvidconv2");
    tee = gst_element_factory_make("tee", "tee");
    queue1 = gst_element_factory_make("queue", "queue1");
    queue2 = gst_element_factory_make("queue", "queue2");
    fpsdisplaysink = gst_element_factory_make("fpsdisplaysink", "fpsdisplaysink");
    encoder = gst_element_factory_make("nvv4l2h264enc", "encoder");
    h264parser = gst_element_factory_make("h264parse", "h264parser");
    splitmuxsink = gst_element_factory_make("splitmuxsink", "splitmuxsink");

    if (!pipeline || !source || !rtph264depay || !h264parse || !decoder || !streammux || !nvinfer || !nvvidconv1 || !nvosd || !nvvidconv2 || !tee || !queue1 || !queue2 || !fpsdisplaysink || !encoder || !h264parser || !splitmuxsink) {
        g_printerr("One or more elements could not be created.\n");
        return -1;
    }

    /* Set up the pipeline */
    // Set the source properties
    g_object_set(G_OBJECT(source), "location", "rtsp://admin:BTS123456@192.168.1.64:554", NULL);

    // Configure the streammux element
    g_object_set(G_OBJECT(streammux), "width", 640, "height", 480, "batch-size", 1, "batched-push-timeout", 4000000, NULL);

    // Configure nvinfer properties
    g_object_set(G_OBJECT(nvinfer), "config-file-path", "/home/zeeshan/Angler/deepstream_yolo_nvdcf/config_infer_primary_yoloV7.txt", NULL);

    /* Add and link elements */
    gst_bin_add_many(GST_BIN(pipeline), source, rtph264depay, h264parse, decoder, streammux, nvinfer, nvvidconv1, nvosd, nvvidconv2, tee, queue1, queue2, fpsdisplaysink, encoder, h264parser, splitmuxsink, NULL);

    /* Linking the dynamic pad of the source element */

    
    g_signal_connect(source, "pad-added", G_CALLBACK(pad_added_handler), rtph264depay);

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
    srcpad = gst_element_get_static_pad (decoder, pad_name_src);
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

    // Link the elements together
    if (!gst_element_link_many(rtph264depay, h264parse, decoder, streammux, nvinfer, nvvidconv1, nvosd, nvvidconv2, tee, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Linking the dynamic pad of the source element
    g_signal_connect(source, "pad-added", G_CALLBACK(pad_added_handler), rtph264depay);

    // Linking the tee to the queues
    if (!gst_element_link_many(tee, queue1, NULL)) {
        g_printerr("Error linking tee to queue1\n");
        gst_object_unref(pipeline);
        return -1;
    }
    if (!gst_element_link_many(tee, queue2, NULL)) {
        g_printerr("Error linking tee to queue2\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Set caps for the queue1 branch
    caps = gst_caps_new_simple("video/x-raw",
                            "width", G_TYPE_INT, 1920,
                            "height", G_TYPE_INT, 1080,
                            "format", G_TYPE_STRING, "I420",
                            NULL);
    if (!gst_element_link_filtered(queue1, nvvidconv1, caps)) {
        g_printerr("Error linking queue1 to nvvidconv1 with caps\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_caps_unref(caps);

    // Link the remaining elements
    if (!gst_element_link_many(nvvidconv1, encoder, h264parser, splitmuxsink, NULL)) {
        g_printerr("Error linking nvvidconv1 to splitmuxsink\n");
        gst_object_unref(pipeline);
        return -1;
    }
    if (!gst_element_link(queue2, fpsdisplaysink)) {
        g_printerr("Error linking queue2 to fpsdisplaysink\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Start playing */
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Add a bus watch */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* Run the main loop */
    g_print("Running...\n");
    g_main_loop_run(loop);

    /* Clean up */
    g_print("Exiting...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;

}
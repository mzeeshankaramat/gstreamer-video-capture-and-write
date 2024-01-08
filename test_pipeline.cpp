#include <gst/gst.h>
#include <iostream>

// Callback function for pad-added signal of rtspsrc
static void cb_new_pad(GstElement *element, GstPad *pad, gpointer data) {
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *)data;

    /* We can now link this pad with the decoder sink pad */
    g_print("Dynamic pad created, linking source/demuxer\n");

    sinkpad = gst_element_get_static_pad(decoder, "sink");

    if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link pads\n");
    }

    gst_object_unref(sinkpad);
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *tee_raw, *queue_raw, *filesink_raw, *nvinfer, *nvtracker, *nvvideoconvert, *nvdsosd, *tee_infer, *queue_display, *sink, *queue_infer, *filesink_infer;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    source = gst_element_factory_make("rtspsrc", "source");
    tee_raw = gst_element_factory_make("tee", "tee_raw");
    queue_raw = gst_element_factory_make("queue", "queue_raw");
    filesink_raw = gst_element_factory_make("filesink", "filesink_raw");
    nvinfer = gst_element_factory_make("nvinfer", "nvinfer");
    nvtracker = gst_element_factory_make("nvtracker", "nvtracker");
    nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    nvdsosd = gst_element_factory_make("nvdsosd", "nosd");
    tee_infer = gst_element_factory_make("tee", "tee_infer");
    queue_display = gst_element_factory_make("queue", "queue_display");
    sink = gst_element_factory_make("autovideosink", "sink");
    queue_infer = gst_element_factory_make("queue", "queue_infer");
    filesink_infer = gst_element_factory_make("filesink", "filesink_infer");


    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("rtsp-inference-pipeline");

    if (!pipeline || !source || !tee_raw || !queue_raw || !filesink_raw || !nvinfer || !nvtracker || !nvvideoconvert || !nvdsosd || !tee_infer || !queue_display || !sink || !queue_infer || !filesink_infer) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    g_signal_connect (source, "pad-added", G_CALLBACK (cb_new_pad), tee_raw);

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, tee_raw, queue_raw, filesink_raw, nvinfer, nvtracker, nvvideoconvert, nvdsosd, tee_infer, queue_display, sink, queue_infer, filesink_infer, NULL);

    if (!gst_element_link_many(source, tee_raw, NULL) ||
        !gst_element_link_many(queue_raw, filesink_raw, NULL)) {
        g_printerr("Raw video elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Link elements for inference pipeline */
    if (!gst_element_link_many(tee_raw, nvinfer, nvtracker, nvvideoconvert, nvdsosd, tee_infer, NULL) ||
        !gst_element_link_many(queue_display, sink, NULL) ||
        !gst_element_link_many(queue_infer, filesink_infer, NULL)) {
        g_printerr("Inference elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Manually link the Tee, which has request pads */
    
    // Linking tee_raw
    GstPadTemplate *tee_raw_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee_raw), "src_%u");
    GstPad *tee_raw_pad_to_queue_raw = gst_element_request_pad(tee_raw, tee_raw_src_pad_template, NULL, NULL);
    GstPad *tee_raw_pad_to_nvinfer = gst_element_request_pad(tee_raw, tee_raw_src_pad_template, NULL, NULL);
    GstPad *queue_raw_sink_pad = gst_element_get_static_pad(queue_raw, "sink");
    GstPad *nvinfer_sink_pad = gst_element_get_static_pad(nvinfer, "sink");

    if (gst_pad_link(tee_raw_pad_to_queue_raw, queue_raw_sink_pad) != GST_PAD_LINK_OK ||
        gst_pad_link(tee_raw_pad_to_nvinfer, nvinfer_sink_pad) != GST_PAD_LINK_OK) {
        g_printerr("tee_raw could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_object_unref(queue_raw_sink_pad);
    gst_object_unref(nvinfer_sink_pad);

    // Linking tee_infer
    GstPadTemplate *tee_infer_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee_infer), "src_%u");
    GstPad *tee_infer_pad_to_queue_display = gst_element_request_pad(tee_infer, tee_infer_src_pad_template, NULL, NULL);
    GstPad *tee_infer_pad_to_queue_infer = gst_element_request_pad(tee_infer, tee_infer_src_pad_template, NULL, NULL);
    GstPad *queue_display_sink_pad = gst_element_get_static_pad(queue_display, "sink");
    GstPad *queue_infer_sink_pad = gst_element_get_static_pad(queue_infer, "sink");

    if (gst_pad_link(tee_infer_pad_to_queue_display, queue_display_sink_pad) != GST_PAD_LINK_OK ||
        gst_pad_link(tee_infer_pad_to_queue_infer, queue_infer_sink_pad) != GST_PAD_LINK_OK) {
        g_printerr("tee_infer could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_object_unref(queue_display_sink_pad);
    gst_object_unref(queue_infer_sink_pad);

    /* Set up the pipeline */
    g_object_set(G_OBJECT(source), "location", "rtsp://your_rtsp_stream", NULL);
    g_object_set(G_OBJECT(nvinfer), "config-file-path", "/home/vresolv/deepstream_yolo_nvdcf/config_infer_primary_yoloV7.txt", NULL);
    g_object_set(G_OBJECT(nvtracker), "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so", NULL);
    g_object_set(G_OBJECT(filesink_raw), "location", "rawVideo.mp4", NULL);
    g_object_set(G_OBJECT(filesink_infer), "location", "inferVideo.mp4", NULL);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));


    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}

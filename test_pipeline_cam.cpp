#include <gst/gst.h>
#include <iostream>

#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *videoconvert, *nvvideoconvert, *streammux, *nvinfer, *nvdsosd, *sink, *tee, *queue_raw, *filesink_raw;
    GstPad *tee_raw_pad, *queue_raw_pad;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    source = gst_element_factory_make("v4l2src", "source");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    streammux = gst_element_factory_make("nvstreammux", "streammux");
    nvinfer = gst_element_factory_make("nvinfer", "nvinfer");
    nvdsosd = gst_element_factory_make("nvdsosd", "nvdsosd");
    sink = gst_element_factory_make("autovideosink", "sink");
    tee = gst_element_factory_make("tee", "tee");
    queue_raw = gst_element_factory_make("queue", "queue_raw");
    filesink_raw = gst_element_factory_make("filesink", "filesink_raw");

    /* Check if all elements are created */
    if (!pipeline || !source || !videoconvert || !nvvideoconvert || !streammux || !nvinfer || !nvdsosd || !sink || !tee || !queue_raw || !filesink_raw) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("webcam-inference-pipeline");

    /* Set up the pipeline */
    // g_object_set(G_OBJECT(streammux), "width", 1280, "height", 720, "batch-size", 1, "batched-push-timeout", 4000000, NULL);
    // g_object_set(G_OBJECT(nvinfer), "config-file-path", "/path/to/your/config_infer_primary.txt", NULL);
    // g_object_set(G_OBJECT(filesink_raw), "location", "rawVideo.mp4", NULL);

    g_object_set(G_OBJECT(streammux), "width", 1280, "height", 720, "batch-size", 1, "batched-push-timeout", 4000000, NULL);
    g_object_set(G_OBJECT(nvinfer), "config-file-path", "/home/zeeshan/Angler/deepstream_yolo_nvdcf/config_infer_primary_yoloV7.txt", NULL);
    // g_object_set(G_OBJECT(nvtracker), "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so", NULL);
    g_object_set(G_OBJECT(filesink_raw), "location", "rawVideo.mp4", NULL);
    // g_object_set(G_OBJECT(filesink_infer), "location", "inferVideo.mp4", NULL);

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, nvvideoconvert, streammux, nvinfer, nvdsosd, tee, queue_raw, filesink_raw, sink, NULL);

    /* Link the elements together */
    if (!gst_element_link_many(source, videoconvert, nvvideoconvert, streammux, nvinfer, nvdsosd, tee, NULL) ||
        !gst_element_link_many(queue_raw, filesink_raw, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Manually link the tee
    tee_raw_pad = gst_element_get_request_pad(tee, "src_%u");
    queue_raw_pad = gst_element_get_static_pad(queue_raw, "sink");
    if (gst_pad_link(tee_raw_pad, queue_raw_pad) != GST_PAD_LINK_OK) {
        g_printerr("Tee could not be linked to queue.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Link tee to sink
    if (!gst_element_link_many(tee, sink, NULL)) {
        g_printerr("Tee could not be linked to sink.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_object_unref(queue_raw_pad);

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


// int main(int argc, char *argv[]) {
//     GstElement *pipeline, *source, *videoconvert, *nvvideoconvert1, *streammux, *nvinfer, *nvtracker, *nvvideoconvert2, *nvdsosd, *nvvideoconvert3, *videoconvert2, *sink, *tee_raw, *tee_infer, *queue_raw, *queue_infer, *filesink_raw, *filesink_infer;

//     /* Initialize GStreamer */
//     gst_init(&argc, &argv);

//     /* Create the elements */
//     source = gst_element_factory_make("v4l2src", "source");
//     videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
//     nvvideoconvert1 = gst_element_factory_make("nvvideoconvert", "nvvideoconvert1");
//     streammux = gst_element_factory_make("nvstreammux", "streammux");
//     nvinfer = gst_element_factory_make("nvinfer", "nvinfer");
//     nvtracker = gst_element_factory_make("nvtracker", "nvtracker");
//     nvvideoconvert2 = gst_element_factory_make("nvvideoconvert", "nvvideoconvert2");
//     nvdsosd = gst_element_factory_make("nvdsosd", "nvdsosd");
//     nvvideoconvert3 = gst_element_factory_make("nvvideoconvert", "nvvideoconvert3");
//     videoconvert2 = gst_element_factory_make("videoconvert", "videoconvert2");
//     sink = gst_element_factory_make("autovideosink", "sink");
//     tee_raw = gst_element_factory_make("tee", "tee_raw");
//     tee_infer = gst_element_factory_make("tee", "tee_infer");
//     queue_raw = gst_element_factory_make("queue", "queue_raw");
//     queue_infer = gst_element_factory_make("queue", "queue_infer");
//     filesink_raw = gst_element_factory_make("filesink", "filesink_raw");
//     filesink_infer = gst_element_factory_make("filesink", "filesink_infer");

//     /* Check if all elements are created */
//     if (!pipeline || !source || !videoconvert || !nvvideoconvert1 || !streammux || !nvinfer || !nvtracker || !nvvideoconvert2 || !nvdsosd || !nvvideoconvert3 || !videoconvert2 || !sink || !tee_raw || !tee_infer || !queue_raw || !queue_infer || !filesink_raw || !filesink_infer) {
//         g_printerr("Not all elements could be created.\n");
//         return -1;
//     }

//     /* Create the empty pipeline */
//     pipeline = gst_pipeline_new("webcam-inference-pipeline");

//     /* Build the pipeline */
//     gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, nvvideoconvert1, streammux, nvinfer, nvtracker, nvvideoconvert2, nvdsosd, nvvideoconvert3, videoconvert2, tee_infer, queue_infer, filesink_infer, tee_raw, queue_raw, filesink_raw, sink, NULL);

//     // Initial linking
//     if (!gst_element_link_many(source, videoconvert, nvvideoconvert1, NULL) ||
//         !gst_element_link_many(nvvideoconvert2, nvdsosd, nvvideoconvert3, videoconvert2, tee_infer, NULL)) {
//         g_printerr("Initial elements could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Link streammux to nvinfer
//     if (!gst_element_link(streammux, nvinfer)) {
//         g_printerr("streammux and nvinfer could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Link nvinfer to nvtracker
//     if (!gst_element_link(nvinfer, nvtracker)) {
//         g_printerr("nvinfer and nvtracker could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Link nvtracker to nvvideoconvert2
//     if (!gst_element_link(nvtracker, nvvideoconvert2)) {
//         g_printerr("nvtracker and nvvideoconvert2 could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Manually link the tee_infer
//     GstPadTemplate *tee_infer_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee_infer), "src_%u");
//     GstPad *tee_infer_pad_to_queue_infer = gst_element_request_pad(tee_infer, tee_infer_src_pad_template, NULL, NULL);
//     GstPad *queue_infer_sink_pad = gst_element_get_static_pad(queue_infer, "sink");

//     if (gst_pad_link(tee_infer_pad_to_queue_infer, queue_infer_sink_pad) != GST_PAD_LINK_OK) {
//         g_printerr("tee_infer could not be linked to queue_infer.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     gst_object_unref(queue_infer_sink_pad);

//     // Link queue_infer to filesink_infer
//     if (!gst_element_link_many(queue_infer, filesink_infer, NULL)) {
//         g_printerr("queue_infer and filesink_infer could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Manually link the tee_raw
//     GstPadTemplate *tee_raw_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee_raw), "src_%u");
//     GstPad *tee_raw_pad_to_queue_raw = gst_element_request_pad(tee_raw, tee_raw_src_pad_template, NULL, NULL);
//     GstPad *queue_raw_sink_pad = gst_element_get_static_pad(queue_raw, "sink");

//     if (gst_pad_link(tee_raw_pad_to_queue_raw, queue_raw_sink_pad) != GST_PAD_LINK_OK) {
//         g_printerr("tee_raw could not be linked to queue_raw.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     gst_object_unref(queue_raw_sink_pad);

//     // Link queue_raw to filesink_raw
//     if (!gst_element_link_many(queue_raw, filesink_raw, NULL)) {
//         g_printerr("queue_raw and filesink_raw could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Link tee_infer to sink
//     if (!gst_element_link_many(tee_infer, sink, NULL)) {
//         g_printerr("tee_infer and sink could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     /* Set up the pipeline */
//     // Set properties for the elements
//     g_object_set(G_OBJECT(streammux), "width", 1280, "height", 720, "batch-size", 1, "batched-push-timeout", 4000000, NULL);
//     g_object_set(G_OBJECT(nvinfer), "config-file-path", "/home/zeeshan/Angler/deepstream_yolo_nvdcf/config_infer_primary_yoloV7.txt", NULL);
//     g_object_set(G_OBJECT(nvtracker), "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so", NULL);
//     g_object_set(G_OBJECT(filesink_raw), "location", "rawVideo.mp4", NULL);
//     g_object_set(G_OBJECT(filesink_infer), "location", "inferVideo.mp4", NULL);

//     /* Start playing */
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     /* Wait until error or EOS */
//     GstBus *bus = gst_element_get_bus(pipeline);
//     GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

//     /* Free resources */
//     if (msg != NULL)
//         gst_message_unref(msg);
//     gst_object_unref(bus);
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     gst_object_unref(pipeline);

//     return 0;
// }

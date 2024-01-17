#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include "gstnvdsmeta.h"
#include <map>
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickImageProvider>
#include <QImage>
#include <QDebug>


#define MUXER_OUTPUT_WIDTH 1920
#define MUXER_OUTPUT_HEIGHT 1080


#define MUXER_BATCH_TIMEOUT_USEC 40000
#define IGNORE_THRESHOLD_SEC 5  // 5 seconds threshold

std::unordered_set<int> initialObjectIds;
guint64 frameCount = 0;
// Declare these variables globally or in class scope
std::chrono::steady_clock::time_point start_time;
int frameCounter = 0;
const int FRAME_INTERVAL = 30;  // compute FPS every 30 frames for instance

class VideoFrameImageProvider : public QQuickImageProvider {
public:
    VideoFrameImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    void updateFrame(const QImage &frame);
private:
    QImage currentFrame;
};

VideoFrameImageProvider::VideoFrameImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) 
{
    QImage whiteImage(1920, 1080, QImage::Format_RGB888);
    whiteImage.fill(Qt::black);
    currentFrame = whiteImage;
}

QImage VideoFrameImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) 
{
    return currentFrame;
}

void VideoFrameImageProvider::updateFrame(const QImage &frame) 
{
    currentFrame = frame;
    // Emit a signal or use some mechanism to notify QML to update the image.
}

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

            if (frameCount <= 30) {
                initialObjectIds.insert(obj->object_id);
            }

            if (initialObjectIds.count(obj->object_id) > 0 || obj->class_id != 7) {
                obj->rect_params.border_color.alpha = 0;
                continue;
            } 

            obj->rect_params.border_color.red = 1.0;
            obj->rect_params.border_color.green = 1.0;
            obj->rect_params.border_color.blue = 0.0;
            obj->rect_params.border_color.alpha = 1.0; // Make sure the box is visible
        }
    }
    return GST_PAD_PROBE_OK;
}

void cb_new_pad (GstElement *src, GstPad *new_pad, gpointer data) 
{
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

/* Callback function for new samples */
static GstFlowReturn new_sample(GstElement *sink, VideoFrameImageProvider *provider) 
{
    // qDebug() << "new_sample_callback";
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    QImage image;
    GstFlowReturn ret;

    // Retrieve the buffer
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) 
    {
        return GST_FLOW_ERROR;
    }

    // Get the buffer data
    buffer = gst_sample_get_buffer(sample);
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    image = QImage(map.data, 1920, 1080, QImage::Format_RGB888);
    image = image.rgbSwapped();
    provider->updateFrame(image);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}


int main (int argc, char *argv[]){

  GMainLoop *loop = NULL;
  GstElement *pipeline = NULL, *source = NULL, *rtph264depay = NULL, *h264parser = NULL, *nvv4l2h264enc = NULL, *qtdemux = NULL,
             *nvv4l2decoder = NULL, *streammux = NULL, *sink = NULL, *nvvidconv = NULL, *qtmux = NULL,
             *pgie = NULL, *tracker = NULL, *nvvidconv2 = NULL, *nvosd = NULL, *h264parser2 = NULL, *videoconv = NULL, *appsink = NULL, *tee1 =NULL, *splitmuxsink = NULL, *queue1 = NULL, *queue2 = NULL, *nvvidconv3 = NULL;

  GstElement *transform = NULL;
  GstBus *bus = NULL;
  guint bus_watch_id;
  GstCaps *caps,*caps1, *caps2, *caps3;

  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  QGuiApplication app(argc, argv);

  QQmlApplicationEngine engine;
  VideoFrameImageProvider *provider = new VideoFrameImageProvider();
  engine.addImageProvider(QLatin1String("videoframes"), provider);

  engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

  if (engine.rootObjects().isEmpty())
      return -1;

  pipeline = gst_pipeline_new ("deepstream_tutorial_app1");

  source = gst_element_factory_make ("rtspsrc", "rtsp-source");

  rtph264depay = gst_element_factory_make ("rtph264depay", "rtph264depay");

  h264parser = gst_element_factory_make ("h264parse", "h264-parser");

  nvv4l2decoder = gst_element_factory_make ("nvv4l2decoder", "nvv4l2-decoder");

  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  tracker = gst_element_factory_make ("nvtracker", "tracker");

  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  nvvidconv2 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter2");

  nvvidconv3 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter3");

  videoconv = gst_element_factory_make("videoconvert", "video-convert");

  appsink = gst_element_factory_make ("appsink", "appsink");

  nvv4l2h264enc = gst_element_factory_make ("nvv4l2h264enc", "nvv4l2h264enc");

  h264parser2 = gst_element_factory_make ("h264parse", "h264parser2");

  qtmux = gst_element_factory_make ("qtmux", "qtmux");

  sink = gst_element_factory_make ("filesink", "filesink");

  tee1 = gst_element_factory_make("tee", "tee");

  queue1 = gst_element_factory_make("queue", "queue1");
  queue2 = gst_element_factory_make("queue", "queue2");

  splitmuxsink = gst_element_factory_make("splitmuxsink", "splitmuxsink");


    if (!pipeline || !source || !h264parser || !rtph264depay ||
      !nvv4l2decoder || !streammux || !pgie || !tracker || !nvv4l2h264enc || !h264parser2 ||
      !nvvidconv || !nvosd || !nvvidconv2 || !nvvidconv3 || !videoconv || !appsink || !tee1 || !queue1 || !queue2) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

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

  g_object_set(splitmuxsink,
    "location", "output_%05d.mp4",
    "max-size-time", (guint64)(10 * GST_SECOND),
    NULL);

  g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);

  g_object_set(splitmuxsink, "location", "output_%05d.mp4", NULL);
  
  caps = gst_caps_from_string("video/x-raw, format=BGR");
  caps1 = gst_caps_from_string("video/x-raw, width=1920, height=1080, format=I420");
  caps2 = gst_caps_from_string("video/x-raw(memory:NVMM), format=NV12");
  caps3 = gst_caps_from_string("video/x-h264, stream-format=byte-stream'");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);


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
    nvvidconv3,
    videoconv,
    tee1,
    queue1,
    queue2,
    splitmuxsink,
    appsink,
    h264parser2,
    nvv4l2h264enc,
    NULL
  );

  g_signal_connect (source, "pad-added", G_CALLBACK (cb_new_pad), rtph264depay);
  g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample), provider);

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

  sinkpad = gst_element_get_request_pad (streammux, pad_name_sink);
  if (!sinkpad) {
    g_printerr ("Streammux request sink pad failed. Exiting.\n");
    return -1;
  }

  srcpad = gst_element_get_static_pad (nvv4l2decoder, pad_name_src);
  if (!srcpad) {
    g_printerr ("Decoder request src pad failed. Exiting.\n");
    return -1;
  }

  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
      g_printerr ("Failed to link decoder to stream muxer. Exiting.\n");
      return -1;
  }

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);
  gst_object_unref (osd_sink_pad);



  if (!gst_element_link_many (h264parser, nvv4l2decoder, NULL)) {
    g_printerr ("H264Parse and NvV4l2-Decoder could not be linked: 2. Exiting.\n");
    return -1;
  }

  if (!gst_element_link_many (streammux, pgie, tracker, nvvidconv, nvosd, nvvidconv2, tee1, NULL)) {
    g_printerr("Failed to link elements up to tee.\n");
    return -1;
  }

  // Link the first branch (tee -> queue1 -> videoconvert -> appsink)
  GstPad *tee_appsink_pad = gst_element_get_request_pad(tee1, "src_%u");
  GstPad *queue1_sink_pad = gst_element_get_static_pad(queue1, "sink");
  if (gst_pad_link(tee_appsink_pad, queue1_sink_pad) != GST_PAD_LINK_OK) 
  {
      g_printerr("Failed to link tee to queue1.\n");
      return -1;
  }

  gst_object_unref(queue1_sink_pad);

  // Link the first branch (tee -> queue1 -> videoconvert -> appsink) with caps
  if (!gst_element_link_filtered(queue1, videoconv, caps1)) {
      g_printerr("Failed to link queue1 to videoconvert with caps1.\n");
      return -1;
  }
  if (!gst_element_link_filtered(videoconv, appsink, caps)) {
      g_printerr("Failed to link videoconvert to appsink with caps2.\n");
      return -1;
  }

  // Link the second branch (tee -> queue2 -> splitmuxsink)
  GstPad *tee_splitmuxsink_pad = gst_element_get_request_pad(tee1, "src_%u");
  GstPad *queue2_sink_pad = gst_element_get_static_pad(queue2, "sink");
  if (gst_pad_link(tee_splitmuxsink_pad, queue2_sink_pad) != GST_PAD_LINK_OK) {
      g_printerr("Failed to link tee to queue2.\n");
      return -1;
  }
  gst_object_unref(queue2_sink_pad);
  
  if (!gst_element_link_filtered(queue2, nvvidconv3, caps1)) {
      g_printerr("Failed to link queue2 to nvvidconv3 with caps1.\n");
      return -1;
  }

  if (!gst_element_link_filtered(nvvidconv3, nvv4l2h264enc, caps2)) {
      g_printerr("Failed to link nvvidconv3 to nvv4l2h264enc with caps2.\n");
      return -1;
  }
  
  // Link queue2 to the encoder
  if (!gst_element_link_filtered(nvv4l2h264enc, h264parser2, caps3))  {
      g_printerr("Failed to link nvv4l2h264enc to h264parser2.\n");
      return -1;
  }

  // Link the parser to splitmuxsink
  if (!gst_element_link(h264parser2, splitmuxsink)) {
      g_printerr("Failed to link h264parser2 to splitmuxsink.\n");
      return -1;
  }

  // Unref the request pads on the tee when no longer needed
  gst_object_unref(tee_appsink_pad);
  gst_object_unref(tee_splitmuxsink_pad);

  // Unreference the caps after linking
  gst_caps_unref(caps);
  gst_caps_unref(caps1);
  gst_caps_unref(caps2);
  gst_caps_unref(caps3);

  g_print ("Using file: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_print ("Running...\n");
  g_main_loop_run (loop);

  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  gst_deinit();
  return app.exec();
}

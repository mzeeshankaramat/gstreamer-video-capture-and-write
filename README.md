# GStreamer, OpenCV, and QT Integration

This repository contains a demonstration of how to integrate GStreamer, OpenCV, and QT for video capture, display, and writing. The application supports various functionalities including capturing from an RTSP stream, a dummy video source, or a webcam, and then either displaying it in a QT window or writing it to a file.

## Features

1. **Capture & Display in OpenCV**:
   - Dummy Video Capture
   - RTSP Video Capture
2. **Capture & Display in QT**:
   - Webcam Video Capture & Write
   - RTSP Video Capture & Display
   - Dummy RTSP Video Capture & Write
   - Dummy TestSrc Video Capture & Write
3. **Convert Image to Video**:
   - Convert a static image to an MP4 video file

## Prerequisites

- QT development libraries
- OpenCV with GStreamer support
- GStreamer runtime and development libraries


### Using Docker

If you don't want to install the dependencies locally, you can use a custom Docker image.

#### Building from Docker Repository:
Clone and build from my repository: [open_cv_gstreamer_docker](https://github.com/mzeeshankaramat/open_cv_gstreamer_docker.git).

#### Pulling Pre-Built Docker Image:
You can pull the pre-built image directly from Docker Hub:

```
$ docker pull zeeshankaramat25/gstreamer-opencv-docker
```

## Usage

1. Compile the code with necessary libraries.
2. Run the main application.
3. Based on the uncommented code sections, different features will be executed.

## Code Structure

- `main()`: Entry point for the application. Initializes QT Application and invokes desired video handler methods.
- `VideoHandler` Class: Contains various methods to handle video capture, display, and write operations using GStreamer and OpenCV.

### Important Methods

- `gstreamerDummyVideoCaptureAndShowInCV()`: Captures dummy video and displays it using OpenCV.
- `gstreamerRTSPVideoCaptureAndShowInCV(std::string url)`: Captures RTSP stream and displays it using OpenCV.
- `gstreamerVideoCaptureAndWriteWebcam(QLabel &label)`: Captures video from webcam and writes it to an MP4 file. Displays the captured video in a QT window.
- `gstreamerRTSPVideoCaptureAndShowInQt(std::string url, QLabel &label)`: Captures RTSP stream and displays it in a QT window.
- `gstreamerMp4ViderWriterFromImage(std::string imgName, QLabel &label)`: Converts a static image to an MP4 video file.

## Demo

If you want to see this integration in action, follow the steps below:

### Using `gstreamerVideoCaptureAndWriteWebcam(QLabel &label)`

1. **Setup**:
   - Ensure you have all the prerequisites installed or have the Docker image ready.
   - If using Docker, start the Docker container and access the terminal.

2. **Run the Application**:
   - Navigate to the root directory of this repository.
   - Compile the code, ensuring all the necessary libraries are linked.
   - Run the main application. By default, this should trigger the `gstreamerVideoCaptureAndWriteWebcam(QLabel &label)` method.

3. **Perform the Demo**:
   - When you run the application, a QT window should appear showing the video feed from your webcam.
   - This method will also write the captured video to an MP4 file. You can find this file in the output directory specified in the code (or root directory by default).
   - Review the captured video using any standard video player to verify the result.

    <img src=webcam.png width="720" height="480">

## Contributing

If you'd like to contribute, please fork the repository and make changes as you'd like. Pull requests are warmly welcome.



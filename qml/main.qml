import QtQuick 2.12
import QtQuick.Controls 2.12

ApplicationWindow {
    visible: true
    width: 1920
    height: 1080
    title: "GStreamer Video"

    Timer {
            interval: 30 // Update every 30 milliseconds (adjust as needed for your application)
            running: true
            repeat: true
            onTriggered: {
                var uniqueId = Date.now().toString();
                liveDisplay.source = "image://videoframes/live.png?id=" + uniqueId;
            }
        }

        Image {
            id: liveDisplay
            anchors.fill: parent
            source: "image://videoframes/live.png" // Get the video frames from C++ backend
            fillMode: Image.PreserveAspectFit
        }

        Button {
            id: recordButton
            anchors.horizontalCenter : parent.horizontalCenter
            anchors.bottom: parent.bottom
            text: "Toggle Recording"
            onClicked: {
                text = "Hello G"
            }
        }
}
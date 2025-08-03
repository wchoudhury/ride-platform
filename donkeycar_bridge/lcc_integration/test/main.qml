// main.qml
// Main QML file for testing the Donkeycar camera viewer

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

Window {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Donkeycar Camera Viewer Test"
    
    Rectangle {
        anchors.fill: parent
        color: "#303030"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            
            Label {
                Layout.fillWidth: true
                text: "Donkeycar Camera Viewer Test"
                color: "white"
                font.pixelSize: 20
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#202020"
                
                // Embed the Donkeycar Camera Viewer component
                Rectangle {
                    id: cameraViewer
                    anchors.fill: parent
                    anchors.margins: 10
                    color: "#202020"
                    
                    property var cameraAggregator: donkeycarCameraAggregator
                    property int selectedVehicle: -1
                    property bool autoUpdate: true
                    property int updateInterval: 100
                    
                    Timer {
                        id: updateTimer
                        interval: cameraViewer.updateInterval
                        repeat: true
                        running: cameraViewer.autoUpdate
                        onTriggered: {
                            if (cameraViewer.selectedVehicle >= 0) {
                                cameraImage.source = ""
                                cameraImage.source = "image://donkeycar/" + cameraViewer.selectedVehicle + "?" + Math.random()
                            }
                        }
                    }
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10
                        
                        Label {
                            Layout.fillWidth: true
                            text: "Donkeycar Camera Viewer"
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                        }
                        
                        ComboBox {
                            id: vehicleSelector
                            Layout.fillWidth: true
                            model: cameraViewer.cameraAggregator ? cameraViewer.cameraAggregator.getVehicleIds() : []
                            
                            displayText: currentIndex >= 0 ? "Vehicle " + currentValue : "Select Vehicle"
                            
                            onCurrentValueChanged: {
                                if (currentValue !== undefined) {
                                    cameraViewer.selectedVehicle = currentValue
                                }
                            }
                            
                            Connections {
                                target: cameraViewer.cameraAggregator
                                function onCameraUpdated(vehicle_id) {
                                    // Refresh the model if new vehicles are detected
                                    var currentIds = vehicleSelector.model
                                    var found = false
                                    for (var i = 0; i < currentIds.length; i++) {
                                        if (currentIds[i] === vehicle_id) {
                                            found = true
                                            break
                                        }
                                    }
                                    
                                    if (!found) {
                                        vehicleSelector.model = cameraViewer.cameraAggregator.getVehicleIds()
                                    }
                                    
                                    // Update the image if this is the selected vehicle
                                    if (cameraViewer.selectedVehicle === vehicle_id && !cameraViewer.autoUpdate) {
                                        cameraImage.source = ""
                                        cameraImage.source = "image://donkeycar/" + vehicle_id + "?" + Math.random()
                                    }
                                }
                            }
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "black"
                            
                            Image {
                                id: cameraImage
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                                cache: false
                                
                                // Default to a placeholder when no camera is selected
                                source: cameraViewer.selectedVehicle >= 0 ? 
                                    "image://donkeycar/" + cameraViewer.selectedVehicle + "?" + Math.random() : 
                                    ""
                                
                                // Show text when no camera selected or no image available
                                Text {
                                    anchors.centerIn: parent
                                    color: "white"
                                    font.pixelSize: 14
                                    text: cameraViewer.selectedVehicle < 0 ? 
                                        "Select a vehicle to view camera feed" : 
                                        (cameraImage.status === Image.Error ? "No camera feed available" : "")
                                    visible: text !== ""
                                }
                                
                                // Loading indicator
                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: cameraImage.status === Image.Loading
                                    visible: running
                                }
                            }
                        }
                        
                        Row {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            Switch {
                                text: "Auto Update"
                                checked: cameraViewer.autoUpdate
                                onCheckedChanged: cameraViewer.autoUpdate = checked
                                
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "white"
                                    leftPadding: parent.indicator.width + parent.spacing
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            
                            Button {
                                text: "Refresh"
                                enabled: !cameraViewer.autoUpdate && cameraViewer.selectedVehicle >= 0
                                onClicked: {
                                    if (cameraViewer.selectedVehicle >= 0) {
                                        cameraImage.source = ""
                                        cameraImage.source = "image://donkeycar/" + cameraViewer.selectedVehicle + "?" + Math.random()
                                    }
                                }
                            }
                            
                            Label {
                                text: "Update: " + cameraViewer.updateInterval + "ms"
                                color: "white"
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            Slider {
                                from: 50
                                to: 1000
                                stepSize: 50
                                value: cameraViewer.updateInterval
                                onValueChanged: cameraViewer.updateInterval = value
                                
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
            
            RowLayout {
                Layout.fillWidth: true
                
                Button {
                    text: "Close"
                    onClicked: Qt.quit()
                    Layout.alignment: Qt.AlignRight
                }
            }
        }
    }
}
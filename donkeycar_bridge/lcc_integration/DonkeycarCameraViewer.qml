// DonkeycarCameraViewer.qml
// QML component for viewing Donkeycar camera feeds in the Lab Control Center

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    
    property var cameraAggregator: null
    property int selectedVehicle: -1
    property bool autoUpdate: true
    property int updateInterval: 100  // milliseconds
    
    color: "#202020"
    
    Timer {
        id: updateTimer
        interval: root.updateInterval
        repeat: true
        running: root.autoUpdate
        onTriggered: {
            if (root.selectedVehicle >= 0) {
                cameraImage.source = ""
                cameraImage.source = "image://donkeycar/" + root.selectedVehicle + "?" + Math.random()
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
            model: cameraAggregator ? cameraAggregator.getVehicleIds() : []
            textRole: "display"
            valueRole: "value"
            
            displayText: currentIndex >= 0 ? "Vehicle " + currentValue : "Select Vehicle"
            
            onCurrentValueChanged: {
                if (currentValue !== undefined) {
                    root.selectedVehicle = currentValue
                }
            }
            
            Connections {
                target: cameraAggregator
                function onCameraUpdated(vehicle_id) {
                    // Refresh the model if new vehicles are detected
                    if (!vehicleSelector.model.includes(vehicle_id)) {
                        vehicleSelector.model = cameraAggregator.getVehicleIds()
                    }
                    
                    // Update the image if this is the selected vehicle
                    if (root.selectedVehicle === vehicle_id && !root.autoUpdate) {
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
                source: root.selectedVehicle >= 0 ? 
                    "image://donkeycar/" + root.selectedVehicle + "?" + Math.random() : 
                    ""
                
                // Show text when no camera selected or no image available
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pixelSize: 14
                    text: root.selectedVehicle < 0 ? 
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
                checked: root.autoUpdate
                onCheckedChanged: root.autoUpdate = checked
                
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
                enabled: !root.autoUpdate && root.selectedVehicle >= 0
                onClicked: {
                    if (root.selectedVehicle >= 0) {
                        cameraImage.source = ""
                        cameraImage.source = "image://donkeycar/" + root.selectedVehicle + "?" + Math.random()
                    }
                }
            }
            
            Label {
                text: "Update: " + root.updateInterval + "ms"
                color: "white"
                verticalAlignment: Text.AlignVCenter
            }
            
            Slider {
                from: 50
                to: 1000
                stepSize: 50
                value: root.updateInterval
                onValueChanged: root.updateInterval = value
                
                Layout.fillWidth: true
            }
        }
    }
}
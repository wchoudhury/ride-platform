/*
 * DonkeycarCameraAggregator.hpp
 * 
 * Plugin for Lab Control Center to display camera feeds from Donkeycars
 */

#pragma once

#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <memory>
#include <QImage>
#include <QObject>
#include <QVariant>
#include <QTimer>
#include <cpm/AsyncReader.hpp>
#include <cpm/Logging.hpp>
#include <cpm/dds/Participant.hpp>

class DonkeycarCameraAggregator : public QObject
{
    Q_OBJECT

public:
    explicit DonkeycarCameraAggregator(QObject* parent = nullptr);
    virtual ~DonkeycarCameraAggregator();

    /**
     * Initialize the aggregator
     */
    void initialize();

    /**
     * Get the camera feed for a specific vehicle
     * @param vehicle_id The ID of the vehicle
     * @return The camera image as a QImage
     */
    Q_INVOKABLE QImage getCameraFeed(const int vehicle_id) const;

    /**
     * Get a list of vehicle IDs that have camera feeds
     * @return List of vehicle IDs
     */
    Q_INVOKABLE QVariantList getVehicleIds() const;

    /**
     * Check if a vehicle has a camera feed
     * @param vehicle_id The ID of the vehicle
     * @return True if the vehicle has a camera feed
     */
    Q_INVOKABLE bool hasCamera(const int vehicle_id) const;

signals:
    /**
     * Signal emitted when a new camera image is received
     * @param vehicle_id The ID of the vehicle
     */
    void cameraUpdated(int vehicle_id);

private:
    /**
     * Process a camera message
     * @param vehicle_id The ID of the vehicle
     * @param message The JSON message containing camera data
     */
    void processCameraMessage(const int vehicle_id, const std::string& message);

    /**
     * Update the list of available vehicles with cameras
     */
    void updateVehicleList();

private:
    // Map of vehicle ID to camera image
    std::map<int, QImage> camera_images_;
    
    // Mutex for thread-safe access to camera images
    mutable std::mutex mutex_;
    
    // Readers for camera feeds
    std::map<int, std::unique_ptr<cpm::AsyncReader<std::string>>> readers_;
    
    // List of vehicle IDs with camera feeds
    std::vector<int> vehicle_ids_;
    
    // Timer for updating the vehicle list
    QTimer* update_timer_;
    
    // Maximum number of vehicles to check
    static constexpr int max_vehicles_ = 20;
};
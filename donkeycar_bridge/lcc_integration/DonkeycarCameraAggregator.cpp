/*
 * DonkeycarCameraAggregator.cpp
 * 
 * Implementation of the Donkeycar camera feed aggregator for Lab Control Center
 */

#include "DonkeycarCameraAggregator.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QBuffer>

DonkeycarCameraAggregator::DonkeycarCameraAggregator(QObject* parent)
    : QObject(parent)
    , update_timer_(new QTimer(this))
{
    // Set up timer to check for new cameras
    connect(update_timer_, &QTimer::timeout, this, &DonkeycarCameraAggregator::updateVehicleList);
    update_timer_->start(5000); // Check for new cameras every 5 seconds
    
    cpm::Logging::Instance().write(cpm::LogLevel::Debug, "DonkeycarCameraAggregator created");
}

DonkeycarCameraAggregator::~DonkeycarCameraAggregator()
{
    update_timer_->stop();
    cpm::Logging::Instance().write(cpm::LogLevel::Debug, "DonkeycarCameraAggregator destroyed");
}

void DonkeycarCameraAggregator::initialize()
{
    // Perform initial update of vehicle list
    updateVehicleList();
    
    cpm::Logging::Instance().write(cpm::LogLevel::Info, "DonkeycarCameraAggregator initialized");
}

QImage DonkeycarCameraAggregator::getCameraFeed(const int vehicle_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = camera_images_.find(vehicle_id);
    if (it != camera_images_.end())
    {
        return it->second;
    }
    
    // Return empty image if no camera feed found
    return QImage();
}

QVariantList DonkeycarCameraAggregator::getVehicleIds() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    QVariantList result;
    for (int id : vehicle_ids_)
    {
        result.append(id);
    }
    
    return result;
}

bool DonkeycarCameraAggregator::hasCamera(const int vehicle_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    return camera_images_.find(vehicle_id) != camera_images_.end();
}

void DonkeycarCameraAggregator::processCameraMessage(const int vehicle_id, const std::string& message)
{
    try
    {
        // Parse the JSON message
        QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(message).toUtf8());
        if (doc.isNull() || !doc.isObject())
        {
            cpm::Logging::Instance().write(cpm::LogLevel::Warning, 
                "Invalid camera message format for vehicle " + std::to_string(vehicle_id));
            return;
        }
        
        QJsonObject obj = doc.object();
        
        // Extract image data
        if (!obj.contains("image_data") || !obj["image_data"].isString())
        {
            cpm::Logging::Instance().write(cpm::LogLevel::Warning, 
                "Missing image data in camera message for vehicle " + std::to_string(vehicle_id));
            return;
        }
        
        // Get base64 encoded image data
        QString base64Data = obj["image_data"].toString();
        QByteArray imageData = QByteArray::fromBase64(base64Data.toUtf8());
        
        // Create QImage from data
        QImage image;
        if (!image.loadFromData(imageData, "JPEG"))
        {
            cpm::Logging::Instance().write(cpm::LogLevel::Warning, 
                "Failed to load image data for vehicle " + std::to_string(vehicle_id));
            return;
        }
        
        // Update the image in our map
        {
            std::lock_guard<std::mutex> lock(mutex_);
            camera_images_[vehicle_id] = image;
        }
        
        // Emit signal that camera has been updated
        emit cameraUpdated(vehicle_id);
        
    }
    catch (const std::exception& e)
    {
        cpm::Logging::Instance().write(cpm::LogLevel::Error, 
            "Error processing camera message: " + std::string(e.what()));
    }
}

void DonkeycarCameraAggregator::updateVehicleList()
{
    try
    {
        // Check for camera feeds for vehicles
        for (int i = 0; i < max_vehicles_; ++i)
        {
            // Skip if we already have a reader for this vehicle
            if (readers_.find(i) != readers_.end())
            {
                continue;
            }
            
            // Try to create a reader for this vehicle's camera feed
            std::string topic = "donkeycar_camera_" + std::to_string(i);
            
            auto reader = std::make_unique<cpm::AsyncReader<std::string>>(
                topic,
                [this, i](const std::string& message)
                {
                    processCameraMessage(i, message);
                }
            );
            
            // Wait a bit to see if we get any data
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Check if we got any data
            if (reader->matched())
            {
                cpm::Logging::Instance().write(cpm::LogLevel::Info, 
                    "Found camera feed for vehicle " + std::to_string(i));
                
                // Add to our map of readers
                readers_[i] = std::move(reader);
                
                // Add to our list of vehicle IDs
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    
                    // Check if this vehicle is already in our list
                    auto it = std::find(vehicle_ids_.begin(), vehicle_ids_.end(), i);
                    if (it == vehicle_ids_.end())
                    {
                        vehicle_ids_.push_back(i);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        cpm::Logging::Instance().write(cpm::LogLevel::Error, 
            "Error updating vehicle list: " + std::string(e.what()));
    }
}
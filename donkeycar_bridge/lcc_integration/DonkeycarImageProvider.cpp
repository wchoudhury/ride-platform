/*
 * DonkeycarImageProvider.cpp
 * 
 * Implementation of Qt image provider for Donkeycar camera feeds
 */

#include "DonkeycarImageProvider.hpp"
#include "DonkeycarCameraAggregator.hpp"

#include <QImage>
#include <QString>
#include <cpm/Logging.hpp>

DonkeycarImageProvider::DonkeycarImageProvider(DonkeycarCameraAggregator* aggregator)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , aggregator_(aggregator)
{
    cpm::Logging::Instance().write(cpm::LogLevel::Debug, "DonkeycarImageProvider created");
}

QImage DonkeycarImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    // Parse the ID to get the vehicle ID
    // The ID format is: "vehicleId?timestamp" where timestamp is added to prevent caching
    QString vehicleIdStr = id.split('?').first();
    bool ok;
    int vehicleId = vehicleIdStr.toInt(&ok);
    
    if (!ok)
    {
        cpm::Logging::Instance().write(cpm::LogLevel::Warning, 
            "Invalid vehicle ID in image request: " + id.toStdString());
        return QImage();
    }
    
    // Get the camera image for this vehicle
    QImage image = aggregator_->getCameraFeed(vehicleId);
    
    if (image.isNull())
    {
        cpm::Logging::Instance().write(cpm::LogLevel::Debug, 
            "No camera feed available for vehicle " + std::to_string(vehicleId));
        
        // Return a placeholder image
        QImage placeholder(320, 240, QImage::Format_RGB32);
        placeholder.fill(Qt::black);
        
        // Set the size if requested
        if (size)
        {
            *size = placeholder.size();
        }
        
        return placeholder;
    }
    
    // Set the size if requested
    if (size)
    {
        *size = image.size();
    }
    
    // Scale the image if requested
    if (requestedSize.width() > 0 && requestedSize.height() > 0)
    {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return image;
}
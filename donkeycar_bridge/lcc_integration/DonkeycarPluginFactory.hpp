/*
 * DonkeycarPluginFactory.hpp
 * 
 * Factory for creating and registering Donkeycar components in the Lab Control Center
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <memory>

#include "DonkeycarCameraAggregator.hpp"
#include "DonkeycarImageProvider.hpp"

class DonkeycarPluginFactory : public QObject
{
    Q_OBJECT

public:
    static DonkeycarPluginFactory& instance();
    
    /**
     * Register the Donkeycar components with the QML engine
     * @param engine The QML engine to register with
     */
    void registerComponents(QQmlEngine* engine);
    
    /**
     * Get the camera aggregator instance
     * @return The camera aggregator
     */
    DonkeycarCameraAggregator* getCameraAggregator() const;
    
private:
    explicit DonkeycarPluginFactory(QObject* parent = nullptr);
    ~DonkeycarPluginFactory();
    
    // Pointer to the camera aggregator
    std::unique_ptr<DonkeycarCameraAggregator> camera_aggregator_;
};
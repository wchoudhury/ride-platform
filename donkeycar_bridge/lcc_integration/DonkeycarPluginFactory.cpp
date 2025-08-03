/*
 * DonkeycarPluginFactory.cpp
 * 
 * Implementation of factory for creating and registering Donkeycar components
 */

#include "DonkeycarPluginFactory.hpp"
#include <cpm/Logging.hpp>

DonkeycarPluginFactory& DonkeycarPluginFactory::instance()
{
    static DonkeycarPluginFactory instance;
    return instance;
}

DonkeycarPluginFactory::DonkeycarPluginFactory(QObject* parent)
    : QObject(parent)
    , camera_aggregator_(std::make_unique<DonkeycarCameraAggregator>())
{
    cpm::Logging::Instance().write(cpm::LogLevel::Info, "DonkeycarPluginFactory initialized");
    
    // Initialize the camera aggregator
    camera_aggregator_->initialize();
}

DonkeycarPluginFactory::~DonkeycarPluginFactory()
{
    cpm::Logging::Instance().write(cpm::LogLevel::Debug, "DonkeycarPluginFactory destroyed");
}

void DonkeycarPluginFactory::registerComponents(QQmlEngine* engine)
{
    if (!engine)
    {
        cpm::Logging::Instance().write(cpm::LogLevel::Error, "Invalid QML engine");
        return;
    }
    
    // Register the image provider
    engine->addImageProvider("donkeycar", new DonkeycarImageProvider(camera_aggregator_.get()));
    
    // Register the camera aggregator as a context property
    engine->rootContext()->setContextProperty("donkeycarCameraAggregator", camera_aggregator_.get());
    
    // Register the QML files location
    engine->addImportPath("/home/icarus/school/RIDE-project/donkeycar_bridge/lcc_integration");
    
    cpm::Logging::Instance().write(cpm::LogLevel::Info, "Donkeycar components registered with QML engine");
}

DonkeycarCameraAggregator* DonkeycarPluginFactory::getCameraAggregator() const
{
    return camera_aggregator_.get();
}
/*
 * test_camera_viewer.cpp
 *
 * Simple test application for the Donkeycar camera viewer
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <cpm/init.hpp>
#include <iostream>

#include "../DonkeycarPluginFactory.hpp"

int main(int argc, char** argv)
{
    try
    {
        // Initialize CPM
        cpm::init("donkeycar_camera_viewer_test");
        cpm::Logging::Instance().set_id("donkeycar_camera_viewer_test");
        
        // Create Qt application
        QApplication app(argc, argv);
        
        // Create QML engine
        QQmlApplicationEngine engine;
        
        // Register Donkeycar components
        DonkeycarPluginFactory::instance().registerComponents(&engine);
        
        // Load the QML file
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
        
        if (engine.rootObjects().isEmpty())
        {
            std::cerr << "Failed to load QML" << std::endl;
            return -1;
        }
        
        // Run the application
        return app.exec();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
/*
 * DonkeycarImageProvider.hpp
 * 
 * Qt image provider for Donkeycar camera feeds
 */

#pragma once

#include <QQuickImageProvider>
#include <memory>

class DonkeycarCameraAggregator;

class DonkeycarImageProvider : public QQuickImageProvider
{
public:
    explicit DonkeycarImageProvider(DonkeycarCameraAggregator* aggregator);
    
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
    
private:
    DonkeycarCameraAggregator* aggregator_;
};
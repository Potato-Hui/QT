#ifndef GSTVIDEORECEIVER_H
#define GSTVIDEORECEIVER_H

#include <QImage>
#include <QObject>

#include <atomic>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include "latest_value_slot.hpp"

class QTimer;

class GstVideoReceiver : public QObject
{
    Q_OBJECT

public:
    explicit GstVideoReceiver(QObject *parent = nullptr);
    ~GstVideoReceiver() override;
    enum class VideoSource
        {
            RknnTcpJpeg,
            ThermalRtsp
        };
    bool start(VideoSource source);
    void stop();
    bool isRunning() const;

signals:
    void frameReady(const QImage &frame);
    void firstFrameReceived();
    void streamError(const QString &message);

private slots:
    void checkBus();
    void deliverLatestFrame();

private:
    static GstFlowReturn onNewSample(
        GstAppSink *sink,
        gpointer userData);

    GstFlowReturn processSample(GstAppSink *sink);

    GstElement *m_pipeline = nullptr;
    GstElement *m_appSink = nullptr;
    GstBus *m_bus = nullptr;

    QTimer *m_busTimer = nullptr;
    QTimer *m_deliveryTimer = nullptr;

    LatestValueSlot<QImage> m_latestFrame;

    gulong m_newSampleHandler = 0;

    std::atomic_bool m_firstFrameEmitted{false};
};

#endif

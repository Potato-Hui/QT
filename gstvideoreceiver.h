#ifndef GSTVIDEORECEIVER_H
#define GSTVIDEORECEIVER_H

#include <QImage>
#include <QObject>
#include <QString>

#include <atomic>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include "latest_value_slot.hpp"

class QTimer;

enum class VideoSource
{
    None,
    TcpJpeg,
    ThermalRtsp
};

class GstVideoReceiver : public QObject
{
    Q_OBJECT

public:
    explicit GstVideoReceiver(QObject *parent = nullptr);
    ~GstVideoReceiver() override;

    bool start(VideoSource source,
               const QString &rtspUrl = QString(),
               int rtspLatency = 0);
    void stop();
    bool isRunning() const;
    VideoSource source() const;

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
    VideoSource m_source = VideoSource::None;
};

#endif

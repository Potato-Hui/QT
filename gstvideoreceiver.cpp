#include "gstvideoreceiver.h"

#include <QTimer>
#include <utility>
/*
 * GstVideoReceiver
 *
 * 接收管线：
 *
 * tcpclientsrc
 *   -> jpegparse
 *   -> jpegdec
 *   -> videoconvert
 *   -> BGR
 *   -> appsink
 */
GstVideoReceiver::GstVideoReceiver(QObject *parent)
    : QObject(parent)
    , m_busTimer(new QTimer(this))
    , m_deliveryTimer(new QTimer(this))
{
    /*
     * 如果 main.cpp 已经调用 gst_init()，
     * 这里不会重复初始化。
     */
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    /*
     * Qt 没有直接运行 GLib MainLoop，因此使用
     * QTimer 定期检查 GStreamer Bus。
     */
    m_busTimer->setInterval(100);

    connect(
        m_busTimer,
        &QTimer::timeout,
        this,
        &GstVideoReceiver::checkBus);

    m_deliveryTimer->setInterval(33);
    connect(
        m_deliveryTimer,
        &QTimer::timeout,
        this,
        &GstVideoReceiver::deliverLatestFrame);
}

GstVideoReceiver::~GstVideoReceiver()
{
    stop();
}

bool GstVideoReceiver::start(VideoSource source)
{
    /*
     * 防止重复启动造成旧管线、Bus 和 signal handler 泄漏。
     */
    stop();

    m_firstFrameEmitted.store(false);
    QByteArray pipelineDescription;
    if (source == VideoSource::ThermalRtsp) {
           pipelineDescription =
               "rtspsrc "
               "location=\"rtsp://admin:admin123@192.168.1.111:554/video1/stream0\" "
               "protocols=tcp "
               "latency=0 ! "
               "rtph264depay ! "
               "h264parse ! "
               "mppvideodec ! "
               "videoconvert ! "
               "video/x-raw,format=BGR ! "
               "appsink name=qt_video_sink "
               "emit-signals=true "
               "max-buffers=1 "
               "drop=true "
               "sync=false";
       } else {
           pipelineDescription =
               "tcpclientsrc host=127.0.0.1 port=5000 ! "
               "jpegparse ! "
               "jpegdec ! "
               "videoconvert ! "
               "video/x-raw,format=BGR ! "
               "appsink name=qt_video_sink "
               "emit-signals=true "
               "max-buffers=1 "
               "drop=true "
               "sync=false";
       }

    GError *error = nullptr;

    m_pipeline =
        gst_parse_launch(pipelineDescription, &error);

    /*
     * gst_parse_launch 有可能在返回非空 pipeline 的同时
     * 返回解析错误，因此只要 error 非空就视为启动失败。
     */
    if (error != nullptr) {
        const QString errorMessage =
            QString::fromUtf8(error->message);

        g_error_free(error);
        error = nullptr;

        stop();

        emit streamError(
            QStringLiteral("创建视频管线失败：%1")
                .arg(errorMessage));

        return false;
    }

    if (m_pipeline == nullptr) {
        emit streamError(
            QStringLiteral("创建视频管线失败"));

        return false;
    }

    /*
     * 根据管线中的名字查找 appsink。
     */
    m_appSink =
        gst_bin_get_by_name(
            GST_BIN(m_pipeline),
            "qt_video_sink");

    if (m_appSink == nullptr) {
        stop();

        emit streamError(
            QStringLiteral(
                "找不到 GStreamer appsink：qt_video_sink"));

        return false;
    }

    /*
     * 当 appsink 收到新帧时，GStreamer 的流线程会调用
     * GstVideoReceiver::onNewSample()。
     */
    m_newSampleHandler =
        g_signal_connect(
            m_appSink,
            "new-sample",
            G_CALLBACK(
                &GstVideoReceiver::onNewSample),
            this);

    if (m_newSampleHandler == 0) {
        stop();

        emit streamError(
            QStringLiteral(
                "无法连接 GStreamer appsink 回调"));

        return false;
    }

    /*
     * 获取 Bus，用于接收 TCP 连接失败、解码错误和 EOS。
     */
    m_bus =
        gst_element_get_bus(m_pipeline);

    if (m_bus == nullptr) {
        stop();

        emit streamError(
            QStringLiteral(
                "无法获取 GStreamer Bus"));

        return false;
    }

    /*
     * 启动视频管线。
     *
     * GST_STATE_CHANGE_ASYNC 是正常返回值，
     * 只有 GST_STATE_CHANGE_FAILURE 表示失败。
     */
    const GstStateChangeReturn result =
        gst_element_set_state(
            m_pipeline,
            GST_STATE_PLAYING);

    if (result == GST_STATE_CHANGE_FAILURE) {
        stop();

        emit streamError(
            QStringLiteral(
                "视频接收管线启动失败"));

        return false;
    }

    /*
     * 开始检查 GStreamer Bus。
     */
    m_busTimer->start();
    m_deliveryTimer->start();

    return true;
}

void GstVideoReceiver::stop()
{
    /*
     * 停止 Bus 定时检查。
     */
    if (m_busTimer != nullptr) {
        m_busTimer->stop();
    }
    if (m_deliveryTimer != nullptr) {
        m_deliveryTimer->stop();
    }

    /*
     * 先断开 new-sample 回调，防止停止过程中继续触发回调。
     */
    if (m_appSink != nullptr &&
        m_newSampleHandler != 0) {

        g_signal_handler_disconnect(
            m_appSink,
            m_newSampleHandler);

        m_newSampleHandler = 0;
    }

    /*
     * 将管线切换到 NULL 状态，停止内部线程和网络连接。
     */
    if (m_pipeline != nullptr) {
        gst_element_set_state(
            m_pipeline,
            GST_STATE_NULL);
    }

    /*
     * 释放 Bus。
     */
    if (m_bus != nullptr) {
        gst_object_unref(m_bus);
        m_bus = nullptr;
    }

    /*
     * 释放 appsink。
     */
    if (m_appSink != nullptr) {
        gst_object_unref(m_appSink);
        m_appSink = nullptr;
    }

    /*
     * 释放整个管线。
     */
    if (m_pipeline != nullptr) {
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }

    m_firstFrameEmitted.store(false);
    m_latestFrame.clear();
}

bool GstVideoReceiver::isRunning() const
{
    return m_pipeline != nullptr;
}

/*
 * GStreamer 静态回调。
 *
 * 这个函数运行在 GStreamer 的流线程中，
 * 不能在这里直接操作 QWidget。
 */
GstFlowReturn GstVideoReceiver::onNewSample(
    GstAppSink *sink,
    gpointer userData)
{
    auto *receiver =
        static_cast<GstVideoReceiver *>(userData);

    if (receiver == nullptr) {
        return GST_FLOW_ERROR;
    }

    return receiver->processSample(sink);
}

/*
 * 从 appsink 获取视频帧并转换成拥有独立内存的 QImage。
 */
GstFlowReturn GstVideoReceiver::processSample(
    GstAppSink *sink)
{
    if (sink == nullptr) {
        return GST_FLOW_ERROR;
    }

    /*
     * 获取当前帧。
     *
     * 此 GstSample 必须在函数结束前释放。
     */
    GstSample *sample =
        gst_app_sink_pull_sample(sink);

    if (sample == nullptr) {
        return GST_FLOW_ERROR;
    }

    GstCaps *caps =
        gst_sample_get_caps(sample);

    GstBuffer *buffer =
        gst_sample_get_buffer(sample);

    if (caps == nullptr ||
        buffer == nullptr) {

        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    /*
     * 从 caps 中读取视频格式、宽、高和 stride。
     */
    GstVideoInfo videoInfo;
    gst_video_info_init(&videoInfo);

    if (!gst_video_info_from_caps(
            &videoInfo,
            caps)) {

        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    /*
     * 接收管线已经指定输出 BGR，
     * 这里再次检查实际协商结果。
     */
    if (GST_VIDEO_INFO_FORMAT(&videoInfo) !=
        GST_VIDEO_FORMAT_BGR) {

        gst_sample_unref(sample);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    const int width =
        static_cast<int>(
            GST_VIDEO_INFO_WIDTH(&videoInfo));

    const int height =
        static_cast<int>(
            GST_VIDEO_INFO_HEIGHT(&videoInfo));

    if (width <= 0 || height <= 0) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    /*
     * GstVideoFrame 能正确处理每行字节数和内存对齐。
     * 不要使用 mapInfo.size / height 猜测 stride。
     */
    GstVideoFrame videoFrame = {};

    if (!gst_video_frame_map(
            &videoFrame,
            &videoInfo,
            buffer,
            GST_MAP_READ)) {

        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const auto *data =
        static_cast<const uchar *>(
            GST_VIDEO_FRAME_PLANE_DATA(
                &videoFrame,
                0));

    const int bytesPerLine =
        GST_VIDEO_FRAME_PLANE_STRIDE(
            &videoFrame,
            0);

    if (data == nullptr ||
        bytesPerLine <= 0) {

        gst_video_frame_unmap(&videoFrame);
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    /*
     * GStreamer 输出 BGR。Qt 5.12 没有 Format_BGR888，先按 RGB888
     * 包装，再交换红蓝通道，兼容 Qt 5.12/5.15。
     */
    const QImage wrappedImage(
        data,
        width,
        height,
        bytesPerLine,
        QImage::Format_RGB888);

    /*
     * copy() 确保 QImage 拥有自己的像素内存。
     *
     * GstBuffer 在 unmap/unref 后会失效，因此不能直接
     * 把 wrappedImage 发送到界面线程。
     */
    QImage ownedImage = wrappedImage.rgbSwapped().copy();

    /*
     * QImage 完成深拷贝后，可以释放 GStreamer 视频帧。
     */
    gst_video_frame_unmap(&videoFrame);
    gst_sample_unref(sample);

    if (ownedImage.isNull()) {
        return GST_FLOW_ERROR;
    }
    /*
     * 流线程只覆盖最新帧，不向 Qt 事件队列逐帧排队。
     * GUI 线程中的 delivery timer 会按自身处理能力取走最新帧。
     */
    m_latestFrame.publish(std::move(ownedImage));

    /*
     * exchange(true) 是原子操作，确保 firstFrameReceived
     * 在一次启动期间只发送一次。
     */
    if (!m_firstFrameEmitted.exchange(true)) {
        emit firstFrameReceived();
    }

    return GST_FLOW_OK;
}

void GstVideoReceiver::deliverLatestFrame()
{
    QImage frame;
    if (m_latestFrame.takeLatest(frame)) {
        emit frameReady(frame);
    }
}

/*
 * 检查 GStreamer Bus。
 *
 * 这个函数由 QTimer 调用，因此运行在 Qt 所在线程中。
 */
void GstVideoReceiver::checkBus()
{
    if (m_bus == nullptr) {
        return;
    }

    while (true) {
        GstMessage *message =
            gst_bus_pop_filtered(
                m_bus,
                static_cast<GstMessageType>(
                    GST_MESSAGE_ERROR |
                    GST_MESSAGE_EOS));

        if (message == nullptr) {
            break;
        }

        switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
        {
            GError *error = nullptr;
            gchar *debugInformation = nullptr;

            gst_message_parse_error(
                message,
                &error,
                &debugInformation);

            QString errorMessage =
                QStringLiteral(
                    "未知 GStreamer 错误");

            if (error != nullptr) {
                errorMessage =
                    QString::fromUtf8(error->message);

                g_error_free(error);
                error = nullptr;
            }

            /*
             * debugInformation 通常很长，暂时不显示在 UI。
             * 后续可以写入日志文件。
             */
            if (debugInformation != nullptr) {
                g_free(debugInformation);
                debugInformation = nullptr;
            }

            gst_message_unref(message);

            /*
             * 先停止并释放管线，再通知上层。
             */
            stop();

            emit streamError(
                QStringLiteral("视频流错误：%1")
                    .arg(errorMessage));

            return;
        }

        case GST_MESSAGE_EOS:
        {
            gst_message_unref(message);

            stop();

            emit streamError(
                QStringLiteral("视频流已经结束"));

            return;
        }

        default:
            gst_message_unref(message);
            break;
        }
    }
}

#include "Receiver.h"
#include <QThread>
#include <QDebug>
#include <unordered_map>
#include <string>
Receiver::Receiver(QObject *obj) : QObject(obj)
{
    if (!NDIlib_initialize())
    {
        qDebug() << "Cannot initialize NDI.\n";
        return;
    }
    NDIlib_find_instance_t finder = NDIlib_find_create_v2();
    if (!finder) return;
    uint32_t no_sources = 0;
    const NDIlib_source_t* p_sources = nullptr;

    NDIlib_destroy();
    while(!no_sources)
    {
        qDebug() << "looking for sources ...\n";
        NDIlib_find_wait_for_sources(finder, 5000);
        p_sources = NDIlib_find_get_current_sources(finder, &no_sources);
    }
    NDIlib_recv_create_v3_t recv_desc = {};
    recv_desc.color_format = (NDIlib_recv_color_format_e)NDIlib_recv_color_format_compressed_v4_with_audio;
//    recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;       // HX3默认
    _ndiRecv = NDIlib_recv_create_v3(&recv_desc);
    if (!_ndiRecv)
            return;
    // Connect to our sources
    NDIlib_recv_connect(_ndiRecv, p_sources + 0);
    // Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
    NDIlib_find_destroy(finder);
}

Receiver::~Receiver()
{
    NDIlib_recv_destroy(_ndiRecv);
    NDIlib_destroy();
}

void Receiver::GetFrame(NDIlib_video_frame_v2_t &frame)
{
    NDIlib_audio_frame_v2_t audio_frame;
    NDIlib_metadata_frame_t metadata_frame;
    auto result = NDIlib_recv_capture_v2(
        _ndiRecv, &frame, &audio_frame, &metadata_frame, 100);

    if (result == NDIlib_frame_type_video)
    {
        if (frame.p_metadata) {
            // XML video metadata information.
//            qDebug() <<"XML video metadata"<< frame.p_metadata << ".\n";
        }
        if (frame.p_data) {
            // Video frame buffer.
            // Check video_frame.FourCC for the pixel format of this buffer
            qDebug() <<"receive success!\n";
        }
        // 使用完后释放
        NDIlib_recv_free_video_v2(_ndiRecv, &frame);
    }
    else if(result == NDIlib_frame_type_none)
    {
        qDebug() << "No data received.\n";
    }
}

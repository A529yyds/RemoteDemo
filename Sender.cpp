#include <iostream>
#include <QDebug>
#include "Sender.h"
#include "defines.h"
#include "OptFFmpeg.h"
#include "ExampleDemo.h"
#include "Processing.NDI.Send.h"

#include <fstream>

Sender::Sender(const QString &name, QObject *obj) : QObject(obj)
{
    if (!NDIlib_initialize())
    {
        qDebug() << "Cannot initialize NDI.\n";
        return;
    }
    NDIlib_send_create_t send_desc;
    send_desc.p_ndi_name = name.toUtf8().constData(); // 名称
    send_desc.clock_video = true;
    send_desc.clock_audio = false;
    _ndiSend = NDIlib_send_create_v2(&send_desc, nullptr);

    if (!_ndiSend)
    {
        qDebug() << "Failed to create NDI sender\n";
        return;
    }
    // test demo
    _videoFrame.xres = W_4K; // Must match H.265 data
    _videoFrame.yres = H_4K; // Must match H.265 data
    _videoFrame.FourCC = (NDIlib_FourCC_video_type_e)NDIlib_FourCC_type_HEVC_highest_bandwidth;
    _videoFrame.p_data = nullptr;
    _videoFrame.data_size_in_bytes = 0;
    _videoFrame.frame_rate_N = 60000;
    _videoFrame.frame_rate_D = 1001;
    _videoFrame.picture_aspect_ratio = 16.0f/9.0f;
    _videoFrame.frame_format_type = NDIlib_frame_format_type_progressive;
//    _opt = new OptFFmpeg(W_4K, H_4K, nullptr);
}

Sender::~Sender()
{
    NDIlib_send_send_video_scatter_async(_ndiSend, nullptr, nullptr);

    NDIlib_send_destroy(_ndiSend);
    NDIlib_destroy();
    //    DELETE(_fullbuffer);
}


void Sender::SendFrame1(video_frame vf)
{
    /*uint32_t packet_size = sizeof(NDIlib_compressed_packet_t) + vf.data_size + vf.extra_size*/;
    NDIlib_compressed_packet_t p_packet;
    p_packet.version = sizeof(NDIlib_compressed_packet_t);
    p_packet.pts = vf.pts;
    p_packet.dts = vf.dts;
    p_packet.flags = vf.is_keyframe ? NDIlib_compressed_packet_t::flags_keyframe : NDIlib_compressed_packet_t::flags_none;
    p_packet.data_size = vf.data_size;
    p_packet.extra_data_size = vf.extra_size;
    p_packet.fourCC = NDIlib_compressed_FourCC_type_HEVC;
    std::vector<uint8_t*> scatter_data;
    std::vector<int> scatter_data_size;
    scatter_data.clear();
    scatter_data_size.clear();
    scatter_data.push_back((uint8_t*)&p_packet);
    scatter_data_size.push_back((int)sizeof(NDIlib_compressed_packet_t));
    scatter_data.push_back((uint8_t*)vf.p_data);
    scatter_data_size.push_back((int)vf.data_size);
    if (vf.extra_size != 0) {
        scatter_data.push_back((uint8_t*)vf.p_extra);
        scatter_data_size.push_back((int)vf.extra_size);
    }
    scatter_data.push_back(nullptr);
    scatter_data_size.push_back(0);
    _videoFrame.timecode = vf.dts;
    _videoFrame.p_metadata = "Hello!\n";
    NDIlib_video_frame_v2_t& highQ_frame = _videoFrame;
    NDIlib_frame_scatter_t   highQ_scatter = {scatter_data.data(), scatter_data_size.data()};
    NDIlib_send_send_video_scatter_async(_ndiSend, &highQ_frame, &highQ_scatter);
}

void Sender::SendFrame(video_frame vf)
{
    // This is probably zero for non I-frames, but MUST be set of I-frames
    // Compute the total size of the structure
    uint32_t packet_size = sizeof(NDIlib_compressed_packet_t) + vf.data_size + vf.extra_size;
    // Allocate the structure
    NDIlib_compressed_packet_t* p_packet = (NDIlib_compressed_packet_t*)malloc(packet_size);
    // Fill in the settings
    p_packet->version = NDIlib_compressed_packet_t::version_0;
    p_packet->fourCC = NDIlib_FourCC_type_HEVC;
    p_packet->pts = 0; // These should be filled in correctly !
    p_packet->dts = 0;
    p_packet->flags = vf.is_keyframe ? NDIlib_compressed_packet_t::flags_keyframe: NDIlib_compressed_packet_t::flags_none;
    p_packet->data_size = vf.data_size;
    p_packet->extra_data_size = vf.extra_size;

    // Compute the pointer to the compressed hevc data, then copy the memory into place.
    uint8_t* p_dst_hevc_data = (uint8_t*)(1 + p_packet);
    memcpy(p_dst_hevc_data, vf.p_data, vf.data_size);
    // Compute the pointer to the ancillary extra data
    uint8_t* p_dst_extra_hevc_data = p_dst_hevc_data + vf.data_size;
    memcpy(p_dst_extra_hevc_data, vf.p_extra, vf.extra_size);
    // Choose a good value, this is an example
    _videoFrame.timecode = p_packet->pts;
    _videoFrame.p_data = (uint8_t*)p_packet;
    _videoFrame.data_size_in_bytes = packet_size;
    _videoFrame.p_metadata = "reinterpret_cast<const char*>(vf.p_data)\n";
//    if(p_packet->flags == 1)
//    {
//        uint8_t* pData = (uint8_t*)(p_packet + 1);
//        qDebug() << "send data size is " << p_packet->data_size << "; sender p_packet.pts:" << p_packet->pts << "flags status is " << p_packet->flags;
//        _opt->Write1Frame2File(packet_size, reinterpret_cast<const char*>(pData), "Sender");
//    }
    NDIlib_send_send_video_async_v2(_ndiSend, &_videoFrame);

}

void Sender::SendFrame(uint8_t *data, int size)
{
    if (NDIlib_send_is_keyframe_required(_ndiSend)) {
        _frame_num = 0;
    }
//    SetupFrame(_frame_num, _videoInfo, _sendData);

    _sendData.packet.data_size = size;
    _sendData.packet.fourCC = NDIlib_FourCC_type_HEVC;//NDIlib_compressed_FourCC_type_HEVC;
    _sendData.scatter_data.clear();
    _sendData.scatter_data_size.clear();

    // Push the compressed packet structure to the scatter-gather lists
    _sendData.scatter_data.push_back((uint8_t*)&_sendData.packet);
    _sendData.scatter_data_size.push_back((int)sizeof(NDIlib_compressed_packet_t));
    // Push the actual frame data to the scatter-gather lists
    _sendData.scatter_data.push_back((uint8_t*)data);
    _sendData.scatter_data_size.push_back((int)size);
    // NULL terminate both lists
    _sendData.scatter_data.push_back(nullptr);
    _sendData.scatter_data_size.push_back(0);

    _sendData.frame.p_data =  data;
    _sendData.frame.data_size_in_bytes =  size;

    NDIlib_video_frame_v2_t& frame = _sendData.frame;
    NDIlib_frame_scatter_t scatter = {_sendData.scatter_data.data(), _sendData.scatter_data_size.data()};
    _videoInfo.num_frames++;
    const int frameSize = NDIlib_send_get_target_frame_size(_ndiSend, &frame);
    NDIlib_send_send_video_scatter_async(_ndiSend, &frame, &scatter);
//    _videoFrame.p_data = data;
    // 拷贝实际 HEVC 数据到结构体后面
//     memcpy(_fullbuffer + sizeof(NDIlib_compressed_packet_t), data, size);
//    _videoFrame.p_data = _fullbuffer;
//    _videoFrame.line_stride_in_bytes = sizeof(NDIlib_compressed_packet_t) + size;
//    _videoFrame.p_data = reinterpret_cast<uint8_t*>(&_compressPkg);
//    NDIlib_send_send_video_v2(_ndiSend, &_videoFrame);
    // 5. 构建 scatter（这里只有一块）
    // 用 scatter-gather 封装压缩数据帧（NDIlib_compressed_packet_t + HEVC 数据）
//    const size_t pkt_header_size = sizeof(NDIlib_compressed_packet_t);
//    memcpy(_fullbuffer, &_compressPkg, pkt_header_size);
//    memcpy(_fullbuffer + pkt_header_size, data, size);

//    // 设置 scatter-gather 列表（注意：compressed_packet_t 必须在第一个块）
//    const uint8_t* data_blocks[] = { _fullbuffer, nullptr };
//    int block_sizes[] = { static_cast<int>(pkt_header_size + size), 0 };

//    _scatter.p_data_blocks = data_blocks;
//    _scatter.p_data_blocks_size = block_sizes;
//    // 6. 发送（同步）
               //     NDIlib_send_send_video_scatter(_ndiSend, &_videoFrame, &_scatter);
}

void Sender::InitVideoData(video_send_data &p_send_data, const stream_info_type &stream_info, NDIlib_FourCC_video_type_ex_e stream_type)
{
//        NDIlib_video_frame_v2_t& frame = p_send_data.frame;

        // The following data is pretty much constant throughout the stream
        p_send_data.frame.FourCC = (NDIlib_FourCC_video_type_e)stream_type;
        p_send_data.frame.xres = stream_info.xres;
        p_send_data.frame.yres = stream_info.yres;
        p_send_data.frame.p_data = nullptr;
        p_send_data.frame.data_size_in_bytes = 0;
        p_send_data.frame.frame_rate_N = stream_info.frame_rate_n;
        p_send_data.frame.frame_rate_D = stream_info.frame_rate_d;
        p_send_data.frame.frame_format_type = NDIlib_frame_format_type_progressive;
        p_send_data.frame.picture_aspect_ratio = (float)stream_info.aspect_ratio_n / (float)stream_info.aspect_ratio_d;

}

void Sender::SetupFrame(int frame_num, const stream_info_type &stream_info, video_send_data &dst_data)
{
    const video_frame& src_frame = stream_info.p_frames[frame_num];
    dst_data.frame.timecode = src_frame.dts;
    dst_data.packet.version = sizeof(NDIlib_compressed_packet_t);
    dst_data.packet.pts = src_frame.pts;
    dst_data.packet.dts = src_frame.dts;
    dst_data.packet.flags = src_frame.is_keyframe ? NDIlib_compressed_packet_t::flags_keyframe : NDIlib_compressed_packet_t::flags_none;

    dst_data.packet.data_size = src_frame.data_size;
    dst_data.packet.extra_data_size = src_frame.extra_size;
    dst_data.packet.fourCC = NDIlib_compressed_FourCC_type_HEVC;
    dst_data.scatter_data_size.clear();
    dst_data.scatter_data.push_back((uint8_t*)&dst_data.packet);
    dst_data.scatter_data_size.push_back((int)sizeof(NDIlib_compressed_packet_t));
    dst_data.scatter_data.push_back((uint8_t*)src_frame.p_data);
    dst_data.scatter_data_size.push_back((int)src_frame.data_size);
    if (src_frame.extra_size != 0) {
        dst_data.scatter_data.push_back((uint8_t*)src_frame.p_extra);
        dst_data.scatter_data_size.push_back((int)src_frame.extra_size);
    }
    dst_data.scatter_data.push_back(nullptr);
    dst_data.scatter_data_size.push_back(0);

}

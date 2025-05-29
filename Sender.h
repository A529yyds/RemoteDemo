#ifndef SENDER_H
#define SENDER_H

#include <QObject>
#include <Processing.NDI.Advanced.h>
#include <Processing.NDI.Lib.h>

#include "defines.h"

class OptFFmpeg;

struct stream_info_type {
    int xres, yres;
    int frame_rate_n, frame_rate_d;
    int aspect_ratio_n, aspect_ratio_d;
    int num_frames;
    const video_frame* p_frames;
};

// Basic set of data for a video frame to be sent via a scatter-gather list
struct video_send_data {
    NDIlib_compressed_packet_t packet;
    NDIlib_video_frame_v2_t    frame;
    std::vector<uint8_t*>      scatter_data;
    std::vector<int>           scatter_data_size;
};

class Sender : public QObject
{
    Q_OBJECT
public:
//    Sender(const QString &name, NDIlib_video_frame_v2_t fmt, QObject *obj = nullptr);
    Sender(const QString &name, QObject *obj = nullptr);
    ~Sender();
    void SendFrame(video_frame vf);
    void SendFrame1(video_frame vf);
    void SendFrame(uint8_t *data, int size = 0);

private:
    void InitVideoData(video_send_data &p_send_data, const stream_info_type& stream_info, NDIlib_FourCC_video_type_ex_e stream_type);
    void SetupFrame(int frame_num, const stream_info_type& stream_info, video_send_data& dst_data);

private:
    int _frame_num = 0;
    NDIlib_send_instance_t _ndiSend;
    NDIlib_video_frame_v2_t _videoFrame;
    NDIlib_compressed_packet_t  _compressPkg;
    uint8_t *_fullbuffer = nullptr;
    NDIlib_frame_scatter_t _scatter = {};
    video_send_data _sendData;
    stream_info_type _videoInfo;
    OptFFmpeg *_opt = nullptr;
};

#endif // SENDER_H

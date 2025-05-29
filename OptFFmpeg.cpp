#include <QDebug>
#include <QImage>
#include "OptFFmpeg.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}


#include <iostream>
#include <fstream>
const int BUF_SIZE = 8192;
#define MAX_CACHE_SIZE 5
std::ofstream _outFile("sendPlayData.h265", std::ios::binary);
std::ofstream _outFile1("recvPlayData.h265", std::ios::binary);


OptFFmpeg::OptFFmpeg(int w, int h, QObject *obj): QObject(obj), m_iWidth(w), m_iHeight(h)
{
    InitVariant();
}

bool OptFFmpeg::EncodeFrame(const uint8_t* rgb, video_frame& videoframe, int line)
{
    if (!m_pEnCodecCtx || !m_pFrame || !m_pEnSwsCtx) {
            return false;
        }
    // 填充 RGB 数据临时帧
    uint8_t* inData[1] = { const_cast<uint8_t*>(rgb) };
    int inLinesize[1] = { static_cast<int>(line) };
    // Convert RGB32 (BGRA) to YUV420P
    sws_scale(m_pEnSwsCtx, inData, inLinesize, 0, m_iHeight,
                  m_pFrame->data, m_pFrame->linesize);
//    if(m_iPts%3 == 0)
//    {
//        m_pFrame->pict_type = AV_PICTURE_TYPE_I;
//    }
//    else
//        m_pFrame->pict_type = AV_PICTURE_TYPE_NONE;
    m_pFrame->pts = m_iPts++;
    int ret = avcodec_send_frame(m_pEnCodecCtx, m_pFrame);
    if (ret < 0) return false;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;
    ret = avcodec_receive_packet(m_pEnCodecCtx, &pkt);

//    m_pExtraData = m_pEnCodecCtx->extradata;
//    m_iExtraDataSize = m_pEnCodecCtx->extradata_size;
//    if (m_pEnCodecCtx->extradata && m_pEnCodecCtx->extradata_size > 0) {
//        // 成功获取 extradata，通常包含 VPS/SPS/PPS
//        FILE* f = fopen("extradata.h265", "wb");
//        fwrite(m_pEnCodecCtx->extradata, 1, m_pEnCodecCtx->extradata_size, f);
//        fclose(f);

//        qDebug() << "Encoderctx is keyframe\n";
//    }

    bool result = false;
    if (ret == 0) {
        if(pkt.data == nullptr)
            qDebug() << "pkt.data is nullptr.\n";
        videoframe.p_data = reinterpret_cast<const uint8_t*>(pkt.data);
        videoframe.p_extra = m_pEnCodecCtx->extradata;
        videoframe.data_size =pkt.size;
        videoframe.extra_size = m_pEnCodecCtx->extradata_size;
        videoframe.is_keyframe = (pkt.flags & AV_PKT_FLAG_KEY);
        videoframe.pts = pkt.pts;
        videoframe.dts = pkt.dts;
        result = true;
    } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    } else {
        qWarning() << "receive_packet error:" ;
    }
    av_packet_unref(&pkt);
    return result;
}

bool OptFFmpeg::EncodeFrame(const uint8_t *rgb, QByteArray &output, int line)
{
    if (!m_pEnCodecCtx || !m_pFrame || !m_pEnSwsCtx) {
            return false;
        }
//    AVFrame* f = (AVFrame*)m_pFrame;
    // 填充 RGB 数据临时帧
    uint8_t* inData[1] = { const_cast<uint8_t*>(rgb) };
    int inLinesize[1] = { static_cast<int>(line) };
    // Convert RGB32 (BGRA) to YUV420P
    sws_scale(m_pEnSwsCtx, inData, inLinesize, 0, m_iHeight,
                  m_pFrame->data, m_pFrame->linesize);
    m_pFrame->pts = m_iPts++;
    int ret = avcodec_send_frame(m_pEnCodecCtx, m_pFrame);
    if (ret < 0) return false;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;
//    AVCodecContext* ctx = (AVCodecContext*)m_pEnCodecCtx;
    ret = avcodec_receive_packet(m_pEnCodecCtx, &pkt);
    while(true){
    if (ret == 0) {
        output = QByteArray(reinterpret_cast<char*>(pkt.data), pkt.size);
        av_packet_unref(&pkt);
        return true;
    } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
    } else {
        qWarning() << "receive_packet error:" ;
        break;
    }
    }
    return false;
}

void OptFFmpeg::CleanEncoder()
{
    if (m_pFrame) {
        av_frame_free((AVFrame**)&m_pFrame);
        m_pFrame = nullptr;
    }
    if (m_pEnCodecCtx) {
        avcodec_free_context((AVCodecContext**)&m_pEnCodecCtx);
        m_pEnCodecCtx = nullptr;
    }
    if (m_pEnSwsCtx) {
        sws_freeContext((SwsContext*)m_pEnSwsCtx);
        m_pEnSwsCtx = nullptr;
    }
}

bool OptFFmpeg::DecodeFrame(const uint8_t *data, int size, QImage &image)
{
    if (!m_pDeCodecCtx || !m_pDecoderParser || !m_pDeFrame || !m_pDeRgbFrame || !m_pDePkt) {
            qDebug() << "Failed to allocate FFmpeg structures\n";
            return false;
        }
    const uint8_t* input = data;
    int input_size = size;
    while(input_size > 0)
    {
        int chunk_size = FFMIN(input_size, BUF_SIZE);
        const uint8_t* chunk = input;
        while(chunk_size > 0)
        {
            av_packet_unref(m_pDePkt);
            int parsed = av_parser_parse2(m_pDecoderParser, m_pDeCodecCtx,
                                       &m_pDePkt->data, &m_pDePkt->size, chunk, chunk_size,
                                       AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (parsed < 0) {
                            qDebug() << "Parser error\n";
                            break;
                        }
            chunk += parsed;
            chunk_size -= parsed;

            if (m_pDePkt->size > 0) {
                if (avcodec_send_packet(m_pDeCodecCtx, m_pDePkt) == 0){
                    while (avcodec_receive_frame(m_pDeCodecCtx, m_pDeFrame) == 0) {
                        int width = m_pDeFrame->width;
                        int height = m_pDeFrame->height;

                        m_pDeSwsCtx = sws_getContext(width, height, (AVPixelFormat)m_pDeFrame->format,
                                                 width, height, AV_PIX_FMT_RGB24,
                                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
//                        av_image_alloc(m_pDeRgbFrame->data, m_pDeRgbFrame->linesize,
//                                                                   width, height, AV_PIX_FMT_RGB24, 1);
//                        m_pDeRgbFrame->width = width;
//                        m_pDeRgbFrame->height = height;
//                        m_pDeRgbFrame->format = AV_PIX_FMT_RGB24;

//                        int rgb_buf_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
//                        rgb_buffer = (uint8_t*)av_malloc(rgb_buf_size);
//                        av_image_fill_arrays(m_pDeRgbFrame->data, m_pDeRgbFrame->linesize,
//                                             rgb_buffer, AV_PIX_FMT_RGB24, width, height, 1);
                        int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_pDeFrame->width, m_pDeFrame->height, 1);
                        std::vector<uint8_t> rgb_buffer(num_bytes);
                        av_image_fill_arrays(m_pDeRgbFrame->data, m_pDeRgbFrame->linesize,
                                                                     rgb_buffer.data(), AV_PIX_FMT_RGB24,
                                                                     m_pDeFrame->width, m_pDeFrame->height, 1);
                        sws_scale(m_pDeSwsCtx,
                                  m_pDeFrame->data, m_pDeFrame->linesize,
                                  0, m_pDeFrame->height,
                                  m_pDeRgbFrame->data, m_pDeRgbFrame->linesize);
                    }
                }
            }
        }
        QImage img(m_pDeRgbFrame->data[0], m_iWidth, m_iHeight, m_pDeRgbFrame->linesize[0], QImage::Format_RGB888);
        image = img.copy();
        return true;
    }

    return false;
//    if (avcodec_send_packet(m_pDeCodecCtx, &pkt) < 0)
//        return false;
//    if (avcodec_receive_frame(m_pDeCodecCtx, m_pDeFrame) == 0) {
//        if (!m_pDeSwsCtx) {
//            m_pDeSwsCtx = sws_getContext(m_iWidth, m_iHeight,
//                                    (AVPixelFormat)m_pDeFrame->format,
//                                    m_iWidth, m_iHeight,
//                                    AV_PIX_FMT_RGB24, SWS_BILINEAR,
//                                    nullptr, nullptr, nullptr);
//            int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_iWidth, m_iHeight, 1);
//            uint8_t* rgb_data = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
//            av_image_fill_arrays(m_pDeRgbFrame->data, m_pDeRgbFrame->linesize,
//                                 rgb_data, AV_PIX_FMT_RGB24, m_iWidth, m_iHeight, 1);
//        }
////        QImage img(m_iWidth,m_iWidth, QImage::Format_RGB888);
////        uint8_t* dst[1] = { img.bits() };
////        int linesize[1] = { img.bytesPerLine() };
//        sws_scale((SwsContext*)m_pDeSwsCtx, m_pDeFrame->data, m_pDeFrame->linesize,
//                  0, m_iHeight, m_pDeRgbFrame->data, m_pDeRgbFrame->linesize);
//        QImage img(m_pDeRgbFrame->data[0], m_iWidth, m_iHeight, m_pDeRgbFrame->linesize[0], QImage::Format_RGB888);
//        image = img.copy();
//        return true;
//    }
//    return false;
}

void OptFFmpeg::CleanDecoder()
{
    if (m_pDeCodecCtx) avcodec_free_context((AVCodecContext**)&m_pDeCodecCtx);
    if (m_pDecoderParser) av_parser_close(m_pDecoderParser);
    if (m_pDeSwsCtx) sws_freeContext((SwsContext*)m_pDeSwsCtx);
    if(m_pDePkt)
        av_packet_free(&m_pDePkt);
    if(m_pDeFrame){
        av_frame_free(&m_pDeFrame);
        m_pDeFrame = nullptr;
    }
    if(m_pDeRgbFrame){
        av_frame_free(&m_pDeRgbFrame);
        m_pDeRgbFrame = nullptr;
    }
}

void OptFFmpeg::CacheWrite(QByteArray frame)
{
    if (_dataCache.size() >= 1)
           _dataCache.dequeue();  // 移除最早的一帧
    _dataCache.enqueue(frame);  // 添加新帧
}

void OptFFmpeg::Cache2File(const char *filename)
{
    std::ofstream out_file(filename, std::ios::binary);
    if (!out_file.is_open()) {
        std::cerr << "无法打开输出文件" << std::endl;
    }
    for (const QByteArray& frame : _dataCache)
    {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(frame.constData());
        int size = frame.size();
        out_file.write(reinterpret_cast<const char*>(data), size);
        out_file.flush();
        qDebug() << "frame size is " << size << "\n";
    }
    out_file.close();
}

void OptFFmpeg::Write1Frame2File(int size, const char *data, const char *filename)
{
    std::ofstream out_file(filename, std::ios::binary);
    if (!out_file.is_open()) {
        std::cerr << "无法打开输出文件" << std::endl;
    }
    out_file.write(data, size);
    out_file.flush();
    qDebug() << "one frame size is " << size << "\n";
    out_file.close();
}

void OptFFmpeg::WriteFrames2File(int size, const char *data, bool bSender)
{
    if(bSender)
    {
        if (!_outFile.is_open()) {
            std::cerr << "无法打开输出文件" << std::endl;
        }
        _outFile.write(data, size);
        _outFile.flush();
    }
    else
    {
        if (!_outFile1.is_open()) {
            std::cerr << "无法打开输出文件" << std::endl;
        }
        _outFile1.write(data, size);
        _outFile1.flush();
    }

}

void OptFFmpeg::CloseFile(bool bSender)
{
    if(bSender)
    {
        _outFile.close();
    }
    else
    {
        _outFile1.close();
    }
}

void OptFFmpeg::InitVariant()
{
    m_pEnCodecCtx = nullptr;
    m_pEnSwsCtx = nullptr;
    m_pDeCodecCtx = nullptr;
    m_pDeSwsCtx = nullptr;
    m_pEncoder = nullptr;
    m_pDecoder = nullptr;
    m_pFrame = nullptr;
    _dataCache.clear();
}

bool OptFFmpeg::InitEncoder()
{
//    m_pEncoder =  avcodec_find_encoder_by_name("libx265");//
    m_pEncoder = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if(!m_pEncoder)
    {
        qDebug() << "HEVC encoder not found \n";
        return false;
    }
    qDebug() << "Codec name: " << m_pEncoder->name << "\n";

    m_pEnCodecCtx =  avcodec_alloc_context3(m_pEncoder);
    m_pEnCodecCtx->width = m_iWidth;
    m_pEnCodecCtx->height = m_iHeight;
    m_pEnCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pEnCodecCtx->time_base = AVRational{1, 25};
    m_pEnCodecCtx->framerate = AVRational{25, 1};
    m_pEnCodecCtx->bit_rate = 4000000;
    m_pEnCodecCtx->gop_size = 5;
    m_pEnCodecCtx->max_b_frames = 0;
//    m_pEnCodecCtx->keyint_min = 1;    // 最小关键帧间隔为 1
//    av_opt_set(m_pEnCodecCtx->priv_data, "x265-params", "repeat-headers=1", 0); // 对 libx265 有效

//    av_opt_set(m_pEnCodecCtx->priv_data, "preset", "llhp", 0);  // 示例：低延迟高性能
//    av_opt_set(m_pEnCodecCtx->priv_data, "rc", "constqp", 0);   // 固定量化
//    av_opt_set(m_pEnCodecCtx->priv_data, "qp", "25", 0);

    av_opt_set(m_pEnCodecCtx->priv_data, "preset", "p4", 0);  // 示例：低延迟高性能
    av_opt_set(m_pEnCodecCtx->priv_data, "rc", "cbr", 0);
    av_opt_set(m_pEnCodecCtx->priv_data, "forced-idr", "1", 0);
    av_opt_set(m_pEnCodecCtx->priv_data, "repeat-headers", "1", 0);
//    av_opt_set(m_pEnCodecCtx->priv_data, "g", "1", 0);       // 每 3 帧一个 I 帧
//    av_opt_set(m_pEnCodecCtx->priv_data, "repeat-pps", "1", 0);
//    av_opt_set(m_pEnCodecCtx->priv_data, "preset", "ultrafast", 0);
//    av_opt_set(m_pEnCodecCtx->priv_data, "tune", "zerolatency", 0);
//    av_opt_set(m_pEnCodecCtx->priv_data, "annexb", "1", 0);
//    av_opt_set(m_pEnCodecCtx->priv_data, "repeat-headers", "1", 0);
//    m_pEnCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//    av_opt_set(m_pEnCodecCtx->priv_data, "g", "2", 0);       // 每 3 帧一个 I 帧
    av_opt_set(m_pEnCodecCtx->priv_data, "bf", "0", 0);      // 0 B-frames

    if (avcodec_open2(m_pEnCodecCtx, m_pEncoder, nullptr) < 0) {
        qDebug() << "Failed to open m_pDecoder";
        return false;
    }

    m_pFrame = av_frame_alloc();
    ((AVFrame*)m_pFrame)->format = m_pEnCodecCtx->pix_fmt;
    ((AVFrame*)m_pFrame)->width = m_pEnCodecCtx->width;
    ((AVFrame*)m_pFrame)->height = m_pEnCodecCtx->height;
    if (av_frame_get_buffer(m_pFrame, 32) < 0) {
           qDebug() << "Could not allocate frame buffer\n";
           return false;
       }
    m_pEnSwsCtx = sws_getContext(m_iWidth, m_iHeight, AV_PIX_FMT_BGRA,
                               m_iWidth, m_iHeight, AV_PIX_FMT_YUV420P,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_pEnSwsCtx) {
        qDebug() << "sws_getContext failed!";
        return false;
    }
    return true;
}

bool OptFFmpeg::InitDecoder()
{
    m_pDecoder = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    m_pDecoderParser = av_parser_init(m_pDecoder->id);
    if(!m_pDecoder)
    {
        qDebug() << "HEVC decoder not found \n";
        return false;
    }
    m_pDeCodecCtx =  avcodec_alloc_context3(m_pDecoder);
    if(!m_pDeCodecCtx) return false;
//    m_pDeCodecCtx->bit_rate = 4000000;
//    m_pDeCodecCtx->width = m_iWidth;
//    m_pDeCodecCtx->height = m_iHeight;
//    m_pDeCodecCtx->time_base = AVRational{1, 30};
//    m_pDeCodecCtx->framerate = AVRational{30, 1};
//    m_pDeCodecCtx->gop_size = 3;
//    m_pDeCodecCtx->max_b_frames = 0;
//    m_pDeCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
//    av_opt_set(m_pDeCodecCtx->priv_data, "repeat-headers", "1", 0);
//    av_opt_set(m_pDeCodecCtx->priv_data, "forced-idr", "1", 0);
//    av_opt_set(m_pDeCodecCtx->priv_data, "repeat-pps", "1", 0);
//    av_opt_set(m_pDeCodecCtx->priv_data, "preset", "ultrafast", 0);
//    av_opt_set(m_pDeCodecCtx->priv_data, "tune", "zerolatency", 0);
//    av_opt_set(m_pDeCodecCtx->priv_data, "bf", "0", 0);      // 0 B-frames

    if (avcodec_open2(m_pDeCodecCtx, m_pDecoder, nullptr) < 0) {
        qDebug() << "Failed to open m_pDecoder";
        return false;
    }
    m_pDePkt = av_packet_alloc();
    m_pDeSwsCtx = nullptr;
    m_pDeFrame = av_frame_alloc();
    m_pDeRgbFrame = av_frame_alloc();
    return true;

}

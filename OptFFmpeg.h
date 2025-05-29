#ifndef OPTFFMPEG_H
#define OPTFFMPEG_H

#include <QObject>
#include <QByteArray>
#include "defines.h"
#include <QQueue>

class AVCodecContext;
class SwsContext;
class AVCodec;
class AVFrame;
class AVPacket;
class AVCodecParserContext;

class OptFFmpeg: public QObject
{
    Q_OBJECT
public:
    OptFFmpeg(int w, int h, QObject *obj);
    bool InitEncoder();
    bool InitDecoder();
    bool EncodeFrame(const uint8_t* rgb, QByteArray &output, int line);
    bool EncodeFrame(const uint8_t* rgb, video_frame& videoframe, int line);
    void CleanEncoder();
    bool DecodeFrame(const uint8_t* data, int size, QImage& image);
    void CleanDecoder();

    void CacheWrite(QByteArray arr);
    void Cache2File(const char *filename);
    void Write1Frame2File(int size, const char *data, const char *filename);
    void WriteFrames2File(int size, const char *data, bool bSender = true);
    void CloseFile(bool bSender = true);

private:
    void InitVariant();
private:
    int m_iWidth;
    int m_iHeight;
    int m_iPts = 0;
    AVCodecContext *m_pEnCodecCtx = nullptr;
    AVCodecContext *m_pDeCodecCtx = nullptr;
    SwsContext *m_pEnSwsCtx = nullptr;
    SwsContext *m_pDeSwsCtx = nullptr;
    const AVCodec *m_pEncoder = nullptr;
    const AVCodec *m_pDecoder = nullptr;
    AVFrame *m_pFrame = nullptr;
    AVFrame *m_pDeFrame = nullptr;
    AVFrame *m_pDeRgbFrame = nullptr;
    unsigned char *m_pExtraData = nullptr;
    int m_iExtraDataSize;
    AVCodecParserContext *m_pDecoderParser = nullptr;

    AVPacket *m_pDePkt = nullptr;
    uint8_t* rgb_buffer = nullptr;
    QQueue<QByteArray> _dataCache;
};

#endif // OPTFFMPEG_H

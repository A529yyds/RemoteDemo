#include "defines.h"
#include "Receiver.h"
#include "OptFFmpeg.h"
#include "RecvAnalyzerTask.h"

RecvAnalyzerTask::RecvAnalyzerTask(QObject *obj): QThread(obj)
{
    m_OptFFmpeg = nullptr;
    m_pRecv = nullptr;
    m_running = true;
}

RecvAnalyzerTask::~RecvAnalyzerTask()
{
    if(m_OptFFmpeg)
        m_OptFFmpeg->CleanDecoder();
    DELETE(m_OptFFmpeg);
    DELETE(m_pRecv);
}

void RecvAnalyzerTask::stop()
{
    m_running = false;
}

#include <QDebug>
void RecvAnalyzerTask::run()
{
    m_OptFFmpeg = new OptFFmpeg(W_4K, H_4K, this);
    m_pRecv = new Receiver(this);
    NDIlib_video_frame_v2_t frame;
    QImage img;
    if(m_OptFFmpeg->InitDecoder())
    {
        uint8_t* pData;
        size_t iSize;
        while(m_running)
        {
            m_pRecv->GetFrame(frame);
            NDIlib_compressed_packet_t* p_packet = (NDIlib_compressed_packet_t*)(frame.p_data);
            if(p_packet && iSize != p_packet->data_size){
                pData = (uint8_t*)(p_packet + 1);
                iSize = p_packet->data_size;//frame.data_size_in_bytes;
                qDebug() << "recv data size is " << iSize << "; sender p_packet.pts:" << p_packet->pts << "; flags status is " << p_packet->flags;

                qDebug() <<"video p_packet size is"<< frame.data_size_in_bytes << ".\n";
                qDebug() <<"video p_hevc_data size is"<< iSize << ".\n";
                QByteArray f(reinterpret_cast<const char*>(/*frame.p_data*/pData), iSize);
                if(p_packet->flags == 1)
                {
//                    m_OptFFmpeg->Write1Frame2File(iSize, f.data(), "recvFrame");
//                    m_OptFFmpeg->CacheWrite(f);
                }
                m_OptFFmpeg->WriteFrames2File(iSize, f.data(), false);
//                m_OptFFmpeg->DecodeFrame(pData, iSize, img);
                emit signalRecvImage(img);
            }
        }
        m_OptFFmpeg->CloseFile(false);

//        m_OptFFmpeg->Cache2File("recvData.h265");
    }
}

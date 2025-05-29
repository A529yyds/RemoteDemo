#include "ScreenShotTask.h"
#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QDebug>

#include "Sender.h"
#include "OptFFmpeg.h"
#include "defines.h"

#include <QQueue>
#include <QByteArray>
#define MAX_CACHE_SIZE 5

ScreenShotTask::ScreenShotTask(QObject *obj): QThread(obj), m_running(true)
{
    m_OptFFmpeg = nullptr;
    m_pSender = nullptr;
}

ScreenShotTask::~ScreenShotTask()
{
    if(m_OptFFmpeg)
        m_OptFFmpeg->CleanEncoder();
    DELETE(m_OptFFmpeg);
    DELETE(m_pSender);
}

void ScreenShotTask::stop()
{
    m_running = false;
}

#include <iostream>
void ScreenShotTask::run()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if(!screen) return;
    m_OptFFmpeg = new OptFFmpeg(W_4K, H_4K, this);
    // config video frames' format
    m_pSender = new Sender("HEVC HX TEST", this);
    QQueue<QByteArray> hevcCache;
    video_frame videoFrame;
    if(m_OptFFmpeg->InitEncoder())
    {
        while(m_running)
        {
            QPixmap pix = screen->grabWindow(0);
            QImage image = pix.toImage().convertToFormat(QImage::Format_RGB32);
            image.bytesPerLine();
            if (m_OptFFmpeg->EncodeFrame(image.constBits(), videoFrame, image.bytesPerLine())) {
                QByteArray frame(reinterpret_cast<const char*>(videoFrame.p_data), videoFrame.data_size);
                if(videoFrame.is_keyframe == 1){
                    m_OptFFmpeg->CacheWrite(frame);
                    m_OptFFmpeg->Write1Frame2File(frame.size(), frame.data(), "ScreenShotTaskFrame");
                }
                m_OptFFmpeg->WriteFrames2File(frame.size(), frame.data());
                m_pSender->SendFrame(videoFrame);

            }
        }
        m_OptFFmpeg->CloseFile();
        m_OptFFmpeg->Cache2File("senderData.h265");

    }
//    DELETE(rgbData);
}

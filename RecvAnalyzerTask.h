#ifndef RECVANALYZERTASK_H
#define RECVANALYZERTASK_H
#include <QImage>
#include <QThread>
class OptFFmpeg;
class Receiver;
class RecvAnalyzerTask: public QThread
{
    Q_OBJECT
public:
    explicit RecvAnalyzerTask(QObject *obj = nullptr);
    ~RecvAnalyzerTask();
    void stop();

private:
    void run() override;

private:
    bool m_running;
    OptFFmpeg *m_OptFFmpeg;
    Receiver *m_pRecv;

signals:
    void signalRecvImage(QImage img);

};

#endif // RECVANALYZERTASK_H

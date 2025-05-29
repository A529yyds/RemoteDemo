#ifndef SCREENSHOTTASK_H
#define SCREENSHOTTASK_H
#include <QThread>
class OptFFmpeg;
class Sender;
class ScreenShotTask: public QThread
{
    Q_OBJECT
public:
    ScreenShotTask(QObject *parent = nullptr);
    ~ScreenShotTask();
    void stop();

protected:
    void run() override;

private:
    bool m_running;
    OptFFmpeg *m_OptFFmpeg;
    Sender *m_pSender;
};

#endif // SCREENSHOTTASK_H

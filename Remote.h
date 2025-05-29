#ifndef REMOTE_H
#define REMOTE_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Remote; }
QT_END_NAMESPACE

class ScreenShotTask;
class RecvAnalyzerTask;
class QWidget;
class QLabel;
class QPushButton;

class Remote : public QMainWindow
{
    Q_OBJECT

public:
    Remote(QWidget *parent = nullptr);
    ~Remote();

private:
    Ui::Remote *ui;
    ScreenShotTask *m_pScreenShotTask;
    RecvAnalyzerTask *m_pRecvImgTask;
    QWidget *m_pRecvWidget;
    QLabel *m_pImageLbl;
    QWidget *m_pSendWidget;
    QPushButton *m_pExitSenderBtn;

public slots:
    void slotBtnClicked();
    void slotStopSender();
    void slotRecvImage(QImage img);

private:
    void Init();
    void SenderFunc();
    void ReceiverFunc();
    uint8_t *getScreenData();

};
#endif // REMOTE_H

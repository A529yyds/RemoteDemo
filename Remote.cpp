#include "Remote.h"
#include "ScreenShotTask.h"
#include "RecvAnalyzerTask.h"
#include "ui_Remote.h"
#include "defines.h"

#include <QApplication>
#include <QDebug>
#include "ExampleDemo.h"

Remote::Remote(QWidget *parent)
    : QMainWindow(parent)
    , m_pScreenShotTask(nullptr)
    , m_pRecvImgTask(nullptr)
    , m_pRecvWidget(nullptr)
    , m_pImageLbl(nullptr)
    , m_pSendWidget(nullptr)
    , m_pExitSenderBtn(nullptr)
    , ui(new Ui::Remote)
{
    ui->setupUi(this);
    Init();
}

Remote::~Remote()
{
    DELETE(m_pScreenShotTask);
    DELETE(m_pRecvImgTask);
    DELETE(m_pRecvWidget);
    DELETE(m_pImageLbl);
    DELETE(m_pSendWidget);
    DELETE(m_pExitSenderBtn);
    DELETE(ui);
}

void Remote::slotBtnClicked()
{
    bool bDemo = false;
    QString objName = sender()->objectName();
    if(objName == "m_pSenderBtn")
    {
        qDebug()<<"m_pSenderBtn clicker \n";
        if(!bDemo)
            SenderFunc();
        else
            senderStart();
    }
    else if(objName == "m_pReceiverBtn")
    {
        qDebug()<<"m_pReceiverBtn clicker \n";
        if(!bDemo)
            ReceiverFunc();
        else
            receiverStart();
    }
}

void Remote::slotStopSender()
{
    if(sender()->objectName() == "StopSender")
        m_pScreenShotTask->stop();
    else if(sender()->objectName() == "StopRecv")
        m_pRecvImgTask->stop();
    this->show();
}

void Remote::slotRecvImage(QImage img)
{
    m_pRecvWidget->show();
    qDebug() << "slotRecvImage img size " << img.size() << "\n";
    m_pImageLbl->setPixmap(QPixmap::fromImage(img));
}

void Remote::Init()
{
    m_pScreenShotTask = new ScreenShotTask(this);
    m_pRecvImgTask = new RecvAnalyzerTask(this);
    m_pRecvWidget = new QWidget();
    m_pRecvWidget->setGeometry(0, 0, 200, 60);// W_4K, H_4K);
    m_pImageLbl = new QLabel(m_pRecvWidget);
    m_pImageLbl->setGeometry(m_pRecvWidget->geometry());
    m_pRecvWidget->hide();

    m_pSendWidget = new QWidget();
    m_pSendWidget->setGeometry(0, 0, 200, 60);
    m_pExitSenderBtn = new QPushButton(m_pSendWidget);
    m_pExitSenderBtn->setGeometry(0, 0, 200, 60);
    m_pExitSenderBtn->setText("StopSender");
    m_pExitSenderBtn->setObjectName("StopSender");
    connect(m_pExitSenderBtn, SIGNAL(clicked()), this, SLOT(slotStopSender()));

    QPushButton *pExitRecvBtn = new QPushButton(m_pRecvWidget);
    pExitRecvBtn->setGeometry(0, 0, 200, 60);
    pExitRecvBtn->setText("StopRecv");
    pExitRecvBtn->setObjectName("StopRecv");
    connect(pExitRecvBtn, SIGNAL(clicked()), this, SLOT(slotStopSender()));
    connect(m_pRecvImgTask, &RecvAnalyzerTask::signalRecvImage, this, &Remote::slotRecvImage);
    connect(ui->m_pSenderBtn, SIGNAL(clicked()), this, SLOT(slotBtnClicked()));
    connect(ui->m_pReceiverBtn, SIGNAL(clicked()), this, SLOT(slotBtnClicked()));
    connect(ui->m_pExitBtn, SIGNAL(clicked()), this, SLOT(close()));
 }

void Remote::SenderFunc()
{
    this->hide();
    m_pSendWidget->show();
    m_pScreenShotTask->start();

}

void Remote::ReceiverFunc()
{
    m_pRecvImgTask->start();
}

uint8_t *Remote::getScreenData()
{

}


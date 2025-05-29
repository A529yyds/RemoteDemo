/********************************************************************************
** Form generated from reading UI file 'Remote.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REMOTE_H
#define UI_REMOTE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *m_pSenderBtn;
    QPushButton *m_pReceiverBtn;
    QPushButton *m_pExitBtn;
    QLabel *label;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(327, 467);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        m_pSenderBtn = new QPushButton(centralwidget);
        m_pSenderBtn->setObjectName(QString::fromUtf8("m_pSenderBtn"));
        m_pSenderBtn->setGeometry(QRect(100, 140, 121, 61));
        m_pSenderBtn->setStyleSheet(QString::fromUtf8("font-size: 20px;\n"
""));
        m_pReceiverBtn = new QPushButton(centralwidget);
        m_pReceiverBtn->setObjectName(QString::fromUtf8("m_pReceiverBtn"));
        m_pReceiverBtn->setGeometry(QRect(100, 240, 121, 61));
        m_pReceiverBtn->setStyleSheet(QString::fromUtf8("font-size: 20px;"));
        m_pExitBtn = new QPushButton(centralwidget);
        m_pExitBtn->setObjectName(QString::fromUtf8("m_pExitBtn"));
        m_pExitBtn->setGeometry(QRect(100, 340, 121, 61));
        m_pExitBtn->setStyleSheet(QString::fromUtf8("font-size: 20px;\n"
""));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(70, 60, 191, 61));
        label->setStyleSheet(QString::fromUtf8("font-size: 25px;bond;"));
        label->setLineWidth(1);
        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        m_pSenderBtn->setText(QCoreApplication::translate("MainWindow", "Sender", nullptr));
        m_pReceiverBtn->setText(QCoreApplication::translate("MainWindow", "Receiver", nullptr));
        m_pExitBtn->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Remote Desktop", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Remote: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REMOTE_H

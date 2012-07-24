#ifndef UPDATE_APPLICATION_H
#define UPDATE_APPLICATION_H
// ****************************************************************************
//  update_application.h                                            Tao project
// ****************************************************************************
//
//   File Description:
//
//     Define how update Tao application.
//
//
//
//
//
//
//
// ****************************************************************************
// This software is property of Taodyne SAS - Confidential
// Ce logiciel est la propriété de Taodyne SAS - Confidentiel
//  (C) 2012 Baptiste Soulisse <baptiste.soulisse@taodyne.com>
//  (C) 2012 Taodyne SAS
// ****************************************************************************
#include "repository.h"
#include "process.h"
#include "tao.h"
#include <QObject>
#include <QString>
#include <QProgressDialog>
#include <QFile>
#include <QtNetwork>
#include <QMessageBox>
#include <QGridLayout>
#include <QPushButton>
#include <QProgressBar>

namespace Tao {


class UpdateApplication : public QObject
// ------------------------------------------------------------------------
//   Asynchronously update the main application
// ------------------------------------------------------------------------
{
    Q_OBJECT

public:
    UpdateApplication();
    ~UpdateApplication();

    void     close();
    void     check(bool msg = false);

private:
    void     update();
    void     readIniFile();
    void     createFile();
    void     writeReplyToFile();

private slots:
    void     processCheckForUpdateReply();
    void     downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void     cancelDownload();
    void     downloadFinished();

    std::ostream & debug();

private:
    // Tao info
    double                   version;
    QString                  edition;
    QString                  system;

    // Update info
    double                   remoteVersion;
    QString                  fileName;
    QUrl                     url;

    // I/O
    QFile *                  file;
    QFileInfo                info;
    QProgressDialog *        dialog;
    bool                     useMsg;       // Show message boxes

    // Network
    QNetworkReply *          reply;
    QNetworkRequest          request;
    QNetworkAccessManager *  manager;
    QTime                    downloadTime;
    bool                     updating;
    bool                     downloadRequestAborted;
};

}

#endif // UPDATE_APPLICATION_H

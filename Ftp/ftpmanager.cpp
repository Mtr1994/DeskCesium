#include "ftpmanager.h"

#include <QFile>

// test
#include <QDebug>

FtpProtocol::FtpProtocol(const QString &host, const QString &user, const QString &pass)
    : mFtpHost(host), mFtpUserName(user), mFtpUserPass(pass)
{
    // 填充状态码
    mStatusObject = QJsonDocument::fromJson("{"
                                            "\"CONE\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"N\"},"
                                            "\"ABOR\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"ALLO\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"DELE\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"CWD\":  {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"CDUP\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"SMNT\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"HELP\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"MODE\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"NOOP\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"PASV\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"QUIT\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"SITE\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"PORT\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"SYST\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"STAT\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"RMD\":  {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"MKD\":  {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"PWD\":  {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"STRU\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"TYPE\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"APPE\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"LIST\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"NLST\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"REIN\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"RETR\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"STOR\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"STOU\": {\"1\": \"W\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"RNFR\": {\"1\": \"E\", \"2\": \"E\", \"3\": \"V\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"RNTO\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"REST\": {\"1\": \"E\", \"2\": \"E\", \"3\": \"V\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"USER\": {\"1\": \"E\", \"2\": \"E\", \"3\": \"V\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"PASS\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"V\", \"4\": \"F\", \"5\": \"F\"},"
                                            "\"ACCT\": {\"1\": \"E\", \"2\": \"S\", \"3\": \"E\", \"4\": \"F\", \"5\": \"F\"}"
                                            "}").object();
}

void FtpProtocol::slot_start_connect_ftp_server()
{
    if ((nullptr != mSocketCommand) && (mSocketCommand->state() == QTcpSocket::ConnectedState)) return;

    mListCommand.clear();

    // 准备连接文件服务
    mListCommand.append("CONE");

    mSocketCommand = new QTcpSocket(this);
    connect(mSocketCommand, &QTcpSocket::disconnected, this, &FtpProtocol::slot_command_socket_closed);
    connect(mSocketCommand, &QTcpSocket::readyRead, this, &FtpProtocol::slot_recv_command_socket_data);
    mSocketCommand->connectToHost(QHostAddress(mFtpHost), 21);
}

void FtpProtocol::slot_start_file_download()
{
    mDownloadFileFlag = true;
    mLocalFileInfo = QFileInfo(QString("%1/%2").arg(mDownloadPath, mFileName));
}

void FtpProtocol::slot_start_upload_file(const QString &file, const QString &remotePath)
{
    mUploadFileFlag = true;
    mFileName = file;
    mRemoteFilePath = remotePath;
    mLocalFileInfo = QFileInfo(file);
    // 重置上传的文件大小
    mTotalUploadLength = 0;

    mListCommand.append(QString("PASV\r\n").toUtf8());
    mListCommand.append(QString("TYPE A\r\n").toUtf8());

    // 返回到顶层文件夹
    mListCommand.append(QString("CWD /home/ftp_root\r\n").toUtf8());

    // 切换路径 (目录只能一层层进入，因为碰到最内部文件夹不存在，没办法一次创建多层文件夹)
    auto listDirName = QDir(mRemoteFilePath).path().split('/');
    for (auto &name : listDirName)
    {
        mListCommand.append(QString("CWD %1\r\n").arg(name).toUtf8());
    }

    mListCommand.append(QString("TYPE I\r\n").toUtf8());
    mListCommand.append(QString("STOR %1\r\n").arg(mLocalFileInfo.fileName()).toUtf8());

    sendNextCommannd();
}

void FtpProtocol::slot_data_socket_recv_data()
{
    QByteArray array = mSocketData->readAll();
    if (mListCommand.size() > 0)
    {
        QString cmd = mListCommand.first().left(4).trimmed();
        if (cmd == "NLST")
        {
            mServerDirInfo += array.toStdString().data();
        }
        else if (cmd == "LIST")
        {
            mServerFileInfo += array.toStdString().data();
        }
        else if (cmd == "RETR")
        {
            mTotalDownloadLength += array.size();

            float percent = mTotalDownloadLength * 1.0 / mTotalFileSize;
            emit sgl_file_download_process(mFileName, percent * 100);
            mFileStream.write(array, array.size());
        }
    }
}

void FtpProtocol::slot_data_socket_closed()
{
    // qDebug() << "slot_data_socket_closed " << mListCommand;
    mDataRecvFlag = true;
    if (!mCommandRecvFlag) return;
    if (mListCommand.size() == 0) return;
    QString cmd = mListCommand.first().left(4).trimmed();
    if (cmd == "NLST")
    {
        parse_nlst_data_pack();
    }
    else if (cmd == "LIST")
    {
        parse_list_data_pack();
    }
    else if (cmd == "RETR")
    {
        parse_retr_data_pack();
    }
    else
    {
        mResultMessage = "数据传输功能异常";
        return clear();
    }
}

void FtpProtocol::slot_command_socket_connect()
{
    qDebug() << "want server ";
    mListCommand.clear();

    // 准备登录
    mListCommand.append(QString("USER %1\r\n").arg(mFtpUserName).toUtf8());
    mListCommand.append(QString("PASS %1\r\n").arg(mFtpUserPass).toUtf8());

    sendNextCommannd();
}

void FtpProtocol::slot_command_socket_closed()
{
    //qDebug() << "slot_command_socket_closed ";
    if (mTaskStatus) return;
    // 服务连接失败
    mResultMessage = "服务器连接断开";
    clear();
}

void FtpProtocol::slot_recv_command_socket_data()
{
    if (mListCommand.size() == 0) return;
    QByteArray datagram = mSocketCommand->readAll();
    QString str = QString::fromLocal8Bit(datagram).split("\r\n", Qt::SkipEmptyParts).last();
    QString cmd = mListCommand.first().left(4).trimmed();
    QString status = mStatusObject.value(cmd).toObject().value(str.at(0)).toString();

    //qDebug() << "mResultMessage " << str << " " << status << " " << mListCommand << " " << cmd;
    if (cmd != "QUIT") mResultMessage = str;

    // 如果命令失败，立即返回
    if ((status == "E") || ((status == "F")))
    {
        // 但是如果是改变目录失败，允许直接发送新建
        if (cmd == "CWD")
        {
            QString floder = mListCommand.first().split(" ").last().remove("\r\n");
            mListCommand.insert(0, QString("MKD %1\r\n").arg(floder).toUtf8());
            sendNextCommannd();
            return;
        }
        else
        {
            qDebug() << "文件传输异常";
            clear();
            return;
        }
    }

    // 部分命令需要处理
    if (cmd == "CONE")
    {
        mListCommand.removeFirst();
        // 准备登录
        mListCommand.append(QString("USER %1\r\n").arg(mFtpUserName).toUtf8());
        mListCommand.append(QString("PASS %1\r\n").arg(mFtpUserPass).toUtf8());
    }
    else if (cmd == "PASS")
    {
        mListCommand.removeFirst();
        emit sgl_connect_to_ftp_server_status_change(status == "S");

        // 不存在其他命令，不用继续执行
        return;
    }
    else if (cmd == "PASV")
    {
        mListCommand.removeFirst();
        QStringList list = str.split(',');
        uint32_t p1 = list.at(4).toUInt() * 256;
        uint32_t p2 = QString(list.at(5)).remove(".").remove(")").toUInt();
        if (nullptr == mSocketData)
        {
            mSocketData = new QTcpSocket;
            connect(mSocketData, &QTcpSocket::readyRead, this, &FtpProtocol::slot_data_socket_recv_data);
            connect(mSocketData, &QTcpSocket::disconnected, this, &FtpProtocol::slot_data_socket_closed);
        }
        mSocketData->close();
        mSocketData->connectToHost(QHostAddress(mFtpHost), p1 + p2);
    }
    else if ((cmd == "NLST") && (status == "S"))
    {
        mCommandRecvFlag = true;
        if (!mDataRecvFlag) return;
        return parse_nlst_data_pack();
    }
    else if ((cmd == "LIST") && (status == "S"))
    {
        mCommandRecvFlag = true;
        if (!mDataRecvFlag) return;
        return parse_list_data_pack();
    }
    else if (((cmd == "APPE") || (cmd == "STOR")) && (status == "W"))
    {
        //mListCommand.removeFirst();
        mFileStream.open(mFileName.toLocal8Bit().toStdString(), ios::binary | ios::in);
        if (!mFileStream.is_open())
        {
            mResultMessage = "本地文件打开失败";
            mListCommand.append(QString("QUIT\r\n").toUtf8());
        }

        mTotalFileSize = mLocalFileInfo.size();
        if (mTotalUploadLength > mTotalFileSize)
        {
            mResultMessage = "服务器文件大小异常";
            mListCommand.append(QString("QUIT\r\n").toUtf8());
        }
        else if (mTotalUploadLength == mTotalFileSize)
        {
            emit sgl_file_upload_process(mFileName, 100);
            mResultMessage = "文件上传完成";
            mTaskStatus = true;
        }

        // 检查网络状态
        int tryCount = 0;
        while (mSocketData->state() != QTcpSocket::ConnectedState)
        {
            tryCount++;
            if(tryCount > 5 * 15)
            {
                mResultMessage = "无法连接到服务器";
                return clear();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // 定位本地文件读取位置
        mFileStream.seekg(mTotalUploadLength);
        //qDebug() << "mTotalUploadLength " << mTotalUploadLength;
        while (mFileStream.peek() != EOF)
        {
            uint64_t subSize = 0;
            QByteArray array;
            if (mTotalFileSize <= 1460) subSize = mTotalFileSize;
            else if ((mTotalFileSize - mTotalUploadLength) <= 1460) subSize = mTotalFileSize - mTotalUploadLength;
            else subSize = 1460;

            array.resize(subSize);
            mFileStream.read(array.data(), subSize);
            // 有可能还没有成功连接
            mSocketData->write(array.data(), subSize);
            mSocketData->flush();
            mSocketData->waitForBytesWritten(-1);

            mTotalUploadLength += subSize;
            float percent = mTotalUploadLength * 1.0 / mTotalFileSize;
            emit sgl_file_upload_process(mFileName, percent * 100);
        }

        mSocketData->close();
        // 等待上传完成的信号就好，不用继续执行后续逻辑
        return;
    }
    else if ((cmd == "STOR") && (status == "S"))
    {
        mListCommand.removeFirst();

        mResultMessage = "文件上传完成，退出登录";
        mTaskStatus = true;

        // 发送结果消息
        emit sgl_ftp_task_response(mFileName, mTaskStatus, mResultMessage);

        // 发送任务结束信号
        emit sgl_ftp_upload_task_finish(mFileName, mTaskStatus);
    }
    else if ((cmd == "QUIT") && (status == "S"))
    {
        mListCommand.removeFirst();
        clear();
    }
    else if ((cmd == "MKD") && (status == "S"))
    {
        mListCommand.removeFirst();
    }
    else
    {
        if (status == "W") return;
        mListCommand.removeFirst();
    }

    sendNextCommannd();
}

void FtpProtocol::clear()
{
    //qDebug() << "stop ftp protocol";
    // 文件流关闭
    if (mFileStream.is_open())
    {
        mFileStream.close();
    }
    // 数据通道关闭
    if (nullptr != mSocketData)
    {
        disconnect(mSocketData, &QTcpSocket::readyRead, this, &FtpProtocol::slot_data_socket_recv_data);
        disconnect(mSocketData, &QTcpSocket::disconnected, this, &FtpProtocol::slot_data_socket_closed);
        mSocketData->close();
        mSocketData->deleteLater();
    }
    // 命令通道关闭
    if (nullptr != mSocketCommand)
    {
        mSocketCommand->close();
        mSocketCommand->deleteLater();
    }

    mListCommand.clear();

    // 发送结果消息
    emit sgl_ftp_task_response(mFileName, mTaskStatus, mResultMessage);

    // 发送任务结束信号
    emit sgl_ftp_upload_task_finish(mFileName, mTaskStatus);
    this->deleteLater();
}

void FtpProtocol::sendNextCommannd()
{
    if (mListCommand.length() > 0)
    {
        QString cmd = mListCommand.first();
        //qDebug() << "send " << cmd << " size " << mListCommand;
        mSocketCommand->write(cmd.toStdString().data());
        mSocketCommand->flush();
    }
}

int64_t FtpProtocol::parseFileSize()
{
    auto list =  mServerFileInfo.split(' ', Qt::SkipEmptyParts);
    //qDebug() << "mServerFileInfo " << mServerFileInfo;
    if (mServerFileInfo.startsWith('-')) // linux 下的文件标志
    {
        // 文件名称可能有空格存在，导致数量大于 9
        if (list.size() < 9)
        {
            mResultMessage = "Linux 系统文件大小解析失败";
            clear();
            return -1;
        }
        else
        {
            return list.at(4).toULongLong();
        }
    }
    else
    {
        if (list.size() != 4)
        {
            mResultMessage = "Windows 系统文件大小解析失败";
            clear();
            return -1 ;
        }
        else
        {
            return list.at(2).toULongLong();
        }
    }
}

void FtpProtocol::downloadFile()
{
    QString filePath = QString("%1/%2").arg(mDownloadPath, mFileName);
    uint64_t length = mLocalFileInfo.size();
    mTotalDownloadLength = length; // 默认已下载大小

    mFileStream.open(filePath.toLocal8Bit().toStdString(), ios::binary | ios::out | ((length > 0) ? ios::app : ios::trunc));
    if (!mFileStream.is_open())
    {
        mResultMessage = "无法创建本地文件";
        return clear();
    }

    mListCommand.append(QString("PASV\r\n").toUtf8());
    mListCommand.append(QString("TYPE I\r\n").toUtf8());

    if (length > 0)
    {
        mListCommand.append(QString("REST %1\r\n").arg(QString::number(length)).toUtf8());
    }

    mListCommand.append(QString("RETR %1\r\n").arg(mLocalFileInfo.fileName()).toUtf8());
    sendNextCommannd();
}

void FtpProtocol::uploadFile()
{

    mListCommand.append(QString("PASV\r\n").toUtf8());
    mListCommand.append(QString("TYPE I\r\n").toUtf8());
    mListCommand.append(QString("STOR %1\r\n").arg(mLocalFileInfo.fileName()).toUtf8());

    sendNextCommannd();
}

void FtpProtocol::parse_nlst_data_pack()
{
    //qDebug() << "parse nlst " << mServerDirInfo << " " << mFileName;
    mCommandRecvFlag = false;
    mDataRecvFlag = false;
    mListCommand.removeFirst();
    auto list = mServerDirInfo.split("\r\n", Qt::SkipEmptyParts);

    if (!list.contains(mLocalFileInfo.fileName()))
    {
        if (mDownloadFileFlag)
        {
            mResultMessage = "文件不存在";

            // 退出登录
            mListCommand.append(QString("QUIT\r\n").toUtf8());
            sendNextCommannd();
        }
        else if (mUploadFileFlag)
        {
            mTotalUploadLength = 0;
            uploadFile();
        }
    }
    else
    {
        mListCommand.append(QString("PASV\r\n").toUtf8());
        mListCommand.append(QString("TYPE A\r\n").toUtf8());
        mListCommand.append(QString("LIST %1\r\n").arg(mLocalFileInfo.fileName().toUtf8().data()));
        sendNextCommannd();
    }
}

void FtpProtocol::parse_list_data_pack()
{
    mCommandRecvFlag = false;
    mDataRecvFlag = false;
    mListCommand.removeFirst();
    if (mDownloadFileFlag)
    {
        mTotalFileSize = parseFileSize();
        if (mTotalFileSize < 0) return;
        downloadFile();
    }
    else if (mUploadFileFlag)
    {
        mTotalUploadLength = parseFileSize();
        if (mTotalUploadLength < 0) return;
        uploadFile();
    }
}

void FtpProtocol::parse_retr_data_pack()
{
    mCommandRecvFlag = false;
    mDataRecvFlag = false;
    mListCommand.removeFirst();
    mFileStream.close();
    if (mTotalDownloadLength != mTotalFileSize)
    {
        mResultMessage = "文件大小异常";
    }
    else
    {
        emit sgl_file_download_process(mFileName, 100.00);

        mResultMessage = "文件下载完成，退出登录";
        mTaskStatus = true;
    }

    // 退出登录
    mListCommand.append(QString("QUIT\r\n").toUtf8());
    sendNextCommannd();
}

FtpManager::FtpManager(const QString &host, const QString &user, const QString &pass, QObject *parent)
    : QObject{parent}, mFtpProtocol(host, user, pass)
{

}

void FtpManager::downloadFile(const QString &file, const QString &remotePath)
{
    if (mFtpHost.isEmpty())
    {
        emit sgl_ftp_task_response(file, false, "未指定文件服务器地址");
        return;
    }
}

void FtpManager::uploadFile(const QString &file, const QString &remotePath)
{
    if (!QFile::exists(file))
    {
        emit sgl_ftp_task_response(file, false, "文件不存在");
        return;
    }
    qDebug() << "FtpManager::uploadFile " << file;
    emit sgl_start_upload_file(file, remotePath);
}

void FtpManager::slot_ftp_task_response(const QString &file, bool status, const QString &msg)
{
    Q_UNUSED(status);
    if (!mMapThread.contains(file)) return;
    auto thread = mMapThread.take(file);
    connect(thread, &QThread::finished, this, [thread](){ thread->deleteLater();});
    thread->exit(0);

    // 发送消息给前端，判定任务状态
    emit sgl_ftp_task_response(file, status, msg);
}

void FtpManager::init()
{
    mFtpProtocol.moveToThread(&mWorkerThread);
    connect(&mFtpProtocol, &FtpProtocol::sgl_file_upload_process, this, &FtpManager::sgl_file_upload_process, Qt::QueuedConnection);
    connect(&mFtpProtocol, &FtpProtocol::sgl_ftp_task_response, this, &FtpManager::slot_ftp_task_response, Qt::QueuedConnection);
    connect(&mFtpProtocol, &FtpProtocol::sgl_ftp_upload_task_finish, this, &FtpManager::sgl_ftp_upload_task_finish, Qt::QueuedConnection);

    connect(&mFtpProtocol, &FtpProtocol::sgl_connect_to_ftp_server_status_change, this, &FtpManager::sgl_connect_to_ftp_server_status_change, Qt::QueuedConnection);
    connect(this, &FtpManager::sgl_start_connect_ftp_server, &mFtpProtocol, &FtpProtocol::slot_start_connect_ftp_server, Qt::QueuedConnection);
    connect(this, &FtpManager::sgl_start_upload_file, &mFtpProtocol, &FtpProtocol::slot_start_upload_file, Qt::QueuedConnection);

    mWorkerThread.start();

    // 连接文件服务器
    emit sgl_start_connect_ftp_server();
}

const QString &FtpManager::getDownloadPath() const
{
    return mDownloadPath;
}

void FtpManager::setDownloadPath(const QString &newDownloadPath)
{
    mDownloadPath = newDownloadPath;
}

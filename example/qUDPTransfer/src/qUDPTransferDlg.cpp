//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qUDPTransfer                      #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: XXX                             #
//#                                                                        #
//##########################################################################

// First:
//	Replace all occurrences of 'qUDPTransfer' by your own plugin class name in this file.
//	This includes the resource path to info.json in the constructor.

// Second:
//	Open qUDPTransfer.qrc, change the "prefix" and the icon filename for your plugin.
//	Change the name of the file to <yourPluginName>.qrc

// Third:
//	Open the info.json file and fill in the information about the plugin.
//	 "type" should be one of: "Standard", "GL", or "I/O" (required)
//	 "name" is the name of the plugin (required)
//	 "icon" is the Qt resource path to the plugin's icon (from the .qrc file)
//	 "description" is used as a tootip if the plugin has actions and is displayed in the plugin dialog
//	 "authors", "maintainers", and "references" show up in the plugin dialog as well


#include "qUDPTransferDlg.h"
#include <QDebug>
#include <QtEndian>  // 新增：Qt跨平台字节序转换头文件


// -------------------------- 新增：和C++服务端一致的命令结构体定义 --------------------------
// 1. 命令类型枚举
enum CmdType {
    CMD_HELLO = 1,        // 客户端问候（让服务端记录客户端IP）
    CMD_STARTALGO = 2,    // 开始扫描（对应服务端"mapping command comes!"）
    CMD_STOP = 10         // 停止任务（对应服务端"stop command comes!"）
    // 如需其他命令（如CMD_STARTLOCATION/CMD_RESET），按服务端枚举值补充
};

// 2. 命令头部结构体（和C++服务端CmdPack完全一致，1字节对齐避免解析错位）
#pragma pack(1)
struct CmdPack {
    uint32_t msgType;  // 命令类型（如CMD_STARTALGO=2）
    uint32_t msgLen;   // 命令后续数据长度（无额外数据则为0）
};
#pragma pack()
// ----------------------------------------------------------------------------------------

qUDPTransferDlg::qUDPTransferDlg(ccMainAppInterface* app)
    : QDialog(app ? app->getMainWindow() : nullptr, Qt::WindowCloseButtonHint)
    , Ui::UDPTransferDialog()
    , m_app(app)
    , m_container(new ccHObject)
    , m_pointCloud(new qUDPPointCloud)
    , m_socket(new QUdpSocket)
    , m_scannerIP("10.42.0.1")//扫描默认ip  10.42.0.1
    , m_scannerPort(20000)//扫描默认端口
    , m_colorScale(ccColorScale::Create("Temp Scale"))
    , isListening(false)
{
    setupUi(this);
    {
        m_container->setName(QString("UDP Receive (%1)")
                                 .arg(QDateTime::currentDateTime().toString()));
        m_pointCloud->setName((QString("Point Cloud (%1)")
                                   .arg(QTime::currentTime().toString())));
        m_pointCloud->setGlobalScale(25.0);
        m_container->addChild(m_pointCloud);
        m_app->addToDB(m_container);

    }

    // config socket
    {
        try
        {
            m_socket->bind(20000); // FIXME: AnyIpv4 have big problem!!!! app cannot receive data!!!!!!!
        }
        catch (const std::exception&)
        {
            dispToConsole("Invalid IP or Port");
            return;
        }
        IPInput->setText("10.42.0.1"); // UI输入框默认显示广播地址
        PortInput->setText("20000");// UI输入框默认显示本地端口

    }

    // {
    //     try
    //     {



    //         bool bindOk = m_socket->bind(QHostAddress::Any, 20000);
    //         if (bindOk)
    //         {
    //             dispToConsole("UDP socket bound to all interfaces: 0.0.0.0:20000");
    //         }
    //         else
    //         {
    //             dispToConsole("UDP bind failed: " + m_socket->errorString());
    //         }
    //     }
    //     catch (const std::exception& e)
    //     {
    //         dispToConsole("Bind error: " + QString(e.what()));
    //         return;
    //     }
    //     IPInput->setText("10.42.0.1");
    //     PortInput->setText("20000");
    // }

    // {
    //     try
    //     {
    //         dispToConsole("UDP客户端初始化完成（无固定端口，系统分配临时端口）");


    //     }
    //     catch (const std::exception& e)
    //     {
    //         dispToConsole("客户端初始化错误：" + QString(e.what()));
    //         return;
    //     }
    //     // UI输入框显示的是“服务端的IP和端口”，保留不变
    //     IPInput->setText("10.42.0.1");   // 服务端IP（和C++服务端一致）
    //     PortInput->setText("20000");     // 服务端端口（和C++服务端一致）
    // }

    // connect signals
    {
        connect(StartStopButton, &QPushButton::clicked,
                this, &qUDPTransferDlg::onStartStopButtonClick);
        connect(UpdateAddressButton, &QPushButton::clicked,
                this, &qUDPTransferDlg::onUpdateAddressButtonClick);
         connect(m_socket, &QUdpSocket::readyRead,
                 this, &qUDPTransferDlg::dataProcessor);
    }

    // init color scale
    {
        m_colorScale->insert(ccColorScaleElement(0.0, QColor("black")));
        m_colorScale->insert(ccColorScaleElement(1.0, QColor("white")));
        m_colorScale->insert(ccColorScaleElement(0.1, QColor("red")));
        m_colorScale->insert(ccColorScaleElement(0.25, QColor("green")));
        m_colorScale->insert(ccColorScaleElement(0.75, QColor("blue")));
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &qUDPTransferDlg::dataProcessor);
    m_timer->setInterval(30); // 设置定时器间隔，例如20毫秒
}

qUDPTransferDlg::~qUDPTransferDlg()
{
    m_socket->disconnectFromHost();
    delete m_socket;

    if (m_pointCloud->size() <= 100)
    {
        m_app->removeFromDB(m_container);
        m_container = nullptr;
        m_pointCloud = nullptr;
    }

    /*m_pointCloud->showColors(true);
    m_pointCloud->setRGBColorByHeight(2, m_colorScale);*/
}

void qUDPTransferDlg::sendHexData(const QString& hexString)
{
    // 去除字符串中的空格和其他可能的分隔符
    QString cleanedHex = hexString.trimmed().replace(" ", "");

    // 检查字符串长度是否为偶数（每个字节需要两个十六进制字符）
    if (cleanedHex.length() % 2 != 0) {
        dispToConsole("错误: 十六进制字符串长度必须为偶数");
        return;
    }

    // 将十六进制字符串转换为QByteArray[1,6](@ref)
    QByteArray data = QByteArray::fromHex(cleanedHex.toLatin1());

    if (data.isEmpty()) {
        dispToConsole("错误: 十六进制转换失败，请检查输入格式");
        return;
    }

    // 使用已绑定的socket发送数据[2](@ref)
    qint64 bytesSent = m_socket->writeDatagram(data, QHostAddress(m_scannerIP), m_scannerPort);

    if (bytesSent == -1) {
        dispToConsole("发送失败: " + m_socket->errorString());
    } else {
        dispToConsole(QString("sand success %1 hex %2:%3")
                          .arg(bytesSent)
                          .arg(m_scannerIP.toString())
                          .arg(m_scannerPort));
    }
}

void qUDPTransferDlg::dataProcessor()
{
    dispToConsole("is in dataProcessor");
    if (!isListening) return; // 检查是否在监听状态

    while (m_socket->hasPendingDatagrams()) { // 使用 while 循环读取所有数据报
        auto datagram = m_socket->receiveDatagram();
        m_pointCloud->addPoints(datagram.data());
    }
    m_pointCloud->refreshDisplay();
    dispToConsole(QString("Current point size: %1").arg(m_pointCloud->size()));
}

void qUDPTransferDlg::dispToConsole(const QString& msg)
{
    m_app->dispToConsole(msg);
}

void qUDPTransferDlg::onStartStopButtonClick()
{
   // m_pointCloud->addPoints(QStringLiteral("D:\\200_work\\cc\\pcData\\points.xyzi"));
    if (isListening)
    {
        // 停止状态：发送CMD_STOP命令给C++服务端
        isListening = false;
        StartStopButton->setText("Start");
        // QByteArray ba("Stop work");
        // if (m_socket->writeDatagram(ba, QHostAddress(m_scannerIP), m_scannerPort) > 0)
        // {
        //     m_app->dispToConsole("Command send success!");
        // }
        QString hexData = "0000000000000000";
        sendHexData(hexData);
         m_timer->stop();
    }
    else
    {
        // 启动状态：发送CMD_STARTALGO命令给C++服务端
        isListening = true;
        StartStopButton->setText("Stop");

        // QByteArray ba("Start work");
        // if (m_socket->writeDatagram(ba, QHostAddress(m_scannerIP), m_scannerPort) > 0)
        // {
        //     m_app->dispToConsole("Command send success!");
        // }


        QString hexData = "0000000000000000323032352d30392d30392d30382d34362d343700000000000000000000000000323032352d30392d30392d30382d34362d343702870200000000000000000000307df07e87020000a09ac40287020000307df07e87020000e0efa916f87f00000000fe73870200000000000000000000500ec4028702000010fc44028702000003000000a5000000800ec402870200000800000000000000feffffffffffffff1300c40287020000f778bf68000000000100010001010100006d000000000000000061696e657220ffcce97387020000888fe973870200000000440287020000";
        sendHexData(hexData);
         m_timer->start();
       // dataProcessor();
    }
}

void qUDPTransferDlg::onUpdateAddressButtonClick()
{
    // 更新服务端IP和端口（从UI输入框读取）
    m_scannerIP = QHostAddress(IPInput->text());
    m_scannerPort = PortInput->text().toUInt();
    //新增
    dispToConsole(QString("Scanner address updated to:1 %1:%2").arg(m_scannerIP.toString()).arg(m_scannerPort));
    dispToConsole("Scanner address update success!");
}

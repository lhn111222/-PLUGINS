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


#pragma once
#include "ui_qUDPTransfer.h"
#include "qUDPPointCloud.h"

#include <ccMainAppInterface.h>
#include <ccGLWindow.h>
#include <ccPointCloud.h>
#include <ccColorScale.h>
#include <ccHObject.h>

#include <QMainWindow>
#include <QFile>
#include <QNetworkDatagram> // UDP数据报封装类（Qt 5.12+，用于处理UDP数据包）
#include <QUdpSocket>      // UDP套接字类（核心网络通信类）
#include <QtConcurrent>
#include <QColor>
#include <QTimer>


class qUDPTransferDlg : public QDialog, public Ui::UDPTransferDialog
{
    Q_OBJECT

public:
	explicit qUDPTransferDlg(ccMainAppInterface* app = nullptr);
	~qUDPTransferDlg();
	void dataProcessor();

public slots:
	void dispToConsole(const QString& msg);
	void onStartStopButtonClick();
	void onUpdateAddressButtonClick();
    void qUDPTransferDlg::sendHexData(const QString& hexString);

private:
	ccMainAppInterface* m_app;
	ccHObject* m_container;
	qUDPPointCloud* m_pointCloud;
	QUdpSocket* m_socket;
	QHostAddress m_scannerIP;
	quint16 m_scannerPort;
	ccColorScale::Shared m_colorScale;
	bool isListening;
    QTimer *m_timer;
};

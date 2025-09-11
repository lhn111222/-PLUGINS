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


#include "qUDPPointCloud.h"
#include <QDebug>

qUDPPointCloud::qUDPPointCloud(QString name) throw()
	:ccPointCloud(name)
{
}

qUDPPointCloud::~qUDPPointCloud()
{
}
/*              转换为点云                 */


/*
//设备 发给 客户端，的数据包格式
struct DataMsg
{
    char packHead[8];  //包头
    uint32_t devID;      //设备id xx xx x x xxx (年、月、类别、随机、编号(xx日，x编号))
    ScannerState stateFlag;  //设备当前工作模式
    uint16_t flags;  //设备的一些状态信息（备用）

    uint16_t contents_flag; //指示当前这个包包含哪些内容 1-心跳包；2-传状态的包；3-全局点云包；4-全局位姿包；5-全局点云+位姿；6-原始点云包；
                                    //11-FilesInDec； 12-盘满了，要清理一下；13-删除了，给个反馈；
                                     //21-没有loop文件； 22-没有locate文件； 23-没有remap文件；24-正在扫描收尾中； 25-扫描真正结束了；
                                     //26-任务开始时ros服务调用不成功，死循环；27-kill时，调用存闭环文件服务不成功，死循环； 28-闭环文件检测不到，死循环；
                                     // 29-提示界面闭环再处理已经结束；30-闭环再处理已经开始了，但还没正式开始传数据呢；
                                    //41-闭环点出现


    uint32_t dataLength1;  //注意，它的意义可能根据包的内容有所不同，有时是数据抽象个数（如点云、位姿个数）；有时是数据内存大小。
    uint32_t dataLength2;

    //....实际数据
    //针对5，先位姿，再点云
};
*/


struct VPointI {
    float x;          // X坐标
    float y;          // Y坐标
    float z;          // Z坐标
    uint8_t intensity;
    uint8_t reserved[3];
};

struct DataMsg {
    char packHead[8];     // 头部标识："\xff\xcc\x33\x44\x2d\x42\x4f\0"
    uint32_t devID;       // 设备ID
    uint8_t stateFlag;    // 状态标志
    uint16_t flags;        // 预留标志
    uint16_t contents_flag; // 指令标志（3表示点云包）
    uint32_t dataLength1; // 当前包的点数量
    uint32_t dataLength2;// 未使用，0
};

void qUDPPointCloud::addPoints(const QByteArray& data)
{
    const uint32_t HEADER_SIZE = sizeof(DataMsg);
    const uint32_t POINT_SIZE = sizeof(VPointI);

    // 1. 校验数据包长度
    if (static_cast<uint32_t>(data.size()) < HEADER_SIZE) {
        qWarning() << "Point cloud data too short, less than header size!";
        return;
    }

    // 2. 解析包头
    const DataMsg* header = reinterpret_cast<const DataMsg*>(data.data());
    if (strcmp(header->packHead, "\xff\xcc\x33\x44\x2d\x42\x4f") != 0) {
        qWarning() << "Point cloud header mismatch, ignore!";
        return;
    }

    // 3. 校验点数据长度
    uint32_t expectedDataSize = header->dataLength1 * POINT_SIZE;
    uint32_t actualDataSize = data.size() - HEADER_SIZE;
    if (actualDataSize != expectedDataSize) {
        qWarning() << "Point data length mismatch! Expected:" << expectedDataSize << ", Actual:" << actualDataSize;
        return;
    }

    // 4. 解析点数据
    const VPointI* points = reinterpret_cast<const VPointI*>(data.data() + HEADER_SIZE);

    // 5. 遍历点
    for (uint32_t i = 0; i < header->dataLength1; ++i) {
        const VPointI& p = points[i];
        if (std::isnan(p.x) || std::isnan(p.y) || std::isnan(p.z)) {
            qDebug() << "Skip invalid point (NaN), index:" << i;
            continue;
        }
        //m_points.push_back(CCVector3::fromArray(reinterpret_cast<const float*>(points+i)));
        //cloud->addPoint(CCVector3(p.x, p.y, p.z));
        m_points.push_back(CCVector3(p.x, p.y, p.z));
        //qDebug() << "Added point" << i << ":" << p.x << "," << p.y << "," << p.z << "," << p.intensity;
    }
    qWarning() << "Parse point cloud success:" << header->dataLength1 << "points, Total:" << m_points.size();

    // 6. 更新显示
    prepareDisplayForRefresh();
    m_bbox.setValidity(false);
    }
    /*
    const quint16 POINTSIZE = sizeof(point);
	auto points = reinterpret_cast<const point*>(data.data());
	quint64 len = data.size() / (POINTSIZE);

	for (size_t i = 0; i < len; i++)
	{
		if (points[i].x != points[i].x
			|| points[i].y != points[i].y
			|| points[i].z != points[i].z)
		{
			continue;
		}
		else
		{
			m_points.push_back(CCVector3::fromArray(reinterpret_cast<const float*>(points+i)));
		}
	}
	prepareDisplayForRefresh();
    m_bbox.setValidity(false);
    */





void qUDPPointCloud::addPoints(const QString fileName)
{
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly))
	{
		return;
	}
	QDataStream data(&in);
	point p;
	char* buffer = reinterpret_cast<char*>(&p);
	while(!in.atEnd())
	{
		data.readRawData(buffer, sizeof(p));
		m_points.push_back(CCVector3::fromArray(
			reinterpret_cast<float*>(buffer)));
	}
	in.close();
	redrawDisplay();
	m_bbox.setValidity(false);
}

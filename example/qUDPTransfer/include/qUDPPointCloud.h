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
#include <ccPointCloud.h>
#include <QByteArray>
#include <QDataStream>
#include <QThread>

class qUDPPointCloud :
    public ccPointCloud
{
    struct point 
    {
        float x, y, z, i;
    };
public:
    qUDPPointCloud(QString name = QString()) throw();
    ~qUDPPointCloud();
    void addPoints(const QByteArray& data);
    void addPoints(const QString);
      const CCVector3& getPointAt(size_t index) const { return m_points[index]; }
};


/*
    Copyright 2014 Giorgos Tsiapaliokas <giorgos.tsiapaliokas@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KDEVPLATFORM_PLUGIN_PLASMAPROJECTMANAGER_H
#define KDEVPLATFORM_PLUGIN_PLASMAPROJECTMANAGER_H

#include <project/abstractfilemanagerplugin.h>
#include <util/path.h>

using namespace KDevelop;

class PlasmaProjectManager: public AbstractFileManagerPlugin
{
    Q_OBJECT

public:
    PlasmaProjectManager(QObject* parent = nullptr,
                         const QVariantList &args = QVariantList());
    ~PlasmaProjectManager();

protected:
    ProjectFileItem* createFileItem(IProject *project, const Path &path,
                                    ProjectBaseItem *parent) override;

    ProjectFolderItem* createFolderItem(IProject *project, const Path &path,
                                        ProjectBaseItem *parent) override;

};

#endif

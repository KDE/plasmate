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

import QtQuick 2.3
import QtQuick.Controls 1.2 as QtControls
import QtQuick.Layouts 1.1 as QtLayouts

QtLayouts.ColumnLayout {
    QtControls.Label {
        text: i18n("Your project isn't in a git repository. \n Do you want to create one??")
    }

    QtControls.Button {
        text: i18n("Yes, I want to create a new git repository for my project")
        onClicked:  {
            var success = git.initializeRepository()
            if (success) {
                stackView.push(Qt.resolvedUrl("CommitsView/CommitsView.qml"))
            }
        }
    }
}


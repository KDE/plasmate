/*
  Copyright 2014 Giorgos Tsiapaliokas <giorgos.tsiapaliokas@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NOTESEDITOR_H
#define NOTESEDITOR_H

#include <QDockWidget>

namespace KParts {
    class ReadOnlyPart;
};

class NotesEditor : public QDockWidget
{
    Q_OBJECT;
public:
    NotesEditor(const QString &projectPath, QWidget* parent = 0);
    ~NotesEditor();

    void refreshNotes();

private:
    void init();

    KParts::ReadOnlyPart *m_notesPart;
    QString m_projectPath;
};

#endif // NOTESEDITOR_H

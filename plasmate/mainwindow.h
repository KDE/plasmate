/*
Copyright 2009 Riccardo Iaconelli <riccardo@kde.org>
Copyright 2014 Giorgos Tsipalaiokas <giorgos.tsiapaliokas@kde.org>

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <KParts/MainWindow>

class QAction;
class QShortcut;

namespace KParts {
    class ReadWritePart;
};

namespace KTextEditor {
    class Document;
    class View;
};


class DockWidgetsHandler;
class PackageHandler;

class MainWindow : public KParts::MainWindow
{
    Q_OBJECT
public:
        MainWindow(QWidget *parent = 0);
        ~MainWindow();

        void loadProject(const QString &projectPath);

private Q_SLOTS:
    void editToolbars();
    void slotNewToolbarConfig();

protected:
    void closeEvent(QCloseEvent *event);

private:
    QAction *addAction(QString text, const char * icon,
                       const  char *slot, const char *name);

    void setupActions();
    void setupStartPage();
    void saveProjectState();
    void loadProject(const QUrl &path);

    KParts::ReadWritePart *m_part;

    KTextEditor::Document *m_doc;
    KTextEditor::View *m_view;

    DockWidgetsHandler *m_dockWidgetsHandler;
    PackageHandler *m_packageHandler;
};


#endif // MAINWINDOW_H

/*
  Copyright (c) 2009 Riccardo Iaconelli <riccardo@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QDir>
#include <QDockWidget>
#include <QListWidgetItem>
#include <QModelIndex>
#include <QLabel>
#include <QGridLayout>

#include <KTextEdit>

#include <KAction>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMenu>
#include <KMenuBar>
#include <KStandardAction>
#include <KUrl>
#include <KListWidget>
#include <KActionCollection>
#include <KParts/Part>
#include <KStandardDirs>

#include <Plasma/PackageMetadata>

#include "editors/editpage.h"
#include "savesystem/timeline.h"
#include "mainwindow.h"
#include "packagemodel.h"
#include "sidebar.h"
#include "startpage.h"
#include "ui_mainwindow.h"
#include "previewer/previewer.h"


MainWindow::MainWindow(QWidget *parent)
        : KParts::MainWindow(parent, 0),
        m_workflow(0),
        m_sidebar(0),
        m_dockTimeLine(0),
        m_timeLine(0),
        m_previewer(0),
        m_model(0),
        m_oldTab(0) // we start from startPage
{
    setXMLFile("plasmateui.rc");
    createMenus();

    m_startPage = new StartPage(this);
    connect(m_startPage, SIGNAL(projectSelected(QString, QString)),
            this, SLOT(loadProject(QString, QString)));
    setCentralWidget(m_startPage);
}

MainWindow::~MainWindow()
{
    // Saving docks position
    KConfig c;
    KConfigGroup configDock = c.group("DocksPosition");

    if (m_workflow) {
        configDock.writeEntry("FloatingStartPage", m_workflow->isFloating());
        configDock.writeEntry("StartPagePosition", convertDockState(m_workflow));
        delete m_startPage;
        delete m_workflow;
    }

    if (m_previewer) {
        configDock.writeEntry("FloatingPreviewer", m_previewerWidget->isFloating());
        configDock.writeEntry("PreviewerPosition", convertDockState(m_previewerWidget));
        delete m_previewer;
        delete m_previewerWidget;
    }
    if (m_dockTimeLine) {
        configDock.writeEntry("FloatingTimeLine", m_dockTimeLine->isFloating());
        configDock.writeEntry("TimeLinePosition", convertDockState(m_dockTimeLine));
        delete m_timeLine;
        delete m_dockTimeLine;
    }

    c.sync();
}

int MainWindow::convertDockState(QDockWidget *widget)
{
    switch (dockWidgetArea(widget)) {
    case Qt::LeftDockWidgetArea:
        return 1;
    case Qt::RightDockWidgetArea:
        return 2;
    case Qt::TopDockWidgetArea:
        return 4;
    case Qt::BottomDockWidgetArea:
        return 8;
    case Qt::AllDockWidgetAreas:
        return 16;
    case Qt::NoDockWidgetArea:
        return 0;
    }
}

Qt::DockWidgetArea MainWindow::convertDockState(int id)
{
    switch (id) {
    case 1:
        return Qt::LeftDockWidgetArea;
    case 2:
        return Qt::RightDockWidgetArea;
    case 4:
        return Qt::TopDockWidgetArea;
    case 8:
        return Qt::BottomDockWidgetArea;
    case 16:
        return Qt::AllDockWidgetAreas;
    case 0:
        return Qt::NoDockWidgetArea;
    }
}

void MainWindow::createMenus()
{
    KStandardAction::quit(this, SLOT(quit()), actionCollection());
    menuBar()->addMenu(helpMenu());
    setupGUI();
}

void MainWindow::createDockWidgets()
{
    KConfig c;
    KConfigGroup configDock = c.group("DocksPosition");
    /////////////////////////////////////////////////////////////////////////
    m_workflow = new QDockWidget(i18n("Workflow"), this);
    m_workflow->setObjectName("workflow");
    m_sidebar = new Sidebar(m_workflow);

    m_sidebar->addItem(KIcon("go-home"), i18n("Start page"));
    m_sidebar->addItem(KIcon("accessories-text-editor"), i18n("Edit"));
    m_sidebar->addItem(KIcon("krfb"), i18n("Publish"));
    m_sidebar->addItem(KIcon("help-contents"), i18n("Documentation"));

    connect(m_sidebar, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeTab(int)));

    m_workflow->setWidget(m_sidebar);
    addDockWidget(convertDockState(configDock.readEntry("StartPagePosition", 1)) , m_workflow);

    m_workflow->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_sidebar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

    m_workflow->setFloating(configDock.readEntry("FloatingStartPage", false));

    /////////////////////////////////////////////////////////////////////////
    m_dockTimeLine = new QDockWidget(i18n("TimeLine"), this);
    m_dockTimeLine->setObjectName("timeline");
    m_timeLine = new TimeLine(this, m_model->package());

    connect(m_timeLine, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeTab(int)));

    m_dockTimeLine->setWidget(m_timeLine);

    addDockWidget(convertDockState(configDock.readEntry("TimeLinePosition", 1)) , m_dockTimeLine);

    m_dockTimeLine->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_timeLine->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

    m_workflow->setFloating(configDock.readEntry("FloatingTimeLine", false));

    /////////////////////////////////////////////////////////////////////////
    m_previewerWidget = new QDockWidget(i18n("Previewer"), this);
    m_previewerWidget->setObjectName("workflow");
    m_previewer = new Previewer();
    m_previewerWidget->setWidget(m_previewer);
    addDockWidget(convertDockState(configDock.readEntry("PreviewerPosition", 1)) , m_previewerWidget);

    m_workflow->setFloating(configDock.readEntry("FloatingPreviewer", false));
}

void MainWindow::quit()
{
    qApp->closeAllWindows();
//     deleteLater();
}

void MainWindow::changeTab(int tab)
{
    if (tab == m_oldTab) { // user clicked on the current tab
        if (tab == StartPageTab) {
            m_startPage->resetStatus();
        }
        return;
    }

    centralWidget()->deleteLater();
    m_startPage = 0;

    switch (tab) {
    case StartPageTab: {
        m_startPage = new StartPage(this);
        setCentralWidget(m_startPage);
    }
    break;
    case EditTab: {
        m_editPage = new EditPage(this);
        m_editPage->setModel(m_model);
        setCentralWidget(m_editPage);
        connect(m_editPage, SIGNAL(loadEditor(KService::List)), this, SLOT(loadRequiredEditor(const KService::List)));
    }
    break;
    case PublishTab: {
        QLabel *l = new QLabel(i18n("Publish widget will go here!"), this);
        setCentralWidget(l);
    }
    break;
    case DocsTab: {
        QLabel *l = new QLabel(i18n("Documentation widget will go here!"), this);
        setCentralWidget(l);
    }
    break;
    }

    m_oldTab = tab;
}

void MainWindow::loadRequiredEditor(const KService::List offers)
{
    if (offers.isEmpty()) {
        kDebug() << "No offers for editor, can not load.";
        return;
    }

    QWidget *newWidget = new QWidget(this);

    QVariantList args;
    QString error; // we should show this via debug if we fail
    m_part = offers.at(0)->createInstance<KParts::Part>(newWidget, args, &error);

    if (!m_part) {
        kDebug() << "Failed to load editor:" << error;
    }

    setCentralWidget(m_part->widget());

    //Add the part's GUI
    createGUI(m_part);
}

void MainWindow::loadProject(const QString &name, const QString &type)
{
    kDebug() << "Loading project named" << name << "...";
    delete m_model;

    QString packagePath = KStandardDirs::locateLocal("appdata", name + '/');
    QString actualType = type;

    if (actualType.isEmpty()) {
        QDir dir(packagePath);
        if (dir.exists("metadata.desktop")) {
            Plasma::PackageMetadata metadata(packagePath + "metadata.desktop");
            actualType = metadata.serviceType();
        }
    }

    // Add it to the recent files first.
    m_model = new PackageModel(this);
    m_model->setPackageType(actualType);
    m_model->setPackage(packagePath);

    QStringList recentFiles;
    KConfigGroup cg = KGlobal::config()->group("General");
    recentFiles = recentProjects();

    if (recentFiles.contains(name)) {
        recentFiles.removeAt(recentFiles.indexOf(name));
    }

    if (!name.isEmpty()) {
        recentFiles.prepend(name);
    } else
        return;

    kDebug() << "Writing the following m_sidebar of recent files to the config:" << recentFiles;

    cg.writeEntry("recentFiles", recentFiles);
    KGlobal::config()->sync();

    // Load the needed widgets, switch to page 1 (edit)...
    createDockWidgets();
    m_sidebar->setCurrentIndex(1);
}

QStringList MainWindow::recentProjects() // TODO Limit to 5?
{
    KConfigGroup cg = KGlobal::config()->group("General");
    QStringList l = cg.readEntry("recentFiles", QStringList());
//     kDebug() << l.toStringList();

    return l;
}


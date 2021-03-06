/*
    Copyright 2014 Giorgos Tsiapaliokas <giorgos.tsiapaliokas@kde.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "plasmateapp.h"
#include "plasmateextention.h"
#include "startpage/startpage.h"


#include <shell/core.h>
#include <shell/documentcontroller.h>
#include <shell/projectcontroller.h>
#include <shell/uicontroller.h>


#include <sublime/mainwindow.h>


#include <interfaces/idocumentcontroller.h>
#include <interfaces/iprojectcontroller.h>

#include <kdevplatform_version.h>

#include <QMenuBar>
#include <QDockWidget>


#include <KLocalizedString>
#include <KMessageBox>
#include <KConfigGroup>
#include <KToolBar>

#include <QDebug>

PlasmateApp::PlasmateApp(QObject *parent)
    : QObject(parent)
{
    init();
    setupStartPage(true);
}

PlasmateApp::~PlasmateApp()
{
}

void PlasmateApp::init()
{
    QString session;
    PlasmateExtension::init();

#if KDEVPLATFORM_VERSION >= QT_VERSION_CHECK(5, 1, 40)
    KDevelop::Core::initialize(KDevelop::Core::Default, session);
#else
    QObject *splash = 0;
    KDevelop::Core::initialize(splash, KDevelop::Core::Default, session);
#endif

    m_core = KDevelop::Core::self();
    m_uiControllerInternal = m_core->uiControllerInternal();
    m_projectController = m_core->projectController();
    m_documentController = m_core->documentController();

    connect(m_documentController, SIGNAL(documentClosed(KDevelop::IDocument*)),
            this, SLOT(checkStartPage()));

    connect(m_projectController, SIGNAL(projectClosed(KDevelop::IProject*)),
            this, SLOT(checkStartPage()));
}

void PlasmateApp::checkStartPage()
{
    if (m_documentController->openDocuments().isEmpty() &&
        m_projectController->projects().isEmpty()) {
        // we don't have any project and documents so we don't
        // have to close anything :)
        setupStartPage(false);
    }
}

void PlasmateApp::setupStartPage(bool closeProjectsAndDocuments)
{
    if (!m_uiControllerInternal->activeSublimeWindow()) {
        return;
    }

    if (closeProjectsAndDocuments) {
        // when plasmate is about to go to the StartPage  we *don't* want
        // to have any opened documents and projects so lets close them if there are any

        // close all documents
        m_documentController->closeAllDocuments();

        //close all projects
        for (auto project : m_projectController->projects()) {
            m_projectController->closeProject(project);
        }
    }

    StartPage *startPage = new StartPage();

    connect(startPage, &StartPage::projectSelected, [=](const QUrl &projectFile) {
        loadMainWindow(projectFile);
    });

    m_uiControllerInternal->activeSublimeWindow()->setBackgroundCentralWidget(startPage);

    QList<QToolBar*> toolBars = m_uiControllerInternal->activeSublimeWindow()->findChildren<QToolBar*>();

    if (!m_toolbarActions.isEmpty()) {
        m_toolbarActions.clear();
    }

    for (auto toolBar : toolBars) {
        m_toolbarActions.append(toolBar->actions());
    }

    showKDevUi(false);
}

void PlasmateApp::loadMainWindow(const QUrl &projectPath)
{
    const QString metadataFilePath = projectPath.path() + QStringLiteral("/metadata.desktop");

    if (!QFile(metadataFilePath).exists()) {
        KMessageBox::error(m_uiControllerInternal->activeMainWindow(), i18n("Your package is invalid"));
        return;
    }

    const QString projectName = QDir(projectPath.path()).dirName();
    const QString projectPlasmateFile = projectPath.path() + QLatin1Char('/') + projectName + QLatin1Char('.') +
                                        PlasmateExtension::getInstance()->projectFileExtension();

    if (!QFile(projectPlasmateFile).exists()) {
        KConfigGroup configPlasmate(KSharedConfig::openConfig(projectPlasmateFile), QStringLiteral("Project"));
        configPlasmate.writeEntry("Name", projectName);
        configPlasmate.writeEntry("Manager", "KDevPlasmaManager");
        configPlasmate.sync();
    }

    m_projectController->openProject(QUrl::fromLocalFile(projectPlasmateFile));
    m_documentController->openDocument(QUrl::fromLocalFile(metadataFilePath));

    showKDevUi(true);

   for (auto it: m_uiControllerInternal->activeMainWindow()->menuBar()->actions()) {
        if(it->objectName() == QStringLiteral("run") || it->objectName() == QStringLiteral("project")) {
            it->setVisible(false);
        }
    }
}

void PlasmateApp::showKDevUi(bool visible)
{
    for (auto action : m_toolbarActions) {
        action->setVisible(visible);
        if (action->isChecked()) {
            action->setChecked(false);
        }
    }

    for (auto obj : m_uiControllerInternal->activeSublimeWindow()->children()) {
        QDockWidget *dockWidget = qobject_cast<QDockWidget *>(obj);
        if (dockWidget && dockWidget->isVisible()) {
            dockWidget->close();
        }
    }

    m_uiControllerInternal->activeMainWindow()->menuBar()->setVisible(visible);
}


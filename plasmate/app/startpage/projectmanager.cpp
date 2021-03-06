/*
 *   Copyright 2009 Lim Yuen Hoe <yuenhoe@hotmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QDir>
#include <QPushButton>
#include <QUrl>
#include <QDebug>
#include <QMenu>
#include <QStandardPaths>

#include <KConfig>
#include <KIO/DeleteJob>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KZip>

#include "projectmanager.h"
#include "projecthandler.h"
#include "../../packagehandler/packagehandler.h"
#include "../../editors/metadata/metadatahandler.h"

ProjectManager::ProjectManager(ProjectHandler *projectHandler,
                               PackageHandler *packageHandler, QWidget *parent)
    : QDialog(parent),
      m_projectHandler(projectHandler),
      m_packageHandler(packageHandler)
{
    m_projectList = new QListWidget(this);
    m_projectList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_projectList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(emitProjectSelected()));
    connect(m_projectList,SIGNAL(itemSelectionChanged()), this, SLOT(checkButtonState()));

    m_loadButton = new QPushButton(i18n("Load Project"), this);
    connect(m_loadButton, SIGNAL(clicked()), this, SLOT(emitProjectSelected()));

    m_removeMenuButton = new QPushButton(i18n("Remove Project"), this);

    m_removeMenu = new QMenu(i18n("Remove Project"), this);

    m_removeMenuButton->setMenu(m_removeMenu);
    m_removeMenu->addAction(i18n("From List"), this, SLOT(confirmRemoveFromList()));
    m_removeMenu->addAction(i18n("From Disk"), this, SLOT(confirmRemoveFromDisk()));

    QVBoxLayout *l = new QVBoxLayout();

    m_loadButton->setDisabled(true);

    m_removeMenuButton->setDisabled(true);
    l->addWidget(m_projectList);
    l->addWidget(m_loadButton);
    l->addWidget(m_removeMenuButton);

    setLayout(l);
    populateList();
}

void ProjectManager::populateList()
{
    m_items.clear();
    m_projectList->clear();

    for (const auto it : m_projectHandler->loadProjectsList()) {
        const QString projectPath = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
        QString tmpIt = it;

        const QString projectName = tmpIt.replace(projectPath + QLatin1Char('/'), "");
        m_items[new QListWidgetItem((projectName), m_projectList)] = it;
    }
}

void ProjectManager::confirmRemoveFromList()
{
    const int count = m_projectList->selectedItems().count();
    QString dialogText = i18np("Are you sure you want to remove the selected project from the project list? %2 \nNumber of project %1",
                               "Are you sure you want to remove the selected projects from the project list? %2 \nNumber of projects %1",
                               count, removeListItem(m_projectList->selectedItems()));
    const int code = KMessageBox::warningContinueCancel(this, dialogText);
    if (code == KMessageBox::Continue) {
        removeSelectedProjects(false);
    }
}

void ProjectManager::confirmRemoveFromDisk()
{
    const int count = m_projectList->selectedItems().count();
    QString dialogText = i18np("Are you sure you want to remove the selected project from disk? This cannot be undone %2 \nNumber of project %1",
                               "Are you sure you want to remove the selected projects from disk? This cannot be undone %2 \nNumber of projects  %1",
                               count, removeListItem(m_projectList->selectedItems()));
    const int code = KMessageBox::warningContinueCancel(this, dialogText);
    if (code == KMessageBox::Continue) {
        removeSelectedProjects(true);
    }
}

void ProjectManager::removeSelectedProjects(bool deleteFromDisk)
{
    QList<QListWidgetItem *> items = m_projectList->selectedItems();

    for (QListWidgetItem *item : items) {
        if (deleteFromDisk) {
            m_projectHandler->removeProjectFromDisk(m_items[item]);
            m_items.remove(item);
            delete item;
        } else  {
            m_projectHandler->removeProject(m_items[item]);
            m_items.remove(item);
            delete item;
        }

    }

    done(QDialog::Accepted);
}

const QString ProjectManager::removeListItem(const QList<QListWidgetItem*> &items)
{
    QString itemList;
    for (QListWidgetItem *item : items) {
        itemList.append(QChar('\n') + m_items[item]);
    }

    return itemList;
}

void ProjectManager::checkButtonState()
{
    QList<QListWidgetItem *> l = m_projectList->selectedItems();
    m_loadButton->setEnabled(l.count() == 1);
    m_removeMenuButton->setEnabled(l.count() > 0);
}

void ProjectManager::addProject(QListWidgetItem *item)
{
    m_projectHandler->addProject(item->text());
    m_items[item] = item->text();
}

void ProjectManager::emitProjectSelected()
{
    QList<QListWidgetItem *> l = m_projectList->selectedItems();
    if (l.isEmpty()) {
        return;
    }

    QString url = m_items[l.at(0)];
    if (!url.isEmpty()) {
        m_projectHandler->recentProject(url);
        emit projectSelected(url);
        done(QDialog::Accepted);
    }
}

bool ProjectManager::importPackage(const QUrl &toImport, const QUrl &targetLocation)
{
    bool ret = true;
    KZip plasmoid(toImport.path());
    if (!plasmoid.open(QIODevice::ReadOnly)) {
        qDebug() << "ProjectManager::importPackage can't open the plasmoid archive";
        return false;
    }
    plasmoid.directory()->copyTo(targetLocation.path());
    plasmoid.close();
    return ret;
}


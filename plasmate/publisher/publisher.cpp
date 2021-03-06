/*
  Copyright 2009 Lim Yuen Hoe <yuenhoe@hotmail.com>
  Copyright 2012 Giorgos Tsiapaliwkas <terietor@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QDBusInterface>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QDebug>

#include <KConfigGroup>
#include <KIO/DeleteJob>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KService>
#include <KServiceTypeTrader>
#include <KMessageBox>
#include <KNS3/UploadDialog>

#include "publisher.h"
//#include "signingwidget.h"
//#include "remoteinstaller/remoteinstallerdialog.h"

Publisher::Publisher(QWidget *parent, const QUrl &path, const QString& type)
        : QDialog(parent),
        // TODO
        //m_signingWidget(0),
        m_projectPath(path),
        m_projectType(type),
        m_comboBoxIndex(0)
{
    QWidget *uiWidget = new QWidget();
    m_ui.setupUi(uiWidget);
    m_ui.exporterUrl->setReadOnly(true);
    // TODO
    //m_signingWidget = new SigningWidget();

    //merge the ui file with the SigningWidget
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(uiWidget);
    //TODO
    //layout->addWidget(m_signingWidget);

    setLayout(layout);

    if (type == QStringLiteral("Plasma/Applet") ||
    type == QStringLiteral("Plasma/PopupApplet") ||
    type == QStringLiteral("KWin/WindowSwitcher") ||
    type == QStringLiteral("KWin/Effect") ||
    type == QStringLiteral("KWin/Script")) {
        m_extension = QStringLiteral("plasmoid");
    } else if (m_extension.isEmpty()) {
        m_extension = QStringLiteral("zip");
    }

    //we want the installButton to be enabled only when the comboBox's current index is valid.
    m_ui.installButton->setEnabled(false);

    //populate the comboBox
    m_ui.installerButton->addItem("");
    m_ui.installerButton->addItem("Use PlasmaPkg");
    m_ui.installerButton->addItem("Remote Install");

    connect(m_ui.exporterDialogButton, &QPushButton::clicked, [&]() {
        const QString exportDestination = QFileDialog::getSaveFileName(this, i18n("Save"), QString(), i18n("Plasmoid (*.plasmoid)"));
        m_ui.exporterUrl->setText(exportDestination);
    });
    connect(m_ui.exporterButton, &QPushButton::clicked, this, &Publisher::doExport);
    //the new signal and slot syntax doesn't work with overloaded functions.
    //so we are using the old syntax for the time being.
    connect(m_ui.installerButton, SIGNAL(currentIndexChanged(int)), this, SLOT(checkInstallButtonState(int)));
    connect(m_ui.installButton, &QPushButton::clicked, this, &Publisher::doInstall);
    connect(m_ui.publisherButton, &QPushButton::clicked, this, &Publisher::doPublish);

    m_ui.publisherButton->setEnabled(m_extension == QStringLiteral("plasmoid") || m_extension == QStringLiteral("zip"));

    setLayout(layout);
}

void Publisher::setProjectName(const QString &name)
{
    m_projectName = name;
}

bool Publisher::exportPackage(const QUrl &toExport, const QUrl &targetFile)
{
    // Think ONE minute before committing nonsense: if you want to zip a folder,
    // and you create the *.zip file INSIDE that folder WHILE copying the files,
    // guess what happens??
    // This also means: always try at least once, before committing changes.
    if (targetFile.path().contains(toExport.path())) {
        // Sounds like we are attempting to create the package from inside the package folder, noooooo :)
        return false;
    }

    if (QFile::exists(targetFile.path())) {
        //TODO: Make sure this succeeds
        QFile::remove(targetFile.path()); // overwrite!
    }

    // Create an empty zip file
    KZip zip(targetFile.path());
    zip.open(QIODevice::ReadWrite);
    zip.close();

    // Reopen for writing
    if (zip.open(QIODevice::ReadWrite)) {
        qDebug() << "zip file opened successfully";
        zip.addLocalDirectory(toExport.path(), ".");
        zip.close();
        return true;
    }

    qDebug() << "Cant open zip file" ;
    return false;
}

void Publisher::doExport()
{
    if (QFile(m_ui.exporterUrl->text()).exists()) {
        QString dialogText = i18n("A file already exists at %1. Do you want to overwrite it?",
                                  m_ui.exporterUrl->text());
        int code = KMessageBox::warningYesNo(0,dialogText);
        if (code != KMessageBox::Yes) return;
    }
    bool ok = exportToFile(QUrl(m_ui.exporterUrl->text()));

    // TODO
    // If signing is enabled, lets do that!
    //if (m_signingWidget->signingEnabled()) {
    //    ok = ok && m_signingWidget->sign(m_ui.exporterUrl->url());
    //}
    if (QFile::exists(m_ui.exporterUrl->text()) && ok) {
        KMessageBox::information(this, i18n("Project has been exported to %1.", m_ui.exporterUrl->text()));
    } else {
        KMessageBox::error(this, i18n("An error has occurred during the export. Please check the write permissions "
        "in the target directory."));
    }
}

//we want the installButton to be enabled only when comboBox's index is valid.
void Publisher::checkInstallButtonState(int comboBoxCurrentIndex)
{
    if (comboBoxCurrentIndex == 1) {
        m_ui.installButton->setEnabled(true);
        m_comboBoxIndex = 1;
    } else if (comboBoxCurrentIndex == 2) {
        m_ui.installButton->setEnabled(true);
        m_comboBoxIndex = 2;
    } else if (comboBoxCurrentIndex == 3) {
        m_ui.installButton->setEnabled(true);
        m_comboBoxIndex = 3;
    } else {
        m_ui.installButton->setEnabled(false);
        m_comboBoxIndex = 0;
    }
}

//choose the method which which we will install the project
//according to comboBox's index
void Publisher::doInstall()
{
    if (m_comboBoxIndex == 1) {
        doPlasmaPkg();
    } else if (m_comboBoxIndex == 2) {
        // TODO
        //doRemoteInstall();
    }
}

void Publisher::doPlasmaPkg()
{
    const QUrl tempPackage(tempPackagePath());
    qDebug() << "tempPackagePath" << tempPackage.path();
    qDebug() << "m_projectPath" << m_projectPath.path();

    exportPackage(m_projectPath, tempPackage); // create temporary package

    QStringList argv("plasmapkg2");
    argv.append("-t");
    if (m_projectType == "Plasma/Theme") {
        argv.append("theme");
    } else if (m_projectType == "Plasma/Applet") {
        argv.append("plasmoid");
    } else if (m_projectType == "KWin/WindowSwitcher") {
        argv.append("windowswitcher");
    } else if (m_projectType == "KWin/Script") {
        argv.append("kwinscript");
    } else if (m_projectType == "KWin/Effect") {
        argv.append("kwineffect");
    }

    // we do a plasmapkg -u in case the package was installed before
    // in which case it should be updated. -u also installs the
    // package if it hasn't been installed before, so it's all good.
    argv.append("-u");
    argv.append(tempPackage.path());
    bool ok = (KProcess::execute(argv) >= 0 ? true: false);
    if(ok) {
        QDBusInterface dbi("org.kde.kded", "/kbuildsycoca", "org.kde.kbuildsycoca");
        dbi.call(QDBus::NoBlock, "recreate");
    } else {
        KMessageBox::error(this, i18n("Project has not been installed"));
        return;
    }

    // TODO
   /* if (m_signingWidget->signingEnabled()) {
        ok = ok && m_signingWidget->sign(tempPackage);

        QString signatureDestPath = KStandardDirs::locateLocal("data", "plasma/plasmoids/");
        signatureDestPath.append(m_projectName).append(".plasmoid.asc");

        QString signatureOrigPath(tempPackage.path().append(".asc"));

        QFile signatureDest(signatureDestPath);
        if(signatureDest.open(QIODevice::ReadWrite)) {
            signatureDest.remove(signatureDestPath);
            signatureDest.close();
        }

        QFile signatureOrig(signatureOrigPath);
        if(signatureOrig.open(QIODevice::ReadWrite)) {
            ok = signatureOrig.copy(signatureOrigPath,signatureDestPath);
        } else {
            ok = false;
        }

    }*/

    QFile::remove(tempPackage.path()); // delete temporary package
    // TODO: probably check for errors and stuff instead of announcing
    // succcess in a 'leap of faith'
    if(ok) {
        KMessageBox::information(this, i18n("Project has been installed"));
    } else {
        KMessageBox::error(this, i18n("Project has not been installed"));
    }
}

/*void Publisher::doRemoteInstall()
{
    QScopedPointer<RemoteInstallerDialog> dialog(new RemoteInstallerDialog());

    //get the source directory from plasmaterc
    QString path = currentPackagePath();
    dialog->setPackagePath(path);

    dialog->exec();
}*/

const QString Publisher::tempPackagePath()
{
    QString tempPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first();
    //does it end with '/'? if not append it.
    tempPath = tempPath.endsWith(QLatin1Char('/')) ? tempPath : tempPath + QLatin1Char('/');

    return tempPath + m_projectName + QLatin1Char('.') + m_extension;
}

void Publisher::doPublish()
{
    // TODO: make sure this works with non-plasmoids too?
    qDebug() << "projectPath:" << m_projectPath.path();

    qDebug() << "Exportando no tmp: file://" + tempPackagePath();
    QUrl url(tempPackagePath());

    bool ok = exportToFile(url);
    // TODO
   // if (m_signingWidget->signingEnabled()) {
   //     ok = ok && m_signingWidget->sign(url);
   // }
    if (ok) {
        KNS3::UploadDialog *mNewStuffDialog = new KNS3::UploadDialog("plasmate.knsrc", this);
        mNewStuffDialog->setUploadFile(url);
        mNewStuffDialog->exec();
        QFile::remove(url.path()); // delete temporary package
    } else {
        // should probably eventually use a better error message
        KMessageBox::error(this, i18n("An error has occurred during the export. Please check the write permissions in the target directory."));
    }
}

bool Publisher::exportToFile(const QUrl& url)
{
    if (!url.isValid()) {
        KMessageBox::error(this, i18n("The file you entered is invalid."));
        return false;
    }

    return exportPackage(m_projectPath, url); // will overwrite if exists!
}

QString Publisher::currentPackagePath() const
{
    KConfigGroup cg(KSharedConfig::openConfig(qApp->applicationDisplayName()), "PackageModel::package");
    return cg.readEntry("lastLoadedPackage", QString());
}


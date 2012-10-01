/*

Copyright 2009-2010 Aaron Seigo <aseigo@kde.org>
Copyright 2009-2010 Artur Duque de Souza <asouza@kde.org>
Copyright 2009-2010 Diego '[Po]lentino' Casella <polentino911@gmail.com>
Copyright 2009-2010 Shantanu Tushar Jha <shaan7in@gmail.com>
Copyright 2009-2010 Sandro Andrade <sandroandrade@kde.org>
Copyright 2009-2010 Lim Yuen Hoe <yuenhoe@hotmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <packagemodel.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KDirWatch>
#include <KIcon>
#include <KMimeType>
#include <KUser>

#include <Plasma/Package>
#include <Plasma/PackageMetadata>

PackageModel::PackageModel(QObject *parent)
        : QAbstractItemModel(parent),
        m_directory(0),
        m_structure(0),
        m_package(0)
{
}

PackageModel::~PackageModel()
{
    delete m_directory;
    delete m_package;
    m_structure = 0;
}

void PackageModel::setPackageType(const QString &type)
{
    m_structure = 0;
    m_structure = Plasma::PackageStructure::load(type);
}

QString PackageModel::packageType() const
{
    if (m_structure) {

        //Plasma:PackageStructure is not able to understand the KWin staff that are
        //written with QML. So we need to retrieve that information inside from the
        //metadata.desktop file
        const QString metadataFile = package() + "metadata.desktop";
        KConfig c(metadataFile);
        KConfigGroup projectInformation(&c, "Desktop Entry");
        const QString projectType = projectInformation.readEntry("X-KDE-ServiceTypes");
        return projectType;
    }

    return QString();
}

QString PackageModel::contentsPrefix() const
{
    return m_structure->contentsPrefix();
}

QString PackageModel::implementationApi() const
{
    if (m_structure) {
        return const_cast<Plasma::PackageStructure::Ptr &>(m_structure)->metadata().implementationApi();
    }

    return QString();
}

int PackageModel::setPackage(const QString &path)
{
    if (!m_structure) {
        kDebug() << "Must set the package type FIRST!";
        return 0;
    }

    m_structure->setPath(path);

    delete m_package;
    m_package = new Plasma::Package(path, m_structure);

    delete m_directory;
    m_directory = new KDirWatch(this);
    m_directory->addDir(path, KDirWatch::WatchSubDirs | KDirWatch::WatchFiles);

    if (!loadPackage()) {
        delete m_directory;
        m_directory = 0;
        delete m_package;
        m_package = 0;
        return 0;
    }

    connect(m_directory, SIGNAL(created(QString)), this, SLOT(fileAddedOnDisk(QString)));
    connect(m_directory, SIGNAL(deleted(QString)), this, SLOT(fileDeletedOnDisk(QString)));

    return 1;
}

QString PackageModel::package() const
{
    if (m_package) {
        KConfigGroup cg(KGlobal::config(), "PackageModel::package");
        cg.writeEntry("lastLoadedPackage", m_package->path());
        cg.sync();
        return m_package->path();
    }

    return QString();
}

KUrl PackageModel::urlForIndex(const QModelIndex &index) const
{
    const char *key = static_cast<const char *>(index.internalPointer());
    QList<const char *> named = m_namedFiles.value(key);
    int row = index.row() - 1;
    QString path;
    QString file;
    if (row < 0) {
        // 'New' entry, return the directory path
        path = m_package->filePath(key);
    } else if (row < named.count()) {
        path = m_package->path();
        QString contents = m_package->structure()->contentsPrefix();
        path = path.endsWith("/") ? path + contents : path + "/" + contents;
        file = m_package->structure()->path(named.at(row));
    } else {
        row -= named.count();
        QStringList l = m_files.value(key);
        if (row < l.count()) {
            path = m_package->filePath(key);
            file = l.at(row);
        }
    }

    if (!path.endsWith('/')) {
        path.append('/');
    }

    return path + file;
}

QVariant PackageModel::data(const QModelIndex &index, int role) const
{
    if (!m_package || !index.isValid()) {
        return QVariant();
    }

    const char *key = static_cast<const char *>(index.internalPointer());
    if (key) {
        // we have a child item
        switch (role) {
        case MimeTypeRole: {
            if (index.row() == 0) {
                if (!qstrcmp(key, "images")) {
                    return QStringList("[plasmate]/imageDialog");
                }else {
                    return QStringList("[plasmate]/new");
                }
            }
            if (!qstrcmp(key, "images")) {
                return QStringList("[plasmate]/imageViewer");
            }

            if (!qstrcmp(key, "config")) {
                return QStringList("[plasmate]/kconfigxteditor");
            }
            return m_package->structure()->mimetypes(key);
        }
        break;
        case UrlRole: {
            return urlForIndex(index).pathOrUrl();
        }
        break;
        case Qt::DisplayRole: {
            if (index.row() == 0) {
                return i18n("New...");
            }

            QList<const char *> named = m_namedFiles.value(key);
            int row = index.row() - 1;
            if (row < named.count()) {
                //kDebug() << m_package->structure()->name(named.at(row));
                return m_package->structure()->name(named.at(row));
            }
            row -= named.count();
            QStringList l = m_files.value(key);
            if (row < l.count()) {
                //kDebug() << "got" << l.at(index.row() - 1);
                return l.at(row);
            }
        }
        break;
        case packagePathRole: {
            return m_package ? m_package->path() : QString();
        }
        case Qt::DecorationRole: {
            if (index.row() == 0) {
                return KIcon("document-new");
            } else {
                return KIcon(KMimeType::iconNameForUrl(urlForIndex(index)));
            }
        }
        break;
        }
    } else {
        // it's a top level item
        //kDebug() << "data for top level item" << index.row() << m_topEntries.count() << role << Qt::DisplayRole;
        switch (role) {
        case Qt::DisplayRole: {
            if (index.row() == m_topEntries.count()) {
                return i18n("Metadata");
            }

            return m_structure->name(m_topEntries.at(index.row()));
        }
        break;
        case MimeTypeRole: {
            if (index.row() == m_topEntries.count()) {
                // not sure if this is good, but will do for now
                // use special wildcard to indicate stuff that
                // plasmate should handle in it's own way
                return QStringList("[plasmate]/metadata");
            }
        }
        break;
        case UrlRole: {
            if (index.row() == m_topEntries.count()) {
                return m_package->path() + "metadata.desktop";
            }
        }
        break;
        }
    }
    return QVariant();
}

QModelIndex PackageModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.row() >= m_topEntries.count() || parent.parent().isValid()) {
            //kDebug() << "FAIL" << row << column;
            return QModelIndex();
        }

        const char *key = m_topEntries.at(parent.row());

        if (row <= m_files[key].count() + m_namedFiles[key].count()) {
            //kDebug() << "going to return" << row << column << key;
            return createIndex(row, column, (void*)key);
        } else {
            //kDebug() << "FAIL";
            return QModelIndex();
        }
    }

    // top level indexes; do a sanity check first
    if (row < 0 || row > m_topEntries.count() ||
        column < 0 || column > MAX_COLUMN) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex PackageModel::parent(const QModelIndex &index) const
{
    const char *key = static_cast<const char *>(index.internalPointer());

    if (m_topEntries.contains(key)) {
        return createIndex(m_topEntries.indexOf(key), 0);
    }

    return QModelIndex();
}

int PackageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    //FIXME: need to accomodate more information
    return MAX_COLUMN + 1;
}

int PackageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.row() < m_topEntries.count()) {
            //const char *key = m_topEntries.at(parent.row());
            const char *key = static_cast<const char *>(parent.internalPointer());
            if (!key) {
                key = m_topEntries.at(parent.row());
            } else if (m_namedFiles.contains(key)) {
                //It is a leaf
                return 0;
            }

            //kDebug() << "looking for" << key << m_files[key].count() << m_namedFiles[key]<<key<<parent.parent();
            return m_files.contains(key) ? m_files[key].count() + m_namedFiles[key].count() + 1 : 0;
        } else {
            return 0;
        }
    }

    return m_topEntries.count() + 1;
}

bool PackageModel::loadPackage()
{
    beginResetModel();

    m_topEntries.clear();
    m_files.clear();
    m_namedFiles.clear();

    if (!m_package) {
        kDebug() << "No package to load.";
        return false;
    }

    QDir dir(m_package->path());
    Plasma::PackageStructure::Ptr structure = m_package->structure();

    if (!dir.exists(structure->contentsPrefix())) {
        kDebug() << "This is not a valid package.";
        return false;
    }

    if (!dir.exists("metadata.desktop")) {
        KUser user;
        Plasma::PackageMetadata metadata;
        metadata.setAuthor(user.property(KUser::FullName).toString());
        metadata.setLicense("GPL");
        metadata.setName(dir.dirName());
        metadata.setServiceType(structure->type());
        metadata.write(dir.path() + "/metadata.desktop");
    }

    QStringList contents = structure->contentsPrefixPaths();

    foreach(const QString& content, contents) {
        if (!content.isEmpty()) {
            dir.mkpath(content);
            dir.cd(content);
        }
    }

    const QList<const char*> dirs = structure->directories();
    foreach (const char *key, dirs) {
        QStringList paths = structure->searchPath(key);
        foreach(const QString& path, paths){
            if (!dir.exists(path)) {
                dir.mkpath(path);
            }

            m_topEntries.append(key);
        }
    }

    QHash<QString, const char *> indexedFiles;
    foreach (const char *key, structure->requiredFiles()) {
        QString path = structure->path(key);
        if (!dir.exists(path)) {
            QFileInfo info(dir.path() + '/' + path);
            dir.mkpath(dir.relativeFilePath(info.absolutePath()));
            // Create all files, but 'code/main', as this is replaced by a template
            if (path != "code/main") {
                QFile f(dir.path() + '/' + path);
                f.open(QIODevice::WriteOnly);
            }
        }

        indexedFiles.insert(path, key);
    }


    foreach (const char *key, structure->directories()) {
        QString path = structure->path(key);
        if (!path.endsWith('/')) {
            path += '/';
        }

        QStringList files = m_package->entryList(key);
        QList<const char *> namedFiles;
        QStringList userFiles;
        foreach (const QString &file, files) {
            QString filePath = path + file;
            if (indexedFiles.contains(filePath)) {
                namedFiles.append(indexedFiles.value(filePath));
                indexedFiles.remove(filePath);
            } else if (!file.endsWith('~')) {
                userFiles.append(file);
            }
        }

        //kDebug() << "results for" << m_topEntries.indexOf(key) << key << "are:" << namedFiles.count() << userFiles.count();
        m_namedFiles.insert(key, namedFiles);
        m_files.insert(key, userFiles);
    }

    if (!indexedFiles.empty()) {
        foreach (const char *key, indexedFiles) {
            m_topEntries.append(key);
        }

        //kDebug() << "counts:" << m_topEntries.count() << indexedFiles.count();
    }

    endResetModel();
    return true;
}

void PackageModel::fileAddedOnDisk(const QString &path)
{
    if (QFileInfo(path).fileName().at(0) == QChar('.') ||
        path.endsWith('~')) {
        // we ignore hidden files and backup files
        return;
    }

    const KUrl toAdd(path);
    const QString toAddDir = toAdd.directory();
    const int parentCount = rowCount(QModelIndex());

    for (int i = 0; i < parentCount - 1; ++i) {
        const char *key = m_topEntries.at(i);
        QList<const char *> named = m_namedFiles.value(key);
        KUrl target(m_package->filePath(key));
        if (target.equals(toAddDir)) {
            QModelIndex parent = index(i, 0, QModelIndex());
            int ind = rowCount(parent);
            for (int ii = 0; ii < ind; ++ii) {
                QModelIndex child = index(ii, 0, parent);
                KUrl childPath(child.data(PackageModel::UrlRole).toString());
                if (childPath.equals(toAdd)) {
                    // let's not double-add
                    return;
                }
            }

            beginInsertRows(parent, ind, ind);
            m_files[key].append(toAdd.fileName());
            endInsertRows();
            break;
        }
    }
}

void PackageModel::fileDeletedOnDisk(const QString &path)
{
    if (QFile::exists(path)) {
        // KDirWatch seems overparanoid, so we quickcheck
        return;
    }

    // Probably not the most efficient way to do it but
    // it works :)
    const KUrl toDelete(path);

    // Iterate through every tree element and check if it matches
    // the deleted file
    const int parentCount = rowCount(QModelIndex());
    for (int i = 0; i < parentCount; ++i) {
        QModelIndex parent = index(i, 0, QModelIndex());
        int childCount = rowCount(parent);
        for (int ii = 1; ii < childCount; ++ii) {
            QModelIndex child = index(ii, 0, parent);
            KUrl childPath(child.data(PackageModel::UrlRole).toString());
            if (childPath.equals(toDelete)) {
                // match!! remove it!
                beginRemoveRows(parent, ii, ii);
                const char *key = m_topEntries.at(i);
                QList<const char *> &named = m_namedFiles[key];
                int row = ii - 1;
                if (row < named.count()) {
                    named.removeAt(row);
                } else {
                    row -= named.count();
                    QStringList &l = m_files[key];
                    if (row < l.count()) {
                        l.removeAt(row);
                    }
                }
                endRemoveRows();
                break;
            }
        }
    }
}


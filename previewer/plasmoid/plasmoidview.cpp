/*
 * Copyright 2007 Frerich Raabe <raabe@kde.org>
 * Copyright 2007 Aaron Seigo <aseigo@kde.org>
 * Copyright 2008 Aleix Pol <aleixpol@gmail.com>
 * Copyright 2009 Artur Duque de Souza <morpheuz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "plasmoidview.h"
#include "previewcontainment.h"

#include <QFileInfo>
#include <QDir>

#include <Plasma/Containment>

PlasmoidView::PlasmoidView(QWidget *parent)
        : QGraphicsView(parent),
        m_containment(0),
        m_applet(0)
{
    setScene(&m_corona);
    connect(&m_corona, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(sceneRectChanged(QRectF)));
    setAlignment(Qt::AlignCenter);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // containment stuff (must go to constructor / init ?)
    m_containment = m_corona.addContainment("studiopreviewer");
    m_containment->setFormFactor(Plasma::Planar);
    m_containment->setLocation(Plasma::Floating);
    connect(m_containment, SIGNAL(appletRemoved(Plasma::Applet*)), this, SLOT(appletRemoved()));

    // we do a two-way connect here to allow the previewer containment
    // and main window to tell each other to save/refresh
    connect(m_containment, SIGNAL(refreshClicked()), parent, SLOT(emitRefreshRequest()));
    connect(parent, SIGNAL(refreshView()), m_containment, SLOT(refreshApplet()));

    setScene(m_containment->scene());
}

void PlasmoidView::addApplet(const QString &name, const QVariantList &args)
{
    QFileInfo info(name);
    if (!info.isAbsolute()) {
        info = QFileInfo(QDir::currentPath() + "/" + name);
    }

    // load from package if we have a path
    if (info.exists()) {
        m_applet = Applet::loadPlasmoid(info.absoluteFilePath());
    }

    if (!m_applet) {
        m_applet = m_containment->addApplet(name, args, QRectF(0, 0, -1, -1));
    } else {
        m_containment->addApplet(m_applet, QPointF(-1, -1), false);
    }

    m_applet->setFlag(QGraphicsItem::ItemIsMovable, false);
    //resize(m_applet->preferredSize().toSize());
}

void PlasmoidView::clearApplets()
{
    m_containment->clearApplets();
}

void PlasmoidView::appletRemoved()
{
    m_applet = 0;
}

void PlasmoidView::sceneRectChanged(const QRectF &rect)
{
    Q_UNUSED(rect);

    if (m_containment) {
        setSceneRect(m_containment->geometry());
    }
}

void PlasmoidView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    // if we do not have any applet being shown
    // there is no need to do all this stuff
    if (!m_applet || m_applet->aspectRatioMode() != Plasma::KeepAspectRatio) {
        m_containment->resize(size());
        return;
    }
    QSize newSize = m_containment->size().toSize();
    newSize.scale(size(), Qt::KeepAspectRatio);
    m_containment->resize(newSize);
    centerOn(m_containment);
}

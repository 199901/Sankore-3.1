/*
 * Copyright (C) 2012 Webdoc SA
 *
 * This file is part of Open-Sankoré.
 *
 * Open-Sankoré is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation, version 2,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * Open-Sankoré is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with Open-Sankoré; if not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef UBPAGENAVIGATIONWIDGET_H
#define UBPAGENAVIGATIONWIDGET_H

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QLabel>
#include <QString>

#include "UBDocumentNavigator.h"
#include "UBDockPaletteWidget.h"
#include "document/UBDocumentProxy.h"

class UBPageNavigationWidget : public UBDockPaletteWidget
{
    Q_OBJECT
public:
    UBPageNavigationWidget(QWidget* parent=0, const char* name="UBPageNavigationWidget");
    ~UBPageNavigationWidget();
    //void setDocument(UBDocumentProxy* document);
    void refresh();

    bool visibleInMode(eUBDockPaletteWidgetMode mode)
    {
        return mode == eUBDockPaletteWidget_BOARD;
    }

signals:
    void resizeRequest(QResizeEvent* event);

public slots:
    void setPageNumber(int current, int total);

protected:
    virtual void timerEvent(QTimerEvent *event);


private:
    void updateTime();
    int customMargin();
    int border();

    /** The thumbnails navigator widget */
    UBDocumentNavigator* mNavigator;
    /** The layout */
    QVBoxLayout* mLayout;
    QHBoxLayout* mHLayout;
    QLabel* mPageNbr;
    QLabel* mClock;
    QString mTimeFormat;
    int mTimerID;

};

#endif // UBPAGENAVIGATIONWIDGET_H

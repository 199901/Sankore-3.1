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


#ifndef UBBOARDPALETTEMANAGER_H_
#define UBBOARDPALETTEMANAGER_H_

#include <QtGui>
#include <QtWebKit>
 
#include "gui/UBLeftPalette.h"
#include "gui/UBRightPalette.h"
#include "gui/UBPageNavigationWidget.h"
#include "gui/UBCachePropertiesWidget.h"
#include "gui/UBDockDownloadWidget.h"
#include "core/UBApplicationController.h"
#include "gui/UBFeaturesWidget.h"


class UBWebToolsPalette;
class UBStylusPalette;
class UBClockPalette;
class UBPageNumberPalette;
class UBZoomPalette;
class UBActionPalette;
class UBBoardController;
class UBServerXMLHttpRequest;
class UBKeyboardPalette;
class UBMainWindow;
class UBApplicationController;
class UBDockTeacherGuideWidget;

// Uncomment this to use old-styles lib paletter
// #define USE_WEB_WIDGET


class UBBoardPaletteManager : public QObject
{
    Q_OBJECT

    public:
        UBBoardPaletteManager(QWidget* container, UBBoardController* controller);
        virtual ~UBBoardPaletteManager();

        void setupLayout();
        UBLeftPalette* leftPalette(){return mLeftPalette;}
        UBRightPalette* rightPalette(){return mRightPalette;}
        UBStylusPalette* stylusPalette(){return mStylusPalette;}
        UBActionPalette *addItemPalette() {return mAddItemPalette;}
        void showVirtualKeyboard(bool show = true);
        void initPalettesPosAtStartup();
        void connectToDocumentController();
        void refreshPalettes();

        UBKeyboardPalette *mKeyboardPalette;

        void setCurrentWebToolsPalette(UBWebToolsPalette *palette) {mWebToolsCurrentPalette = palette;}
        UBWebToolsPalette* mWebToolsCurrentPalette;

        UBDockTeacherGuideWidget* teacherGuideDockWidget() { return mpTeacherGuideWidget;}

        void processPalettersWidget(UBDockPalette *paletter, eUBDockPaletteWidgetMode mode);
        void changeMode(eUBDockPaletteWidgetMode newMode, bool isInit = false);
        void startDownloads();
        void stopDownloads();

    signals:
        void connectToDocController();
        void signal_changeMode(eUBDockPaletteWidgetMode newMode);

    public slots:

        void activeSceneChanged();
        void containerResized();
        void addItem(const QUrl& pUrl);
        void addItem(const QPixmap& pPixmap, const QPointF& p = QPointF(0.0, 0.0), qreal scale = 1.0, const QUrl& sourceUrl = QUrl());

        void slot_changeMainMode(UBApplicationController::MainMode);
        void slot_changeDesktopMode(bool);

        void toggleErasePalette(bool ckecked);

    private:

        void setupPalettes();
        void connectPalettes();
        void positionFreeDisplayPalette();
        void setupDockPaletteWidgets();

        QWidget* mContainer;
        UBBoardController *mBoardControler;

        UBStylusPalette *mStylusPalette;

        UBZoomPalette *mZoomPalette;

        /** The left dock palette */
        UBLeftPalette* mLeftPalette;
        /** The right dock palette */
        UBRightPalette* mRightPalette;

        UBActionPalette *mBackgroundsPalette;
        UBActionPalette *mToolsPalette;
        UBActionPalette* mAddItemPalette;
        UBActionPalette* mErasePalette;
        UBActionPalette* mPagePalette;

        QUrl mItemUrl;
        QPixmap mPixmap;
        QPointF mPos;
        qreal mScaleFactor;

        QTime mPageButtonPressedTime;
        bool mPendingPageButtonPressed;

        QTime mZoomButtonPressedTime;
        bool mPendingZoomButtonPressed;

        QTime mPanButtonPressedTime;
        bool mPendingPanButtonPressed;

        QTime mEraseButtonPressedTime;
        bool mPendingEraseButtonPressed;

        /** The page navigator widget */
        UBPageNavigationWidget* mpPageNavigWidget;
        
        /** The cache properties widget */
        UBCachePropertiesWidget* mpCachePropWidget;

		UBFeaturesWidget *mpFeaturesWidget;

        /** The download widget */
        UBDockDownloadWidget* mpDownloadWidget;
        UBDockTeacherGuideWidget* mpTeacherGuideWidget;

        bool mDownloadInProgress;

    private slots:

        void changeBackground();

        void toggleBackgroundPalette(bool checked);
        void backgroundPaletteClosed();

        void toggleStylusPalette(bool checked);
        void tooglePodcastPalette(bool checked);

        void erasePaletteButtonPressed();
        void erasePaletteButtonReleased();

        void erasePaletteClosed();

        void togglePagePalette(bool ckecked);
        void pagePaletteClosed();

        void pagePaletteButtonPressed();
        void pagePaletteButtonReleased();

        void addItemToCurrentPage();
        void addItemToNewPage();
        void addItemToLibrary();

        void purchaseLinkActivated(const QString&);

        void linkClicked(const QUrl& url);

        void zoomButtonPressed();
        void zoomButtonReleased();
        void panButtonPressed();
        void panButtonReleased();

        void changeStylusPaletteOrientation(QVariant var);
};

#endif /* UBBOARDPALETTEMANAGER_H_ */

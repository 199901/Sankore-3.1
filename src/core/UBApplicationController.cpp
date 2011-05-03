#include "UBApplicationController.h"

#include "frameworks/UBPlatformUtils.h"
#include "frameworks/UBVersion.h"

#include "core/UBApplication.h"
#include "core/UBPersistenceManager.h"
#include "core/UBSettings.h"
#include "core/UBSetting.h"
#include "core/UBDocumentManager.h"
#include "core/UBDisplayManager.h"

#include "softwareupdate/UBSoftwareUpdateController.h"
#include "softwareupdate/UBSoftwareUpdate.h"

#include "adaptors/UBImportVirtualPrinter.h"

#include "board/UBBoardView.h"
#include "board/UBBoardController.h"
#include "board/UBBoardPaletteManager.h"
#include "board/UBDrawingController.h"


#include "document/UBDocumentProxy.h"
#include "document/UBDocumentController.h"

#include "desktop/UBDesktopPalette.h"
#include "desktop/UBDesktopAnnotationController.h"

#include "web/UBWebController.h"

#include "gui/UBScreenMirror.h"
#include "gui/UBMainWindow.h"

#include "domain/UBGraphicsPixmapItem.h"
#include "domain/UBW3CWidget.h"

#include "podcast/UBPodcastController.h"

#include "ui_mainWindow.h"

#ifdef Q_WS_MAC
#include <Carbon/Carbon.h>
#endif

UBApplicationController::UBApplicationController(UBBoardView *pControlView, UBBoardView *pDisplayView,
        UBMainWindow* pMainWindow, QObject* parent)
    : QObject(parent)
    , mMainWindow(pMainWindow)
    , mControlView(pControlView)
    , mDisplayView(pDisplayView)
    , mMirror(0)
    , mMainMode(Board)
    , mDisplayManager(0)
    , mAutomaticCheckForUpdates(false)
    , mCheckingForUpdates(false)
    , mIsShowingDesktop(false)
{
    mDisplayManager = new UBDisplayManager(this);

    mUninoteController = new UBDesktopAnnotationController(this);

    connect(mDisplayManager, SIGNAL(screenLayoutChanged()), this, SLOT(screenLayoutChanged()));
    connect(mDisplayManager, SIGNAL(screenLayoutChanged()), mUninoteController, SLOT(screenLayoutChanged()));
    connect(mDisplayManager, SIGNAL(screenLayoutChanged()), UBApplication::webController, SLOT(screenLayoutChanged()));

    connect(mUninoteController, SIGNAL(imageCaptured(const QPixmap &, bool)), this, SLOT(addCapturedPixmap(const QPixmap &, bool)));
    connect(mUninoteController, SIGNAL(restoreUniboard()), this, SLOT(hideDesktop()));

    for(int i = 0; i < mDisplayManager->numPreviousViews(); i++)
    {
        UBBoardView *previousView = new UBBoardView(UBApplication::boardController, UBItemLayerType::FixedBackground, UBItemLayerType::Tool, 0);
        previousView->setInteractive(false);
        mPreviousViews.append(previousView);
    }

    mBlackScene = new UBGraphicsScene(0); // deleted by UBApplicationController::destructor
    mBlackScene->setBackground(true, false);

    if (mDisplayManager->numScreens() >= 2)
    {
        mMirror = new UBScreenMirror();
    }

    connect(UBApplication::webController, SIGNAL(imageCaptured(const QPixmap &, bool, const QUrl&))
            , this, SLOT(addCapturedPixmap(const QPixmap &, bool, const QUrl&)));

    networkAccessManager = new QNetworkAccessManager (this);
    connect (networkAccessManager, SIGNAL (finished (QNetworkReply*)), this, SLOT (downloadJsonFinished (QNetworkReply*)));
    QTimer::singleShot (1000, this, SLOT (checkUpdateAtLaunch()));

#ifdef Q_WS_X11
//    mMainWindow->setStyleSheet("QToolButton { font-size: 11px}");
#endif

}


UBApplicationController::~UBApplicationController()
{
    foreach(UBBoardView* view, mPreviousViews)
    {
        delete view;
    }

    delete mBlackScene;
    delete mMirror;
}


void UBApplicationController::initViewState(int horizontalPosition, int verticalPostition)
{
    mInitialHScroll = horizontalPosition;
    mInitialVScroll = verticalPostition;
}


void UBApplicationController::initScreenLayout()
{
    mDisplayManager->setAsControl(mMainWindow);
    mDisplayManager->setAsDisplay(mDisplayView);

    mDisplayManager->setAsPreviousDisplays(mPreviousViews);

    mDisplayManager->adjustScreens(-1);
}


void UBApplicationController::screenLayoutChanged()
{
    initViewState(mControlView->horizontalScrollBar()->value(),
            mControlView->verticalScrollBar()->value());

    adaptToolBar();

    adjustDisplayView();

    if (mDisplayManager->hasDisplay())
    {
        UBApplication::boardController->setBoxing(mDisplayView->geometry());
    }
    else
    {
       UBApplication::boardController->setBoxing(QRect());
    }

    adjustPreviousViews(0, 0);
}


void UBApplicationController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;
    mMainWindow->actionClearPage->setVisible(Board == mMainMode && highResolution);
    mMainWindow->actionBoard->setVisible(Board != mMainMode || highResolution);
    mMainWindow->actionDocument->setVisible(Document != mMainMode || highResolution);
    mMainWindow->actionWeb->setVisible(Internet != mMainMode || highResolution);
    mMainWindow->boardToolBar->setIconSize(QSize(highResolution ? 48 : 42, mMainWindow->boardToolBar->iconSize().height()));

    mMainWindow->actionBoard->setEnabled(mMainMode != Board);
    mMainWindow->actionWeb->setEnabled(mMainMode != Internet);
    mMainWindow->actionDocument->setEnabled(mMainMode != Document);

    if (Document == mMainMode)
    {
        connect(UBApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), UBApplication::documentController, SLOT(focusChanged(QWidget *, QWidget *)));
    }
    else
    {
        disconnect(UBApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), UBApplication::documentController, SLOT(focusChanged(QWidget *, QWidget *)));
        if (Board == mMainMode)
            mMainWindow->actionDuplicate->setEnabled(true);
    }

    UBApplication::boardController->setToolbarTexts();

    UBApplication::webController->adaptToolBar();

}


void UBApplicationController::adjustDisplayView()
{
    if (mDisplayView)
    {
        qreal systemDisplayViewScaleFactor = 1.0;

        QSize pageSize = UBApplication::boardController->activeScene()->nominalSize();
        QSize displaySize = mDisplayView->size();

        qreal hFactor = ((qreal)displaySize.width()) / ((qreal)pageSize.width());
        qreal vFactor = ((qreal)displaySize.height()) / ((qreal)pageSize.height());

        systemDisplayViewScaleFactor = qMin(hFactor, vFactor);

        QTransform tr;
        qreal scaleFactor = systemDisplayViewScaleFactor * UBApplication::boardController->currentZoom();

        tr.scale(scaleFactor, scaleFactor);

        QRect rect = mControlView->rect();
        QPoint center(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);

        mDisplayView->setTransform(tr);

        mDisplayView->centerOn(mControlView->mapToScene(center));
    }
}


void UBApplicationController::adjustPreviousViews(int pActiveSceneIndex, UBDocumentProxy *pActiveDocument)
{
    int viewIndex = pActiveSceneIndex;

    foreach(UBBoardView* previousView, mPreviousViews)
    {
        if (viewIndex > 0)
        {
            viewIndex--;

            UBGraphicsScene* scene = UBPersistenceManager::persistenceManager()->loadDocumentScene(pActiveDocument, viewIndex);

            if (scene)
            {
                previousView->setScene(scene);

                qreal ratio = ((qreal)previousView->geometry().width()) / ((qreal)previousView->geometry().height());
                QRectF sceneRect = scene->normalizedSceneRect(ratio);
                qreal scaleRatio = previousView->geometry().width() / sceneRect.width();

                previousView->resetTransform();

                previousView->scale(scaleRatio, scaleRatio);

                previousView->centerOn(sceneRect.center());
            }
        }
        else
        {
            previousView->setScene(mBlackScene);
        }
    }
}


void UBApplicationController::blackout()
{
    mDisplayManager->blackout();
}


void UBApplicationController::addCapturedPixmap(const QPixmap &pPixmap, bool pageMode, const QUrl& sourceUrl)
{
    if (!pPixmap.isNull())
    {
        qreal sf = UBApplication::boardController->systemScaleFactor();
        qreal scaledWidth = ((qreal)pPixmap.width()) / sf;
        qreal scaledHeight = ((qreal)pPixmap.height()) / sf;

        QSize pageNominalSize = UBApplication::boardController->activeScene()->nominalSize();

        int newWidth = qMin((int)scaledWidth, pageNominalSize.width());
        int newHeight = qMin((int)scaledHeight, pageNominalSize.height());

        if (pageMode)
        {
            newHeight = pPixmap.height();
        }

        QSizeF scaledSize(scaledWidth, scaledHeight);
        scaledSize.scale(newWidth, newHeight, Qt::KeepAspectRatio);

        qreal scaleFactor = qMin(scaledSize.width() / (qreal)pPixmap.width(), scaledSize.height() / (qreal)pPixmap.height());

        QPointF pos(0.0, 0.0);

        if (pageMode)
        {
            pos.setY(pageNominalSize.height() / -2  + scaledSize.height() / 2);
        }

        UBApplication::boardController->paletteManager()->addItem(pPixmap, pos, scaleFactor, sourceUrl);
    }
}


void UBApplicationController::addCapturedEmbedCode(const QString& embedCode)
{
    if (!embedCode.isEmpty())
    {
        showBoard();

        const QString userWidgetPath = UBSettings::settings()->uniboardInteractiveUserDirectory() + "/" + tr("Web"); // TODO UB 4.x synch with w3cWidget
        QDir userWidgetDir(userWidgetPath);

        int width = 300;
        int height = 150;

        QString widgetPath = UBW3CWidget::createHtmlWrapperInDir(embedCode, userWidgetDir,
                QSize(width, height), UBStringUtils::toCanonicalUuid(QUuid::createUuid()));

        if (widgetPath.length() > 0)
            UBApplication::boardController->downloadURL(QUrl::fromLocalFile(widgetPath));
    }
}


void UBApplicationController::showBoard()
{
    mMainWindow->webToolBar->hide();
    mMainWindow->documentToolBar->hide();
    mMainWindow->tutorialToolBar->hide();
    mMainWindow->boardToolBar->show();

    mMainMode = Board;

    adaptToolBar();

    mirroringEnabled(false);

    mMainWindow->switchToBoardWidget();
    mDisplayManager->setAsDisplay(mDisplayView);

    if (UBApplication::boardController)
        UBApplication::boardController->show();

        UBPlatformUtils::setDesktopMode(false);

    mUninoteController->hideWindow();
    mMainWindow->show();

    emit mainModeChanged(Board);
}


void UBApplicationController::showInternet()
{
    if (UBSettings::settings()->webUseExternalBrowser->get().toBool())
    {
        showDesktop(true);
    }
    else
    {
        if (UBApplication::boardController)
        {
            UBApplication::boardController->persistCurrentScene();
            UBApplication::boardController->hide();
        }

        mMainWindow->boardToolBar->hide();
        mMainWindow->documentToolBar->hide();
        mMainWindow->tutorialToolBar->hide();
        mMainWindow->webToolBar->show();

        mMainMode = Internet;

        adaptToolBar();

        mMainWindow->show();
        mUninoteController->hideWindow();

        UBApplication::webController->show(UBWebController::WebBrowser);

        emit mainModeChanged(Internet);
    }
}


void UBApplicationController::showDocument()
{
    mMainWindow->webToolBar->hide();
    mMainWindow->boardToolBar->hide();
    mMainWindow->tutorialToolBar->hide();
    mMainWindow->documentToolBar->show();


    mMainMode = Document;

    adaptToolBar();

    mirroringEnabled(false);

    mMainWindow->switchToDocumentsWidget();

    if (UBApplication::boardController)
    {
        if (UBApplication::boardController->activeScene()->isModified())
            UBApplication::boardController->persistCurrentScene();
        UBApplication::boardController->hide();
    }

    if (UBApplication::documentController)
        UBApplication::documentController->show();

    mMainWindow->show();

    mUninoteController->hideWindow();

    emit mainModeChanged(Document);
}


void UBApplicationController::showDesktop(bool dontSwitchFrontProcess)
{
    int desktopWidgetIndex = qApp->desktop()->screenNumber(mMainWindow);

    if (UBApplication::boardController)
        UBApplication::boardController->hide();

    mMainWindow->hide();
    mUninoteController->showWindow();

    if (mMirror)
    {
        QRect rect = qApp->desktop()->screenGeometry(desktopWidgetIndex);
        mMirror->setSourceRect(rect);
    }

    mIsShowingDesktop = true;
    emit desktopMode(true);

    if (!dontSwitchFrontProcess) {
        UBPlatformUtils::bringPreviousProcessToFront();
    }

    UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);
}


void UBApplicationController::showTutorial()
{

    if (UBApplication::boardController)
    {
        UBApplication::boardController->persistCurrentScene();
        UBApplication::boardController->hide();
    }

    UBApplication::webController->show(UBWebController::Tutorial);

    mMainWindow->webToolBar->hide();
    mMainWindow->boardToolBar->hide();
    mMainWindow->documentToolBar->hide();
    mMainWindow->tutorialToolBar->show();


    mMainMode = Tutorial;

    adaptToolBar();

    mUninoteController->hideWindow();

    mirroringEnabled(false);
    emit mainModeChanged(mMainMode);
}


void UBApplicationController::showSankoreEditor()
{

    if (UBApplication::boardController)
    {
        UBApplication::boardController->persistCurrentScene();
        UBApplication::boardController->hide();
    }

    UBApplication::webController->show(UBWebController::Paraschool);

    mMainWindow->webToolBar->hide();
    mMainWindow->boardToolBar->hide();
    mMainWindow->documentToolBar->hide();
    mMainWindow->tutorialToolBar->show();


    mMainMode = ParaschoolEditor;

    adaptToolBar();

    mUninoteController->hideWindow();

    mirroringEnabled(false);
    emit mainModeChanged(mMainMode);
}


void UBApplicationController::checkUpdate()
{
    networkAccessManager->get (QNetworkRequest (QUrl ("http://getuniboard.com/dl/sankore/update.json")));
}

void UBApplicationController::downloadJsonFinished(QNetworkReply* pReply)
{
    QString currentJson = pReply->readAll();
    QScriptValue scriptValue;
    QScriptEngine scriptEngine;
    scriptValue = scriptEngine.evaluate ("(" + currentJson + ")");

    UBVersion installedVersion (qApp->applicationVersion());
    UBVersion jsonVersion (scriptValue.property("version").toString());

    if (installedVersion.isValid() &&  jsonVersion.isValid() && jsonVersion > installedVersion) {
            QMessageBox msgBox;
            msgBox.setText (tr ("New update available, would you go to the web page ?"));
            msgBox.setStandardButtons (QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton (QMessageBox::Ok);
            int ret = msgBox.exec();
            switch (ret) {
                case QMessageBox::Ok: {
                    QUrl url(scriptValue.property ("url").toString());
                    QDesktopServices::openUrl (url);
                    break;
                }
                case QMessageBox::Cancel:
                    // do nothing
                    break;
                 default:
                    // should never be reached
                    break;
            }
    } else {
        if (isNoUpdateDisplayed) {
            QMessageBox msgBox;
            msgBox.setText (tr ("No update available"));
            msgBox.exec();
        }
    }
}

void UBApplicationController::checkUpdateAtLaunch()
{
    if(UBSettings::settings()->appEnableAutomaticSoftwareUpdates->get().toBool()){
        isNoUpdateDisplayed = false;
        checkUpdate ();
    }
}

void UBApplicationController::checkUpdateRequest()
{
    isNoUpdateDisplayed = true;
    checkUpdate ();
}

void UBApplicationController::hideDesktop()
{
    mDisplayManager->adjustScreens(-1);

    if (mMainMode == Board)
    {
        showBoard();
    }
    else if (mMainMode == Internet)
    {
        showInternet();
    }
    else if (mMainMode == Document)
    {
        showDocument();
    }
    else if (mMainMode == Tutorial)
    {
        showTutorial();
    }
    else if (mMainMode == ParaschoolEditor)
    {
        showSankoreEditor();
    }

    mIsShowingDesktop = false;
    emit desktopMode(false);
}


void UBApplicationController::setMirrorSourceWidget(QWidget* pWidget)
{
    if (mMirror)
    {
        mMirror->setSourceWidget(pWidget);
    }
}


void UBApplicationController::mirroringEnabled(bool enabled)
{
    if (mMirror)
    {
        if (enabled)
        {
            mMirror->start();
            mDisplayManager->setAsDisplay(mMirror);

        }
        else
        {
            mDisplayManager->setAsDisplay(mDisplayView);
            mMirror->stop();
        }

        mMirror->setVisible(enabled && mDisplayManager->numScreens() > 1);
        mUninoteController->updateShowHideState(enabled);
        UBApplication::mainWindow->actionWebShowHideOnDisplay->setChecked(enabled);
    }
    else
    {
        mDisplayManager->setAsDisplay(mDisplayView);
    }
}


void UBApplicationController::closing()
{
    if (mMirror)
        mMirror->stop();

    if (mUninoteController)
    {
        mUninoteController->hideWindow();
        mUninoteController->close();
    }

    if (UBApplication::documentController)
        UBApplication::documentController->closing();
}


void UBApplicationController::showMessage(const QString& message, bool showSpinningWheel)
{
    if (!UBApplication::closingDown())
    {
        if (mMainMode == Document)
        {
            UBApplication::boardController->hideMessage();
            UBApplication::documentController->showMessage(message, showSpinningWheel);
        }
        else
        {
            UBApplication::documentController->hideMessage();
            UBApplication::boardController->showMessage(message, showSpinningWheel);
        }
    }
}


void UBApplicationController::importFile(const QString& pFilePath)
{
    const QFile fileToOpen(pFilePath);

    if (!fileToOpen.exists())
        return;

    UBDocumentProxy* document = 0;

    bool success = false;

#ifdef Q_WS_WIN

    if (UBImportVirtualPrinter::pendingDocument.isNull())
    {

#endif
        document = UBDocumentManager::documentManager()->importFile(fileToOpen,
                UBSettings::defaultDocumentGroupName);

        success = (document != 0);

#ifdef Q_WS_WIN

    }
    else
    {
        document = UBImportVirtualPrinter::pendingDocument;

        UBImportVirtualPrinter::pendingDocument = 0;

        success = UBDocumentManager::documentManager()->addFileToDocument(document, fileToOpen);
        if (success)
            document->setMetaData(UBSettings::documentUpdatedAt, UBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }

#endif

    if (success && document)
    {
        if (mMainMode == Board || mMainMode == Internet)
        {
            if (UBApplication::boardController)
            {
                UBApplication::boardController->setActiveDocumentScene(document, 0);
                showBoard();
            }
        }
        else if (mMainMode == Document)
        {
            if (UBApplication::documentController)
                UBApplication::documentController->selectDocument(document);
        }
    }
}

void UBApplicationController::useMultiScreen(bool use)
{
    mDisplayManager->setUseMultiScreen(use);

}


QStringList UBApplicationController::widgetInlineJavaScripts()
{
    QString scriptDirPath = UBPlatformUtils::applicationResourcesDirectory() + "/widget-inline-js";
    QDir scriptDir(scriptDirPath);

    QStringList scripts;

    if (scriptDir.exists())
    {
        QStringList files = scriptDir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

        foreach(QString file, files)
        {
            QFile scriptFile(scriptDirPath + "/" + file);
            if (file.endsWith(".js") && scriptFile.open(QIODevice::ReadOnly))
            {
                QString s = QString::fromUtf8(scriptFile.readAll());

                if (s.length() > 0)
                    scripts << s;

            }
        }
    }

    qSort(scripts);

    return scripts;
}



void UBApplicationController::actionCut()
{
    if (!UBApplication::closingDown())
    {
        if (mMainMode == Board)
        {
            UBApplication::boardController->cut();
        }
        else if(mMainMode == Document)
        {
            UBApplication::documentController->cut();
        }
        else if(mMainMode == Internet)
        {
            UBApplication::webController->cut();
        }
    }
}


void UBApplicationController::actionCopy()
{
    if (!UBApplication::closingDown())
    {
        if (mMainMode == Board)
        {
            UBApplication::boardController->copy();
        }
        else if(mMainMode == Document)
        {
            UBApplication::documentController->copy();
        }
        else if(mMainMode == Internet)
        {
            UBApplication::webController->copy();
        }
    }
}


void UBApplicationController::actionPaste()
{
    if (!UBApplication::closingDown())
    {
        if (mMainMode == Board)
        {
            UBApplication::boardController->paste();
        }
        else if (mMainMode == Document)
        {
            UBApplication::documentController->paste();
        }
        else if(mMainMode == Internet)
        {
            UBApplication::webController->paste();
        }
    }
}
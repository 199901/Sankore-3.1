/*
 * UBApplication.cpp
 *
 *  Created on: Sep 11, 2008
 *      Author: luc
 */

#include "UBApplication.h"

#include <QtGui>
#include <QtWebKit>

#if defined(Q_WS_MACX)
#include <Carbon/Carbon.h>
#endif

#include "frameworks/UBPlatformUtils.h"
#include "frameworks/UBFileSystemUtils.h"
#include "frameworks/UBStringUtils.h"

#include "UBSettings.h"
#include "UBSetting.h"
#include "UBPersistenceManager.h"
#include "UBDocumentManager.h"
#include "UBPreferencesController.h"
#include "UBIdleTimer.h"
#include "UBApplicationController.h"

#include "softwareupdate/UBSoftwareUpdateController.h"

#include "board/UBBoardController.h"
#include "board/UBDrawingController.h"
#include "board/UBLibraryController.h"
#include "board/UBBoardView.h"

#include "web/UBWebController.h"

#include "document/UBDocumentController.h"
#include "document/UBDocumentProxy.h"

#include "gui/UBMainWindow.h"
#include "gui/UBResources.h"

#include "adaptors/publishing/UBSvgSubsetRasterizer.h"

#include "ui_mainWindow.h"

QPointer<QUndoStack> UBApplication::undoStack;

UBApplicationController* UBApplication::applicationController = 0;
UBBoardController* UBApplication::boardController = 0;
UBWebController* UBApplication::webController = 0;
UBDocumentController* UBApplication::documentController = 0;
UBSoftwareUpdateController* UBApplication::softwareUpdateController = 0;

UBMainWindow* UBApplication::mainWindow = 0;

const QString UBApplication::mimeTypeUniboardDocument = QString("application/vnd.mnemis-uniboard-document");
const QString UBApplication::mimeTypeUniboardPage = QString("application/vnd.mnemis-uniboard-page");
const QString UBApplication::mimeTypeUniboardPageItem =  QString("application/vnd.mnemis-uniboard-page-item");
const QString UBApplication::mimeTypeUniboardPageThumbnail = QString("application/vnd.mnemis-uniboard-thumbnail");


QObject* UBApplication::staticMemoryCleaner = 0;

#if defined(Q_WS_MAC)
static OSStatus ub_appleEventProcessor(const AppleEvent *ae, AppleEvent *event, long handlerRefCon)
{
    Q_UNUSED(event);
    OSType aeID = typeWildCard;
    OSType aeClass = typeWildCard;

    AEGetAttributePtr(ae, keyEventClassAttr, typeType, 0, &aeClass, sizeof(aeClass), 0);
    AEGetAttributePtr(ae, keyEventIDAttr, typeType, 0, &aeID, sizeof(aeID), 0);

    if (aeClass == kCoreEventClass && aeID == kAEReopenApplication)
    {
        // User clicked on Uniboard in the Dock
        ((UBApplicationController*)handlerRefCon)->hideDesktop();
        return noErr;
    }

    return eventNotHandledErr;
}
#endif


UBApplication::UBApplication(const QString &id, int &argc, char **argv)
    : QtSingleApplication(id, argc, argv)
{

    staticMemoryCleaner = new QObject(0); // deleted in UBApplication destructor

    setOrganizationName("Sankore");
    setOrganizationDomain("sankore.org");
    setApplicationName("Sankore 3.1");

    setApplicationVersion(UBVERSION);

#if defined(Q_WS_MAC) && !defined(QT_NO_DEBUG)
    CFStringRef shortVersion = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleShortVersionString"));
    const char *version = CFStringGetCStringPtr(shortVersion, kCFStringEncodingMacRoman);
    Q_ASSERT(version);
    setApplicationVersion(version);

#endif

    QStringList args = arguments();

    mIsVerbose = args.contains("-v")
        || args.contains("-verbose")
        || args.contains("verbose")
        || args.contains("-log")
        || args.contains("log");

    UBPlatformUtils::init();
    UBResources::resources();

    if (!undoStack)
    {
        undoStack = new QUndoStack(staticMemoryCleaner);
    }

    QTranslator *translator = new QTranslator(this);

    translator->load(UBPlatformUtils::preferredTranslation());

    installTranslator(translator);

    QString localString;

    if (!translator->isEmpty())
    {
        localString = UBPlatformUtils::preferredLanguage();
    }
    else
    {
        // Make sure we don't mix English language with unsupported language dates
        //QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        localString = "en_US";
    }

    QLocale::setDefault(QLocale(localString));
    qDebug() << "Running application in:" << localString;

    UBSettings *settings = UBSettings::settings();

    connect(settings->appToolBarPositionedAtTop, SIGNAL(changed(QVariant)), this, SLOT(toolBarPositionChanged(QVariant)));
    connect(settings->appToolBarDisplayText, SIGNAL(changed(QVariant)), this, SLOT(toolBarDisplayTextChanged(QVariant)));
    updateProtoActionsState();

#ifndef Q_WS_MAC
    setWindowIcon(QIcon(":/images/uniboard.png"));
#endif

    setStyle(new UBStyle()); // Style is owned and deleted by the application

    QString css = UBFileSystemUtils::readTextFile(UBPlatformUtils::applicationResourcesDirectory() + "/etc/Uniboard.css");
    if (css.length() > 0)
        setStyleSheet(css);

    QApplication::setStartDragDistance(8); // default is 4, and is a bit small for tablets

    installEventFilter(this);

}


UBApplication::~UBApplication()
{
    UBPlatformUtils::destroy();

    UBFileSystemUtils::deleteAllTempDirCreatedDuringSession();

//    delete mainWindow;
    mainWindow = 0;

//    delete staticMemoryCleaner;
    staticMemoryCleaner = 0;
}

void UBApplication::showMinimized()
{
    mainWindow->showMinimized();
}

int UBApplication::exec(const QString& pFileToImport)
{
    QPixmapCache::setCacheLimit(1024 * 100);

    QString webDbPath = UBSettings::uniboardDataDirectory() + "/web-databases";
    QDir webDbDir(webDbPath);
    if (!webDbDir.exists(webDbPath))
        webDbDir.mkpath(webDbPath);

    QWebSettings::setIconDatabasePath(webDbPath);
    QWebSettings::setOfflineStoragePath (webDbPath);

    QWebSettings *gs = QWebSettings::globalSettings();
    gs->setAttribute(QWebSettings::PluginsEnabled, true);
    gs->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);
    gs->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
    gs->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    gs->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    gs->setAttribute(QWebSettings::DnsPrefetchEnabled, true);


    mainWindow = new UBMainWindow(0, Qt::FramelessWindowHint); // deleted by application destructor
    mainWindow->setAttribute(Qt::WA_NativeWindow, true);

    mainWindow->actionCopy->setShortcuts(QKeySequence::Copy);
    mainWindow->actionPaste->setShortcuts(QKeySequence::Paste);
    mainWindow->actionCut->setShortcuts(QKeySequence::Cut);

    connect(mainWindow->actionBoard, SIGNAL(triggered()), this, SLOT(showBoard()));
    connect(mainWindow->actionWeb, SIGNAL(triggered()), this, SLOT(showInternet()));
    connect(mainWindow->actionDocument, SIGNAL(triggered()), this, SLOT(showDocument()));
    connect(mainWindow->actionQuit, SIGNAL(triggered()), this, SLOT(closing()));

    boardController = new UBBoardController(mainWindow);
    boardController->init();

    webController = new UBWebController(mainWindow);
    documentController = new UBDocumentController(mainWindow);

    applicationController = new UBApplicationController(boardController->controlView(), boardController->displayView(), mainWindow, staticMemoryCleaner);

    connect(mainWindow->actionDesktop, SIGNAL(triggered(bool)), applicationController, SLOT(showDesktop(bool)));
    connect(mainWindow->actionHideApplication, SIGNAL(triggered()), this, SLOT(showMinimized()));

    mPreferencesController = new UBPreferencesController(mainWindow);

    connect(mainWindow->actionPreferences, SIGNAL(triggered()), mPreferencesController, SLOT(show()));
    connect(mainWindow->actionTutorial, SIGNAL(triggered()), applicationController, SLOT(showTutorial()));
    connect(mainWindow->actionSankoreEditor, SIGNAL(triggered()), applicationController, SLOT(showSankoreEditor()));
    connect(mainWindow->actionCheckUpdate, SIGNAL(triggered()), applicationController, SLOT(checkUpdateRequest()));

    UBDrawingController::drawingController()->setStylusTool((int)UBStylusTool::Pen);

    toolBarPositionChanged(UBSettings::settings()->appToolBarPositionedAtTop->get());

    connect(mainWindow->actionMultiScreen, SIGNAL(triggered(bool)), applicationController, SLOT(useMultiScreen(bool)));
    connect(mainWindow->actionWidePageSize, SIGNAL(triggered(bool)), boardController, SLOT(setWidePageSize(bool)));
    connect(mainWindow->actionRegularPageSize, SIGNAL(triggered(bool)), boardController, SLOT(setRegularPageSize(bool)));

    connect(mainWindow->actionCut, SIGNAL(triggered()), applicationController, SLOT(actionCut()));
    connect(mainWindow->actionCopy, SIGNAL(triggered()), applicationController, SLOT(actionCopy()));
    connect(mainWindow->actionPaste, SIGNAL(triggered()), applicationController, SLOT(actionPaste()));

//#ifndef __ppc__
//    // this cause a problem on MACX/PPC (see https://trac.assembla.com/uniboard/ticket/862)
//    installEventFilter(new UBIdleTimer(this));
//#endif

    // TODO UB 4.x make it better and reenable it ... or dump
    //installEventFilter(new UBMousePressFilter);

    applicationController->initScreenLayout();
    boardController->setupLayout();

    if (pFileToImport.length() > 0)
    {
        UBApplication::applicationController->importFile(pFileToImport);
    }

#if defined(Q_WS_MAC)
    static AEEventHandlerUPP ub_proc_ae_handlerUPP = AEEventHandlerUPP(ub_appleEventProcessor);
    AEInstallEventHandler(kCoreEventClass, kAEReopenApplication, ub_proc_ae_handlerUPP, SRefCon(UBApplication::applicationController), true);
#endif


    if (UBSettings::settings()->appIsInSoftwareUpdateProcess->get().toBool())
    {
        UBSettings::settings()->appIsInSoftwareUpdateProcess->set(false);

        // clean potential updater in temp directory
        UBFileSystemUtils::cleanupGhostTempFolders();

        QUuid docUuid( UBSettings::settings()->appLastSessionDocumentUUID->get().toString());

        if (!docUuid.isNull())
        {
            UBDocumentProxy* proxy = UBPersistenceManager::persistenceManager()->documentByUuid(docUuid);

            if (proxy)
            {
                bool ok;
                int lastSceneIndex = UBSettings::settings()->appLastSessionPageIndex->get().toInt(&ok);

                if (!ok)
                    lastSceneIndex = 0;

                boardController->setActiveDocumentScene(proxy, lastSceneIndex);
            }
        }
    }

    UBLibraryController::preloadFirstOnlineLibrary();

    return QApplication::exec();
}



void UBApplication::showBoard()
{
    applicationController->showBoard();
}

void UBApplication::showInternet()
{
    applicationController->showInternet();
    webController->showTabAtTop(true);
}

void UBApplication::showDocument()
{
    applicationController->showDocument();
}


int UBApplication::toolBarHeight()
{
    return mainWindow->boardToolBar->rect().height();
}


void UBApplication::toolBarPositionChanged(QVariant topOrBottom)
{
    Qt::ToolBarArea area;

    if (topOrBottom.toBool())
        area = Qt::TopToolBarArea;
    else
        area = Qt::BottomToolBarArea;

    mainWindow->addToolBar(area, mainWindow->boardToolBar);
    mainWindow->addToolBar(area, mainWindow->webToolBar);
    mainWindow->addToolBar(area, mainWindow->documentToolBar);
    mainWindow->addToolBar(area, mainWindow->tutorialToolBar);

    webController->showTabAtTop(topOrBottom.toBool());

}


void UBApplication::toolBarDisplayTextChanged(QVariant display)
{
    Qt::ToolButtonStyle toolButtonStyle = display.toBool() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;
    mainWindow->boardToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->webToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->documentToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->tutorialToolBar->setToolButtonStyle(toolButtonStyle);
}


void UBApplication::closing()
{

    if (boardController)
        boardController->closing();

    if (applicationController)
        applicationController->closing();

    if (webController)
        webController->closing();

    UBSettings::settings()->appToolBarPositionedAtTop->set(mainWindow->toolBarArea(mainWindow->boardToolBar) == Qt::TopToolBarArea);

    quit();
}


void UBApplication::showMessage(const QString& message, bool showSpinningWheel)
{
    if (applicationController)
        applicationController->showMessage(message, showSpinningWheel);
}


void UBApplication::setDisabled(bool disable)
{
    boardController->setDisabled(disable);
}


void UBApplication::decorateActionMenu(QAction* action)
{
    foreach(QWidget* menuWidget,  action->associatedWidgets())
    {
        QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

        if (tb && !tb->menu())
        {
            tb->setObjectName("ubButtonMenu");
            tb->setPopupMode(QToolButton::InstantPopup);
            QMenu* menu = new QMenu(mainWindow);

            QActionGroup* pageSizeGroup = new QActionGroup(mainWindow);
            pageSizeGroup->addAction(mainWindow->actionWidePageSize);
            pageSizeGroup->addAction(mainWindow->actionRegularPageSize);
            pageSizeGroup->addAction(mainWindow->actionCustomPageSize);

            QMenu* documentSizeMenu = menu->addMenu(QIcon(":/images/toolbar/pageSize.png"),tr("Page Size"));
            documentSizeMenu->addAction(mainWindow->actionWidePageSize);
            documentSizeMenu->addAction(mainWindow->actionRegularPageSize);
            documentSizeMenu->addAction(mainWindow->actionCustomPageSize);
            menu->addAction(mainWindow->actionCut);
            menu->addAction(mainWindow->actionCopy);
            menu->addAction(mainWindow->actionPaste);

            menu->addSeparator();
            menu->addAction(mainWindow->actionPreferences);
            menu->addAction(mainWindow->actionHideApplication);
            menu->addAction(mainWindow->actionSleep);

            menu->addSeparator();
            menu->addAction(mainWindow->actionSankoreEditor);
            menu->addAction(mainWindow->actionCheckUpdate);

#ifndef Q_WS_X11 // No Podcast on Linux yet
            menu->addAction(mainWindow->actionPodcast);
            mainWindow->actionPodcast->setText(tr("Podcast"));
#endif
            menu->addAction(mainWindow->actionMultiScreen);
            menu->addSeparator();
            menu->addAction(mainWindow->actionQuit);

            tb->setMenu(menu);
        }
    }
}


void UBApplication::updateProtoActionsState()
{
    if (mainWindow)
    {
        mainWindow->actionMultiScreen->setVisible(true);
    }

    foreach(QMenu* protoMenu, mProtoMenus)
        protoMenu->setVisible(true);

}


void UBApplication::insertSpaceToToolbarBeforeAction(QToolBar* toolbar, QAction* action, int width)
{
    QWidget* spacer = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout();

    if (width >= 0)
        layout->addSpacing(width);
    else
        layout->addStretch();

    spacer->setLayout(layout);
    toolbar->insertWidget(action, spacer);

}


bool UBApplication::eventFilter(QObject *obj, QEvent *event)
{
    bool result = QObject::eventFilter(obj, event);

    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *fileToOpenEvent = static_cast<QFileOpenEvent *>(event);

#if defined(Q_WS_MACX)
        ProcessSerialNumber psn;
        GetCurrentProcess(&psn);
        SetFrontProcess(&psn);
#endif

        applicationController->importFile(fileToOpenEvent->file());
    }

    if (event->type() == QEvent::TabletLeaveProximity)
    {
        if (boardController && boardController->controlView())
            boardController->controlView()->forcedTabletRelease();
    }

    return result;
}


bool UBApplication::handleOpenMessage(const QString& pMessage)
{
    qDebug() << "received message" << pMessage;

    if (pMessage == UBSettings::appPingMessage)
    {
        qDebug() << "received ping";
        return true;
    }

    qDebug() << "importing file" << pMessage;

    UBApplication::applicationController->importFile(pMessage);

    return true;
}



#if defined(Q_WS_MACX) && !defined(QT_MAC_USE_COCOA)

bool UBApplication::macEventFilter(EventHandlerCallRef caller, EventRef event)
{
    Q_UNUSED(caller);

    if ((GetEventClass(event) == kEventClassCommand) && (GetEventKind(event) == kEventProcessCommand))
    {
        HICommand cmd;
        GetEventParameter(event, kEventParamDirectObject, typeHICommand, 0, sizeof(cmd), 0, &cmd);
        if (cmd.commandID == kHICommandHide)
        {
            // Override the command + H (Hide Uniboard) behavior
            applicationController->showDesktop();
            return true;
        }
    }
    return false;
}
#endif


void UBStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                          bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;

    QPen savedPen;
    if (textRole != QPalette::NoRole)
    {
        savedPen = painter->pen();
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    }

    if (!enabled)
    {
        QPen pen = painter->pen();
        QColor half = pen.color();

        half.setRed(half.red() / 2);
        half.setGreen(half.green() / 2);
        half.setBlue(half.blue() / 2);

        painter->setPen(half);
        painter->drawText(rect, alignment, text);
        painter->setPen(pen);
    }

    /*
     *
#if defined(Q_WS_MACX)

    if (pal.brush(textRole).color() == Qt::black)
    {
        painter->save();
        painter->translate(0, 0.1);

        QBrush brush = pal.brush(QPalette::Light);
        QColor color = brush.color();
        color.setAlpha(color.alpha() * 0.6);
        brush.setColor(color);

        painter->setPen(QPen(brush, savedPen.widthF() * 0.8));

        painter->drawText(rect, alignment, text);

        painter->restore();
    }

#endif

    */
    painter->drawText(rect, alignment, text);

    if (textRole != QPalette::NoRole)
        painter->setPen(savedPen);
}
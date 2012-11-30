TARGET = "Open-Sankore"
TEMPLATE = app

CONFIG -= flat
CONFIG += debug_and_release \
          no_include_pwd

CORE_ROOT = .
CORE_SRC = $$CORE_ROOT/src
THIRD_PARTY_PATH=$$CORE_SRC/../../Sankore-ThirdParty

VERSION_MAJ = 2
VERSION_MIN = 00 
VERSION_TYPE = b # a = alpha, b = beta, r = release, other => error
VERSION_PATCH = 10 

VERSION = "$${VERSION_MAJ}.$${VERSION_MIN}.$${VERSION_TYPE}.$${VERSION_PATCH}"
VERSION = $$replace(VERSION, "\\.r", "")

LONG_VERSION = "$${VERSION}.$${SVN_VERSION}"
macx:OSX_VERSION = "$${VERSION} (r$${SVN_VERSION})"

VERSION_RC = $$VERSION_MAJ,$$VERSION_MIN,$$VERSION_TYPE,$$VERSION_PATCH
VERSION_RC = $$replace(VERSION_RC, "a", "160") # 0xA0
VERSION_RC = $$replace(VERSION_RC, "b", "176") # 0xB0
VERSION_RC = $$replace(VERSION_RC, "r", "240") # 0xF0

QT += webkit
QT += svg
QT += network
QT += phonon
QT += xml
QT += script
QT += xmlpatterns

INCLUDEPATH += src

include($$THIRD_PARTY_PATH/libs.pri)
include($$CORE_SRC/adaptors/adaptors.pri)
include($$CORE_SRC/api/api.pri)
include($$CORE_SRC/board/board.pri)
include($$CORE_SRC/core/core.pri)
include($$CORE_SRC/document/document.pri)
include($$CORE_SRC/domain/domain.pri)
include($$CORE_SRC/gui/gui.pri)
include($$CORE_SRC/network/network.pri)
include($$CORE_SRC/pdf/pdf.pri)
include($$CORE_SRC/podcast/podcast.pri)
include($$CORE_SRC/tools/tools.pri)
include($$CORE_SRC/desktop/desktop.pri)
include($$CORE_SRC/web/web.pri)
include($$CORE_SRC/transition/transition.pri)

# Static libraries
SANKOREAPI_BASEPATH = $$PWD/../Sankore-API
SANKOREAPI_PATH = $$SANKOREAPI_BASEPATH/build
macx:SANKOREAPI_PATH = $$SANKOREAPI_PATH/macx
win32:SANKOREAPI_PATH = $$SANKOREAPI_PATH/win32
linux-g++*:SANKOREAPI_PATH = $$SANKOREAPI_PATH/linux
CONFIG(debug, debug|release):SANKOREAPI_PATH = $$SANKOREAPI_PATH/debug/product
CONFIG(release, debug|release) {
   SANKOREAPI_PATH = $$SANKOREAPI_PATH/release/product
}
LIBS += -L$$SANKOREAPI_PATH -lSankoreAPI
INCLUDEPATH += $$SANKOREAPI_BASEPATH

# 3rd party static libraries
THIRDPARTY_PATH = $$PWD/../Sankore-ThirdParty
QUAZIP_BASEPATH = $$THIRDPARTY_PATH/quazip
QUAZIP_PATH = $$QUAZIP_BASEPATH/lib
macx:QUAZIP_PATH = $$QUAZIP_PATH/macx
win32:QUAZIP_PATH = $$QUAZIP_PATH/win32
linug-g++*:QUAZIP_PATH = $$QUAZIP_PATH/linux
LIBS += -L$$QUAZIP_PATH -lquazip
INCLUDEPATH += $$QUAZIP_BASEPATH/quazip-0.3

DEPENDPATH += src/pdf-merger
INCLUDEPATH += src/pdf-merger
include($$CORE_SRC/pdf-merger/pdfMerger.pri)

# plugins (no, don't use it like that)
include(plugins/plugins.pri)
INCLUDEPATH += plugins/cffadaptor/src

# ThirdParty
DEPENDPATH += $$THIRD_PARTY_PATH/trolltech/singleapplication
INCLUDEPATH += $$THIRD_PARTY_PATH/trolltech/singleapplication
include($$THIRD_PARTY_PATH/trolltech/singleapplication/qtsingleapplication.pri)

FORMS += resources/forms/mainWindow.ui \
   resources/forms/preferences.ui \
   resources/forms/brushProperties.ui \
   resources/forms/documents.ui \
   resources/forms/blackoutWidget.ui \
   resources/forms/trapFlash.ui \
   resources/forms/youTubePublishingDialog.ui \
   resources/forms/webPublishing.ui \
   resources/forms/capturePublishing.ui \
   resources/forms/intranetPodcastPublishingDialog.ui

UB_ETC.files = resources/etc
UB_I18N.files = resources/i18n/*.qm
UB_LIBRARY.files = resources/library
UB_FONTS.files = resources/fonts
UB_THIRDPARTY_INTERACTIVE.files = thirdparty/interactive
UB_PLUGINS.files = plugins/bin

DEFINES += NO_THIRD_PARTY_WARNINGS
DEFINES += UBVERSION=\"\\\"$${LONG_VERSION}\"\\\" \
   UBVERSION_RC=$$VERSION_RC
ALPHA_BETA_STR = $$find(VERSION, "[ab]")
count(ALPHA_BETA_STR, 1):DEFINES += PRE_RELEASE
BUILD_DIR = build

macx:BUILD_DIR = $$BUILD_DIR/macx
win32:BUILD_DIR = $$BUILD_DIR/win32
linux-g++*:BUILD_DIR = $$BUILD_DIR/linux

CONFIG(debug, debug|release):BUILD_DIR = $$BUILD_DIR/debug
CONFIG(release, debug|release) {
   BUILD_DIR = $$BUILD_DIR/release
   CONFIG += warn_off
}

DESTDIR = $$BUILD_DIR/product
OBJECTS_DIR = $$BUILD_DIR/objects
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui

win32 {
   RC_FILE = resources/win/sankore.rc
   CONFIG += qaxcontainer
   exists(console):CONFIG += console
   QMAKE_CXXFLAGS += /MP
   QMAKE_CXXFLAGS_RELEASE += /Od /Zi
   QMAKE_LFLAGS_RELEASE += /DEBUG
   UB_LIBRARY.path = $$DESTDIR
   UB_I18N.path = $$DESTDIR/i18n
   UB_ETC.path = $$DESTDIR
   UB_THIRDPARTY_INTERACTIVE.path = $$DESTDIR/library
   UB_PLUGINS.path = $$DESTDIR
   system(md $$replace(BUILD_DIR, /, \\))
   system(echo "$$VERSION" > $$BUILD_DIR/version)
   system(echo "$$LONG_VERSION" > $$BUILD_DIR/longversion)
   system(echo "$$SVN_VERSION" > $$BUILD_DIR/svnversion)

}

macx {
   LIBS += -framework Foundation 
   LIBS += -lcrypto
   #commented because Sankore crashes on Java Script. It seems to backends dependencies.
   #LIBS += -framework AppKit 
   #LIBS += -framework WebKit

   CONFIG(release, debug|release):CONFIG += x86

   # [03-02-2011] We must use the 32bit version for the moment
   # because the Quicktime components used by this application
   # are not yet available in 64bits.
   CONFIG(debug, debug|release):CONFIG += x86

   QMAKE_MAC_SDK = "/Developer/SDKs/MacOSX10.6.sdk"
   QMAKE_MACOSX_DEPLOYMENT_TARGET = "10.5"

   VERSION_RC_PATH = "$$BUILD_DIR/version_rc"

   # Embed version into executable for breakpad
   QMAKE_LFLAGS += -sectcreate \
       __DATA \
       __version \
       $$VERSION_RC_PATH

   QMAKE_CXXFLAGS_RELEASE += -gdwarf-2 \
       -mdynamic-no-pic

   QMAKE_CFLAGS += -fopenmp
   QMAKE_CXXFLAGS += -fopenmp
   QMAKE_LFLAGS += -fopenmp

   CONTENTS_DIR = "Contents"
   RESOURCES_DIR = "Contents/Resources"
   FRAMEWORKS_DIR = "Contents/Frameworks"
   PLUGINS_DIR = "Contents/Plugins"

   UB_ETC.files = "resources/etc"
   UB_ETC.path = "$$RESOURCES_DIR"
   UB_PLUGINS.files = "plugins/bin"
   UB_PLUGINS.path = "$$PLUGINS_DIR"
   UB_LIBRARY.files = "resources/library"
   UB_LIBRARY.path = "$$RESOURCES_DIR"
   UB_FONTS.files = "resources/fonts"
   UB_FONTS.path = "$$RESOURCES_DIR"
   UB_THIRDPARTY_INTERACTIVE.files = $$files($$THIRD_PARTY_PATH/interactive/*)
   UB_THIRDPARTY_INTERACTIVE.path = "$$RESOURCES_DIR/library/interactive"
   UB_MACX_ICNS.files = $$files(resources/macx/*.icns)
   UB_MACX_ICNS.path = "$$RESOURCES_DIR"
   UB_MACX_EXTRAS.files = "resources/macx/Save PDF to Open-Sankore.workflow"
   UB_MACX_EXTRAS.path = "$$RESOURCES_DIR"
   UB_I18N.path = $$DESTDIR/i18n # not used

   exists(resources/i18n/sankore_en.qm) {
       TRANSLATION_en.files = resources/i18n/sankore_en.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_en.path = "$$RESOURCES_DIR/en.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_en
   }
   exists(resources/i18n/sankore_en_UK.qm) {
       TRANSLATION_en_UK.files = resources/i18n/sankore_en_UK.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_en_UK.path = "$$RESOURCES_DIR/en_UK.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_en_UK
   }
   exists(resources/i18n/sankore_fr.qm) {
       TRANSLATION_fr.files = resources/i18n/sankore_fr.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_fr.path = "$$RESOURCES_DIR/fr.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_fr
   }
   exists(resources/i18n/sankore_fr_CH.qm) {
       TRANSLATION_fr_CH.files = resources/i18n/sankore_fr_CH.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_fr_CH.path = "$$RESOURCES_DIR/fr_CH.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_fr_CH
   }
   exists(resources/i18n/sankore_de.qm) {
       TRANSLATION_de.files = resources/i18n/sankore_de.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_de.path = "$$RESOURCES_DIR/de.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_de
   }
   exists(resources/i18n/sankore_nl.qm) {
       TRANSLATION_nl.files = resources/i18n/sankore_nl.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_nl.path = "$$RESOURCES_DIR/nl.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_nl
   }
   exists(resources/i18n/sankore_es.qm) {
       TRANSLATION_es.files = resources/i18n/sankore_es.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_es.path = "$$RESOURCES_DIR/es.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_es
   }
   exists(resources/i18n/sankore_it.qm) {
       TRANSLATION_it.files = resources/i18n/sankore_it.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_it.path = "$$RESOURCES_DIR/it.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_it
   }
   exists(resources/i18n/sankore_pl.qm) {
       TRANSLATION_pl.files = resources/i18n/sankore_pl.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_pl.path = "$$RESOURCES_DIR/pl.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_pl
   }
   exists(resources/i18n/sankore_ru.qm) {
       TRANSLATION_ru.files = resources/i18n/sankore_ru.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_ru.path = "$$RESOURCES_DIR/ru.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_ru
   }
   exists(resources/i18n/sankore_da.qm) {
       TRANSLATION_da.files = resources/i18n/sankore_da.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_da.path = "$$RESOURCES_DIR/da.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_da
   }
   exists(resources/i18n/sankore_nb.qm) {
       TRANSLATION_nb.files = resources/i18n/sankore_nb.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_nb.path = "$$RESOURCES_DIR/nb.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_nb
   }
   exists(resources/i18n/sankore_rm.qm) {
       TRANSLATION_rm.files = resources/i18n/sankore_rm.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_rm.path = "$$RESOURCES_DIR/rm.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_rm
   }
   exists(resources/i18n/sankore_sv.qm) {
       TRANSLATION_sv.files = resources/i18n/sankore_sv.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_sv.path = "$$RESOURCES_DIR/sv.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_sv
   }
   exists(resources/i18n/sankore_ja.qm) {
       TRANSLATION_ja.files = resources/i18n/sankore_ja.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_ja.path = "$$RESOURCES_DIR/ja.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_ja
   }
   exists(resources/i18n/sankore_ko.qm) {
       TRANSLATION_ko.files = resources/i18n/sankore_ko.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_ko.path = "$$RESOURCES_DIR/ko.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_ko
   }
   exists(resources/i18n/sankore_zh.qm) {
       TRANSLATION_zh.files = resources/i18n/sankore_zh.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_zh.path = "$$RESOURCES_DIR/zh.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_zh
   }
   exists(resources/i18n/sankore_ro.qm) {
       TRANSLATION_ro.files = resources/i18n/sankore_ro.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_ro.path = "$$RESOURCES_DIR/ro.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_ro
   }
   exists(resources/i18n/sankore_ar.qm) {
       TRANSLATION_ar.files = resources/i18n/sankore_ar.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_ar.path = "$$RESOURCES_DIR/ar.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_ar
   }

   exists(resources/i18n/sankore_iw.qm) {
       TRANSLATION_iw.files = resources/i18n/sankore_iw.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_iw.path = "$$RESOURCES_DIR/iw.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_iw
   }

   exists(resources/i18n/sankore_pt.qm) {
       TRANSLATION_pt.files = resources/i18n/sankore_pt.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_pt.path = "$$RESOURCES_DIR/pt.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_pt
   }

   exists(resources/i18n/sankore_sk.qm) {
       TRANSLATION_sk.files = resources/i18n/sankore_sk.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_sk.path = "$$RESOURCES_DIR/sk.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_sk
   }
   
   exists(resources/i18n/sankore_bg.qm) {
       TRANSLATION_bg.files = resources/i18n/sankore_bg.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_bg.path = "$$RESOURCES_DIR/bg.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_bg
   }
   
   exists(resources/i18n/sankore_ca.qm) {
       TRANSLATION_ca.files = resources/i18n/sankore_ca.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_ca.path = "$$RESOURCES_DIR/ca.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_ca
   }

   exists(resources/i18n/sankore_el.qm) {
       TRANSLATION_el.files = resources/i18n/sankore_el.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_el.path = "$$RESOURCES_DIR/el.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_el
   }
   exists(resources/i18n/sankore_tr.qm) {
       TRANSLATION_tr.files = resources/i18n/sankore_tr.qm \
           resources/i18n/Localizable.strings
       TRANSLATION_tr.path = "$$RESOURCES_DIR/tr.lproj"
       QMAKE_BUNDLE_DATA += TRANSLATION_tr
   }
   exists(resources/i18n/sankore_cs.qm) {
       translation_cs.files = resources/i18n/sankore_cs.qm \
           resources/i18n/localizable.strings
       translation_cs.path = "$$resources_dir/cs.lproj"
       qmake_bundle_data += translation_cs
   }
   
   exists(resources/i18n/sankore_mg.qm) {
       translation_mg.files = resources/i18n/sankore_mg.qm \
           resources/i18n/localizable.strings
       translation_mg.path = "$$resources_dir/mg.lproj"
       qmake_bundle_data += translation_mg
   }
   
   QMAKE_BUNDLE_DATA += UB_ETC \
       UB_PLUGINS \
       UB_LIBRARY \
       UB_FONTS \
       UB_THIRDPARTY_INTERACTIVE \
       UB_MACX_ICNS \
       UB_MACX_EXTRAS \
       SPARKLE_KEY \
       FRAMEWORKS

   QMAKE_PKGINFO_TYPEINFO = "UniB"
   QMAKE_INFO_PLIST = "resources/macx/Info.plist"

   # For packger (release.macx.sh script) to know Uniboard version
   system(mkdir -p $$BUILD_DIR)
   system(printf \""$$OSX_VERSION"\" > $$BUILD_DIR/osx_version)
   system(printf \""$$VERSION"\" > $$BUILD_DIR/version)
   system(printf "%02x%02x%02x%02x" `printf $$VERSION_RC | cut -d ',' -f 1` `printf $$VERSION_RC | cut -d ',' -f 2` `printf $$VERSION_RC | cut -d ',' -f 3` `printf $$VERSION_RC | cut -d ',' -f 4` | xxd -r -p > "$$VERSION_RC_PATH")
}

linux-g++* {
    CONFIG += link_prl
    LIBS += -lcrypto
    LIBS += -lX11
    QMAKE_CFLAGS += -fopenmp
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp
    UB_LIBRARY.path = $$DESTDIR
    UB_I18N.path = $$DESTDIR/i18n
    UB_ETC.path = $$DESTDIR
    UB_PLUGINS.path = $$DESTDIR
    UB_THIRDPARTY_INTERACTIVE.path = $$DESTDIR/library
    system(mkdir -p $$BUILD_DIR)
    system(echo "$$VERSION" > $$BUILD_DIR/version)
    system(echo "$$LONG_VERSION" > $$BUILD_DIR/longversion)
    system(echo "$$SVN_VERSION" > $$BUILD_DIR/svnversion)
}

RESOURCES += resources/sankore.qrc
RESOURCES += plugins/cffadaptor/resources/resources.qrc

# When adding a translation here, also add it in the macx part
TRANSLATIONS = resources/i18n/sankore_en.ts \
   resources/i18n/sankore_en_UK.ts \
   resources/i18n/sankore_fr.ts \
   resources/i18n/sankore_fr_CH.ts \
   resources/i18n/sankore_de.ts \
   resources/i18n/sankore_nl.ts \
   resources/i18n/sankore_es.ts \
   resources/i18n/sankore_it.ts \
   resources/i18n/sankore_pl.ts \
   resources/i18n/sankore_ru.ts \
   resources/i18n/sankore_da.ts \
   resources/i18n/sankore_nb.ts \
   resources/i18n/sankore_rm.ts \
   resources/i18n/sankore_sv.ts \
   resources/i18n/sankore_ja.ts \
   resources/i18n/sankore_ko.ts \
   resources/i18n/sankore_zh.ts \
   resources/i18n/sankore_ro.ts \
   resources/i18n/sankore_ar.ts \
   resources/i18n/sankore_iw.ts \
   resources/i18n/sankore_pt.ts \
   resources/i18n/sankore_sk.ts \
   resources/i18n/sankore_bg.ts \
   resources/i18n/sankore_ca.ts \
   resources/i18n/sankore_el.ts \
   resources/i18n/sankore_tr.ts \
   resources/i18n/sankore_cs.ts \
   resources/i18n/sankore_mg.ts

INSTALLS = UB_ETC \
   UB_PLUGINS \
   UB_I18N \
   UB_LIBRARY \
   UB_THIRDPARTY_INTERACTIVE


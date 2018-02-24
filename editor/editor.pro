include(../config)
TEMPLATE = lib
TARGET = kscope_editor
DEPENDPATH += ". ../core"
CONFIG += dll

# Input
HEADERS += viscintilla.h \
    actions.h \
    lexerstyledelegate.h \
    lexerstylemodel.h \
    editor.h \
    configdialog.h \
    fileiothread.h \
    findtextdialog.h \
    config.h
FORMS += configdialog.ui \
    findtextdialog.ui
SOURCES += viscintilla.cpp \
    actions.cpp \
    lexerstylemodel.cpp \
    config.cpp \
    editor.cpp \
    configdialog.cpp \
    findtextdialog.cpp
INCLUDEPATH += .. \
    $${QSCI_ROOT_PATH}/include/qt5/Qsci \
    .
LIBS += -L../core \
    -lkscope_core \
    -L$${QSCI_ROOT_PATH}/lib64 \
    -lqscintilla2_qt5
target.path = $${INSTALL_PATH}/lib64
INSTALLS += target
QT += widgets xml


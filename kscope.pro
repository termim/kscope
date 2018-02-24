include(config)

CONFIG += qscintilla2_qt5

TEMPLATE = subdirs

# Directories
SUBDIRS += core cscope editor app

message(Installation root path is $${INSTALL_PATH})

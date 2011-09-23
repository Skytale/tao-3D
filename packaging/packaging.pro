# ******************************************************************************
# packaging.pro                                                     Tao project
# ******************************************************************************
# File Description:
# Qt build file for the packaging directory
# ******************************************************************************
# This software is property of Taodyne SAS - Confidential
# Ce logiciel est la propriété de Taodyne SAS - Confidentiel
# (C) 2011 Jerome Forissier <jerome@taodyne.com>
# (C) 2011 Taodyne SAS
# ******************************************************************************

TEMPLATE = subdirs
macx:SUBDIRS = macosx
win32:SUBDIRS = win
linux-g++*:SUBDIRS = linux

kit.commands = (cd $$SUBDIRS; \$(MAKE) kit)
kit.depends = FORCE
QMAKE_EXTRA_TARGETS += kit

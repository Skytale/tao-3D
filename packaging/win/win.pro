# ******************************************************************************
# macosx.pro                                                        Tao project
# ******************************************************************************
# File Description:
#
#   Qt build file to generate a Windows package
#
#
# ******************************************************************************
# This software is property of Taodyne SAS - Confidential
# Ce logiciel est la propriété de Taodyne SAS - Confidentiel
# (C) 2011 Jerome Forissier <jerome@taodyne.com>
# (C) 2011 Christophe de Dinechin <christophe@taodyne.com>
# (C) 2011 Taodyne SAS
# ******************************************************************************

TEMPLATE = subdirs

kit.commands   = $(MAKE) -f Makefile.win
prepare.commands   = $(MAKE) -f Makefile.win prepare
clean.commands = $(MAKE) -f Makefile.win clean
distclean.depends = clean

QMAKE_EXTRA_TARGETS = kit prepare clean distclean

include (../../main_defs.pri)
QMAKE_SUBSTITUTES = Makefile.config.in

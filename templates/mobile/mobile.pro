# ******************************************************************************
#  mobile.pro                                                      Tao project
# ******************************************************************************
# File Description:
# Project file for document template: mobile

TEMPLATE = subdirs

include(../templates.pri)

files.path  = $$APPINST/templates/mobile
files.files = template.ini mobile.xl mobile.ddd mobile.jpg

images.path = $$APPINST/templates/mobile/images
images.files = images/*

INSTALLS += files images

SIGN_XL_SOURCES = mobile.xl mobile.ddd
include(../sign_template.pri)

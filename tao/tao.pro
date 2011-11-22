# ******************************************************************************
# tao.pro                                                            Tao project
# ******************************************************************************
# File Description:
# Main Qt build file for Tao
# ******************************************************************************
# This software is property of Taodyne SAS - Confidential
# Ce logiciel est la propriété de Taodyne SAS - Confidentiel
# (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
# (C) 2010 Catherine Burvelle <cathy@taodyne.com>
# (C) 2010 Lionel Schaffhauser <lionel@taodyne.com>
# (C) 2010 Jerome Forissier <jerome@taodyne.com>
# (C) 2010 Taodyne SAS
# ******************************************************************************

include(../main.pri)
include(../version.pri) # required to process Info.plist.in, tao.rc.in

TEMPLATE = app
!macx:TARGET =  Tao
 macx:TARGET = "Tao Presentations"
INC = . \
    include \
    include/tao \
    xlr/xlr/include \
    ../libcryptopp \
    ../keygen
DEPENDPATH += $$INC
INCLUDEPATH += $$INC
LIBS += -L../libxlr/\$(DESTDIR) -lxlr -L../libcryptopp/\$(DESTDIR) -lcryptopp
QT += webkit \
    network \
    opengl \
    svg

macx {
    CFBUNDLEEXECUTABLE=$$TARGET
    XLRDIR = Contents/MacOS
    ICON = tao.icns
    FILETYPES.files = tao-doc.icns
    FILETYPES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += FILETYPES
    QMAKE_SUBSTITUTES += Info.plist.in
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_DISTCLEAN += Info.plist
    QMAKE_CFLAGS += -mmacosx-version-min=10.5 # Avoid warning with font_file_manager_macos.mm
}
win32 {
    QMAKE_SUBSTITUTES += tao.rc.in
    RC_FILE  = tao.rc
    LIBS += -limagehlp
}
linux-g++* {
    LIBS += -lXss
}

# Input
HEADERS += widget.h \
    include/tao/tao_gl.h \
    window.h \
    application.h \
    init_cleanup.h \
    licence.h \
    frame.h \
    svg.h \
    texture.h \
    include/tao/coords.h \
    include/tao/coords3d.h \
    color.h \
    gl_keepers.h \
    drawing.h \
    shapes.h \
    text_drawing.h \
    shapes3d.h \
    path3d.h \
    table.h \
    chooser.h \
    binpack.h \
    glyph_cache.h \
    attributes.h \
    lighting.h \
    transforms.h \
    layout.h \
    page_layout.h \
    space_layout.h \
    justification.h \
    justification.hpp \
    apply_changes.h \
    normalize.h \
    activity.h \
    selection.h \
    manipulator.h \
    menuinfo.h \
    widget_surface.h \
    process.h \
    repository.h \
    git_backend.h \
    tao_utf8.h \
    tao_tree.h \
    font.h \
    drag.h \
    error_message_dialog.h \
    group_layout.h \
    resource_mgt.h \
    tree_cloning.h \
    font_file_manager.h \
    splash_screen.h \
    documentation.h \
    new_document_wizard.h \
    preferences_dialog.h \
    preferences_pages.h \
    module_manager.h \
    text_edit.h \
    tao_main.h \
    tool_window.h \
    include/tao/module_api.h \
    include/tao/module_info.h \
    module_renderer.h \
    layout_cache.h \
    render_to_file_dialog.h \
    inspectordialog.h \
    raster_text.h \
    dir.h \
    templates.h \
    module_info_dialog.h \
    display_driver.h \
    include/tao/matrix.h \
    statistics.h \
    gc_thread.h \
    info_trash_can.h \
    destination_folder_dialog.h \
    include/tao/tao_info.h

SOURCES += tao_main.cpp \
    widget.cpp \
    window.cpp \
    frame.cpp \
    svg.cpp \
    widget_surface.cpp \
    texture.cpp \
    drawing.cpp \
    shapes.cpp \
    text_drawing.cpp \
    shapes3d.cpp \
    path3d.cpp \
    table.cpp \
    chooser.cpp \
    binpack.cpp \
    glyph_cache.cpp \
    attributes.cpp \
    lighting.cpp \
    transforms.cpp \
    layout.cpp \
    page_layout.cpp \
    space_layout.cpp \
    apply_changes.cpp \
    normalize.cpp \
    activity.cpp \
    selection.cpp \
    manipulator.cpp \
    gl_keepers.cpp \
    menuinfo.cpp \
    process.cpp \
    repository.cpp \
    git_backend.cpp \
    application.cpp \
    init_cleanup.cpp \
    font.cpp \
    drag.cpp \
    error_message_dialog.cpp \
    group_layout.cpp \
    resource_mgt.cpp \
    tree_cloning.cpp \
    font_file_manager.cpp \
    splash_screen.cpp \
    documentation.cpp \
    new_document_wizard.cpp \
    preferences_dialog.cpp \
    preferences_pages.cpp \
    module_manager.cpp \
    text_edit.cpp \
    tool_window.cpp \
    module_api_p.cpp \
    module_renderer.cpp \
    layout_cache.cpp \
    render_to_file_dialog.cpp \
    inspectordialog.cpp \
    raster_text.cpp \
    dir.cpp \
    templates.cpp \
    module_info_dialog.cpp \
    display_driver.cpp \
    statistics.cpp \
    gc_thread.cpp \
    info_trash_can.cpp \
    destination_folder_dialog.cpp

win32 {
    HEADERS += dde_widget.h
    SOURCES += dde_widget.cpp
}

# Check compile-time options

contains(DEFINES, CFG_NOGIT) {
    !build_pass:message("Document history and sharing with Git is disabled")
} else {
    HEADERS += \
        ansi_textedit.h \
        branch_selection_combobox.h \
        branch_selection_tool.h \
        checkout_dialog.h \
        clone_dialog.h \
        commit_selection_combobox.h \
        commit_table_model.h \
        commit_table_widget.h \
        diff_dialog.h \
        diff_highlighter.h \
        fetch_dialog.h \
        fetch_push_dialog_base.h \
        git_toolbar.h \
        history_dialog.h \
        history_frame.h \
        history_playback.h \
        history_playback_tool.h \
        merge_dialog.h \
        open_uri_dialog.h \
        pull_from_dialog.h \
        push_dialog.h \
        remote_selection_frame.h \
        selective_undo_dialog.h \
        undo.h \
        uri.h
    SOURCES += \
        ansi_textedit.cpp \
        branch_selection_combobox.cpp \
        branch_selection_tool.cpp \
        checkout_dialog.cpp \
        clone_dialog.cpp \
        commit_selection_combobox.cpp \
        commit_table_model.cpp \
        commit_table_widget.cpp \
        diff_dialog.cpp \
        diff_highlighter.cpp \
        fetch_dialog.cpp \
        fetch_push_dialog_base.cpp \
        git_toolbar.cpp \
        history_dialog.cpp \
        history_frame.cpp \
        history_playback.cpp \
        history_playback_tool.cpp \
        merge_dialog.cpp \
        open_uri_dialog.cpp \
        pull_from_dialog.cpp \
        push_dialog.cpp \
        remote_selection_frame.cpp \
        selective_undo_dialog.cpp \
        undo.cpp \
        uri.cpp
    FORMS += \
        pull_from_dialog.ui \
        remote_selection_frame.ui \
        clone_dialog.ui \
        merge_dialog.ui \
        history_dialog.ui \
        open_uri_dialog.ui \
        fetch_push_dialog.ui \
        history_frame.ui \
        diff_dialog.ui
}
contains(DEFINES, CFG_NOSTEREO) {
    !build_pass:message("Stereoscopic display support is disabled")
}
contains(DEFINES, CFG_NOSRCEDIT) {
    !build_pass:message("Document source editor is disabled")
} else {
    HEADERS += \
        xl_source_edit.h \
        xl_highlighter.h
    SOURCES += \
        xl_source_edit.cpp \
        xl_highlighter.cpp
}
contains(DEFINES, CFG_NORELOAD) {
    !build_pass:message("Automatic document reload is disabled")
}
contains(DEFINES, CFG_NOEDIT) {
    !build_pass:message("Editing functions are disabled (Edit, Insert, Format, Arrange, Share)")
}
CXXTBL_SOURCES += graphics.cpp \
    formulas.cpp
NOWARN_SOURCES += licence.cpp

!macx {
    HEADERS += include/tao/GL/glew.h \
        include/tao/GL/glxew.h \
        include/tao/GL/wglew.h
    SOURCES += include/tao/GL/glew.c
    DEFINES += GLEW_STATIC
}
macx {
    OBJECTIVE_SOURCES += font_file_manager_macos.mm
    !contains(DEFINES, CFG_NODISPLAYLINK):LIBS += -framework CoreVideo
    LIBS += -framework ApplicationServices -framework Foundation \
        -Wl,-macosx_version_min,10.5 \
        -Wl,-rpath,@executable_path/../Frameworks \
        -Wl,-rpath,$$QMAKE_LIBDIR_QT

    # Make sure libGLC references the Qt libraries bundled with the application
    # and not the ones that may be installed on the target system, otherwise
    # they may clash
    FIX_QT_REFS = ../modules/fix_qt_refs
    QMAKE_POST_LINK = $$FIX_QT_REFS "$(TARGET)" \"$$QMAKE_LIBDIR_QT\"
}
RESOURCES += tao.qrc

# Files loaded at runtime
SUPPORT_FILES = xlr/xlr/builtins.xl \
    tao_fr.xl \
    xl.syntax \
    C.syntax \
    xl.stylesheet \
    git.stylesheet \
    nocomment.stylesheet \
    debug.stylesheet \
    welcome.ddd

# Other files to show in the Qt Creator interface
OTHER_FILES +=  \
    licence.cpp \
    tao.xl.in \
    $${SUPPORT_FILES} \
    traces.tbl \
    graphics.tbl \
    attributes.tbl \
    shapes.tbl \
    shapes3d.tbl \
    manipulator.tbl \
    transforms.tbl \
    text_drawing.tbl \
    table.tbl \
    widget_surface.tbl \
    chooser.tbl \
    frame.tbl \
    lighting.tbl \
    Info.plist.in \
    html/module_info_dialog.html \
    html/module_info_dialog_fr.html \
    tao_fr.ts

FORMS += error_message_dialog.ui \
    render_to_file_dialog.ui \
    inspectordialog.ui

# Automatic embedding of Git version
QMAKE_CLEAN += version.h
PRE_TARGETDEPS += version.h
revtarget.target = version.h
revtarget.commands = ./updaterev.sh "$${TAO_EDITION}"
revtarget.depends = $$SOURCES \
    $$HEADERS \
    $$FORMS
QMAKE_EXTRA_TARGETS += revtarget

# Automatic embedding of changelog file (NEWS)
system(cp ../NEWS ./NEWS)
QMAKE_CLEAN += NEWS
changelog.target = NEWS
changelog.commands = cp ../NEWS .
changelog.depends = ../NEWS
QMAKE_EXTRA_TARGETS += changelog

# Pre-processing of tao.xl.in to obtain tao.xl
# preprocessor.pl comes from http://software.hixie.ch/utilities/unix/preprocessor/
!system(perl -e "exit"):error("Can't execute perl")
DEFS = $$join(DEFINES, " -D", " -D")
tao_xl.target = tao.xl
tao_xl.commands = perl preprocessor.pl $$DEFS tao.xl.in > tao.xl && cp tao.xl \"$$APPINST\"
tao_xl.files = tao.xl
tao_xl.path = $$APPINST
tao_xl.depends = tao.xl.in
INSTALLS += tao_xl
QMAKE_EXTRA_TARGETS += tao_xl
QMAKE_CLEAN += tao.xl

# What to install
xl_files.path  = $$APPINST
xl_files.files = $${SUPPORT_FILES}
CONFIG(debug, debug|release):xl_files.files += xlr/xlr/debug.stylesheet
fonts.path  = $$APPINST/fonts
fonts.files = fonts/*
INSTALLS    += xl_files fonts
macx {
  # Workaround install problem: on Mac, the standard way of installing (the 'else'
  # part of this block) starts by recursively deleting $$target.path/Tao.app.
  # This is bad since we have previously stored libraries there :(
  app.path    = $$INSTROOT
  app.extra   = \$(INSTALL_DIR) \"$${TARGET}.app\" $$INSTROOT
  INSTALLS   += app
} else {
  target.path = $$INSTROOT
  INSTALLS   += target
}

# Create license directory
licdir.commands = mkdir -p \"$${APPINST}/licenses\"
licdir.path = .
licdir.depends = FORCE
INSTALLS += licdir

TRANSLATIONS = tao_fr.ts
include(../translations.pri)
translations.path = $$APPINST
translations.files = *.qm
INSTALLS += translations

shaders.path = $$APPINST$
shaders.files = lighting.vs lighting.fs
INSTALLS += shaders

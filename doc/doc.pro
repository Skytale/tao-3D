# ******************************************************************************
#  doc.pro                                                          Tao project
# ******************************************************************************
# File Description:
# Qt build file for tao documentation
# ******************************************************************************
# This document is released under the GNU General Public License.
# See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
# (C) 2011 Catherine Burvelle <catherine@taodyne.com>
# (C) 2011 Taodyne SAS
# ******************************************************************************

include (../main.pri)

TEMPLATE = subdirs

DOCDIR = $$INSTROOT/doc
TAODIR = $$DOCDIR/tao_doc
HTMLDIR = $$DOCDIR/html

doc_xl.commands = (cd $$APPINST; echo \"generate_all_doc \\\"$$TAODIR/all_documentation.xl\\\"; exit 0 \" > genDoc.ddd; DYLD_LIBRARY_PATH=../Frameworks ./Tao -norepo -nosplash genDoc.ddd; /bin/rm genDoc.ddd)
doc_xl.depends = $$APPINST/Tao create_dirs

doc_tao.commands = awk -v TAODOC=$$TAODIR/tao_doc.ddd -f tools/listFamilies.awk $$TAODIR/all_documentation.xl
doc_tao.depends = doc_xl

doc_html.commands = doxygen-1.7.1/bin/doxygen tools/tao_doc.cfg
doc_html.depends = doc_tao doxygen-1.7.1/bin/doxygen tools/tao_doc.cfg

create_dirs.depends = c_files_dir tao_doc_dir
c_files_dir.commands = mkdir -p c_files
tao_doc_dir.commands = mkdir -p $$TAODIR

doxy.target = doxygen-1.7.1/bin/doxygen
doxy.commands = (cd doxygen-1.7.1 && sh ./configure && make)

doc.depends = doc_html

QMAKE_EXTRA_TARGETS += doc_xl doc_tao doc_html doc doxy create_dirs c_files_dir tao_doc_dir clean

tao_theme.path = $$TAODIR
tao_theme.files = tools/theme.xl
tao_theme.depends = doc_tao

tao_html.path = $$DOCDIR
tao_html.files = tao_doc/html src
tao_html.depends = doc_html

INSTALLS += tao_theme tao_html

QMAKE_DISTCLEAN +=  $$TAODIR/all_documentation.xl  $$TAODIR/tao_doc.ddd

clean.commands = /bin/rm -r c_files/ tao_doc/; (cd doxygen-1.7.1 && make clean)

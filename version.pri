# Version: 3 integers >= 0
# MacOSX: the CFBundleVersion and CFBundleShortVersionString keys in Info.plist
# take the value of $$VERSION as is
# Windows: FILEVERSION, PRODUCTVERSION, and FileVersion are built using
# $$MAJOR, $$MINOR and $$RELEASE
VERSION = "1.4.2"
!build_pass:message(Version is $$VERSION)
MAJOR = $$replace(VERSION, "\\.[0-9]+\\.[0-9]+\$", "")
MINOR = $$replace(VERSION, "^[0-9]+\\.", "")
MINOR = $$replace(MINOR, "\\.[0-9]+\$", "")
RELEASE = $$replace(VERSION, "^[0-9]+\\.[0-9]+\\.", "")

GITREV = $$system(bash -c \"$$PWD/tao/updaterev.sh -n\")

#!/bin/bash
#
# Convenience script to create the official build of Tao Presentations. 
# When done, packages are copied into .. (parent directory).
#
# Usage: ./build_all.sh
#
# This software is property of Taodyne SAS - Confidential
# Ce logiciel est la propriété de Taodyne SAS - Confidentiel
# (C) 2012 Jerome Forissier <jerome@taodyne.com>
# (C) 2012 Taodyne SAS

doo() {
  echo "$@" >&4
  eval "$@"
}

cp_kit() {
  case `uname` in
    Darwin)
      doo cp "packaging/macosx/*.dmg" ..
      ;;
    MINGW*)
      doo cp "packaging/win/*.exe" ..
      ;;
    Linux)
      doo cp packaging/linux/*.deb packaging/linux/*.tar.bz2 ..
      ;;
  esac
}

die() {
  echo $1 >&2
  exit 1
}

set_mflags() {
  case `uname` in
    MINGW*)
      CCACHE=$(which ccache)
      [ "$CCACHE" ] && MAKEFLAGS="CXX=\"ccache g++\" CC=\"ccache gcc\""
      ;;
  esac

}

git diff-index --quiet HEAD -- || die "Error: Git work area is dirty."

set_mflags

LOG=$PWD/taobuild-$(date +%H:%M:%S-%Y.%m.%d).log
echo Log file: $LOG >&2

exec 4>&2
{ time {
  doo make distclean
  doo ./configure --with-eula player && doo "make -j3 kit $MAKEFLAGS" && cp_kit
  [ $? -ne 0 ] && exit $?
  doo make distclean
  doo ./configure --with-eula --with-cfu && doo "make -j3 kit $MAKEFLAGS" && cp_kit
} >$LOG 2>&1; }
exec 4>&-

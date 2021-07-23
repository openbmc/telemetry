#!/bin/bash

PARAMNAME=b_lto
PARAMVAL=false
FLAGTOMOD="-D${PARAMNAME}=$PARAMVAL"

function _apply_mod()
{
  local BUILDDIR=$1
  local RETCODE=1

  if [ -n "$BUILDDIR" ]; then
    if [ -d $BUILDDIR ]; then
      meson configure $FLAGTOMOD $BUILDDIR
      if [ $? -eq 0 ]; then
        meson --reconfigure $BUILDDIR
        RETCODE=$?
      fi
    else
      meson $FLAGTOMOD $BUILDDIR
      RETCODE=$?
    fi
  fi
  return $RETCODE
}

function _help()
{
  local SCRIPTDIR=scripts
  local SCRIPT="$SCRIPTDIR/$(basename $0)"

  echo "##################### SCRIPT USAGE #####################"
  echo "Please call this script from telemetry main directory"
  echo "with parameter specifying name of your build directory"
  echo "example:"
  echo "./$SCRIPT BUILDDIRNAME"
  echo "./$SCRIPT build"
  echo "########################################################"
}
trap "_help" ERR

### main ###

_apply_mod $@
exit $?

### end ###


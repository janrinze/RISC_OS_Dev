#!/bin/bash
# Environment to build user disc image (also useful for any other soft-loadable binaries)
#
# This can either be applied to the current shell using the "source" command,
# or executed directly. In the latter case, you will want to describe a
# a command to be executed by a subshell using the parameters to this script.
# In either case, the working directory must be set to the root of your sandbox on entry.

export LOCALE=UK
export KEYBOARD=All
export MACHINE=All
export SYSTEM=Ursula
export USERIF=Iyonix
export DISPLAYTYPE=PAL
export IMAGESIZE=4096K
export HALSIZE=64K
export BUILD=ROOL/Disc
export APCS=APCS-32

. Env/!Common.sh

[ -z "$@" ] || bash -c "$@"

#!/usr/bin/env bash
# apps/tools/mkkconfig.sh
#
#   Copyright (C) 2015 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# Get the input parameter list

USAGE="USAGE: mkkconfig.sh [-d] [-h] [-m <menu>] [-o <kconfig-file>]"
KCONFIG=Kconfig
unset MENU

while [ ! -z "$1" ]; do
  case $1 in
    -d )
      set -x
      ;;
    -m )
      shift
      MENU=$1
      ;;
    -o )
      shift
      KCONFIG=$1
      ;;
    -h )
      echo $USAGE
      exit 0
      ;;
    * )
      echo "ERROR: Unrecognized argument: $1"
      echo $USAGE
      exit 1
      ;;
    esac
  shift
done


if [ -f ${KCONFIG} ]; then
  rm ${KCONFIG} || { echo "ERROR: Failed to remove $PWD/${KCONFIG}"; exit 1; }
fi

echo mkkconfig in $PWD

KCONFIG_LIST=`ls -1 $PWD/*/Kconfig 2>/dev/null`

echo "#" > ${KCONFIG}
echo "# For a description of the syntax of this configuration file," >> ${KCONFIG}
echo "# see the file kconfig-language.txt in the NuttX tools repository." >> ${KCONFIG}
echo "#" >> ${KCONFIG}
echo "# This file is autogenerated, do not edit." >> ${KCONFIG}
echo "#" >> ${KCONFIG}
echo "" >> ${KCONFIG}

if [ ! -z "${MENU}" ]; then
  echo "menu \"${MENU}\"" >> ${KCONFIG}
fi

for FILE in ${KCONFIG_LIST}; do
  echo "source \"${FILE}\"" >> ${KCONFIG}
done

if [ ! -z "${MENU}" ]; then
  echo "endmenu # ${MENU}" >> ${KCONFIG}
fi


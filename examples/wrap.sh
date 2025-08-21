#!/bin/bash

myname=$(basename "$0")
if test "${myname}" = 'wrap.sh' -o "${myname}" = './wrap.sh' ; then
    echo DO NOT CALL THIS PROGRAM
    exit 1
fi

mydir=$(dirname "$0")

cd "${mydir}"

export LD_LIBRARY_PATH="${mydir}:${LD_LIBRARY_PATH}"

exec ./"${myname}"_elf "$@"

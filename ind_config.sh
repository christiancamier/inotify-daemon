#!/usr/bin/bash

function ind_execute() {
    "$@" && return 0
    echo "'$@' failed"
    exit 1
}

ind_execute wget -O config.sub   'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
ind_execute wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
ind_execute ./configure "$@"
exit 0


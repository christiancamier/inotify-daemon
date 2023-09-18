#!/usr/bin/bash

function ind_execute() {
    "$@" && return 0
    echo "'$@' failed"
    exit 1
}

echo "Retrieving config.guess"
ind_execute wget -q -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
echo "Retrieving config.sub"
ind_execute wget -q -O config.sub   'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
echo "Running ./configure $@"
ind_execute ./configure "$@"
exit 0


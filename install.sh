#!/bin/bash


function execute {
    "$@" && return 0
    exec 1>&2
    echo "Error while executing : $@"
    exit 1
}

instdir=/tmp
instgrp=bin
instmod=0640
instown=bin

while getopts "g:m:o:t:" option
do
    case "${option}" in
	g)
	    instgrp="${OPTARG}"
	    ;;
	m)
	    instmod="${OPTARG}"
	    ;;
	o)
	    instown="${OPTARG}"
	    ;;
	t)
	    instdir="${OPTARG}"
	    ;;
    esac
done

shift $((OPTIND - 1))

for file in "$@"
do
    execute cp -p "${file}"               "${instdir}/${file}"
    execute chown "${instown}:${instgrp}" "${instdir}/${file}"
    execute chmod "${instmod}"            "${instdir}/${file}"
done

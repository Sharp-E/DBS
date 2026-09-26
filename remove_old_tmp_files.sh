#!/bin/bash

if [ $# -lt 1 ]; then
    echo "usage: $0 <path>"
    exit 1
fi

echo "Removing temp files created more than one day ago from $1"
if [ -d "$1" ]; then
    find "$1" -maxdepth 1 -mindepth 1 -ctime 1 -exec rm -rf '{}' \;
fi


#!/bin/bash

set -e

if [ $# -ne 2 ]
then
    echo "Usage: $0 writefile writestr"
    exit 1
fi

writefile=$1
writestr=$2

mkdir -p "$(dirname "$writefile")"
echo "$writestr" > "$writefile"

if [ $? -ne 0 ]
then
    echo "The file $writefile could not be written"
    exit 1
fi
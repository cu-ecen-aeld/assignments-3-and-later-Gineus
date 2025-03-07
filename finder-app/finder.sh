#!/bin/sh

set -e

if [ $# -ne 2 ]
then
    echo "Usage: $0 filesdir searchstr"
    exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]
then
    echo "Directory $filesdir does not exist"
    exit 1
fi

echo "The number of files are $(find $filesdir -type f | wc -l) and the number of matching lines are $(grep -r $searchstr $filesdir | wc -l)"
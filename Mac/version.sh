#!/bin/sh

USAGE="Usage: $0 <a> <b> <c> <d>"

if [ $# -lt 4 ] ; then 
    echo "$USAGE"
    exit 1
fi

cd Src

VERS=`expr $1 \* 100 + $2 \* 10 + $3`

agvtool new-version -all $VERS.$4
agvtool vers

agvtool new-marketing-version $1.$2.$3.$4
agvtool mvers

#perl -pi -e "s/version=\".*\"/version=\"$VERS.$4\"/g" NowPlaying.pmdoc/01deployment.xml


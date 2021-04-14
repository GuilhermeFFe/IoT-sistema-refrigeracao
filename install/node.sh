#!/bin/bash

if [[ $UID != 0 ]]; then
    echo "Por favor, rode o script como super-usuário:"
    echo "sudo $0 $*"
    exit 1
fi

die()
{
    echo $1
    exit 1
}

VERSION='v15.14.0'
ARCH='x64'
INSTALL_DIR='/usr/local/lib/node'

TMP_LOCATION='/tmp/node.tar.xz'

curl "https://nodejs.org/dist/${VERSION}/node-${VERSION}-linux-${ARCH}.tar.xz" -o $TMP_LOCATION
mkdir -p $INSTALL_DIR
tar -xJvf $TMP_LOCATION -C $INSTALL_DIR > /dev/null

mv ${INSTALL_DIR}/node-${VERSION}-linux-${ARCH}/* $INSTALL_DIR
rm -r "${INSTALL_DIR}/node-${VERSION}-linux-${ARCH}"
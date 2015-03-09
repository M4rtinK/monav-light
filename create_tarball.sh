#!/bin/sh

VERSION=`grep ^Version: monav-light.spec | cut -d ':' -f 2 | xargs`

git archive --prefix=monav-light-${VERSION}/ HEAD -o monav-light-${VERSION}.tar.gz

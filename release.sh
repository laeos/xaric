#!/bin/sh

set -e
set -x
root=`pwd`
today=`date +%d-%b-%Y`

cleanup() {
    cd ${root}
    rm -rf build-dist/
}

trap cleanup EXIT

./autogen.sh

mkdir build-dist/

cd build-dist

../configure --enable-maintainer-mode --enable-dependency-tracking

V=`make version`

echo ""
echo "** RELEASE $V **"
echo ""

make dist

# just make sure it builds
mkdir build
cd build
tar xf ../xaric-${V}.tar.gz
cd xaric-${V}
./configure
make -j3

cd ${root}/build-dist/
md5sum xaric-${V}.tar.gz > xaric-${V}.tar.gz.md5

sed -e "s/VERSION/${V}/g" -e "s/RELEASEDATE/${today}/" ${root}/www/index.html > index.html

host=rfeany@tonya.fnordsoft.com
rr=~xaric
sw=${rr}/software/xaric/releases/
idx=${rr}/index.html

scp xaric-${V}.tar.gz xaric-${V}.tar.gz.md5 ${host}:${sw}
ssh ${host} rm ${sw}/LATEST_IS_*
ssh ${host} touch ${sw}/LATEST_IS_${V}
scp index.html ${host}:${idx}

echo ""
echo "Make sure you tag ... "
echo ""
echo "git tag ${V}"
echo "git push --tags"

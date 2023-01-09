#!/bin/bash

TOPDIR=$(cd $(dirname $0)/..; pwd -P)
WRKDIR=$(cd $(dirname $0);    pwd -P)
BLDDIR=${WRKDIR}/builddir

VERSION=$(
    echo '${version_major}.${version_minor}.${version_revision}' > __version.source;
    python3 ${TOPDIR}/util/source2any.py -M ${TOPDIR}/version_informations __version.source;
    cat __version;
    rm -f __version.source __version
       )

rm -fr ${BLDDIR}
rm -fr ${WRKDIR}/inotify-daemon-*.deb

mkdir -p ${BLDDIR}
mkdir -p ${BLDDIR}/DEBIAN

cd ${WRKDIR}
for pkgfile in control new-preinst old-prerm postinst
do
    python3 ${TOPDIR}/util/source2any.py -M ${TOPDIR}/version_informations ${pkgfile}.source
    mv ${pkgfile} ${BLDDIR}/DEBIAN/${pkgfile}
    chmod 555 ${BLDDIR}/DEBIAN/${pkgfile}
done
chmod 444 ${BLDDIR}/DEBIAN/control
(cd ${TOPDIR}/dist && tar cf - .) | (cd ${BLDDIR} && tar xpf -)

dpkg-deb --build builddir inotify-daemon-${VERSION}.deb

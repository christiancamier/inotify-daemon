#!/bin/bash

TOPDIR=$(cd $(dirname $0)/..; pwd -P)
WRKDIR=$(cd $(dirname $0);    pwd -P)
BLDDIR=${WRKDIR}/builddir

exec 4>&1 1>__init_vars.source
echo 'VERSION=${version_major}.${version_minor}.${version_revision};'
echo 'PREFIX=${prefix};'
echo 'SYSCONFDIR=${sysconfdir};'
echo 'DOCDIR=${docdir};'
exec 1>&4 4>&-
python3 ${TOPDIR}/util/source2any.py -M ${TOPDIR}/version_informations -M ${TOPDIR}/Makefile.inc __init_vars.source
. __init_vars

rm -f __init_vars __init_vars.source

rm -fr ${BLDDIR}
rm -fr ${WRKDIR}/inotify-daemon-*.deb

mkdir -p ${BLDDIR}
mkdir -p ${BLDDIR}/DEBIAN

cd ${WRKDIR}
for pkgfile in control new-preinst old-prerm postinst
do
    python3 ${TOPDIR}/util/source2any.py -M ${TOPDIR}/version_informations -M ${TOPDIR}/Makefile.inc ${pkgfile}.source
    mv ${pkgfile} ${BLDDIR}/DEBIAN/${pkgfile}
    chmod 755 ${BLDDIR}/DEBIAN/${pkgfile}
done
chmod 644 ${BLDDIR}/DEBIAN/control
(cd ${TOPDIR}/dist && tar cf - .) | (cd ${BLDDIR} && tar xpf -)

dpkg-deb --build builddir inotify-daemon-${VERSION}.deb

#!/bin/bash


set -x
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

cat <<EOF > __macros.source
#!START
clean_prefix = ${PREFIX:-/}
#!END
EOF

python3 ${TOPDIR}/util/source2any.py -M ${TOPDIR}/version_informations -M ${TOPDIR}/Makefile.inc __macros.source
python3 ${TOPDIR}/util/source2any.py -M ${TOPDIR}/version_informations -M ${TOPDIR}/Makefile.inc -M ${WRKDIR}/__macros inotify-daemon.spec.source

rm -f __init_vars __init_vars.source __macros __macros.source

#rpmbuild -bb --buildroot ${WRKDIR}/builddir -v --clean tis-sysvolsync.spec
rpmbuild -bb --buildroot ${WRKDIR}/builddir -v inotify-daemon.spec
cp RPMS/*/*.rpm .

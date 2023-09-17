#!/bin/bash

#
# Copyright: Christian CAMIER & Quentin PERIDON 2022
#
# christian.c@promethee.services
#
# This software is a computer program whose purpose is to manage filesystems
# events on defined directories.
#
# This software is governed by the CeCILL-B license under French law and
# abiding by the rules of distribution of free software.  You can  use, 
# modify and/ or redistribute the software under the terms of the CeCILL-B
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info". 
# 
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability. 
#
# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or 
# data to be ensured and,  more generally, to use and operate it in the 
# same conditions as regards security. 
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL-B license and that you accept its terms.
#

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

# Inotify Daemon

## Description
Inotify-daemon is a daemon that allows, on given directories, to
initiate actions when some file system events occur. Inotify-daemon is
a service that allows, on given directories, to initiate an action
when certain events of the file system occur.

## License
The rights to use this software are governed by the CeCILL-B license.
For more information see the file "Licence_CeCILL-B_V1-en.txt".

## Compatibility
Inotify-daemon runs only on Linux system having the inotify functions.

## Dependances
### Runtime dependances
Inotify-daemon has no spe!cific dependances.

### Compilation dependances
To compile inotify-daemon, the following packages are required:
- bash
- git
- python3

However, if you want to build the documentation in "info" and "pdf" formats,
you need to install the "texinfo" and "texlive-full" packages.

### Compilation
Use ./ind_config.sh [options] instead ./configure [options]


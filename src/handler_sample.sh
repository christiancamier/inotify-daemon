#!/bin/sh
# Copyright (c) 2022
# 	Christian CAMIER <christian.c@promethee.services>
# 
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

echo "STDOUT" 2>&1
echo "STDERR" 1>&2

echo "My ID: $(id)"

echo "(stdout): handler_sample[$$]: $@" 2>&1
echo "(stderr): handler_sample[$$]: $@" 1>&2

argn=0
echo "args (total $#):"
for argc in "$@"
do
    argn=$(( $argn + 1 ))
    printf "%2d - [%s]\n" $argn "$argc"
done
echo "Handler $$ exiting"

exit 0

#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Run this to set up the build system: configure, makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"

exec autoreconf --force --install --make --verbose --warnings=all "$@"

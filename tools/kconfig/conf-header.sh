#!/bin/sh -e

# Turn .config into a header file

if [ -z "$1" ] ; then
	echo "Usage: conf-header.sh <.config>"
	exit 1
fi

exec \
sed \
	-e '/^#$/d' \
	-e '/^[^#]/s:^\([^=]*\)=\(.*\):#define \1 \2:' \
	-e '/^#define /s: y$: 1:' \
	-e '/^# .* is not set$/s:^# \(.*\) is not set$:#undef \1:' \
	-e 's:^# \(.*\)$:/* \1 */:' \
        $1


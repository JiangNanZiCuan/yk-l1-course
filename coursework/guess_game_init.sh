#!/bin/sh
# The module is installed by the same kernel build as the running firmware.
if [ ! -r /proc/guess_game ]; then
    /sbin/modprobe guess_game || exit 1
fi
/usr/bin/guess_client --auto

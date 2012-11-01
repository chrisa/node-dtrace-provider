#!/bin/sh

set -o errexit

# GYP's MAKEFLAGS confuses libusdt's Makefile
#
unset MAKEFLAGS

# Ask node what arch it's been built for, and build libusdt to match.
#
# (this will need to change at the point that GYP is able to build
# node extensions universal on the Mac - for now we'll go with x86_64
# on a 64 bit Mac)
#
# libusdt/Makefile wants ARCH to be either 'x86_64' or 'i386'.
NODE_ARCH=$(file $npm_config_prefix/bin/node | awk '{print $3}')
if [[ "$NODE_ARCH" == "32-bit" ]]; then
    ARCH=i386
elif [[ "$NODE_ARCH" == "64-bit" ]]; then
    ARCH=x86_64
else
    echo "ERROR: unknown arch for $npm_config_prefix/bin/node: '$NODE_ARCH'"
fi
echo "Building libusdt for ${ARCH}"
export ARCH

# Respect a MAKE variable if set
if [ -z $MAKE ]; then
  MAKE=make
fi

# Build.
#
$MAKE -C libusdt clean all

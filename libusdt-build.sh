#!/bin/sh

# GYP's MAKEFLAGS confuses libusdt's Makefile
#
unset MAKEFLAGS

# Attempt to use the appropriate node binary - respect npm if that's
# how we're being built, otherwise use node from PATH
if [ -z $npm_config_prefix ]; then
  NODE=`which node`
else
  NODE="$npm_config_prefix/bin/node"
fi
echo "Using node: $NODE"

# Ask node what arch it's been built for, and build libusdt to match.
#
# (this will need to change at the point that GYP is able to build
# node extensions universal on the Mac - for now we'll go with x86_64
# on a 64 bit Mac, because that's the default architecture in that
# situation).
#
ARCH=`$NODE libusdt-arch.js`
echo "Building libusdt for ${ARCH}"
export ARCH

# Respect a MAKE variable if set
if [ -z $MAKE ]; then
  MAKE=make
fi

# Build.
#
$MAKE -C libusdt clean all

#!/bin/sh

if [ "$BOOST_ROOT" = "" ]; then
  echo "Please set environment variable BOOST_ROOT to the location of Boost."
  exit 1
fi

if [ ! -f "$BOOST_ROOT/b2" ]; then
  echo "Please compile Boost first."
  exit 1
fi

CD=$(cd $(dirname "$0"); pwd)
pushd "$BOOST_ROOT" > /dev/null
./b2 "$CD"
popd > /dev/null

#!/bin/sh

CD=$(cd $(dirname "$0"); pwd)
pushd "$CD" > /dev/null

which b2
if [ $? = 0 ]; then
  b2
else
  if [ "$BOOST_ROOT" = "" ]; then
    echo "Please set environment variable BOOST_ROOT to the location of Boost."
  elif [ ! -f "$BOOST_ROOT/b2" ]; then
    echo "Please compile Boost first."
  else
    "$BOOST_ROOT/b2"
  fi
fi
popd > /dev/null

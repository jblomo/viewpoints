#!/bin/bash
set -ex

docker run -it -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix viewpoints "$@"

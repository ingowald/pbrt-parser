#!/bin/bash

#call this script from within the build directory.
in=~/models/pbrt-v3-scenes/
out=~/pbf/

make

./pbrt2pbf $in/simple/buddha.pbrt -o $out/buddha.pbf || return 1
./pbrt2pbf $in/ganesha/ganesha.pbrt -o $out/ganesha.pbf || return 1
./pbrt2pbf $in/vw-van/vw-van.pbrt -o $out/vw-van.pbf || return 1
./pbrt2pbf $in/villa/villa-daylight.pbrt -o $out/villa.pbf || return 1
./pbrt2pbf $in/ecosys/ecosys.pbrt -o $out/ecosys.pbf || return 1
./pbrt2pbf $in/landscape/view-0.pbrt -o $out/landscape.pbf || return 1



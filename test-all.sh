#!/bin/bash
for f in `ls ~/models/pbrt-v3-scenes/*/*pbrt | grep -v materials | grep -v geometry | grep -v textures`; do
    ./pbrtlint $f
done

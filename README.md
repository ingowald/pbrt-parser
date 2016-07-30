PBRT-Parser
===========

This is just a toy-project of mine, in order to be able to
(eventually) parse the PBRT file format and convert PBRT scenes to
some of the other formats my other software (e.g., ospray) can also
read.

I'll be working on this on and off in my spare time; right now this is
little more than a few early steps to get a hang of the pbrt grammar.

![ecosys.pbrt](doc/jpg/ecosys.jpg "ecosys")
![landscape.pbrt](doc/jpg/landscape.jpg "landscape")

LICENSE
=======

This project comes with Apache 2.0 license - pretty much "use as you see fit".


STATUS
======

- Parser can now parse most pbrt files in the v1 and v3 models
- 'pbrt2rivl' tool can export models made up of trimeshes, plymeshes,
  and instances to RIVL format (no materials yet)
- pbrt2rivl tested with landscape, ecosys, fractal-buddha, sanmiguel,
  and some few others; all working.

TODO
====

- add material and texture export to pbrt2rivl
- add pbrt2osp 'to ospray' converter
- unify handling of external loaders (ply, textures); rather than
  having this code replicated in the pbrt2xyz exporters, move it into
  scene graph. somehow...!?



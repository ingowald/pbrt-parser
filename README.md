PBRT-Parser
===========

This is just a toy-project of mine, in order to be able to
(eventually) parse the PBRT file format and convert PBRT scenes to
some of the other formats my other software (e.g., ospray) can also
read.

I'll be working on this on and off in my spare time; right now this is
little more than a few early steps to get a hang of the pbrt grammar.


LICENSE
=======

This project comes with Apache 2.0 license - pretty much "use as you see fit".


UPDATES
=======

(newest ones on top)

- can now parse anim-bluespheres.pbrt
- can now parse complete san miguel scene (the new one)
- Can now parse entire GRAMMAR required for the san miguel scene.
  Note the parser isn't actually doing anything with the parsed items,
  yet - they are discarded right after each one is parsed - but at
  least the grammar seems to be correct.
- added namedmaterial, texture, materials.
- implemented 'Include' statement, fixed bug in lexer that would randomly return EOFs.

STATUS
======

The following scenes from the pbrt scenes are known to pass the
grammar parsing stage (we do not more than that for now, anyway)

- 

The following scenes are known to NOT work, yet

<currently none>




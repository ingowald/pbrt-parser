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

 - anim-bluespheres.pbrt
 - anim-killeroos-moving.pbrt
 - buddhamesh.pbrt
 - bunny.pbrt
 - caustic-proj.pbrt
 - dof-dragons.pbrt
 - killeroo-glossy-prt.pbrt
 - killeroo-simple.pbrt
 - sanmiguel.pbrt
 - teapot-subsurface.pbrt

FAILED

 - arcsphere.pbrt
 - ballpile.pbrt
 - buddha.pbrt
 - bump-sphere.pbrt
 - city-ao.pbrt
 - city-env.pbrt
 - cornell-mlt.pbrt
 - killeroo-gold.pbrt
 - metal-ssynth.pbrt
 - microcity.pbrt
 - miscquads.pbrt
 - plants-dusk.pbrt
 - plants-godrays.pbrt
 - probe-cornell.pbrt
 - room-igi.pbrt
 - room-mlt.pbrt
 - room-path.pbrt
 - room-photon-nogather.pbrt
 - room-photon.pbrt
 - sanmiguel_cam14.pbrt
 - sanmiguel_cam15.pbrt
 - sanmiguel_cam18.pbrt
 - sanmiguel_cam1.pbrt
 - sanmiguel_cam20.pbrt
 - sanmiguel_cam25.pbrt
 - sanmiguel_cam3.pbrt
 - sanmiguel_cam4.pbrt
 - sanmiguel_cam5.pbrt
 - sanmiguel_cam6.pbrt
 - sibenik-igi.pbrt
 - smoke-2.pbrt
 - sphere-ewa-vs-trilerp.pbrt
 - spheres-differentials-texfilt.pbrt
 - sponza-fog.pbrt
 - sponza-phomap.pbrt
 - spotfog.pbrt
 - teapot-area-light.pbrt
 - teapot-cornell-brdf.pbrt
 - teapot-metal.pbrt
 - tt.pbrt
 - villa-daylight.pbrt
 - villa-igi.pbrt
 - villa-lights-on.pbrt
 - villa-photons.pbrt
 - yeahright.pbrt

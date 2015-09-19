TODO
====

Compatibility
-------------

* Ensure that FBO format and window format match.

Simulation
----------

* Implement a network graph; splitting of the segments should happen on the
  server which then sends back the split curve to the clients.


Terrain and water
-----------------

* Pre-shade the terrain under the water to achieve more realistic
  lighting. Currently, we only take into account one-way attenuation of the
  light in the water (from the viewer to the terrain fragment).

  If we also shade the attenuation into the terrain fragments, we will get both
  light paths, from the light source to the terrain fragment *and* from the
  terrain fragment to the eye attenuated by the water.

  This will reduce the uncanny effect of having the attenuation largely depend
  on point of view.

* Re-think whether the fresnel effect is applied correctly -- I get the
  thinking it should not be mixed but instead be added to the refractive part
  and/or the refractive part needs different fresnel terms.

* Implement several water geometry rendering paths

  1. (GPU intensive) what we currently have, with geometry optimization in the
     Geometry Shader
  2. (CPU intensive) do what the GS does on the CPU. possibly stream the
     geometry, only cache low LOD parts.
  3. (boring) very simple rendering without fancy geometry optimization.
     basically what we had as first approach

* Implement several water shading paths

  The usual, lowering the level of effects.

Model loading and animations
----------------------------

* Evaluate different options:

  * `glTF <https://github.com/KhronosGroup/glTF>`_ draft spec
  * `assimp <http://assimp.sourceforge.net/>`_

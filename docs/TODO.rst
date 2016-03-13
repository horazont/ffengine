TODO
====

Reliability
-----------

* Ensure that FBO format and window format match.
* Find a solution for the Object-ID + Network Latency issue. What happens if a
  client refers to an Object-ID the server has deleted in the meantime (due to
  network latency)?

Simulation
----------

* Implement a network graph; splitting of the segments should happen on the
  server which then sends back the split curve to the clients.


Terrain and water
-----------------

* Implement several water geometry rendering paths

  1. (GPU intensive) what we currently have, with geometry optimization in the
     Geometry Shader
  2. (CPU intensive) do what the GS does on the CPU. possibly stream the
     geometry, only cache low LOD parts.
  3. (boring) very simple rendering without fancy geometry optimization.
     basically what we had as first approach

Model loading and animations
----------------------------

* Evaluate different options:

  * `glTF <https://github.com/KhronosGroup/glTF>`_ draft spec
  * `assimp <http://assimp.sourceforge.net/>`_

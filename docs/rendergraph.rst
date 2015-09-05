RenderGraph
###########

Scene
=====

A Scene is defined as a tuple of SceneGraph and Camera.


Render pass
===========

A Render Pass consists of:

* A target buffer to render into
* Zero or more blit source buffers, up to one for each attachment
* Zero or more dependent passes
* Render instructions like geometry sorting order


Material
========

A Material consists of:

* possibly shared VBO
* possibly shared IBO
* list of passes in which the geometry renders, where for each pass there is:

  * shader to use
  * VAO to use


Rendering
=========

To render a Scene, only the Scene needs to be known. A selection of objects to
render is created using the Scenegraph (which may use an octree). Each object
submits its render instructions, where each render instruction consists of a
bounding sphere or bounding box, a material, an IBO allocation and a VBO
allocation.

For creating the rendering instructions, the objects receive information about:

* The camera being used (and thus, view matrix)
* The target viewport size (and thus, together with the camera, the projection
  matrix)
* Transforms applied by parent nodes (thus, the model matrix, if any)
  (TODO: replace the model matrix by quaternion + translation in all shaders)

The render instructions are split into instructions for each pass and
upconverted to full render instructions consisting of

    (bounds, shader, vao, ibo_alloc, vbo_alloc).

The full render instructions are keyed by their pass. Now each pass is rendered
by handing the render instructions off to the pass. The ordering of the passes
is determined by their dependencies.

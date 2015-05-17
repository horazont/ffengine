Module overview
###############

Libraries
=========

External libraries
------------------

* ``surface_mesh``: see http://opensource.cit-ec.de/projects/surface_mesh

Internal libraries
------------------

* ``libffengine-core``: Math, IO and other commonly used parts.
* ``libffengine-sim``: Everything related to different parts of the simulation,
  which are not bound to the specific setting of scc. The idea is to keep this
  factorable for use in different freeform and terrain based projects.

  This library uses the ffsim namespace.

* ``libffengine-render``: Everything related to the use of OpenGL.

  This library uses the ffrender namespace.

* ``libscc``: Everything which relates closely to the actual game. This
  includes game-specific protobuffer descriptions and

  This library does not use any specific namespace, as it is equivalent to the
  core application.

Executables
===========

* ``game``: The game frontend. This contains the user interface and everything
  the normal player sees.
* ``tests``: `Catch <https://github.com/philsquared/Catch>`_ -based unit
  testing
* ``dedicated``: (*planned*) The dedicated server frontend. It contains a CLI
  frontend which allows managing the dedicated server.

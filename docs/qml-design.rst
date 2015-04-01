QML design
##########

QML provides us with some challenges: It has a rendering thread, which is
separate from the GUI thread which processes the events.


Synchronizing GUI <-> Rendering
===============================

Approach 1: Use a Big Geometry Lock
-----------------------------------

In this approach, we would have a huge lock which the rendering thread would
hold while it is busy with rendering. In turn, the GUI thread cannot make any
modifications to geometry state during that time.

This is the easiest to implement, but probably a very inefficient
solution.

We should keep anything which moves away from the rendering thread, so that it
can concentrate on rendering. View animations should be handled by the GUI
thread.

Approach 2: Render message queue
--------------------------------

This approach is more complex and requires much more thought. The idea is to
design a message queue which allows all relevant rendering-related commands to
be passed to the rendering thread which then executes them in order.

Approach 3: Clear separation of data modification and data use
--------------------------------------------------------------

In this approach, the automatic data flushing is removed from bind()
calls. Instead, a new sync() call is introduced. This call binds and updates
the contents of the object on the GPU. bind() alone does not use the local
buffer.

This can be used as a synchronization primitive by calling sync() only during
the rendering synchronization phase, with the GUI thread being blocked. bind()
is then called by the rendering thread whenever neccessary.


Synchronizing View <-> Simulation
=================================

This is the much larger issue, although it provides us with a possibly good
place to put an interface which will faciliate network communications.

Some of the data is private to the simulation, and there is no issue here. The
larger issue is with data which is viewable directly, such as statistics,
building locations and data which is viewable sometimes (debug info).

Approach 1: Message-passing un-shared state
-------------------------------------------

In this approach, the shared state gets duplicated and thus un-shared. Messages
are used:

* by the view, to issue commands into the simulation
* by the simulation, to inform the view about updated state

This requires a pretty sophisticated message passing model. We probably need
publish-subscribe and request-response models.

Examples for PubSub use cases:

* view enables debug info, for example transport graph. subscribes to transport
  graph changes, which it normally does not need

Actually, it seems plausible that subscription should be used for *all*
events.


The view sends requests to the simulation and receives responses. E.g. request
to zone a region.

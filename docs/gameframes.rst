Game frames
###########

General design
==============

We want to be network ready, to allow for an OpenTTD-style Co-Op mode. We might
not be able to get all details fast enough in the first iteration, but new
hardware will come.

This implies that all commands which modify the game state need to be
serializable. This must be *fast*. Also, the terrain editor does not need
networking; yet, as it requires the fluid simulation et. al., it makes sense to
re-use the same code here, so make it Network Ready, without it being actually
able to network.

Commmand Queues
---------------

The general idea is to follow the approach, which, I think, OpenTTD uses,
too. Clients send their commands to the server. The server queues the commands
for the next game frame. Then it applies all commands and broadcasts those
which succeeded to the clients and errors for those which failed to the
originator. The batch of commands is finalized with an END_FRAME packet.

The clients keep a buffer of 5 or 10 game frames which they do not process
yet. This allows to hide latency spikes. The clients detect the frame edges by
looking for END_FRAME packets. Every 16 ms or so they take a full frame and
process it, just like the server processed the input. If any command fails to
apply, this is a desync.

When no network game is taking place, the buffer only needs to hold the
commands for the next frame. Still, all commands need to be serializable and to
be queued for the frame. In fact, it would be sensible to still use the whole
server code and just reduce the queue length of the client to 0.

Server
------

A server consists of:

1. A way to receive requests from the client
2. The game state
3. A command processor which applies change requests to the game state
4.


Client
------

A client consists of:

1. An interface to accept commands from frontends
2. A connection to a server-like command processor
3. A local copy of the game state, with an command processor which applies
   changes to the game state.



1. Wait for game sim and fluid sim to finish
2. (client only) Notify that we are in a sane state for GL syncing and wait for
   client to sync
3. Process input
4. (server only) broadcast user input to other users and emit END_FRAME packet
5. Start game sim and fluid sim for next frame

(2) can be implemented by releasing a mutex when the sims finish and re-acquire
    it before continuing

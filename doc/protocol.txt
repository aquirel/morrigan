Morrigan protocol specification.
================================

  * Firt byte - packet id.
  * Packets have fixed sizes.

Processing stages.
------------------

  1. Find packet by id.
  2. Validate packet.
  3. Enqueue packet.
  4. Execute packet handler.
  5. Send response.

Client states.
--------------

  1. Connected (initial state).
  2. Acknowledged.
  3. In game.

Client state transitions.
-------------------------

  Connected -> Acknowledged: client have responded "Hello" packet.
  Acknowledged -> In game: client responded game notification packet.
  In game -> Acknowledged: client responded game notification packet.

Activity diagram.
-----------------

Client.       Server.
"Hello" ->
              Register new client. If client exists - ignore.
           <- "Hello"
"Hello" ->
              Acknowledge client.
           <- "Hello"

Game-related exchanges.
         <->

"Bye"   -> (Sent if client leaves first).
              Unregister client.
           <- "Bye" (Sent if server leaves first).
              Unregister client.

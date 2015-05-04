Morrigan protocol specification.


  * Firt byte - packet id.
  * Packets have fixed sizes.

Processing stages.

---


  1. Find packet by id.
  1. Validate packet.
  1. Enqueue packet.
  1. Execute packet handler.
  1. Send response.

Client states.

---


  1. Connected (initial state).
  1. Acknowledged.
  1. In game.

Client state transitions.

---


```
  Connected -> Acknowledged: client have responded "Hello" packet.
  Acknowledged -> In game: client responded game notification packet.
  In game -> Acknowledged: client responded game notification packet.
```

Activity diagram.

---


```
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
```
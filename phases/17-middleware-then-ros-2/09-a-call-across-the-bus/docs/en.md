# A Call Across the Bus: Requests, Responses, Timeouts and the Command That Ran Twice

> A request that gets no answer has not told you it failed. It has told you nothing.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-05, 17-08, 02-08

## The Problem

A topic is a stream: values flow, and no one answers. A robot also needs to ask: close the gripper, take a picture, tell me your parameters, reset the odometry. That is a **service**: a call from one node to another, with an answer. It looks like a function call, and it is not one, because the two sides are different programs joined by a link that loses things.

The caller sends a request and waits. If nothing comes back, there are three possibilities that look the same from where it is standing. The request never arrived. The request arrived and the work was done, and the answer never came back. Or the answer is on its way, late. Each needs a different action, and the caller cannot tell which it is in.

## The Concept

### A call has a number

The client numbers each call and the answer carries the number, so that an answer is matched to the call it answers and not to whichever is waiting. Two calls in flight together, answered in the other order:

```
call 1 got 'got open', call 2 got 'got close'
```

Each got its own answer. A client that took the next answer for the oldest call would have given the open call the answer to the close. See `E-BUS-0025`.

### A call that times out

A call that has no answer for a **timeout** is sent again, up to a number of **retries**, and when the last wait has run out it has timed out:

```
timeout 10, 2 retries: sent at 0, 10, 20, timed out at 30; attempts 3
```

It becomes `timed_out` and is not left pending for ever. A call that is pending for ever holds whatever is waiting on it, a thread or a state machine, for ever. See `E-BUS-0025`.

### The answer that comes too late

The answer to a call that already timed out may still arrive. The call has been given up on, and the caller has moved on, so the answer is **unwanted**: counted and thrown away, and the call stays timed out. The same goes for an answer for another client or for a call never made: 3 unwanted in the test, and the call still timed out. Letting a late answer revive a call would let a caller see a result it had already acted on as a failure. See `E-BUS-0026`.

### The command that ran twice

Retrying is only safe if asking again does not do it again. A server that is asked the same request twice answers the second time from memory:

| request | work done |
|---|---|
| (client 3, id 1) | yes |
| (client 3, id 1) again | no, the same answer |
| (client 3, id 2) | yes |
| (client 4, id 1) | yes |

The gripper was closed 3 times for 4 requests, and 1 repeat was answered from memory. The key is the client and the number together. See `E-BUS-0027`.

### What a lossy link does to it

300 calls over a link that loses 30 percent each way, a delay of 3 ticks, a timeout of 20:

| retries | server remembers | completed | timed out | times the work ran | requests sent |
|---|---|---|---|---|---|
| 0 | yes | 145 | 155 | 214 | 300 |
| 1 | yes | 218 | 82 | 282 | 455 |
| 3 | yes | 281 | 19 | 299 | 585 |
| 6 | yes | 298 | 2 | 300 | 621 |
| 6 | no | 298 | 2 | 446 | 621 |

With no retries 145 calls completed and **155 were reported timed out, while the work ran 214 times**: for 69 calls the client was told it had failed, and the gripper had closed. A timeout is not a failure, it is not knowing. With retries the completions rise to 298, and a server that remembers ran the work 300 times for 300 calls. The same retries against a server that forgot ran it 446
times: 146 times the work was done a second time, or a third. For a gripper that is harmless and for a command to dispense, to reset or to fire it is not. See `E-BUS-0027`.

## Build It

`handle` on the server and `call`, `tick` and `on_response` on the client, in `exercise/solution.hpp`. The status, answer and counters are given.

```
rcpp verify 17-09
```

The link in the test loses packets by a seeded generator, so the figures above are the same on every run.

## Use It

`rc/bus/service.hpp` joins `rc::bus`.

```cpp
ServiceClient client(my_id, 20, 3);
const int id = client.call("close");
for (const Request& r : client.tick(now)) send(r);
// when an answer arrives: client.on_response(answer);   later: client.status(id)
```

**What this does not do.** The server's memory of what it answered never forgets, and a real one bounds it, which means a very late retry can run twice after all. It treats the work as one step, and a service that takes seconds needs the goal, feedback and cancel of the next lesson. It identifies a client by a number it is given, and two clients with the same number would be taken for one. And a timeout of a fixed number of ticks is a guess:
lesson 17-05 said what a good one needs.

## What Breaks First

- **Matching an answer to whatever is waiting.** The wrong answer to the wrong call. See `E-BUS-0025`.
- **A late answer that revives a call.** A result acted on twice or contradicted. See `E-BUS-0026`.
- **A retry against a server that forgets.** A command carried out more than once. See `E-BUS-0027`.

## Ship It

`rc/bus/service.hpp` holds `Request`, `Response`, `ServiceServer`, `CallStatus` and `ServiceClient`.

A node can now ask another to do something and know what it does and does not know afterwards.

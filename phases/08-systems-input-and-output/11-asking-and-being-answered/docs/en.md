# Asking and Being Answered: Which Question Is This the Answer To

> A response is only an answer if it is to the question that was asked. Give every
> request an id, and make the device unable to do a repeat twice.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-10

## The Problem

So far the device talks and the robot listens. A robot also has to ask: where are
you, set this gain, move by this much. Asking is two messages, and the second is
only an answer if it is to the question that was asked.

The obvious implementation sends the request and takes the next response:

```cpp
send(request);
wait_for(response);      // the next one to arrive
```

On a link that never delays or loses anything that is correct. On one that does,
the next response to arrive can be the answer to a request already given up on.
Once it has been taken for the answer to the wrong request, the real answer arrives
next and is taken for the answer to the one after, and the error **shifts along**
the conversation until a timeout resynchronises it by accident.

There is a second problem hiding behind the first, and it needs a different fix.

## The Concept

### An id on every request

The request carries a number, the device copies it into its response, and the
requester accepts only the response with the number it is waiting for. A response
with any other number is **stale**: it is counted, not used, and the requester goes
on waiting.

```text
request   01  07  10  00 01          type, id, operation, arguments (+1, big endian)
response  81  07  00  00 00 00 C8    type, the same id, status, where it is now (200)
```

The first byte says what kind of message it is, because the same link carries
telemetry, and the code that polls it has to route a message without knowing the
protocol above. `handle` returns `true` for a response, which means it was dealt
with, and `false` for anything else, which stays the caller's.

### What it buys, measured

200 requests, each "move by one", to a simulated device that answers one in ten too
late, after the requester has already resent, and one in fifteen not at all. The
pattern is by count rather than by chance, so the numbers are the same every run.

| requester | answered | gave up | wrong | resent | position |
|---|---|---|---|---|---|
| takes the next response | 200 | 0 | 49 | 8 | 208 |
| matches the id | 200 | 0 | **0** | 27 | 227 |
| matches the id, device remembers | 200 | 0 | **0** | 27 | **200** |

The first row is the one to look at. It answered all 200 and **49 were wrong**, and
it resent only 8 times against 27, because a stale answer satisfied it early. It is
the requester that is most wrong and looks fastest. See `E-IO-0034`.

The second row has no wrong answers and a different problem.

### A resend is a repeated command

The position should be 200 and it is 227. The overshoot is **exactly the number of
resends**, 27.

A requester that does not hear back cannot tell whether its request was lost or only
the answer was. When it was the answer, the device carried the command out and has
now been told to do it again. "Move by one" twice moves two. Matching ids on the
requester's side cannot help, because the damage is done on the device.

So the device has a half of the promise to keep. It remembers the last id it answered
and what it said. When the same id arrives it **replays the answer** and does not
repeat the act:

```cpp
rc::span<const std::uint8_t> remembered;
if (cache.lookup(id, remembered)) {
  reply(remembered);
} else {
  execute();
  cache.remember(id, response);
}
```

That is the third row: the same 27 resends, and the device ends at exactly 200. See
`E-IO-0035`.

One slot is enough while the requester has one request in flight. More in flight
needs more slots, and an id the cache has forgotten gets executed again.

The better fix is in the protocol where it can be. A command that is safe to repeat,
"move to 200", needs no cache at all. A relative one, a toggle, a counter does.

### A request is a small state machine

`Idle`, `Waiting`, `Done`, `TimedOut`, and nothing blocks. `begin` sends, `handle`
takes a message, `tick` watches the clock. The caller owns the loop and can do other
things between them, and a control loop with a deadline can.

The longest a request can take is the **timeout times the attempts**, here 50 ms
times 3, 150 ms. That is the number a caller with a deadline checks, and a requester
with no limit has none to check.

A few details each undo something the rest relies on. See `E-IO-0036`.

- A retry keeps its **id**. A fresh one makes it a different command to the device.
- A send that **fails** does not start the request or use up an id.
- The id **wraps** past zero without ever being zero, because zero means "no
  request".
- One request at a time: `begin` while one is outstanding is refused.

## Build It

`ReplyCache` and `Requester` in `exercise/solution.hpp`. The constants, the states
and the members are given. The comments in each function list what it must do.

```
rcpp verify 08-11
```

The first ten tests are the pieces: the request on the wire, a matching response, a
stale one, a message that is not a response, retries and giving up, one at a time, a
failed send, a late answer to a request already abandoned, the wrap, and the cache.
The last is the table above, run through a simulated device.

## Use It

`rc/io/request.hpp` joins `rc::io`. A loop that has a `Link`:

```cpp
rc::io::Requester requester(link, 0.050, 3);
requester.begin(op, args, now);

// every tick
if (link.poll(now)) {
  if (!requester.handle(link.message())) telemetry(link.message());
}
requester.tick(now);
if (requester.state() == rc::io::RequestState::Done) use(requester.response());
```

The honest limit: an id is eight bits, so a response more than 255 requests late is
indistinguishable from the current one. At one request at a time and a timeout of 150
ms that is not reachable, and a requester with many in flight has to think about it.

## What Breaks First

- **Taking the next response.** The answers shift along the conversation. See
  `E-IO-0034`.
- **A command done twice.** The answer was lost, so the resend was a repeat. See
  `E-IO-0035`.
- **A retry with a new id, no attempt limit, a failed send counted as sent, or an id
  of zero.** See `E-IO-0036`.

## Ship It

`rc/io/request.hpp` holds `Requester`, `RequestState` and `ReplyCache`.

Everything so far has assumed that the time on the message means something. The next
lesson asks what happens when the device's clock and the robot's do not agree.

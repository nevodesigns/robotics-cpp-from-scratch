# Topics by Hand: Publish, Subscribe and the Order Things Happen In

> The part of a robot that makes a value and the part that wants it should not need to know each other's names. A topic is the name they both know instead.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-08, 03-03, 01-05, 03-02

## The Problem

By phase 16 the robot is a pile of parts: a sensor reader, a filter, a planner, a controller, a display. Each was written to be called by the next. That holds until one is replaced, or a second consumer wants the same value, or a log should hear everything. The planner would need to know about the logger, and
the sensor reader about the planner, and every new part changes every old one.

**Middleware** is the layer that removes that. The pieces say what they publish and what they want, by a name, and the layer moves the values. ROS 2 is a large middleware. Before using one, build the smallest honest version, to meet the questions every middleware has to answer.

## The Concept

### A topic and a handler

A **publisher** gives a topic name and a payload. A **subscriber** gives a topic name and a function to call. The bus keeps a list per topic:

```cpp
Token subscribe(const std::string& topic, Handler handler);
int publish(const std::string& topic, const std::string& payload);   // how many were called
```

Subscribers hear a message in the order they subscribed, and only those subscribed to that topic. Published to `scan` with two subscribers, and a third on `pose`, two were called and the log was `a:1 b:1`. See `E-BUS-0001`.

### Nobody listening

A message to a topic with no subscribers is dropped and counted: it is not an error, since a publisher cannot know who is listening and should not have to. Two messages with nobody listening gave a dropped count of 2, and a subscriber that arrived afterwards heard 1 of 3 messages: what was published before it came is gone. Whether a late
subscriber should be able to get it is a decision of its own, a later lesson's. See `E-BUS-0001`.

### Leaving in the middle

A handler may stop listening, its own or another's, while a message is being delivered. The list changes under the loop that is walking it. The bus copies the tokens when the message goes out, and before calling each one checks it is still subscribed:

| message | called | log |
|---|---|---|
| first, where a removes itself and b | a, c (2) | a c |
| second | c (1) | c |

b was removed before its turn and was not called, and a did not hear the second. Without the copy, erasing from the list being walked skips a subscriber or reads past the end. See `E-BUS-0002`.

### A handler that publishes

The usual thing a handler does is publish: a filter hears a scan and publishes a cleaned one. If that publish delivered at once, the stack would deepen with each hop, and the order would be strange: the second subscriber of the first message would hear it after everything the first one caused.

Instead a message published from inside a handler waits its turn in a queue, and the call returns 0:

```
first heard 7
publish inside returned 0
second heard 7
out heard 7
```

Every subscriber of the first message is called before the second message starts. And two topics that publish to each other, a ping answering a pong answering a ping, ran 5000 times each without the stack growing: the queue turns the recursion into a loop. It is still a loop with no end if nothing stops it, and the bus has no way to know that it is one. See `E-BUS-0003`.

## Build It

`subscribe`, `unsubscribe`, `publish` and `deliver` in `exercise/solution.hpp`. The subscriber count, the dropped count and `still_subscribed` are given.

```
rcpp verify 17-01
```

Nothing is threaded or timed, so the figures above are the same on every run.

## Use It

`rc/bus/topic_bus.hpp` opens `rc::bus`.

```cpp
rc::bus::TopicBus bus;
bus.subscribe("scan", [&](const std::string&, const std::string& payload) { filter.update(payload); });
bus.publish("scan", encoded_scan);
```

**What this does not do.** It is one thread: a message is delivered by the thread that published it, which is the right model for understanding the semantics and the wrong one for a robot with a sensor thread and a control thread, which is what a queue per subscriber is for. The payload is a string and the bus does not know what is in it. Delivery is immediate and complete, with no loss and no delay, and a real network has both.
And the loop of a ping and a pong is a loop forever if nothing ends it: a real bus would limit the queue.

## What Breaks First

- **A message to nobody.** Silently lost, and the publisher cannot tell. See `E-BUS-0001`.
- **Unsubscribing from inside a handler.** A skipped subscriber, or a crash. See `E-BUS-0002`.
- **A publish from inside a handler.** Delivered out of order or recursing without limit. See `E-BUS-0003`.

## Ship It

`rc/bus/topic_bus.hpp` holds `TopicBus`.

The robot's parts can now meet by name, which is what the next lessons put rules around.

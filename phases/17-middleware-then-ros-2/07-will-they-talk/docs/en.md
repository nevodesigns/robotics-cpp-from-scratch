# Will They Talk: Offered and Requested Quality of Service, and the Silence When They Do Not Match

> The publisher is running. The subscriber is running. The topic name is the same, to the letter. Nothing arrives, and neither of them says a word.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-05, 17-06, 01-06

## The Problem

Lessons 17-05 and 17-06 each gave a topic a property that costs something: reliability and durability. Each side of a topic sets them for itself: a publisher says what it offers and a subscriber says what it needs. If the offer does not meet the need, there is no connection. The middleware cannot give the subscriber reliable delivery from a publisher that sends once, and it does not pretend to.

What it does instead is the part that costs people days: it connects nothing and delivers nothing, and reports it at most as a warning in a log that nobody is reading. The most common single cause of "my subscriber gets no data" in ROS 2 is this one. The camera driver publishes best effort, the node subscribes with the default, and the two never meet.

## The Concept

### Offered and requested

A **Qos** is a handful of properties: reliability, durability, a deadline, a liveliness kind and a lease. A publisher's is an **offer** and a subscriber's is a **request**. The rule is that the offer must be at least as strong as the request in every property. Strength has an order, from weak to strong:

```cpp
enum class Reliability { best_effort = 0, reliable = 1 };
enum class Durability  { volatile_ = 0, transient_local = 1 };
```

Reliability, offered across requested:

| offered \ requested | best effort | reliable |
|---|---|---|
| best effort | talk | SILENT |
| reliable | talk | talk |

A reliable offer can serve either request, since reliable delivery is a superset of best effort. A best effort offer cannot serve a reliable request. The rule is not symmetric. Durability is the same shape: a transient local publisher serves either subscriber, and a volatile one cannot give a late joiner what it asked for. See `E-BUS-0019`, `E-BUS-0020`.

### Deadlines, leases and kinds

A **deadline** is the longest gap between messages that a publisher promises, or a subscriber needs. An offer of a message every 0.1 s meets a request for one every 0.5 s; one of every 0.5 s does not meet a request for every 0.1 s. A request of no deadline is met by anything, and a request for a deadline is not met by an offer of none, since silence is not a promise.

A **lease** is how long before silence means the publisher is dead (lesson 17-04), with the same rule: the offered lease may be no longer than the requested one. A liveliness **kind** is the other kind of strength: a publisher that reports its own aliveness by hand offers more than one that is assumed alive by its process. See `E-BUS-0021`.

### Why it is named

A check that answers yes or no leaves a person to guess which of five properties differed. Each mismatch carries its reason. A camera driver offering best effort at 1 s, against a node requesting reliable, transient local and 0.2 s:

```
reliability: offered best effort, requested reliable
durability: offered volatile, requested transient local
deadline: offered period is longer than requested, or none offered
```

Three reasons, and a person fixing it knows what to change. Nothing the middleware does at run time can be as helpful as this, since it knows the same facts and says less. See `E-BUS-0021`.

### Putting it together

A matcher connects every publisher to every subscriber of the same topic whose request it meets, and reports the rest with their reasons. A camera in the sensor data profile and a map server with a latched profile:

| subscriber | topic | profile | result |
|---|---|---|---|
| detector | image | default (reliable) | NOT connected, 1 reason |
| viewer | image | sensor data (best effort) | camera -> viewer |
| planner | map | default | map_server -> planner |
| late_planner | map | latched | map_server -> late_planner |
| logger | scan | default | no publisher |

Three links, one mismatch. The detector is the node whose image never arrives. See `E-BUS-0019`.

## Build It

`check` and `match` in `exercise/solution.hpp`. The profiles and types are given.

```
rcpp verify 17-07
```

Nothing is random or timed.

## Use It

`rc/bus/qos.hpp` joins `rc::bus`.

```cpp
const rc::bus::MatchResult r = rc::bus::match(publishers, subscribers);
for (const auto& m : r.mismatches) log_warning(m.publisher, m.subscriber, m.reasons);
```

**What this does not do.** It checks four properties and a lease, where DDS has more (history, lifespan, ownership and others), and in ROS 2 the full table is in the documentation of each distribution. It does not apply the QoS: a connected pair here only says that delivery is allowed, and the delivery behaviour is lessons 17-05 and 17-06. It runs on a list and has no notion of
nodes appearing later, which is where the discovery of lesson 17-04 supplies the changes. And the strength order is the DDS one, with every property independent, where a few real combinations are rejected for other reasons.

## What Breaks First

- **Sensor data against a default subscriber.** No images, no error. See `E-BUS-0019`.
- **A latched topic read by a volatile publisher's subscriber.** The late joiner gets nothing. See `E-BUS-0020`.
- **A mismatch reported without its reason.** Days spent guessing which property. See `E-BUS-0021`.

## Ship It

`rc/bus/qos.hpp` holds `Reliability`, `Durability`, `Liveliness`, `Qos`, the three profiles, `Compatibility`, `check`, `Endpoint`, `Link`, `Mismatch`, `MatchResult` and `match`.

The bus can now say why two nodes that should talk do not.

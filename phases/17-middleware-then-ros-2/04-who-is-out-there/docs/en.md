# Who Is Out There: Announcements, Leases and How Long a Silence Takes to Mean Gone

> There is no list of who is on the bus. There is only who has said so lately.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-01, 03-05, 01-06

## The Problem

A publisher with no subscribers was told nothing, in lesson 17-01, and a subscriber that came late missed what came before. In a real system a node needs to know who else is there: whether the planner it depends on has started, and whether the lidar has died. Nobody keeps the list. Nodes start and stop on their own and there is no
registry to ask, so each announces itself, and the others work out the rest.

ROS 2's discovery is of this kind, and so is the heartbeat in most robot software. Building the table makes one number appear that has to be chosen, and a choice with no right answer: how long a silence means gone.

## The Concept

### An announcement and a lease

A participant announces itself, again and again, with a name and an **incarnation**, a number that changes each time it starts. Each announcement keeps it alive for the **lease**: if it is heard again within the lease it is alive, and once longer than that has passed with no word, it is gone.

```cpp
void announce(const std::string& name, int incarnation, double now);
bool alive(const std::string& name, double now) const;
std::vector<DiscoveryEvent> poll(double now);    // appeared, restarted, gone, in order
```

Time is passed in, so a test decides what the clock says. Announced at time 0 with a lease of 3 s: nothing at 2.9 s, alive at exactly 3.0, and at 3.1 one event, `gone lidar`. It is said once, and ten seconds later there is no second event. A new name is one `appeared`, and its heartbeats afterwards are not news. See `E-BUS-0010`.

### Coming back

A node that was gone and announces again is alive again, and the table says `appeared`. A node that restarts so fast that it is never gone is harder: from the table's side it only ever sent a heartbeat. The incarnation is how to tell. After being gone, one event, `appeared`; a heartbeat with a new incarnation inside the lease, one event, `restarted`, and the next heartbeat with that same incarnation, none.
A subscriber that depended on state in the old one needs to know, and the name alone would not tell it. See `E-BUS-0012`.

### How long a silence is gone

Heartbeats every second, a node that dies at 10 s, and a poll every 0.1 s:

| lease (s) | noticed after (s) |
|---|---|
| 1.5 | 1.6 |
| 2.5 | 2.6 |
| 3.5 | 3.6 |
| 5.5 | 5.6 |

A death is noticed after about one lease, plus the polling interval. So a shorter lease is quicker. A short lease also has to be right, on a link that loses heartbeats. The same heartbeats, with 10 percent of them lost and a node that never dies at all, over 2000 s:

| lease (s) | false `gone` events |
|---|---|
| 1.5 | 172 |
| 2.5 | 17 |
| 3.5 | 3 |
| 5.5 | 0 |

At 1.5 s one lost heartbeat is enough to declare the node dead, and with 10 percent loss that happened 172 times in 2000 s: the node flapped between gone and back. At 5.5 s it never happened and a real death took 5.6 s to see. The lease is a few heartbeat periods, and the right number is where the cost of a late "gone" and that of a false one cross, which is the robot's to decide and the table is how
to see it. See `E-BUS-0011`.

## Build It

`announce`, `alive` and `poll` in `exercise/solution.hpp`. `peers`, `lease` and the event queue are given.

```
rcpp verify 17-04
```

The link's losses come from a generator seeded in the test, so the figures above are the same on every run.

## Use It

`rc/bus/discovery.hpp` joins `rc::bus`.

```cpp
Discovery table(3.5);                       // a few heartbeat periods
table.announce(name, incarnation, now);     // whenever a heartbeat arrives
for (const DiscoveryEvent& e : table.poll(now)) react(e);
```

**What this does not do.** It takes the clock as given: two machines with clocks that disagree measure leases differently, which is a later lesson's subject. It trusts the name, so two participants with the same name look like one that restarts all the time. It keeps no history of what a participant said it offers, only that it is there, and in ROS 2 the announcement carries topic types and QoS too.
And it is a table and not a network: nothing here sends a heartbeat, which is the job of whatever timer a node has.

## What Breaks First

- **Treating a heartbeat as news.** An event for every announcement. See `E-BUS-0010`.
- **A lease of about one period.** A flapping node, on a link that loses a heartbeat. See `E-BUS-0011`.
- **A restart that looks like a heartbeat.** State held for the old one. See `E-BUS-0012`.

## Ship It

`rc/bus/discovery.hpp` holds `DiscoveryEvent` and `Discovery`.

Every part of the robot can now find out who else is there, and notice when one stops.

# A Map of Where the Arm Can Go: A Roadmap Built Once for a Fixed Scene, Queries That Cost a Search, and a Map That Is Wrong When the Scene Is

> If the world stands still, plan once and ask many times; if it does not, the map is the first thing that lies.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-03, 16-06

## The Problem

RRT (30-03) plans one motion by growing a tree, and throws the tree away. An arm that works in a fixed cell (a pick and place with the same table, the same bins, the same fixtures) is asked for hundreds of motions in the same scene. Growing a new tree each time re-discovers the same free space each time. If the world stands still, the free space can be found once.

## The Concept

### A graph of free configurations

A roadmap (the probabilistic roadmap, PRM) is built in two steps. Sample a few hundred random configurations and keep the ones that are free. Connect each to its nearest few with an edge, kept if the straight line between them in joint space is free (conservative advancement, 30-02). The result is a graph: nodes are poses the arm can be in; edges are motions it can make. Building it costs a lot of poses looked at (here 56406 for 250 nodes and 8 neighbours) and it is done once. The graph fell into 5 piece1; if it is one piece, any node can reach any other. See `E-ARM-0061`.

### A query is a search

To plan from a start to a goal, join each to its nearest few nodes (by free edges) and find the shortest path through the graph, by Dijkstra's method: a queue ordered by distance, each node's distance and the node before it, stop when the goal is taken. Five queries took 858 poses each on average, against about 2432 for RRT-Connect, which had to find the free space again each time; and, because the graph search finds the shortest path through the roadmap and not the first path a tree stumbled on, the roadmap's paths were 0.74 times the length of RRT-Connect's. See `E-ARM-0062`.

### More nodes, more answers

The graph needs enough nodes to be one piece: it had 6 pieces with 40 nodes and 5 with 300. In this scene, though, the wall is low and wide and the free space is roomy, and even the small roadmap answered 7 of the 8 queries (one pair had an end that was not free), the same as the others (7 and 7): the pieces that the start and the goal joined were the big ones. (The author expected the small roadmap to fail more queries and it did not; what counts is whether the ends join the same piece, and in a roomy scene they do.) A narrow passage (a gap the arm only just fits through) is the hard case: random samples hardly ever fall in it, so the roadmap needs far more nodes than it looks, and a tree grown toward the goal (RRT) can be better there.

### A map of a world that has moved

The roadmap is a statement about a scene. Change the scene and it is wrong. After a box was put in the cell, 34 percent of the roadmap's edges were no longer free, and a query on the old roadmap returned paths with 12 bad edges: the old map said they were clear and they were not. Every use of a stored roadmap has to be checked against the scene as it is now, or the roadmap rebuilt. See `E-ARM-0063`.

## Build It

The search of `roadmap_query` in `exercise/solution.hpp`.

```
rcpp verify 30-04
```

## Use It

`rc/arm/roadmap.hpp`.

```cpp
const rc::arm::Roadmap map = rc::arm::build_roadmap(problem, 300, 8, seed);   // once, at start up
const rc::arm::QueryResult q = rc::arm::roadmap_query(problem, map, q_from, q_to, 6);
```

**What this does not do.** The roadmap is built for one fixed scene and one arm; a payload that changes the shape of the arm (30-07) changes the roadmap. The nearest neighbours are found by a linear scan and the graph is a list of lists, so it is for hundreds of nodes, not hundreds of thousands. The edges are straight lines in joint space, so the paths are polylines, to be smoothed and timed (30-05, 30-06). The shortest path is shortest in joint distance, which is not the shortest in time or in tool distance. There is no lazy version (checking edges only when a query uses them, which is how large roadmaps are built in practice) and no repair of a roadmap after a change: only a measurement of the damage.

## What Breaks First

- **A roadmap with a hole in it.** See `E-ARM-0061`.
- **A plan from scratch every time in a fixed cell.** See `E-ARM-0062`.
- **A roadmap used after the scene changed.** See `E-ARM-0063`.

## Ship It

`rc/arm/roadmap.hpp` holds `Roadmap`, `build_roadmap`, `components`, `QueryResult`, `roadmap_query` and `edge_free_fraction`.

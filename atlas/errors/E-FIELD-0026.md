id: E-FIELD-0026
title: A picture that is the mirror image of the run
match: a left turn on the field is drawn as a right turn on the screen
match: svg y axis points down
match: a run with no poses still draws the world and the robot at its start
platforms: linux, windows
teaches: 22-09-watching-a-run
---

## Symptom

The plot looks fine, and the robot turns the other way from what it did. The plot was drawn as SVG with the data as it is.

## Cause

The picture is the mirror image of the run: the robot's y axis points up and an SVG's points down, and the data was drawn as it was.

Robot frames have y pointing left of x, up on a map; image and SVG coordinates have y pointing down. Drawn as is, the map is flipped top to bottom and every turn is the wrong way.

## Fix

Draw the world in a group with scale(1,-1), and start the view box at minus the top.

```cpp
o << "<g transform=\"scale(1,-1)\">";
```

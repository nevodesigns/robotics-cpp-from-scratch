id: E-FIELD-0026
title: A picture that is the mirror image of the run
match: a left turn on the field is drawn as a right turn on the screen
match: svg y axis points down
platforms: linux, windows
teaches: 22-09-watching-a-run
---

## Symptom

The plot looks fine and the robot's turn is the wrong way round compared with the robot.

## Cause

The robot's y axis is up and an SVG's is down; the data was drawn as it was.

## Fix

Draw the world in a group with scale(1,-1) and start the view box at minus the top.

```cpp
o << "<g transform=\"scale(1,-1)\">";
```

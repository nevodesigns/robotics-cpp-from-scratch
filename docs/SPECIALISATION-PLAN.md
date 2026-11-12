# Specialisation course: field robots, ground, arms and legs

Status: written on 13 November 2026. **Approved by the owner on 13 November 2026**: the shape (same repository, phases 22 to 37, ground first, the built in simulator as the default and Gazebo Sim as the second backend), the tier and hardware decisions, installing Gazebo Harmonic for testing, real world dimensions for the models, the choice of named parts left to the planner, and the humanoid stretch (phase 37) **paused, not dropped**. Nothing in it is built yet: no lesson, header, test or atlas entry exists for it. Each phase is still started only on the owner's go-ahead, as in the first course.

The plan was made after the 320 lessons of this repository were finished, as agreed, and it was made before any building: research first, then the architecture, then the lesson map, then the list of specialisations, then the questions the owner has to answer.

## 1. What it is, and who it is for

A second course in the same repository, built on `librc` and on the rules of the first. It teaches the part of robotics that the general course could not: robots that have to work in a place that is not a clean room. It covers:

- **Ground vehicles**: wheeled and tracked, differential drive, Ackermann, skid steer, indoors and off road.
- **Arms and manipulators**: serial industrial arms, collaborative arms, parallel and delta robots, SCARA.
- **Mobile manipulators**: an arm on a mobile base.
- **Legged robots**: quadrupeds, and the basics of humanoids.

Out of scope, by the owner's decision of 12 November: aerial, marine, space, medical and agricultural robots.

**Audience, decided by the owner: everyone.** People who finished this repository, people who already know C++, and complete beginners. One course serves the three by the mechanism the repository already has, and no separate editions:

| Reader | How they enter |
|---|---|
| Finished the first course | Start at the first lesson of the track they want. |
| Knows C++, new to robotics | A placement page lists, for each lesson of the course, the lessons of the first course it leans on (the `requires` field says it exactly). They read the three or four they lack, not the 320. |
| Complete beginner | The same `requires` chain, followed from the front. The rule that stops a lesson using vocabulary its prerequisites did not teach (L021) already guarantees the chain is enough. The course adds no on-ramp of its own: it would duplicate phases 00 to 03. |

## 2. The tiers, in plain words, and the decision

A **tier** says how much hardware a lesson needs.

| Tier | Needs | Example |
|---|---|---|
| 0 | Nothing but the computer | A kinematics calculation |
| 1 | Nothing but the computer, with the repository's deterministic simulator standing in for the robot | A rover driving a building |
| 2 | Cheap hardware, under about 60 dollars | A two wheeled ESP32 robot with encoders |
| 3 | A real robot | A hobby arm, a small quadruped |

The **tier mix** is how many of the lessons sit at each tier. The decision:

- **Every lesson passes at tier 0 or 1.** The lesson, its tests and its numbers run in continuous integration on every lane, as in the first course. Nobody needs to buy anything to complete the course, and nothing about the course is unverified because it needs hardware.
- **About a third of the lessons carry a hardware section** at tier 2 or 3: how to run the same lesson on the real thing, what to check first, and what differs. These sections are optional. They are checked the way 08-05 is checked: the lesson ships a **fake** of the device that the same test runs against, so the bring-up code is tested even though the device is not there.
- Roughly: **70 percent of lessons tier 1 or 0 only, 20 percent with a tier 2 section, 10 percent with a tier 3 section.** The real counts come out of the lesson map below, and the audit reports them.

## 3. Hardware: named and generic, and the decision

Both, in layers.

**Generic in the code.** Every robot is behind an interface in a new module `rc::hal`: a drive base (`Drive`: set velocity, read wheel counts), an arm (`ArmBus`: set joint targets, read joint state), a leg set (`LegBus`), and sensors (`Range`, `Imu`, `Gnss`, `Encoder`). Lessons are written against the interface. A simulated implementation and a fake of each device ship with the course, and a **contract test** (the idea of 18-02) says what any implementation must keep, so a learner writing a driver for their own board has a test to pass.

**Named in the bring-up pages.** Each track has one **reference build** with parts named, because "any hardware" is not advice anyone can follow on a shopping day. A page lists the parts, what was checked about them, and what to change for something else. The reference builds, from the research of this plan (prices are those found in 2024 and 2025 listings and **must be checked again** before anyone buys):

| Track | Tier 2 reference | Tier 3 reference |
|---|---|---|
| Ground | An ESP32 with two encoder motors and an IMU. Build guides put it at 60 to 70 dollars, under 50 without the power bank. Cheaper kits with plastic gearbox motors have no encoders, which is why the lessons need them. | A Raspberry Pi class computer with a low cost lidar on a commercial chassis. |
| Arms | A small hobby servo arm (to be priced and tested before it is named) | The SO-ARM100, an open source arm built for LeRobot: roughly 220 to 300 dollars for the servo kit, the 3D printed parts extra (about 65 dollars, or printed from the free files). |
| Legs | A single leg on a stand with two or three hobby servos | A small open quadruped: the Petoi Bittle (225 to 299 dollars) or Quaddle (about 99), or the open source Stanford Pupper. |

## 3b. The models use real world dimensions

Decided by the owner on 13 November 2026 as a preference, and made a rule here: **wherever a robot in the course is based on a real product or a published open design, its model uses that robot's real dimensions**, not made up round numbers.

- **Every number in a robot's description file carries its source**, in the same line: where it was found (a data sheet, an open CAD file, a paper), or the word `typical` when it was not found. A learner can see at a glance which numbers are measured and which are assumed, and the audit refuses a robot file with a number that has neither.
- **Candidates for the reference robots**, to be confirmed and sourced when each lesson is written, not now: a small research rover of the TurtleBot3 class and a one tenth scale car platform of the F1TENTH class for the ground track; the open SO-ARM100 and a published industrial arm for the arms; a Mini Cheetah or Stanford Pupper class leg for the legged track. The plan commits to the rule and not to those values.
- **Where it cannot be done honestly** (a dimension that is not published, a motor whose torque curve is not given), the lesson says so, uses a typical value, marks it, and the bring-up page tells the learner how to measure their own and put it in the file.
- Real dimensions matter for what the lessons teach (a track width decides how a skid steer vehicle turns; link lengths decide what an arm can reach; leg lengths decide a gait), so this is a teaching choice and not decoration.

## 4. Research findings that shaped the plan

Each finding is linked and was read from the search results on 13 November 2026. Where a source was a vendor blog the plan says so.

1. **Nav2 treats the vehicle types differently, and does not mention skid steer.** The MPPI controller supports DiffDrive, Omni and Ackermann motion models, and Ackermann adds a minimum turning radius. Smac Hybrid-A* targets car-like robots with Dubins and Reeds-Shepp models, SmacPlanner2D circular differential drive, SmacPlannerLattice arbitrary shapes. The 2023 maintainers' survey recommends Smac Hybrid-A* with MPPI for Ackermann robots. Skid steer is not named in what was found. **Decision: the ground track teaches skid steer as its own model** (slip makes the effective track width differ from the geometric one), which is a gap in the standard tooling and a real lesson. Sources: [Nav2 documentation](https://docs.nav2.org), [MPPI controller](https://docs.ros.org/en/humble/p/nav2_mppi_controller/), [Nav2 plugin selection](https://docs.nav2.org/setup_guides/algorithm/select_algorithm.html), [the maintainers' survey](https://arxiv.org/pdf/2307.15236).
2. **Standards that bound what the safety lessons may claim.** ISO 10218 (industrial robots; part 1 for manufacturers, part 2 for integrators), ISO/TS 15066 (collaborative operation: safety rated monitored stop, hand guiding, speed and separation monitoring, power and force limiting; a vendor guide says it has been folded into ISO 10218-2:2025, to be checked against the ISO page), ISO 13482 (personal care robots), and ISO 3691-4:2023 (driverless industrial trucks: personnel detection by a safety rated scanner, braking within the detection range, overspeed stop; a vendor figure of 0.3 m/s with detection muted is to be checked against the text). Speed and separation monitoring is a protective distance from the robot's reaction time and stopping time, which is lesson 19-04's sum. **Decision: the course models these ideas, names the standard each idea comes from, and says in every such lesson that it is not a compliance course and that certification measures the real machine.** Sources: [collaborative robot safety standards](https://standardbots.com/blog/collaborative-robot-safety-standards), [NIST on speed and separation monitoring](https://www.nist.gov/publications/implementing-speed-and-separation-monitoring-collaborative-robot-workcells), [ISO on 15066](https://www.iso.org/news/2016/03/Ref2057.html), [ISO 3691-4 overview](https://www.fabrico.io/blog/iso-3691-4-driverless-industrial-trucks/), [AGV standards article](https://www.controleng.com/ensuring-agv-safety-with-standards-compliance).
3. **Legged robots are taught from one leg up.** A course on a Cheetah style leg progresses single leg, half body, full robot, with inverse kinematics for end effector trajectories; the Stanford Pupper was designed as a replicable benchmark platform; the Petoi line is the accessible hardware; trot gaits are taught as foot contact patterns on top of forward and inverse kinematics. **Decision: the legged track follows the same ladder**, and the one leg on a stand is its tier 2. Sources: [single leg control practice](https://www.instructables.com/Taming-a-Leg-With-Python-and-CAN-Bus-MIT-Cheetah-S), [Stanford Pupper](https://ar5iv.labs.arxiv.org/html/2110.00736), [Petoi research page](https://www.petoi.com/pages/research-spotlight-1), [Legged Robots Bootcamp](https://github.imc.re/Dynamics-Learning-Workshop/Legged-Robots-Bootcamp).
4. **Low cost arms exist and are good enough to teach on.** The SO-ARM100 is open source with LeRobot support, uses 12 STS3215 servos in the leader and follower pair, and its price ranges depend on whether the printed parts are included. Source: [CNX Software on the SO-ARM100](https://www.cnx-software.com/2024/12/30/so-arm100-is-an-open-source-ai-robotic-arm-kit-with-lerobot-integration-and-nvidia-jetson-support/), [a retailer listing](https://openelab.io/products/seeed-studio-so-arm100-lerobot).
5. **Cheap ground robots can have encoders, and the cheapest kits do not.** Build guides for ESP32 robots with encoder motors come to 60 to 70 dollars, the motors being the largest cost; cardboard and TT motor kits have none. Sources: [a build guide](https://robofoundry.medium.com/building-cheapest-ros2-robot-using-esp32-part-1-hardware-build-af0044de68ce), [a classroom rover](https://hackaday.com/2020/03/02/esp32-rover-with-pcb-chassis-is-ready-to-roll), [a buying guide](https://microcontrollerslab.com/?p=43084).
6. **Simulators, and Gazebo in particular.** MuJoCo is Apache 2.0 and strong on contact dynamics, Gazebo is the usual choice for ROS 2 integration, Webots is the most classroom oriented. None of the sources established deterministic stepping for any of them, and the repository's rule is that published numbers are byte identical across machines. **Gazebo Classic reached end of life in late January 2025**, and the current Gazebo (Gazebo Sim, formerly Ignition) is the supported one: ROS 2 Jazzy pairs with Gazebo Harmonic, and Humble has official `ros_gz` builds for Fortress and Harmonic. This machine has Ubuntu 22.04, ROS 2 Humble and **only Gazebo Classic 11.10.2**, so a Gazebo Sim bridge could not be tested here without installing it. **Decision, revised after the owner asked for Gazebo support (13 November 2026): the repository's own deterministic simulator is the default and carries the numbers; Gazebo Sim is a full second backend that a learner can switch to and compare, built and tested, not a sketch.** See section 5. Sources: [a comparison of simulation environments](https://ar5iv.labs.arxiv.org/html/2103.04616), [a review of physics engines](https://arxiv.org/html/2407.08590v1), [Gazebo Classic end of life](https://discourse.openrobotics.org/t/gazebo-classic-end-of-life/48748), [Jazzy and Gazebo Harmonic](https://discourse.openrobotics.org/t/gazebo-classic-end-of-life-ros-2-jazzy/36239), [ros_gz](https://github.com/gazebosim/ros_gz).

What the research could not settle, and the plan therefore does not claim: whether Nav2 supports skid steer by configuration; the exact current text of ISO/TS 15066 against ISO 10218-2:2025; current prices; any simulator's determinism; whether Gazebo Sim behaves the same on every machine (the plan assumes it does not and measures the gap).

## 5. Architecture

**Where it lives.** The same repository, as phases 22 to 37, so that it has the tool (`rcpp`), the audit rules, the Failure Atlas (new prefixes `FIELD`, `VEH`, `ARM`, `LEG`), the continuous integration and the `requires` chain for free. A separate repository would need all of that again and would lose the guarantee that a prerequisite is real.

**New modules in `librc`.** `rc::hal` (the interfaces and the contract tests), `rc::gz` (the Gazebo backend, built only where Gazebo is found), `rc::field` (shared: terrain, sensor models for the field, scenarios, scoring), `rc::vehicle`, `rc::arm`, `rc::legged`.

**Rules kept, unchanged.** Plain C++17; every lesson a real CMake target with an exercise that fails cleanly and a reference that passes; every number in a lesson from its own suite; a stub that never hangs; no em dashes; one lesson per commit; the supervisor of phase 19 in every capstone's tests (design rule D9); `rcpp audit` clean and all 8 continuous integration jobs green before the next phase.

**New rules the course would add to the audit.** A lesson with a hardware section ships a fake of the device and a test that runs against it (checked); a lesson that names a standard says it is not a compliance lesson (checked by phrase); a named part appears only in a bring-up page and never in a lesson's tests.

**Two simulators, and how a learner can try both.** The generic interface (section 3) is what makes this cheap. A lesson is written once against `Drive`, `ArmBus`, `LegBus` and the sensor interfaces; behind them sits either the built in simulator or a Gazebo backend, chosen by one line of configuration, as 18-12 chose between a simulation and a bench (`backend = builtin` or `backend = gazebo`). The lesson's code does not change.

- **One description of the robot.** A robot is a small text file (wheel base, link lengths, joint limits, motor limits). The course turns that one file into both the built in model and the **SDF** that Gazebo reads (and a URDF for people who use ROS 2). They cannot disagree, because neither is written by hand. A scenario file likewise produces both the built in scenario and a Gazebo world.
- **Which Gazebo.** Gazebo Sim (Harmonic is the recommended release) is the target. Gazebo Classic is end of life and is not supported; the SDF files will mostly load in it and nothing more is promised.
- **How the bridge talks to it.** Directly through Gazebo's own C++ transport library (`gz-transport`), so that it stays a C++ course and needs no ROS 2. A page shows the same thing through `ros_gz_bridge` for people who live in ROS 2.
- **What a learner sees.** The same exercise, passing in the built in simulator, and a second target that runs the same lesson against Gazebo, with the numbers beside the first run's. A learner without Gazebo never meets it; one with Gazebo can watch the robot in its 3D window.
- **Not the same numbers, and the course says so.** Gazebo is not deterministic across machines in the way the built in simulator is, and its contact model is different. A Gazebo run is checked against **tolerance bands**, never exact numbers, and one lesson measures the **gap** between the two simulators on the same scenario (the method of 18-08 and 18-12) so the difference is a number a learner has measured, not a worry.
- **Testing it honestly.** Gazebo tests are built only where Gazebo is found, and where it is not, `rcpp` says so in plain words (it is never a silent pass: the rule of L019). Every Gazebo lesson also passes entirely on the built in simulator, so a skipped Gazebo target never hides a failing lesson. A new continuous integration lane runs the Gazebo targets headless in a container, **non gating while it settles** and gating once it has been stable for a run of pushes. Until that lane exists and is green, no Gazebo number may be quoted in a lesson as measured.

**Safety.** Phase 19's rule carries over: each track's capstone is run under the supervisor, tied to hazards in the log, and the report names what has no test (21-08 and 21-10 are the models).

**What it will not do.** Cross compile (no cross compiler in the continuous integration, as in phase 20); run on any hardware in continuous integration; claim a standard's compliance; replace ROS 2; support Gazebo Classic.

## 6. The lesson map

Counts are the target. Titles are working titles and will be revised as lessons are written, as phase titles were.

### Phase 22, the shared base (17 lessons)

1. A robot as an interface: `Drive`, `ArmBus`, `LegBus`
2. Frames and transforms for field robots, in 2D and 3D
3. Units and limits that cannot be mixed up
4. Motors, gearboxes and backlash as a plant
5. Terrain and friction in the simulator
6. Sensor models for the field: lidar, IMU, GNSS, encoders, with their faults
7. A field scenario file
8. Recording and replaying a field run
9. Watching a run: the robot as a moving picture (new)
10. Scoring a run: navigation, manipulation and gait metrics
11. The contract test for a hardware interface
12. Describing a robot once: one file, two models (the built in one and the SDF) (new)
13. A world and a scenario for Gazebo, made from the same scenario file (new)
14. The same program, another simulator: the Gazebo backend (new)
15. Two simulators, one scenario, and the gap between them measured (new)
16. A bring-up checklist for a real board
17. A safety case template per robot type (capstone of the base)

Each track's world pack (a building, rough ground, an arm cell, a flat floor with slopes) is generated by lesson 13's tool and is shown in the track's lessons, not taught as extra lessons. An optional page shows the ROS 2 route through `ros_gz_bridge`.

### Track U, ground vehicles (phases 23 to 27, 46 lessons)

**23 Vehicle models (9):** differential drive with slip; skid steer and its instantaneous centre of rotation; Ackermann geometry and the minimum turning radius; tracked vehicles; traction limits; actuator limits; odometry for each type; calibrating the effective track width from a run; checking a model against the simulator.

**24 Perceiving for driving (9):** a lidar scan to points; ground and plane segmentation; obstacle clustering; costmap layers (static, obstacle, inflation); traversability from slope and roughness; drops and negative obstacles; deskewing a moving scan; the camera ground plane; tracking things that move.

**25 Knowing where you are, outdoors (9):** wheels and IMU in a filter; GNSS and the lever arm; fix quality and outliers; scan matching; localising against a map with particles; bounded dead reckoning (the idea of 21-05, extended); estimating a gyro's bias; time alignment; the kidnapped robot.

**26 Planning and following, per vehicle (10):** lattice and hybrid A* for a car-like robot; Dubins and Reeds-Shepp paths; smoothing; pure pursuit and Stanley; a sampling controller for each motion model; a skid steer controller that knows about slip; a speed profile within lateral acceleration (19-09); recovery behaviours; off road costs; how often to replan.

**27 Running a ground vehicle (9):** missions and waypoints; teleoperation (21-06 extended); protective fields and stopping within them (the idea of ISO 3691-4, modelled); battery and heat; field logs; the field checklist; degraded modes; releases to a vehicle (phase 20); the ground capstone, a UGV for a day.

### Track A, arms and manipulators (phases 28 to 32, 40 lessons)

**28 Arm geometry in 3D (9):** frames and a table for an arm in 3D; forward kinematics; the Jacobian; closed form inverse kinematics for a spherical wrist; damped least squares; singularities; joint limits and redundancy; workspace; kinematic calibration.

**29 Arm dynamics and joint control (8):** rigid body dynamics, the short form; gravity compensation; the joint control cascade; friction and backlash; computed torque; impedance and admittance; a servo bus as a model; torque and heat limits.

**30 Moving without hitting (8):** collision geometry; distance queries; RRT and RRT-Connect; roadmaps; shortcutting and smoothing; time parametrisation within jerk limits; a planning scene that changes; replanning around something that moves.

**31 Manipulation (9):** a parallel gripper model; a pick and place state machine; a camera to a pose, and hand eye calibration; contact detection; insertion with compliance; speed and separation monitoring (ISO/TS 15066's idea, modelled); cycle time against the speed limit (21-03 extended); leader and follower teleoperation; recording demonstrations.

**32 Other arm families (6):** SCARA; delta robots; parallel platforms; gantries; comparing workspaces; the arm cell capstone.

### Track M, mobile manipulators (phase 33, 10 lessons)

The frames of a base and an arm together; what the arm can reach from where the base is; choosing where to park; moving base and arm together; tipping with the arm out (19-09 extended); the payload's effect; localisation good enough to grasp; fetch and carry as a mission (21-07 extended); the speeds of base and arm together under the supervisor; the mobile manipulator capstone.

### Track L, legged robots (phases 34 to 36, 25 lessons, and phase 37, 8 lessons that are a stretch)

**34 Legs (8):** leg kinematics for two and three joints; the leg Jacobian and foot force; body pose from foot positions; stance and swing; foot trajectories; servo load and heat; contact detection; calibrating a leg.

**35 Gaits and balance (9):** a static gait; the trot as a contact pattern; phase scheduling; attitude from an IMU; body balance control; the centre of pressure and ZMP; recovering from a push; slopes and steps; falling safely.

**36 A quadruped controller (8):** the velocity command; a state machine (stand, walk, sit, fallen); adapting to terrain; a model predictive controller on the body, in its short form; safety (torque limits, fall detection, motors off); what changes from simulation to a real robot; the quadruped capstone.

**37 Humanoid basics, PAUSED by the owner on 13 November 2026 (8):** the linear inverted pendulum; the capture point; a walking pattern generator; a footstep planner; standing on one leg; the ankle strategy; swinging the arms; the humanoid capstone. Paused, not dropped: the owner judged it a big stretch. It is picked up again, or not, after phases 34 to 36 are built.

### Totals (the base grew from 12 to 17 lessons when Gazebo support and a viewer were added on 13 November)

| Part | Lessons |
|---|---|
| Base | 17 |
| Ground | 46 |
| Arms | 40 |
| Mobile manipulators | 10 |
| Legged | 25 (33 with the humanoid stretch) |
| **Total** | **138 (146 with the stretch)** |

For scale: phases 14 to 21 of the first course were 120 lessons.

## 7. Order of work, and the approvals it needs

The owner approves each stage before it is built, as with the phases of the first course.

| Stage | What | Lessons |
|---|---|---|
| 1 | The base (22) and the ground track (23 to 27) | 63 |
| 2 | The arms track (28 to 32) | 40 |
| 3 | Mobile manipulators (33) and the legged track (34 to 36) | 35 |
| 4, paused | Humanoid basics (37), paused by the owner and not dropped | 8 |

Ground first because the repository's safety, navigation and field work lead straight into it, because it needs the least hardware to be real, and because the arms and legged tracks reuse its simulator and scoring.

## 8. The robotics specialisations, listed

**In scope of this plan:** ground vehicles; arms and manipulators; mobile manipulators; legged robots.

**The cross cutting disciplines every specialisation uses, and where they live:** perception and computer vision (phases 15, 24); state estimation (16, 25); motion planning (16, 26, 30); control (14, 29); manipulation and grasping (31); human robot interaction and safety (19, 21, 27, 31); simulation and testing (18, 22); systems, middleware and deployment (17, 20); learning based methods (not in either course, see below).

**Specialisations that exist and are not covered, by decision:** aerial (multirotors, fixed wing), marine (surface and underwater vehicles), space, medical and surgical, agricultural, humanoid and social robots beyond the basics (37), swarms and multi robot systems, soft and micro robots, exoskeletons and wearable robots, inspection and construction robots as application areas.

**One gap to say plainly: learning based control.** Reinforcement learning and imitation learning are now central to arms (the SO-ARM100 exists for LeRobot) and legged robots. Neither course teaches them, since both are built on plain C++ and deterministic, verifiable lessons. The plan keeps room for it: the arm track records demonstrations (31) and the legged track's controller has a clean command interface (36), so a later learning course can sit on top.

## 9. Decisions of the owner, 13 November 2026

1. **The shape:** approved. The same repository, phases 22 to 37, four stages, ground first, the built in simulator as the default and Gazebo Sim as the second backend.
2. **Tiers and hardware:** approved. Every lesson passes without hardware; optional hardware sections are tested against a fake of the device.
3. **Humanoid basics (phase 37):** paused, not dropped.
4. **Named parts:** left to the planner. The plan's choices stand and may be changed, with the reason recorded here, when each track's reference build is written.
5. **Gazebo Harmonic:** the owner approved installing it on the build machine. The machine needs an administrator password for that, which the assistant cannot give, so the owner runs the install commands; the plan's Gazebo lessons wait for it (phase 22 lessons 12 to 15). Until it is there, nothing about Gazebo is built, and no Gazebo number is quoted.
6. **Real world dimensions:** a preference of the owner, made a rule in section 3b.
7. **Which stage first:** stage 1 (the base and the ground track, 63 lessons), one phase at a time, each started on the owner's go-ahead: phase 22 first.
8. **How stage 1 runs (owner's wish, decided by the planner on 13 November 2026):** the whole of stage 1 (phases 22 to 27) is built in one run, one phase at a time, each pushed and checked on CI before the next starts, with a short report after each. The run stops at the end of stage 1. The arm course (stage 2) is started only when the owner says so, and the same shape then repeats for each specialisation. A red CI, or a decision that is the owner's, stops the run early.

## 10. Progress

- **Phase 22, the shared base: built on 13 November 2026, 17 lessons** (docs/BUILD-LOG.md, "Phase 22"). The lesson map of section 6 held; the Gazebo lessons were tested against Gazebo Sim 8.15 on the build machine, and the continuous integration job for it is not a gate until it has been seen to pass on a hosted runner.
- Phases 23 to 27, the ground track, are next, in one run, stopping at the end of stage 1.

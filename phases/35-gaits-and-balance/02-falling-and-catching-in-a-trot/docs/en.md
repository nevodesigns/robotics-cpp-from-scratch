# Falling and Catching in a Trot: A Body on Two Feet as an Inverted Pendulum, and How Long a Stance May Last Before the Next Feet Must Land

> A trot never stands still: it falls toward the side and is caught by the feet that land a moment later, and the moment has a length.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-01, 34-04, 33-05

## The Problem

35-01's crawl is safe and slow. A **trot** is faster and **economical**, and it has **no support polygon**: with the diagonal feet down (front left and rear right, then the other pair) there are **two feet** on the ground, **a line**, and a body cannot balance on a line. A trotting robot is **always falling**: toward one side while one diagonal pair is down, and then, as that pair lifts and the other lands, it is caught and **falls toward the other side**. It works only if the **catch comes soon enough**. This lesson measures **how soon**.

## The Concept

### An inverted pendulum

With two feet down the body can **only turn about the line joining them**. Near the upright the **sideways distance of the centre of mass from that line** obeys **x'' = (g / h) x**: the further it is from the line, the faster it accelerates away. That is the **inverted pendulum**, and its solution is **x(t) = x0 cosh(t / T) + v0 T sinh(t / T)** with the **time constant T = sqrt(h / g)**. For a body **0.27 m** high **T was 0.166 s**: **a fall begins slowly and then takes off, doubling in a time of order a tenth of a second**. See `E-LEG-0027`.

### How long it takes to fall

With the centre of mass **10 mm** from the line and at rest, it took **0.41 s** to move out **6 cm** (about the half width of the body: by then the diagonal's swing feet need to be on the ground). With it **30 mm** off, only **0.22 s**. If it was **already moving away** at **0.10 m/s** from that offset, **0.15 s**. The time has a **closed form**: with u = e to the t over T the condition x(t) = L is a **quadratic** in u, and the root's logarithm times T is the time (it agrees with a step by step integration of the pendulum to a couple of milliseconds, the test checks). It is **negative** where the centre is moving **back toward the line** fast enough to stop first. See `E-LEG-0028`.

### What it asks of the stance

The **stance has to be shorter than that time**, with room: a stance of **0.25 s** was **1** (1 means short enough, with a margin of 0.7) for the **10 mm** offset and **0** for the **30 mm** one. So the trot **depends on the body being kept over the line of the feet to within a centimetre or two**, and **on the swing feet landing on time**. Two levers help: a **taller** body is slower to fall (**0.50 s** for a body **0.40 m** high, against 0.41 s), and a **shorter stance** (a faster step) leaves less time to fall. A trot that is **stopped in the middle** is falling (34-04, `E-LEG-0010`). See `E-LEG-0029`.

## Build It

`time_to_move` in `exercise/solution.hpp`.

```
rcpp verify 35-02
```

## Use It

`rc/legged/trot_balance.hpp`.

```cpp
if (!rc::legged::trot_stance_ok(t_stance, x_off, v_off, 0.06, com_height, 0.7)) shorten_the_stance();
```

**What this does not do.** The pendulum is **linear** (small angles), **one-dimensional** (across the diagonal) and **rigid**: the legs' springiness and the **body's own rotation** are left out, and the **swing feet are taken to catch the body at the end of the time** without saying where they land (35-07). The distance **L** is a **given number** (the body's half width here). The body is **0.27 m high** as a typical number. The **offset and speed** are **given**; where they come from (35-05's balance control, 35-04's attitude) is the rest of the phase.

## What Breaks First

- **A trot with a stance that is too long for the body's offset.** See `E-LEG-0029`.
- **A body left off the line of its feet.** See `E-LEG-0028`.
- **A fall time worked out as if it were constant.** See `E-LEG-0027`.

## Ship It

`rc/legged/trot_balance.hpp` holds `pendulum_time`, `time_to_move` and `trot_stance_ok`.

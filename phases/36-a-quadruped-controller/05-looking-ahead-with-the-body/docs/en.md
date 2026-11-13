# Looking Ahead With the Body: A Short Horizon Controller for the Body's Sideways Balance, With the Foot's Limit Built Into the Problem

> A controller that knows its own limit can use all of it, and one that finds out by hitting it uses less.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-04, 35-07, 35-02, 35-06

## The Problem

35-05 held the body level by **forces**, 35-07 saved it from a push by a **step**, and 35-02 showed the trot is an **inverted pendulum** that must be **caught**. Each used a **rule** (a fixed law). A rule works one moment at a time, and a legged robot's problem is **not one moment**: what it **can** do (where the **ZMP** may be) **changes in a known way** with the gait (the feet that are down change as the swing feet land), and **what it does now decides what it can do later**. A controller that **looks ahead** a short way can use that. That is **model predictive control** (MPC), here in its **shortest form**, for the body's balance **in one direction**.

## The Concept

### The model and the problem

The body is the **inverted pendulum**: **x'' = (g / h)(x - p)**, with **x** the position and **v** the velocity **relative to the middle of the support** and **p** the **ZMP** (35-06) that the feet can place, within **plus or minus a bound** (the extent of the feet that are down). Over a time step the model is **exact**: **s' = A s + B p** with the matrices in `lipm_discretize` (**cosh** and **sinh** of w dt, w = sqrt(g / h)). Over a **horizon of n steps** the predictions of position and velocity at each step are **matrices times the sequence of ZMP commands** plus the **free motion**. The **cost** is the sum of the **squares** of position and velocity along the horizon plus a small weight on the commands. See `E-LEG-0063`.

### Solving it

The cost is **quadratic** in the commands, with **bounds** on each: a small **quadratic program**. The **projected gradient** method solves it: **step down the gradient, then clip to the bounds**, with the step **one over the trace of the Hessian** (a bound on its size, so it never overshoots), repeated. For a horizon of 15 steps (0.75 s, four of the pendulum's time constants) it needed **300 iterations**, each a few hundred multiplications (a small fraction of a millisecond), and for a push of **0.5 m/s** its first command was **14.8 cm**: ahead of the body, to push it back. **Only the first command is used** and the problem is **solved again at the next step**, with the **new** state: that is the **receding horizon**. See `E-LEG-0064`.

### What it buys

The **bound** here is **15 cm for 0.2 s and 1 cm for 0.2 s, alternately** (while the swing feet land the ZMP has nowhere to go). A **fixed gain law, kx = 3.0 and kv = 0.9, clipped** to the bound that is available **now**, cannot see that the narrow time is coming. The **largest push recovered** was **0.70 m/s** by the fixed gain law and **0.71 m/s** by the predictive controller: **about the same**, because **a saturating law with a high gain already uses all the push there is**. Where the predictive controller **wins** is in **speed**: for a push of **0.6 m/s** the body was back at the middle after **1.00 s** with the fixed law and **0.60 s** with the predictive controller, since it **spends the wide windows well**. And for a **small push** (0.3 m/s) the **fixed law was quieter** (**0.00 s** against **0.35 s**): the predictive controller **plans**, and a plan has a cost when there is nothing to plan for. **It is not a stronger controller; it is one that knows what is coming.** See `E-LEG-0065`.

## Build It

`mpc_zmp` in `exercise/solution.hpp`.

```
rcpp verify 36-05
```

## Use It

`rc/legged/body_mpc.hpp`.

```cpp
double bound[15];  /* the ZMP range for each of the next 15 steps, from the gait's schedule */
const double p = rc::legged::mpc_zmp(x, v, com_height, 0.05, 15, bound, 1.0, 1.0, 0.01, 300, nullptr);
```

**What this does not do.** It is **one direction** (the sideways balance of a trot, say) with a **linear, rigid pendulum** and a **constant height**, **exact in the model** (a real body is not: the legs' springs, the delay, the push's estimate). The **bounds** are **given** (from 35-06 and the gait's timing). There are **no bounds on the state** (the body may leave the middle): only the **ZMP is bounded**. The **solver** is a **plain projected gradient with a fixed number of iterations**, **not** an exact one (a real controller uses a QP solver and a warm start, and checks that it **converged**). The **horizon and weights** are **numbers chosen here**. The numbers are **typical** of a 12 kg robot. A **full** body MPC (the whole body, all directions, friction cones, the swing) is **far larger** and is **not** built in this course.

## What Breaks First

- **A controller that does not know the bound is coming.** See `E-LEG-0065`.
- **A solver with no check that it converged.** See `E-LEG-0064`.
- **A model that is exact in the test and not on the robot.** See `E-LEG-0063`.

## Ship It

`rc/legged/body_mpc.hpp` holds `lipm_discretize`, `pd_zmp` and `mpc_zmp`.

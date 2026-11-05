# The Comment Said Otherwise: Reading Code You Did Not Write

> A reader who trusted every comment in five sensor functions predicted the wrong
> answer in 12 of 25 cases, and was exactly right about one of the five.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-02, 01-04, 01-07, 01-15

## The Problem

Every lesson so far has handed you code to write. Most of a working life is the
other thing: code somebody else wrote, with a comment on top of it, that you have
to understand before you can change it.

Nothing in this curriculum has taught that, and it is a skill with a method. The
method is worth learning on code whose comments you cannot trust, so that is what
this lesson gives you: five functions from a robot's sensor path, written by
someone who is no longer here, each with a comment saying what it does.

## The Concept

### A comment is a claim, and the code is the fact

Nothing checks that a comment and the code under it agree. The code is changed far
more often than the words above it, and the words are what gets read.

```
  five functions, each with a comment, and where the comment was wrong:

    smooth      5 of 6
    deadband    3 of 5
    last_mean   2 of 5
    percent     2 of 5
    is_stale    0 of 4
    all         12 of 25
```

Twenty five cases, each worked out two ways: what a reader who trusted the comment
would predict, and what the code returns. They disagree on 12.

The last row matters as much as the others. `is_stale` is the control: its comment
is exactly right, and a reader who distrusted everything would have wasted time on
it. Reading code is not distrusting it. It is **finding out which comment is the
wrong one**, and the only way to find out is to trace an input and see.

`E-DEBUG-0014`.

### What the wrong ones were

**`smooth`** says its factor is 0.2 and that each reading moves the estimate 20
percent of the way. The code sets `alpha = 0.8` and applies it to the reading.
`smooth(10, 20)` is 18, where the comment predicts 12. Both values are called
alpha, in different conventions, and the comment and the code chose different
ones.

**`deadband`** says anything further out than the width passes through unchanged.
The code subtracts the width from it: 0.5 with a width of 0.1 comes out 0.4. A
deadband that shifts the signal is a real design, and it is a different one from
the one described.

**`last_mean`** says "or of all of them if there are fewer". The loop does that.
The divisor is `n`, the count asked for, and not the count got. Readings 2 and 4
with `n` of 4 give 1.5 where the comment promises 3. When there are `n` or more
readings the two agree, so any test with enough data passes.

`E-NUM-0030`.

**`percent`** says 0 to 1023 maps to 0 to 100. The code divides by 1024, which is
the number of values and not the size of the range, and integer division truncates.
`percent(1023)` is 99, so nothing ever reads 100.

`E-NUM-0029`.

### The method

In the order that works.

1. **Find where it is called**, and what it is given, before reading the body.
   Half of what a function does is decided by what it is never asked.
2. **Read the comment as a claim** and write down what it predicts.
3. **Pick the smallest input** that exercises the line you doubt, and trace it
   with a pencil and the actual numbers. Do not run it.
4. **Predict, then run.** A wrong prediction is the most useful thing that can
   happen, because it marks the exact line you misread. Running first teaches you
   what the code does and nothing about how to read it.
5. **Pin what it does** before you change anything: record the current answers
   in a test. If a change moves one, you will know which.
6. **Change the least you can**, and run the pin again.

Steps 3 and 4 are the whole skill, and the exercise is built around them. The
guesses table in `exercise/solution.hpp` starts as zeros. Work each one out from the
code, write your number, and only then run the suite, which lists every guess that
was wrong with what you said and what the code returned.

### A fix is a second skill, and it depends on the first

`percent` needs to reach 100. The smallest change is the divisor, 1024 to 1023.
Measured against the old function over the whole range:

```
  percent_exact against the old percent, over raw 0 to 1023:
    values where they differ   49 of 1024
    largest gap                1
    old percent(1023)          99
    percent_exact(1023)        100
```

49 answers move, none by more than one, and the top is right. It would have been
natural to round to the nearest percent at the same time. That moves **560** of the
1024 answers, because the old function truncated and every caller has been living
with that. It is a second decision, and made in the same change it cannot be told
apart from the first if something breaks. The suite refuses the larger change for
being larger.

## Build It

Two halves.

**The guesses.** Twenty five of them, in five tables, for the code above. Trace
each on paper before running anything.

**One change.** `percent_exact`: divide by 1023 and nothing else, and clamp the
ends so a reading below 0 gives 0 and one above 1023 gives 100, rather than
extrapolating.

The stub gives zeros, which is right for exactly five of the twenty five guesses.
The suite's first test measures the provided code against its comments, so it
passes whatever you write; the other two do not.

## Use It

When you inherit a file, do steps 1 and 2 before anything else, and write down
every comment's claim. Then pick the three functions you will have to change and
trace one input through each. It takes less time than the first wrong assumption
costs.

Two limits.

**This does not find every difference.** A trace of one input finds the line that
decides it. A function with five branches needs five traces, and the comment may be
right on four.

**A test that pins current behaviour is not a test that behaviour is correct.** It
records what the code does so that a change is visible. Whether what it does is
what is wanted is a separate question, and the comment is where you start asking.

## What Breaks First

`E-DEBUG-0014` is the change made on the strength of a comment that was wrong, with
the table of how often that was true here.

`E-NUM-0029` is the percentage that tops out at 99, because it divided by the
number of values and not the full scale.

`E-NUM-0030` is the mean that divides by the count it asked for, and reads low until
there is enough data, so a long test never sees it.

## Ship It

No header graduates from this lesson. Its output is a method, the six steps above.

From here on, when a lesson in this curriculum hands you code to change, the
comment is a claim, and you predict before you run.

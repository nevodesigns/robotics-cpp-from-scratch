id: E-BUILD-0004
title: A warning that was not a defect
match: (mismatched-new-delete|stringop-overread|pragma GCC diagnostic had no effect)
platforms: linux, windows
teaches: 04-08-which-warnings-to-believe
---

## Symptom

An optimised build reports dozens of warnings about code you have read
carefully and believe to be correct. Switching the warning off for that
region, with a `#pragma`, changes nothing: the warnings are still there.

```
warning: 'void operator delete(void*)' called on pointer returned
from a mismatched allocation function [-Wmismatched-new-delete]
```

## Cause

Some warnings are wrong, and two kinds of wrong matter here.

**The compiler cannot see what you can.** Measured in this curriculum: 51 of
`-Wmismatched-new-delete`, in the six lessons that both include a leak checker
and use raw `new[]` and `delete[]`. gcc traces the `delete[]` into the replaced
global `operator delete[]`, sees `std::free`, and pairs that with the `new[]`
at the call site, without modelling that the global `operator new[]` was
replaced too and allocates with `std::malloc`. The pairing in the learner's
code is correct.

That one was confirmed from the other side: a genuine mismatch in the same
code is caught at runtime by the address sanitizer as
`alloc-dealloc-mismatch`, verified, and the sanitizer lane runs on every push.

**The warning is not about your code at all.** One `-Wstringop-overread`
arrived from inside `<bits/stl_algobase.h>` on gcc 11, reached through
`std::vector::insert` inlined into a test. A known issue in that library
version.

And the reason the `#pragma` did nothing is worth knowing on its own:
`-Wmismatched-new-delete` is emitted in a **late pass, after inlining**, where
the lexical region a `#pragma GCC diagnostic push` describes no longer exists.
The pragma was tried here and reverted, because leaving it in place would have
implied a suppression that was not happening.

## Fix

Decide, and write the decision down.

Three questions settle most cases. **Can this actually happen here?** **Can
the compiler see what you can see?** **Is the fix cheaper than the argument?**

For a warning you have investigated and dismissed, the fix is not a `#pragma`
and not a global `-Wno-`. A global suppression hides the warning for learner
code as well, which in the phase 02 lessons that teach allocation pairing is
exactly the wrong trade. The fix is a written ledger:

```
docs/PROGRESS.md, "Known warnings":
  51 of -Wmismatched-new-delete, from rc/test/leak_check.hpp ...
   1 of -Wstringop-overread, inside <bits/stl_algobase.h> on gcc 11 ...
```

Then count what the build reports and compare it against the ledger, so a
**new** warning is an event rather than one more line in a wall of them:

```cpp
#include <rc/test/warnings.hpp>

const rc::test::WarningCount c = rc::test::count_warnings(build_log);
// compare c.sites, not c.lines: E-DEBUG-0012
```

A warning you have dismissed in writing costs nothing to carry. A warning you
have dismissed in your head costs the next person the whole investigation
again, and this repository made that mistake twice before the ledger existed.

`E-CPP-0061` is the opposite case: warnings that point the wrong way.

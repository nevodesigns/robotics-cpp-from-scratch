#!/usr/bin/env bash
#
# Every exercise that compiles must fail with a report, not a crash.
#
# A shipped exercise is meant to fail: that is what gives the learner something
# to fix. Two ways of failing are not acceptable. One is passing as shipped,
# which means the lesson teaches nothing. The other is crashing, because a stub
# returning zero can walk a test into a division by zero or an assertion, and a
# learner who runs the suite and gets "Aborted (core dumped)" has been told
# nothing about which function to write.
#
# This lives here rather than inline in the workflow so it can be run before a
# push instead of after one. It needs a Debug tree: the assertions it exists to
# catch are compiled out of a release build, which is how a crash in this gate
# reached continuous integration once already.
#
#   tools/ci/exercises-fail-cleanly.sh [build-dir]
#
set -u

build=${1:-build}

if [ ! -x "$build/bin/rcpp" ]; then
  echo "no rcpp in $build/bin: configure a Debug tree and build the rcpp target first" >&2
  exit 2
fi

export QT_QPA_PLATFORM=offscreen
status=0

# Every target listed must produce exactly one line of output. The gate's own
# output is the only evidence that it looked at anything, so it counts.
listed=0
reported=0

# The exercises that are meant not to compile, because the compiler error is
# the lesson. Naming them is the point.
#
# This list used to be absent, and any exercise that failed to build was
# reported as "does not compile on purpose" and skipped. That made the gate
# unable to tell a deliberate compile error from one somebody had just
# introduced, and a transient build failure from either. Both were silently
# green. It happened: a flaky run reported two Qt exercises as deliberate, and
# they build fine.
#
# So a build failure is now only acceptable for a target named here, and a
# target named here that starts compiling is also an error, because the lesson
# has stopped teaching what it claimed to.
expected_not_to_compile="
lesson_00_02_the_build_model_exercise
lesson_00_03_reading_what_the_compiler_tells_you_exercise
lesson_03_01_classes_and_invariants_exercise
lesson_09_02_what_moc_generates_exercise
"

declared_not_to_compile() {
  case "$expected_not_to_compile" in
    *"
$1
"*) return 0 ;;
    *) return 1 ;;
  esac
}

for target in $("$build/bin/rcpp" targets --exercises); do
  listed=$((listed + 1))

  if ! cmake --build "$build" --target "$target" > /dev/null 2>&1; then
    if declared_not_to_compile "$target"; then
      echo "skip $target, it does not compile on purpose"
    else
      echo "${GITHUB_ACTIONS:+::error::}$target failed to build, and it is not one of the exercises declared not to compile"
      status=1
    fi
    reported=$((reported + 1))
    continue
  fi

  if declared_not_to_compile "$target"; then
    echo "${GITHUB_ACTIONS:+::error::}$target is declared not to compile and it compiled, so the lesson no longer teaches the error it claims"
    status=1
    reported=$((reported + 1))
    continue
  fi

  binary=$(find "$build" -name "$target" -type f -perm -u+x | head -1)
  # Written as a full if rather than [ -z ... ] && continue, because the short
  # form evaluates to false whenever the binary does exist, and under errexit
  # that would end the run silently.
  #
  # This branch used to `continue` without a word, and it was the one path
  # through this loop that reported nothing and failed nothing. A target that
  # built but whose executable could not be found was skipped invisibly and
  # the gate still exited 0, so it could report success for work it had not
  # done. That is the same defect the named list above was added to fix,
  # arriving by a different door.
  #
  # It was found by counting rather than by reading: the log had 105 lines for
  # 106 lessons. Nothing in the output said which one was missing.
  if [ -z "$binary" ]; then
    echo "${GITHUB_ACTIONS:+::error::}$target built but no executable was found for it"
    reported=$((reported + 1))
    status=1
    continue
  fi

  # A failing exercise is the expected outcome, so its non zero exit must not
  # be treated as an error of this script.
  code=0
  "$binary" > /dev/null 2>&1 || code=$?

  reported=$((reported + 1))
  if [ "$code" -eq 0 ]; then
    echo "${GITHUB_ACTIONS:+::error::}$target passes as shipped, so it teaches nothing"
    status=1
  elif [ "$code" -gt 128 ]; then
    echo "${GITHUB_ACTIONS:+::error::}$target crashed with signal $((code - 128)) instead of reporting a failure"
    status=1
  else
    echo "ok $target failed cleanly with exit $code"
  fi
done

if [ "$reported" -ne "$listed" ]; then
  echo "${GITHUB_ACTIONS:+::error::}reported on $reported exercise(s) of $listed listed, so this gate did not look at everything"
  status=1
else
  echo "reported on all $listed exercise(s)"
fi

exit $status

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

for target in $("$build/bin/rcpp" targets --exercises); do
  # Some exercises do not compile on purpose, because the compiler error is the
  # lesson. Those have nothing to run.
  if ! cmake --build "$build" --target "$target" > /dev/null 2>&1; then
    echo "skip $target, it does not compile on purpose"
    continue
  fi

  binary=$(find "$build" -name "$target" -type f -perm -u+x | head -1)
  # Written as a full if rather than [ -z ... ] && continue, because the short
  # form evaluates to false whenever the binary does exist, and under errexit
  # that would end the run silently.
  if [ -z "$binary" ]; then
    continue
  fi

  # A failing exercise is the expected outcome, so its non zero exit must not
  # be treated as an error of this script.
  code=0
  "$binary" > /dev/null 2>&1 || code=$?

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

exit $status

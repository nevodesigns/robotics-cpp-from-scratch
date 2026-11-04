#include <rc/test/rc_test.hpp>

#include <iostream>

#include "solution.hpp"

RC_TEST("zero means it worked, and that is the one thing to remember") {
  RC_CHECK(kSuccess == 0);
  RC_CHECK(kFailed != 0);
  RC_CHECK(kMisused != 0);
  RC_CHECK(kFailed != kMisused);

  // The counter-intuitive part, written down once so no caller has to think
  // about it: zero is the success and every other number is a failure, which
  // is the opposite of how a bool reads.
  RC_CHECK(succeeded(kSuccess));
  RC_CHECK(!succeeded(kFailed));
  RC_CHECK(!succeeded(kMisused));
  RC_CHECK(!succeeded(42));

  std::cout << "  success " << kSuccess << ", failed " << kFailed
            << ", misused " << kMisused << "\n\n";
  std::cout << "  a shell asks one question of a finished program: do I carry\n";
  std::cout << "  on? Zero says yes. That is the whole protocol, and it is\n";
  std::cout << "  older than C++.\n\n";
}

RC_TEST("any number of problems is the same answer to the only question asked") {
  RC_CHECK(status_for(0) == kSuccess);
  RC_CHECK(status_for(1) == kFailed);
  RC_CHECK(status_for(2) == kFailed);
  RC_CHECK(status_for(255) == kFailed);
  RC_CHECK(status_for(256) == kFailed);
  RC_CHECK(status_for(100000) == kFailed);

  // Negative counts should not happen and must not be read as success if
  // they do, because a count that went wrong is itself a problem.
  RC_CHECK(status_for(-1) == kFailed);

  std::cout << "  problems   status\n";
  for (const int problems : {0, 1, 2, 255, 256, 100000})
    std::cout << "  " << problems << "\t     " << status_for(problems) << "\n";
  std::cout << "\n";
}

RC_TEST("only the low eight bits of the number survive") {
  // This is the arithmetic the operating system does to main's return value,
  // and it is lesson 00-04's truncation in a place nobody expects it.
  //
  // Measured outside this suite, because a test cannot exit twice: a program
  // returning 256 was seen by the shell as 0, one returning 257 as 1, and one
  // returning -1 as 255. The arithmetic below is why.
  RC_CHECK((0 & 0xFF) == 0);
  RC_CHECK((1 & 0xFF) == 1);
  RC_CHECK((255 & 0xFF) == 255);
  RC_CHECK((256 & 0xFF) == 0);
  RC_CHECK((257 & 0xFF) == 1);
  RC_CHECK((-1 & 0xFF) == 255);

  std::cout << "  returned   what a shell sees\n";
  for (const int returned : {0, 1, 2, 255, 256, 257})
    std::cout << "  " << returned << "\t     " << (returned & 0xFF) << "\n";
  std::cout << "  -1\t     " << (-1 & 0xFF) << "\n\n";
  // Tied to the work this lesson asks for: whatever status_for returns for a
  // count that would truncate to zero, it must not be a status that survives
  // truncation as success.
  RC_CHECK((status_for(256) & 0xFF) != 0);
  RC_CHECK((status_for(512) & 0xFF) != 0);

  std::cout << "  the 256 row is the one that bites. `return failures;` is a\n";
  std::cout << "  natural thing to write and it reports success at exactly\n";
  std::cout << "  256 failures, which is why status_for collapses the count\n";
  std::cout << "  on purpose instead of letting the system do it.\n\n";
}

RC_TEST("the collapse is deliberate, and status_for never truncates to success") {
  // The property that matters, checked over every count that could be
  // mistaken for success if it were passed through untouched. 0, 256, 512 and
  // so on all have a low byte of zero.
  int wrongly_successful = 0;
  for (int problems = 1; problems <= 2000; ++problems) {
    const int status = status_for(problems);
    if (succeeded(status)) ++wrongly_successful;
    if ((status & 0xFF) == 0) ++wrongly_successful;
  }
  RC_CHECK(wrongly_successful == 0);

  // And the counts that would have been truncated to zero, named.
  for (const int dangerous : {256, 512, 768, 1024, 1280})
    RC_CHECK(!succeeded(status_for(dangerous)));

  std::cout << "  2000 non zero problem counts, none of which produced a\n";
  std::cout << "  status that survives truncation as success\n\n";
  std::cout << "  passed straight through, 256, 512, 768, 1024 and 1280 would\n";
  std::cout << "  every one of them have reported that nothing was wrong.\n\n";
}

#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>

#include "solution.hpp"

// File scope, per audit rule L022.
//
// A gcov report, reproduced by hand from a real one so the suite needs no
// coverage build to run. Every one of the five count forms appears, and the
// header lines are the ones gcov actually writes.
const std::string kReport =
    "        -:    0:Source:/repo/librc/include/rc/core/args.hpp\n"
    "        -:    0:Graph:t13-test_args.gcno\n"
    "        -:    0:Data:t13-test_args.gcda\n"
    "        -:    0:Runs:1\n"
    "        -:    1:// a comment, which is not code\n"
    "        -:    2:\n"
    "        1:    3:  const int first = 1;\n"
    "      103:    4:  for (std::size_t i = 0; i < n; ++i) {\n"
    "       1*:    5:    table[i] = compute(i);\n"
    "    #####:    6:    never_called_from_the_suite();\n"
    "    =====:    7:  }\n"
    "        -:    8:}\n";

RC_TEST("the five count forms, each counted for what it is") {
  const LineCoverage c = parse_gcov(kReport);
  RC_CHECK(c.ok);

  // Code: lines 3, 4, 5, 6, 7. Not code: the four headers, the comment, the
  // blank line and the closing brace on line 8.
  RC_CHECK(c.executable == 5);

  // Never run: ##### on line 6 and ===== on line 7. Both mean not covered,
  // and counting only the first is the mistake this lesson was written
  // around: it reported 5 where the answer was 7 on a real file.
  RC_CHECK(c.never_run == 2);

  // The digits* form on line 5 is code that ran, and it is also worth
  // knowing about separately, because "ran, but not in every instance" is a
  // different state from "ran".
  RC_CHECK(c.partly_run == 1);

  RC_CHECK_NEAR(percent_covered(c), 60.0, 1e-9);

  std::cout << "  executable " << c.executable << ", never run " << c.never_run
            << ", partly run " << c.partly_run << ", covered "
            << std::fixed << std::setprecision(2) << percent_covered(c) << "%\n\n";
  std::cout << "  -        not code: a comment, a blank, a brace\n";
  std::cout << "  digits   ran that many times\n";
  std::cout << "  digits*  ran, but not every instance of it did\n";
  std::cout << "  #####    code that never ran\n";
  std::cout << "  =====    code that never ran, block never entered\n\n";
}

RC_TEST("the numbers from a real file, which took three attempts to agree") {
  // The real measurement this lesson is built on: lesson 01-13's suite
  // against its own artifact. gcov reported 89.06% of 64. Getting there by
  // hand took three tries.
  //
  //   counting ##### only                 5 never run, and the total was wrong
  //   counting ##### and =====            7 never run, total still wrong at 56
  //   counting digits* as code as well    7 never run of 64, which is 89.06%
  //
  // Reproduced here as the arithmetic rather than the file, because the file
  // is 180 lines and the point is the definition.
  LineCoverage real;
  real.ok = true;
  real.executable = 64;
  real.never_run = 7;
  real.partly_run = 8;

  RC_CHECK_NEAR(percent_covered(real), 89.0625, 1e-4);

  // The two wrong attempts, kept so the difference is visible.
  LineCoverage first_try = real;
  first_try.executable = 56;
  first_try.never_run = 5;
  LineCoverage second_try = real;
  second_try.executable = 56;

  std::cout << "  attempt                             executable  never run  covered\n";
  std::cout << "  ##### only                                  56          5   "
            << std::fixed << std::setprecision(2) << percent_covered(first_try) << "%\n";
  std::cout << "  ##### and =====                             56          7   "
            << percent_covered(second_try) << "%\n";
  std::cout << "  and digits* counted as code                 64          7   "
            << percent_covered(real) << "%\n";
  std::cout << "  gcov's own figure                           64          7   89.06%\n\n";
  std::cout << "  the third line is the one that agrees. Two of the three\n";
  std::cout << "  wrong answers were wrong in the flattering direction.\n\n";

  RC_CHECK(percent_covered(first_try) > percent_covered(real));
  RC_CHECK(percent_covered(second_try) < percent_covered(real));
}

RC_TEST("a hundred percent does not mean tested") {
  // The claim this lesson exists to make, as data.
  //
  // Measured outside this suite: a program calling five functions and
  // asserting nothing at all reported 100 percent of 5 lines, 0 never run,
  // while a deliberate defect in one of those functions, a > where the
  // intention was >=, survived untouched.
  LineCoverage asserts_nothing;
  asserts_nothing.ok = true;
  asserts_nothing.executable = 5;
  asserts_nothing.never_run = 0;

  RC_CHECK_NEAR(percent_covered(asserts_nothing), 100.0, 1e-9);

  // A file with no code is also 100 percent, which is true and useless.
  LineCoverage empty;
  empty.ok = true;
  RC_CHECK_NEAR(percent_covered(empty), 100.0, 1e-9);
  RC_CHECK(empty.executable == 0);

  std::cout << "  a suite that asserts nothing:   100.00% of 5 lines\n";
  std::cout << "  a file with no code in it:      100.00% of 0 lines\n\n";
  std::cout << "  coverage answers one question: did this line run. It does\n";
  std::cout << "  not ask whether anything checked the result, so it is a\n";
  std::cout << "  floor and not a score. A line that never ran certainly is\n";
  std::cout << "  not tested; a line that ran may or may not be.\n\n";
  std::cout << "  print the executable count beside the percentage, always.\n";
  std::cout << "  100% of 0 and 100% of 600 are the same number and not the\n";
  std::cout << "  same claim.\n\n";
}

RC_TEST("text that is not a gcov report says so rather than reporting zero") {
  const LineCoverage nothing = parse_gcov("");
  RC_CHECK(!nothing.ok);
  RC_CHECK(nothing.executable == 0);

  const LineCoverage prose = parse_gcov("this is not a coverage report at all\n");
  RC_CHECK(!prose.ok);

  // A report with only headers is valid gcov output about a file with no
  // code, which is a different answer from "this was not gcov output".
  const LineCoverage headers_only = parse_gcov("        -:    0:Source:/x.hpp\n");
  RC_CHECK(headers_only.ok);
  RC_CHECK(headers_only.executable == 0);

  std::cout << "  empty input and prose report ok=false; a header only report\n";
  std::cout << "  reports ok=true with nothing executable, because those are\n";
  std::cout << "  different facts and a tool that confused them would report\n";
  std::cout << "  perfect coverage of a file it had failed to read.\n\n";
}

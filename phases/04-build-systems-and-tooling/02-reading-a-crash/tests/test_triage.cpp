#include <rc/test/rc_test.hpp>

#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// Three crash reports captured while writing this lesson. The text is verbatim,
// apart from middle frames removed for length where marked. Using real output
// matters: a parser written against invented text works on invented text.

// A stack array read one past its end, under the address sanitizer.
const char* kStackOverflow = R"(=================================================================
==310960==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffd32512be0 at pc 0x602c848d92b8 bp 0x7ffd32512b70 sp 0x7ffd32512b60
READ of size 4 at 0x7ffd32512be0 thread T0
    #0 0x602c848d92b7 in sum_readings(int const*, int) /tmp/crash.cpp:4
    #1 0x602c848d945c in main /tmp/crash.cpp:9
    #2 0x77e262c29d8f in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #3 0x77e262c29e3f in __libc_start_main_impl ../csu/libc-start.c:392
    #4 0x602c848d9184 in _start (/tmp/crash+0x1184)
SUMMARY: AddressSanitizer: stack-buffer-overflow /tmp/crash.cpp:4 in sum_readings(int const*, int)
)";

// A comparator that reports an element as less than itself, so std::sort walks
// off the end of the range. Every frame but the last is inside the standard
// library. Frames 2 to 6 removed for length.
const char* kBadComparator = R"(==311259==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x511000000140 at pc 0x6308fea44b36 bp 0x7ffc49e3f1c0 sp 0x7ffc49e3f1b0
    #0 0x6308fea44b35 in bool __gnu_cxx::__ops::_Iter_comp_iter<bool (*)(int, int)>::operator()<__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >) /usr/include/c++/11/bits/predefined_ops.h:158
    #1 0x6308fea44fcb in __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > std::__unguarded_partition<__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__ops::_Iter_comp_iter<bool (*)(int, int)> >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__ops::_Iter_comp_iter<bool (*)(int, int)>) /usr/include/c++/11/bits/stl_algo.h:1884
    #7 0x6308fea41d8c in void std::sort<__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, bool (*)(int, int)>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, bool (*)(int, int)) /usr/include/c++/11/bits/stl_algo.h:4875
    #8 0x6308fea415a2 in main /tmp/badcomp.cpp:10
)";

// A debugger backtrace, which writes the location after the word at.
const char* kDebuggerBacktrace = R"(Program received signal SIGSEGV, Segmentation fault.
#0  0x000055555555527c in worse (a=..., b=...) at deep.cpp:7
#1  0x00005555555565b9 in __gnu_cxx::__ops::_Iter_comp_iter<bool (*)(Sample const&, Sample const&)>::operator()<__gnu_cxx::__normal_iterator<Sample*, std::vector<Sample, std::allocator<Sample> > >, __gnu_cxx::__normal_iterator<Sample*, std::vector<Sample, std::allocator<Sample> > > > (this=0x7fffffffd238, __it1={v = 1}, __it2={v = 3}) at /usr/include/c++/11/bits/predefined_ops.h:158
#6  0x000055555555536e in main () at deep.cpp:11
)";

}  // namespace

RC_TEST("frames are found in a sanitizer report") {
  const std::vector<Frame> frames = parse_frames(kStackOverflow);
  RC_REQUIRE_EQ(frames.size(), std::size_t{5});
  RC_CHECK_EQ(frames[0].index, 0);
  RC_CHECK_EQ(frames[1].index, 1);
}

RC_TEST("a sanitizer frame yields its file and line") {
  const std::vector<Frame> frames = parse_frames(kStackOverflow);
  RC_REQUIRE(!frames.empty());
  RC_CHECK_EQ(frames[0].file, std::string("/tmp/crash.cpp"));
  RC_CHECK_EQ(frames[0].line, 4);
  RC_CHECK_EQ(frames[0].function, std::string("sum_readings(int const*, int)"));
}

RC_TEST("frames are found in a debugger backtrace, where the file follows at") {
  const std::vector<Frame> frames = parse_frames(kDebuggerBacktrace);
  RC_REQUIRE_EQ(frames.size(), std::size_t{3});
  RC_CHECK_EQ(frames[0].file, std::string("deep.cpp"));
  RC_CHECK_EQ(frames[0].line, 7);
  RC_CHECK_EQ(frames[0].function, std::string("worse (a=..., b=...)"));
}

RC_TEST("lines that are not frames are ignored") {
  // The summary, the READ line and the banner must not become frames.
  const std::vector<Frame> frames = parse_frames(kStackOverflow);
  for (const Frame& frame : frames) RC_CHECK(!frame.function.empty());
  RC_CHECK_EQ(frames.size(), std::size_t{5});
}

RC_TEST("a frame with no source keeps its place") {
  // Frame 4 is _start, which has no file and line. Dropping it would renumber
  // everything after it and make the report harder to follow.
  const std::vector<Frame> frames = parse_frames(kStackOverflow);
  RC_REQUIRE_EQ(frames.size(), std::size_t{5});
  RC_CHECK_EQ(frames[4].index, 4);
  RC_CHECK(frames[4].file.empty());
  RC_CHECK_EQ(frames[4].line, 0);
}

RC_TEST("a report with no frames yields none") {
  RC_CHECK(parse_frames("just some text\nand another line\n").empty());
  RC_CHECK(parse_frames("").empty());
}

RC_TEST("standard library and C library frames are recognised as somebody else's") {
  Frame standard_library;
  standard_library.file = "/usr/include/c++/11/bits/stl_algo.h";
  RC_CHECK(is_system_frame(standard_library));

  Frame c_library;
  c_library.file = "../sysdeps/nptl/libc_start_call_main.h";
  RC_CHECK(is_system_frame(c_library));

  Frame no_source;
  RC_CHECK(is_system_frame(no_source));
}

RC_TEST("your own files are not system frames") {
  Frame mine;
  mine.file = "/tmp/crash.cpp";
  RC_CHECK(!is_system_frame(mine));

  Frame relative;
  relative.file = "deep.cpp";
  RC_CHECK(!is_system_frame(relative));
}

RC_TEST("the first own frame is frame zero when the crash is in your code") {
  const auto own = first_own_frame(parse_frames(kStackOverflow));
  RC_REQUIRE(own.has_value());
  RC_CHECK_EQ(own.value().index, 0);
  RC_CHECK_EQ(own.value().file, std::string("/tmp/crash.cpp"));
}

RC_TEST("the first own frame skips eight frames of standard library") {
  // The lesson in one check. The crash is inside std::sort, and the only line
  // anybody wrote is the call to it. Reading frame zero here sends you into the
  // standard library for an hour.
  const auto own = first_own_frame(parse_frames(kBadComparator));
  RC_REQUIRE(own.has_value());
  RC_CHECK_EQ(own.value().index, 8);
  RC_CHECK_EQ(own.value().file, std::string("/tmp/badcomp.cpp"));
  RC_CHECK_EQ(own.value().line, 10);
}

RC_TEST("a debugger backtrace finds its own frame too") {
  const auto own = first_own_frame(parse_frames(kDebuggerBacktrace));
  RC_REQUIRE(own.has_value());
  RC_CHECK_EQ(own.value().file, std::string("deep.cpp"));
  RC_CHECK_EQ(own.value().line, 7);
}

RC_TEST("an empty backtrace and an all system backtrace are different answers") {
  const auto none = first_own_frame({});
  RC_REQUIRE(!none.has_value());
  RC_CHECK(none.error() == TriageError::NoFrames);

  Frame only_system;
  only_system.file = "/usr/lib/libc.so";
  const auto foreign = first_own_frame({only_system});
  RC_REQUIRE(!foreign.has_value());
  RC_CHECK(foreign.error() == TriageError::NoOwnFrame);
}

RC_TEST("each report is classified from its first line") {
  RC_CHECK(classify(kStackOverflow) == CrashKind::StackBufferOverflow);
  RC_CHECK(classify(kBadComparator) == CrashKind::HeapBufferOverflow);
  RC_CHECK(classify(kDebuggerBacktrace) == CrashKind::SegmentationFault);
}

RC_TEST("the other kinds are recognised") {
  RC_CHECK(classify("ERROR: AddressSanitizer: heap-use-after-free on address 0x1") ==
           CrashKind::UseAfterFree);
  RC_CHECK(classify("Program received signal SIGFPE, Arithmetic exception.") ==
           CrashKind::ArithmeticFault);
  RC_CHECK(classify("==1==ERROR: LeakSanitizer: detected memory leaks") == CrashKind::MemoryLeak);
  RC_CHECK(classify("something else entirely") == CrashKind::Unknown);
}

RC_TEST("a use after free is not misread as an overflow") {
  // A use after free report also names a heap address, so checking the overflow
  // kinds first would classify it wrongly.
  const std::string report =
      "ERROR: AddressSanitizer: heap-use-after-free on address 0x60300000eff0\n"
      "freed by thread T0 here:\n";
  RC_CHECK(classify(report) == CrashKind::UseAfterFree);
}

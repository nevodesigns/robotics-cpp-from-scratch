// This must not compile.
//
// Writing to a stream hands back the stream, so that the next `<<` has
// something to attach to. That is what makes a chain of them work:
//
//   std::cerr << "cannot open " << path << "\n";
//
// Each `<<` returns the stream to the one after it. What it does not hand
// back is any indication of whether anything worked, so there is nothing
// sensible to turn into an exit status and the compiler refuses to invent one.
//
// This matters more than it looks. A stream that has failed, because the disk
// filled or the pipe at the other end closed, does not say so here and does
// not throw. It sets a flag on itself and silently discards everything
// afterwards. So a program whose only report is its printing can have printed
// nothing at all and still ended with the number 0.
//
// Two lines, as always. The message, then the status.
#include "solution.hpp"

#include <iostream>

int main() {
  return std::cerr << "cannot open the route file\n";
}

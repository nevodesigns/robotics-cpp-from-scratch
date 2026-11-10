// This must not compile.
//
// A class keeps its insides behind a unique_ptr to a type that is only named, and leaves the destructor to
// the compiler. The compiler writes it at the end of the class, where the type is still incomplete, and a
// unique_ptr cannot delete something whose size it does not know.
//
// The fix is one line: declare the destructor in the class and define it where the type is complete.
#include <memory>

class Hidden {
 public:
  Hidden();
  // no destructor declared: the compiler will write one here

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

int main() {
  Hidden h;      // needs the destructor, which needs to delete an Impl
  return 0;
}

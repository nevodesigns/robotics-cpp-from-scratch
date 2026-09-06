// rc/core/registry.hpp
//
// One thing per binary, however many translation units include the header that
// says so, from lesson 04-04.
//
// A header is copied into every file that includes it, so anything it defines
// is defined that many times, and the linker's rule is that one entity may have
// one definition. The three ways of satisfying it are not equivalent:
//
//   written as   what the program gets
//   nothing      two definitions, and the link fails
//   static       one per file, silently
//   inline       one, shared
//
// The middle row is the dangerous one, because it links. Measured on a counter
// bumped twice from one file and once from another: the first file sees 2, the
// second sees 1, three bumps happened, and no counter says three. Nothing
// failed, nothing warned, and both files are consistent with themselves.
//
// A function-local static inside an inline function is the header-only way to
// say "one of these". The inline makes the function one entity across the
// binary and the standard then guarantees its local static is one object,
// constructed once.

#ifndef RC_CORE_REGISTRY
#define RC_CORE_REGISTRY

#include <cstddef>
#include <cstring>
#include <vector>

namespace rc {
namespace core {

// One registry per binary, however many translation units include this header.
//
// A header is copied into every file that includes it, so anything it defines
// is defined that many times. The linker's rule is that one entity may have one
// definition, and the three ways of satisfying it produce three different
// things:
//
//   nothing        two definitions, and the link fails
//   static         one definition per translation unit, and no complaint
//   inline         one definition shared by all of them
//
// The middle one is the dangerous answer, because it links. A registry declared
// static in a header is a separate registry in every file that includes it, and
// each of them is perfectly consistent with itself.
class Registry {
 public:
  struct Entry {
    const char* name = nullptr;
    const double* value = nullptr;
  };

  // The one instance.
  //
  // A function-local static inside an inline function is the header-only way to
  // have exactly one of something. The inline makes the function one entity
  // across the whole binary, and the standard then guarantees that its local
  // static is one object, constructed once, however many translation units call
  // it.
  //
  // A namespace-scope variable in a header cannot do this: without inline it is
  // a definition per file and the link fails, and with static it is a separate
  // object per file, which is the fault this class exists to avoid.
  static Registry& instance() {
    static Registry only;
    return only;
  }

  void add(const char* name, const double* value) {
    if (name == nullptr || value == nullptr) return;
    entries_.push_back(Entry{name, value});
  }

  std::size_t size() const { return entries_.size(); }

  // The value registered under a name, or nothing.
  //
  // Compared by content rather than by pointer, because two translation units
  // that both write "wheel_base" are not required to produce the same address
  // for it.
  const double* find(const char* name) const {
    if (name == nullptr) return nullptr;
    for (const Entry& entry : entries_)
      if (std::strcmp(entry.name, name) == 0) return entry.value;
    return nullptr;
  }

  const std::vector<Entry>& entries() const { return entries_; }

 private:
  std::vector<Entry> entries_;
};

// Registers something the moment it is constructed, which is how a translation
// unit adds to the registry without anybody calling it.
class Registered {
 public:
  Registered(const char* name, const double* value) {
    Registry::instance().add(name, value);
  }
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_REGISTRY

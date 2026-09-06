#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstring>
#include <vector>

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

// The middle row of the table, put here to be measured rather than to be used.
//
// A function with internal linkage, defined in a header, is a separate function
// in every file that includes it, each with its own local static. Nothing
// complains, and the two files then disagree about a count they both believe
// they are keeping.
//
// This is what the registry above exists to avoid. It is here so the tests can
// show the difference, and it is the one thing in this curriculum written as an
// example of what not to do.
static int& per_file_counter() {
  static int count = 0;
  return count;
}

static int bump_per_file() { return ++per_file_counter(); }

static const void* address_of_bump_per_file() {
  return reinterpret_cast<const void*>(&bump_per_file);
}

#endif  // LESSON_SOLUTION_HPP

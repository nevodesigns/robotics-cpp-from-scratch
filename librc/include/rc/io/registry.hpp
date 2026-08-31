// rc/io/registry.hpp
//
// The device registry from lesson 02-04, graduated.
//
// A collection that owns what it holds. Ownership is moved in, moved out, and
// never shared, so at every moment exactly one place is responsible for
// destroying each entry and the question of who closes the port has one answer.
//
// The lesson's version read the identifier off the device, which meant every
// stored type had to have an id() member. Taking the identifier explicitly
// drops that requirement and lets a registry hold anything, which is what a
// driver collection eventually needs.

#ifndef RC_IO_REGISTRY_HPP
#define RC_IO_REGISTRY_HPP

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace rc {
namespace io {

template <class T>
class Registry {
 public:
  Registry() = default;

  // Said explicitly, even though a vector of unique_ptr cannot in fact be
  // copied. Without these lines std::is_copy_constructible still reports true
  // for this class, because std::vector declares its copy constructor
  // unconditionally and only fails when somebody instantiates it. Writing the
  // decision down makes the type honest about itself and moves the error from a
  // page of template output to one clear line.
  Registry(const Registry&) = delete;
  Registry& operator=(const Registry&) = delete;

  Registry(Registry&&) = default;
  Registry& operator=(Registry&&) = default;

  // Refusing a null keeps every later loop free of null checks. Decide once, at
  // the boundary, rather than everywhere afterwards.
  void add(int id, std::unique_ptr<T> item) {
    if (item == nullptr) return;
    entries_.push_back(Entry{id, std::move(item)});
  }

  // Hands ownership back to the caller. Returns null when nothing matches,
  // which is a question rather than a failure.
  std::unique_ptr<T> take(int id) {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      if (entries_[i].id != id) continue;

      // Move the ownership out before erasing, or erase destroys the very thing
      // being handed over.
      std::unique_ptr<T> taken = std::move(entries_[i].item);
      entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(i));
      return taken;
    }
    return nullptr;
  }

  // A borrowed pointer. The registry still owns it, and it dangles the moment
  // the entry is taken or the registry dies, which is why it is a raw pointer:
  // a raw pointer in this codebase means observing without owning, and that is
  // the whole of what it means.
  T* find(int id) {
    for (Entry& entry : entries_)
      if (entry.id == id) return entry.item.get();
    return nullptr;
  }

  const T* find(int id) const {
    for (const Entry& entry : entries_)
      if (entry.id == id) return entry.item.get();
    return nullptr;
  }

  int count() const { return static_cast<int>(entries_.size()); }
  bool empty() const { return entries_.empty(); }
  void clear() { entries_.clear(); }

 private:
  struct Entry {
    int id = 0;
    std::unique_ptr<T> item;
  };

  std::vector<Entry> entries_;
};

}  // namespace io
}  // namespace rc

#endif  // RC_IO_REGISTRY_HPP

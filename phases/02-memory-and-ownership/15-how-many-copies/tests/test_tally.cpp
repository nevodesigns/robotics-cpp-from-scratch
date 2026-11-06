#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>

#include "solution.hpp"

namespace {

// Exactly the Packet above except that the move constructor does not promise anything.
class Careless {
 public:
  explicit Careless(int id) : id_(id) { ++tally().made; }
  Careless(const Careless& other) : id_(other.id_) { ++tally().copies; }
  Careless(Careless&& other) : id_(other.id_) {
    other.id_ = -1;
    ++tally().moves;
  }
  Careless& operator=(const Careless& other) {
    id_ = other.id_;
    ++tally().copies;
    return *this;
  }
  Careless& operator=(Careless&& other) {
    id_ = other.id_;
    other.id_ = -1;
    ++tally().moves;
    return *this;
  }
  ~Careless() { ++tally().destroyed; }
  int id() const { return id_; }

 private:
  int id_;
};

}  // namespace

RC_TEST("a copy is counted as a copy, a move as a move, and the one moved from is emptied") {
  reset_tally();
  {
    Packet a(7);
    Packet b(a);
    Packet c(std::move(a));
    RC_CHECK_EQ(b.id(), 7);
    RC_CHECK_EQ(c.id(), 7);
    RC_CHECK_EQ(a.id(), -1);
    RC_CHECK_EQ(tally().made, 1);
    RC_CHECK_EQ(tally().copies, 1);
    RC_CHECK_EQ(tally().moves, 1);
  }
  RC_CHECK_EQ(tally().destroyed, 3);

  RC_CHECK(std::is_nothrow_move_constructible<Packet>::value);
  RC_CHECK(std::is_nothrow_move_assignable<Packet>::value);
}

RC_TEST("growing a vector moves its packets when the move is noexcept, and copies them when it is not") {
  const int n = 100;

  reset_tally();
  {
    std::vector<Packet> v;
    for (int i = 0; i < n; ++i) v.push_back(Packet(i));
    RC_CHECK_EQ(v.size(), static_cast<std::size_t>(n));
  }
  const Tally with_promise = tally();

  reset_tally();
  {
    std::vector<Careless> v;
    for (int i = 0; i < n; ++i) v.push_back(Careless(i));
    RC_CHECK_EQ(v.size(), static_cast<std::size_t>(n));
  }
  const Tally without = tally();

  std::cout << "  100 push_backs with no reserve:\n"
            << "    move marked noexcept   copies " << std::setw(4) << with_promise.copies << "   moves " << std::setw(4)
            << with_promise.moves << "\n"
            << "    move not marked        copies " << std::setw(4) << without.copies << "   moves " << std::setw(4)
            << without.moves << "\n\n";
  RC_CHECK_EQ(with_promise.copies, 0);
  RC_CHECK(with_promise.moves >= n);
  RC_CHECK(without.copies > n / 2);
  RC_CHECK(without.copies > 0);
  RC_CHECK_EQ(with_promise.destroyed, with_promise.made + with_promise.copies + with_promise.moves);
}

RC_TEST("reserving removes the moves of growth, and building in place removes all of them") {
  const int n = 100;

  reset_tally();
  const std::vector<Packet> plain = build_plain(n);
  const Tally t_plain = tally();

  reset_tally();
  const std::vector<Packet> reserved = build_reserved(n);
  const Tally t_reserved = tally();

  reset_tally();
  const std::vector<Packet> emplaced = build_emplaced(n);
  const Tally t_emplaced = tally();

  std::cout << "  filling a vector with " << n << " packets:\n"
            << "    no reserve            copies " << std::setw(3) << t_plain.copies << "   moves " << std::setw(4)
            << t_plain.moves << "\n"
            << "    reserve, push_back    copies " << std::setw(3) << t_reserved.copies << "   moves " << std::setw(4)
            << t_reserved.moves << "\n"
            << "    reserve, emplace_back copies " << std::setw(3) << t_emplaced.copies << "   moves " << std::setw(4)
            << t_emplaced.moves << "\n\n";

  RC_CHECK_EQ(plain.size(), static_cast<std::size_t>(n));
  RC_CHECK_EQ(reserved.size(), static_cast<std::size_t>(n));
  RC_CHECK_EQ(emplaced.size(), static_cast<std::size_t>(n));
  RC_CHECK_EQ(plain[99].id(), 99);
  RC_CHECK_EQ(reserved[50].id(), 50);
  RC_CHECK_EQ(emplaced[0].id(), 0);

  RC_CHECK_EQ(t_reserved.moves, n);       // one per push_back of a temporary, and none for growth
  RC_CHECK(t_plain.moves > t_reserved.moves);
  RC_CHECK_EQ(t_emplaced.copies, 0);
  RC_CHECK_EQ(t_emplaced.moves, 0);
  RC_CHECK_EQ(t_plain.copies, 0);
  RC_CHECK_EQ(t_reserved.copies, 0);
}

RC_TEST("passing a vector by value copies every element, by reference copies none, and std::move copies none") {
  const int n = 50;
  std::vector<Packet> v = build_emplaced(n);

  reset_tally();
  const long by_value = total_by_value(v);
  const Tally t_value = tally();

  reset_tally();
  const long by_ref = total_by_reference(v);
  const Tally t_ref = tally();

  reset_tally();
  const long by_move = total_by_value(std::move(v));
  const Tally t_move = tally();

  std::cout << "  a vector of " << n << " passed to a function:\n"
            << "    by value              copies " << std::setw(3) << t_value.copies << "\n"
            << "    by const reference    copies " << std::setw(3) << t_ref.copies << "\n"
            << "    by value, std::move   copies " << std::setw(3) << t_move.copies << "   moves " << t_move.moves
            << "\n\n";
  RC_CHECK_EQ(by_value, by_ref);
  RC_CHECK_EQ(by_ref, by_move);
  RC_CHECK_EQ(t_value.copies, n);
  RC_CHECK_EQ(t_ref.copies, 0);
  RC_CHECK_EQ(t_move.copies, 0);
  RC_CHECK_EQ(t_move.moves, 0);   // the vector took the other's buffer, not its elements one by one
}

RC_TEST("returning a vector by value moves or copies none of its packets") {
  reset_tally();
  const std::vector<Packet> v = build_emplaced(30);
  const Tally t = tally();
  std::cout << "  returned a vector of 30 from a function: copies " << t.copies << ", moves " << t.moves << "\n\n";
  RC_CHECK_EQ(v.size(), static_cast<std::size_t>(30));
  RC_CHECK_EQ(t.made, 30);
  RC_CHECK_EQ(t.copies, 0);
  RC_CHECK_EQ(t.moves, 0);
}

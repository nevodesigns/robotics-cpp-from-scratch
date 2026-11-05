#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// The word static has been in the code since lesson 02-03, where
//
//   static int next_id = 1;
//   const int id = next_id++;
//
// hands out a different number each time and nothing said how. It appears in a
// dozen lessons after that, and it means three different things depending on
// where it is written. Lesson 04-04 covers one of them, the file-scope one. This
// covers the other two, because they are the ones that surprise.
//
// Everything below is measured, on gcc 11 and clang 14.
//
// Inside a function, static makes the variable outlive the call. It is built
// the first time control reaches it, once, and it keeps its value from then on.
// Three calls to next_id() gave consecutive numbers. The price is that the
// function now has a memory: what it returns depends on every call before it,
// and a test cannot start it from zero. Lesson 05-06 is about exactly that, in
// the other direction.
//
// When it is built matters. A global object was built before main began. Two
// static locals reached in the order B, A were built in that order, each when its
// function was first called, and a second call built nothing. They were
// destroyed after main returned, in the reverse of the order they were built, and
// the global last. A sensor held in a static local was built 0 times before the
// first call, 1 after it, and 1 after ten more, where the same function without
// static built one per call.
//
// Inside a class, static makes the member belong to the class and not to any
// object. A Wheel with a static count was 8 bytes, the size of its one double,
// and the same wheel with the count kept in each object was 16: a static member
// is not part of the object. A static member function can be called with no
// object at all, Wheel::count(), so it has no this, and using a member that
// belongs to an object inside one is refused: "invalid use of member in static
// member function".
//
// A static data member must be defined exactly once, and declaring it is not
// defining it. A class with static int count_; and nothing else compiled, and the
// program failed to link: undefined reference to W::count_. In C++17,
// inline static int count_ = 0; declares and defines it in one line, which is the
// form used here. That inline is the one from lesson 00-02.
//
// A counter of live objects has to count copies. A copy is a new object that the
// ordinary constructor never saw, so a class that increments only there, and
// decrements in the destructor, drifts downward by one for every copy made. The
// suite makes one copy and checks.
//
// The third meaning, a name at file scope visible to that file only, is lesson
// 04-04. The rule that falls out: static in a function is a variable with a
// memory, static in a class is shared by every object, and both are state that
// the function or class does not own the whole life of.

// A sensor that counts how many have ever been built, so that a test can tell
// when a static local was constructed.
inline int& sensor_constructions() {
  static int built = 0;
  return built;
}

class Sensor {
 public:
  Sensor() : id_(++sensor_constructions()) {}
  int id() const { return id_; }

 private:
  int id_;
};

// TODO 1: a different number every time it is asked, starting at 1.
//
// Keep the counter in a static local, so it is built the first time the function
// runs and keeps its value between calls:
//
//   static int next = 1;
//   return next++;
//
// The stub answers zero every time, which is a function with no memory.
inline int next_id() { return 0; }

// TODO 2: the id of the one sensor this program shares, built the first time
// anyone asks and never again.
//
// The stub builds a sensor on every call, which is correct-looking and gives a
// different id each time. Make the sensor a static local. The suite counts how
// many were built: none before the first call, one after it, one after ten more.
inline int shared_sensor_id() {
  Sensor sensor;
  return sensor.id();
}

// TODO 3: a wheel that knows how many wheels there are.
//
//   - A static data member that holds the count, defined where it is declared:
//     inline static int live_ = 0;
//   - The constructor adds one, and the destructor takes one away.
//   - The copy constructor adds one too. A copy is a new wheel that the ordinary
//     constructor never saw, and the destructor will still count it out.
//   - count() is a static function, callable as Wheel::count(), that returns it.
//
// The stub keeps no count at all.
class Wheel {
 public:
  explicit Wheel(double radius) : radius_(radius) {}

  double radius() const { return radius_; }

  static int count() { return 0; }

 private:
  double radius_;
};

#endif  // LESSON_SOLUTION_HPP

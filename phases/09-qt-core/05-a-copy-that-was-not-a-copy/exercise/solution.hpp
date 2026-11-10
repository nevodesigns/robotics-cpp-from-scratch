#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QByteArray>

#include <utility>

// Qt's containers and strings share. Copying a QByteArray copies a pointer and bumps a count, and the bytes are copied only when
// somebody WRITES to one of the copies, at which point that copy takes its own (it detaches). A copy is cheap, until it is not.
//
// The trap is what counts as writing. Anything that could write, on an object that is not const, detaches first, because Qt
// cannot know you will only read: operator[] on a non-const array, data(), and the begin() that a range-for calls. A loop that only reads can
// copy a megabyte on its first step.

// The sum of the bytes, read without any chance of a copy. at() cannot write, and the array is const.
inline int sum_const(const QByteArray& frame) {
  // TODO: add up the bytes as unsigned values, reading each with frame.at(i). The array is const and at() cannot
  // write, so nothing here can make a copy.
  (void)frame;
  return -1;
}

// The same for an array that arrives by value, which is a shared copy of the caller's, and looped over with a range-for.
//
// A range-for over a non-const array calls the non-const begin(), which detaches: a whole copy of the bytes to read them. std::as_const
// gives the loop a const view. `*detached` reports whether this function ended up with bytes of its own: the pointer to the
// bytes is no longer the caller's.
inline int sum_range_for(QByteArray frame, const char* original, bool* detached) {
  // TODO: add up the bytes with a range-for, but over std::as_const(frame), so the loop uses the const begin() and does not
  // detach. Then set *detached to whether frame.constData() is no longer the same pointer as `original`.
  (void)frame;
  (void)original;
  *detached = true;
  return -1;
}

// Put a marker in the first byte of a frame and give it back.
//
// This one does write, so one copy is unavoidable, and the aim is exactly one: it takes the frame by value so that the caller's
// own is untouched, and writes once.
inline QByteArray with_marker(QByteArray frame, char marker) {
  // TODO: if the frame is not empty, set its first byte to marker (frame[0] = marker: this one is meant to write). Return the frame.
  (void)marker;
  return frame;
}

#endif  // LESSON_SOLUTION_HPP

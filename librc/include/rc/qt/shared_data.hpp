// rc/qt/shared_data.hpp
//
// Reading a shared Qt buffer without copying it, from lesson 09-05, graduated.
//
// A copy of a QByteArray copies a pointer and a count, and the bytes are copied only when one of the copies is written to. Anything that could write detaches first, on an object that is
// not const: measured on Qt 6.2, b[0], b.data() and the begin() of a range-for each copied the bytes of a copy of a 1000 byte array while only looking, and b.at(0), b.constData() and
// b.size() did not. A range-for over a non-const array copied 1000 of 1000 frames of 4096 bytes, and over std::as_const copied none. A function taking an array by value and looping over a const view of it kept the caller's bytes.

#ifndef RC_QT_SHARED_DATA
#define RC_QT_SHARED_DATA

#include <QtCore/QByteArray>

#include <utility>

namespace rc {
namespace qt {

// Qt's containers and strings share. Copying a QByteArray copies a pointer and bumps a count, and the bytes are copied only when
// somebody WRITES to one of the copies, at which point that copy takes its own (it detaches). A copy is cheap, until it is not.
//
// The trap is what counts as writing. Anything that could write, on an object that is not const, detaches first, because Qt
// cannot know you will only read: operator[] on a non-const array, data(), and the begin() that a range-for calls. A loop that only reads can
// copy a megabyte on its first step.

// The sum of the bytes, read without any chance of a copy. at() cannot write, and the array is const.
inline int sum_const(const QByteArray& frame) {
  int total = 0;
  for (int i = 0; i < frame.size(); ++i) total += static_cast<unsigned char>(frame.at(i));
  return total;
}

// The same for an array that arrives by value, which is a shared copy of the caller's, and looped over with a range-for.
//
// A range-for over a non-const array calls the non-const begin(), which detaches: a whole copy of the bytes to read them. std::as_const
// gives the loop a const view. `*detached` reports whether this function ended up with bytes of its own: the pointer to the
// bytes is no longer the caller's.
inline int sum_range_for(QByteArray frame, const char* original, bool* detached) {
  int total = 0;
  for (char c : std::as_const(frame)) total += static_cast<unsigned char>(c);
  *detached = frame.constData() != original;
  return total;
}

// Put a marker in the first byte of a frame and give it back.
//
// This one does write, so one copy is unavoidable, and the aim is exactly one: it takes the frame by value so that the caller's
// own is untouched, and writes once.
inline QByteArray with_marker(QByteArray frame, char marker) {
  if (!frame.isEmpty()) frame[0] = marker;
  return frame;
}

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_SHARED_DATA

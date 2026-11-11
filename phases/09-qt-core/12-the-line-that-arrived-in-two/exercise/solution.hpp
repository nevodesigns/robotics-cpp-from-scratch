#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QByteArray>
#include <QList>

// A device does not hand over lines. It hands over whatever has arrived: half a line, three lines and the start of a fourth, a single byte.
// A reader that treats each chunk as a message splits a line in two, and both halves fail to parse. A line ending written by another
// system may be \r\n and leave a \r on the end of every line. And a line that never ends, because a cable was pulled mid-message or a
// port is noise, makes the buffer grow until the program does not.
//
// LineAssembler keeps what has not ended yet and gives back each line as soon as it is complete:
//   - a line ends at \n, and a \r just before it is removed
//   - an empty line is a line
//   - a line of more than max_line_bytes before its \n is dropped whole, counted, and not held: the assembler stops keeping bytes
//     until the \n that ends it, so its memory is bounded by max_line_bytes however much noise arrives

class LineAssembler {
 public:
  explicit LineAssembler(int max_line_bytes) : max_(max_line_bytes) {}

  // Takes the next chunk, in any size, and returns the lines that it completed, in order.
  QList<QByteArray> feed(const QByteArray& chunk) {
    // TODO: for each byte c of the chunk:
    //  - c is '\n': if skipping_, set it false and ++dropped_ (the long line has ended). Otherwise, if line_ ends with '\r' chop it off
    //    (line_.chop(1)), and append line_ to the list, empty or not. Either way line_.clear().
    //  - anything else, when not skipping_: line_.append(c); if line_.size() > max_, set skipping_ true and clear line_ (it is too long, and
    //    is no longer kept).
    // Return the list of lines that were completed by this chunk.
    (void)chunk;
    (void)max_;
    (void)line_;
    (void)skipping_;
    (void)dropped_;
    return QList<QByteArray>();
  }

  // Bytes held for a line that has not ended. Never more than max_line_bytes.
  int pending() const { return line_.size(); }
  // Lines that were too long and were dropped, counted when their \n arrived.
  long long dropped() const { return dropped_; }
  // True while the assembler is discarding the rest of a line that was too long.
  bool skipping() const { return skipping_; }

 private:
  int max_;
  QByteArray line_;
  bool skipping_ = false;
  long long dropped_ = 0;
};

#endif  // LESSON_SOLUTION_HPP

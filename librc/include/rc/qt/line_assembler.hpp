// rc/qt/line_assembler.hpp
//
// Complete lines from chunks that split them anywhere, from lesson 09-12, graduated.
//
// 33 bytes of two lines read 7 at a time were 3 chunks, and splitting each chunk gave 4 pieces for 2 lines. The assembler gave the same lines for all 629 ways of cutting those bytes into two or three
// chunks and for one byte at a time, removed a \r before the \n and kept one inside a line, and with a limit of 8 bytes held 0 of 100000 bytes of noise with no newline.

#ifndef RC_QT_LINE_ASSEMBLER
#define RC_QT_LINE_ASSEMBLER

#include <QByteArray>
#include <QList>

namespace rc {
namespace qt {

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
    QList<QByteArray> lines;
    for (int i = 0; i < chunk.size(); ++i) {
      const char c = chunk.at(i);
      if (c == '\n') {
        if (skipping_) {
          skipping_ = false;
          ++dropped_;
        } else {
          if (line_.endsWith('\r')) line_.chop(1);
          lines.append(line_);
        }
        line_.clear();
      } else if (!skipping_) {
        line_.append(c);
        if (line_.size() > max_) {
          skipping_ = true;           // too long: stop keeping it, and say so when its end arrives
          line_.clear();
        }
      }
    }
    return lines;
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

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_LINE_ASSEMBLER

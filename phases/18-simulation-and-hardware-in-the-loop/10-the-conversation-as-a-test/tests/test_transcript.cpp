#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

using Bytes = std::vector<std::uint8_t>;

// A small device. The host sends 01, a sequence number; the device answers 81, the same number, a four byte time of day, a two byte reading.
//   request:  [01][seq]                          2 bytes
//   reply:    [81][seq][t0 t1 t2 t3][v0 v1]      8 bytes, the time at offsets 2 to 5 and the reading at 6 and 7
struct Device {
  std::uint32_t clock;
  int bug_on_request = -1;       // a reading that is one too many on this request
  bool extra_byte = false;       // a newer firmware that adds a byte to every reply

  Bytes reply(int seq) {
    Bytes r;
    r.push_back(0x81);
    r.push_back(static_cast<std::uint8_t>(seq));
    clock += 17;                                           // time goes by differently on every run
    for (int i = 0; i < 4; ++i) r.push_back(static_cast<std::uint8_t>((clock >> (8 * i)) & 0xFF));
    int reading = 100 + 3 * seq + (seq == bug_on_request ? 1 : 0);
    r.push_back(static_cast<std::uint8_t>(reading & 0xFF));
    r.push_back(static_cast<std::uint8_t>(reading >> 8));
    if (extra_byte) r.push_back(0xEE);
    return r;
  }
};

Transcript session(std::uint32_t start_clock, int requests, int bug_on_request, bool extra_byte) {
  Device d{start_clock, bug_on_request, extra_byte};
  Transcript t;
  for (int seq = 1; seq <= requests; ++seq) {
    t.add(Direction::to_device, Bytes{0x01, static_cast<std::uint8_t>(seq)});
    t.add(Direction::from_device, d.reply(seq));
  }
  return t;
}

// The reply's time is at offsets 2 to 5, and may differ between runs.
std::vector<Mask> time_of_day() { return {Mask{Direction::from_device, 2, 4}}; }

}  // namespace

RC_TEST("a transcript is written one message to a line, an arrow for the direction and the bytes in hexadecimal, and reads back as it was") {
  Transcript t;
  t.add(Direction::to_device, Bytes{0x01, 0x02});
  t.add(Direction::from_device, Bytes{0x81, 0x02, 0xAB, 0x00, 0xFF});
  t.add(Direction::to_device, Bytes{});
  const std::string text = t.to_text();
  std::cout << "  as text:\n" << text << "\n";
  RC_CHECK(text == "> 01 02\n< 81 02 AB 00 FF\n>\n");
  Transcript back;
  std::string error;
  RC_CHECK(Transcript::from_text(text, back, error));
  RC_CHECK(back.size() == 3);
  if (back.size() != 3) return;
  RC_CHECK(back.at(0).dir == Direction::to_device && back.at(0).bytes == (Bytes{0x01, 0x02}));
  RC_CHECK(back.at(1).dir == Direction::from_device && back.at(1).bytes == (Bytes{0x81, 0x02, 0xAB, 0x00, 0xFF}));
  RC_CHECK(back.at(2).bytes.empty());
  RC_CHECK(back.to_text() == text);
}

RC_TEST("a hand edited transcript may have comments, blank lines and lower case, and a bad line is reported with its number and leaves nothing behind") {
  Transcript t;
  std::string error;
  RC_CHECK(Transcript::from_text("# the first request\n\n> 01 0a\n  < 81 0A  ff \n", t, error));
  RC_CHECK(t.size() == 2);
  RC_CHECK(t.size() == 2 && t.at(0).bytes == (Bytes{0x01, 0x0A}) && t.at(1).bytes == (Bytes{0x81, 0x0A, 0xFF}));

  Transcript bad;
  bad.add(Direction::to_device, Bytes{1});                                // something left from before
  const std::string bad_arrow = "> 01\n? 02\n";
  const std::string odd = "> 01\n> 02 3\n";
  const std::string not_hex = "> 01\n< 0G\n";
  const std::string glued = "> 0102\n";
  std::string e1, e2, e3, e4;
  RC_CHECK(!Transcript::from_text(bad_arrow, bad, e1));
  RC_CHECK(bad.size() == 0);
  RC_CHECK(!Transcript::from_text(odd, bad, e2));
  RC_CHECK(!Transcript::from_text(not_hex, bad, e3));
  RC_CHECK(!Transcript::from_text(glued, bad, e4));
  std::cout << "  bad arrow: " << e1 << "\n  half a byte: " << e2 << "\n  not hexadecimal: " << e3 << "\n  two bytes with no space: " << e4 << "\n\n";
  RC_CHECK(e1 == "line 2: a line starts with > or <");
  RC_CHECK(e2 == "line 2: bytes are two hexadecimal digits each");
  RC_CHECK(e3 == "line 2: bytes are two hexadecimal digits each");
  RC_CHECK(e4 == "line 1: bytes are two hexadecimal digits each");
  RC_CHECK(bad.size() == 0);
}

RC_TEST("two sessions that say the same thing have no difference, and the first one that does is reported with where") {
  const Transcript golden = session(1000, 3, -1, false);
  RC_CHECK(compare(golden, golden, {}).kind == Difference::Kind::none);

  Transcript changed = golden;                                           // one byte of the reading in the second reply
  Transcript t;
  for (std::size_t i = 0; i < golden.size(); ++i) {
    Bytes b = golden.at(i).bytes;
    if (i == 3) b[6] ^= 0x01;
    t.add(golden.at(i).dir, b);
  }
  const Difference content = compare(golden, t, {});
  RC_CHECK(content.kind == Difference::Kind::content);
  RC_CHECK(content.message == 3);
  RC_CHECK(content.offset == 6);

  Transcript flipped;
  for (std::size_t i = 0; i < golden.size(); ++i) flipped.add(i == 2 ? Direction::from_device : golden.at(i).dir, golden.at(i).bytes);
  const Difference dir = compare(golden, flipped, {});
  RC_CHECK(dir.kind == Difference::Kind::direction && dir.message == 2);

  Transcript longer = golden;
  longer.add(Direction::to_device, Bytes{0x01, 0x04});
  const Difference extra = compare(golden, longer, {});
  RC_CHECK(extra.kind == Difference::Kind::extra && extra.message == 6);
  Transcript shorter;
  for (std::size_t i = 0; i < 4; ++i) shorter.add(golden.at(i).dir, golden.at(i).bytes);
  const Difference missing = compare(golden, shorter, {});
  RC_CHECK(missing.kind == Difference::Kind::missing && missing.message == 4);
  std::cout << "  a byte changed in message 3: content at message " << content.message << " byte " << content.offset << "; message 2 sent the other way: direction at " << dir.message
            << "; one more message: extra from " << extra.message << "; two fewer: missing from " << missing.message << "\n\n";
  (void)changed;
}

RC_TEST("a mask lets the part that should differ differ, in the direction it was given for, and only there, and a change next to it is still caught") {
  const Transcript a = session(1000, 3, -1, false);
  const Transcript b = session(70000, 3, -1, false);                   // the same conversation at a different time of day
  const Difference unmasked = compare(a, b, {});
  const Difference masked_run = compare(a, b, time_of_day());
  std::cout << "  the same session at two times of day: without a mask the first difference is message " << unmasked.message << " byte " << unmasked.offset << "; with the time masked: "
            << (masked_run.kind == Difference::Kind::none ? "none" : "a difference") << "\n";
  RC_CHECK(unmasked.kind == Difference::Kind::content);
  RC_CHECK(unmasked.message == 1 && unmasked.offset == 2);
  RC_CHECK(masked_run.kind == Difference::Kind::none);

  // The same bytes of a message the other way are not covered.
  Transcript c;
  for (std::size_t i = 0; i < a.size(); ++i) {
    Bytes bytes = a.at(i).bytes;
    if (i == 0) bytes[1] ^= 0x40;                                      // the host's request, offset 1, is not masked
    c.add(a.at(i).dir, bytes);
  }
  RC_CHECK(compare(a, c, time_of_day()).kind == Difference::Kind::content);

  // A mask over bytes 2 to 5 does not hide byte 6 or byte 1.
  Transcript d;
  for (std::size_t i = 0; i < a.size(); ++i) {
    Bytes bytes = a.at(i).bytes;
    if (i == 1) bytes[6] += 1;                                          // the reading, just after the time
    d.add(a.at(i).dir, bytes);
  }
  const Difference next_to_it = compare(a, d, time_of_day());
  std::cout << "  a change to the reading, one byte past the masked time: content at message " << next_to_it.message << " byte " << next_to_it.offset << "\n\n";
  RC_CHECK(next_to_it.kind == Difference::Kind::content && next_to_it.message == 1 && next_to_it.offset == 6);
}

RC_TEST("a device that changes its replies is caught by a transcript recorded before, at the message and byte where it first differs") {
  const Transcript golden = session(1000, 5, -1, false);
  const Transcript newer = session(5000, 5, -1, true);                 // new firmware: every reply has a byte more
  const Transcript buggy = session(9000, 5, 3, false);                 // a reading one too high on the third request
  const Difference longer = compare(golden, newer, time_of_day());
  const Difference off = compare(golden, buggy, time_of_day());
  std::cout << "  new firmware that adds a byte: " << (longer.kind == Difference::Kind::length ? "length" : "other") << " at message " << longer.message << ", offset " << longer.offset
            << "\n  a reading one too high on request 3: " << (off.kind == Difference::Kind::content ? "content" : "other") << " at message " << off.message << ", byte " << off.offset << "\n\n";
  RC_CHECK(longer.kind == Difference::Kind::length);
  RC_CHECK(longer.message == 1 && longer.offset == 8);
  RC_CHECK(off.kind == Difference::Kind::content);
  RC_CHECK(off.message == 5 && off.offset == 6);
  RC_CHECK(compare(golden, session(123456, 5, -1, false), time_of_day()).kind == Difference::Kind::none);
}

RC_TEST("a golden transcript is a file a person can read and review: the whole of a session fits on a screen, and a change shows as a line that differs") {
  const std::string a = session(1000, 3, -1, false).to_text();
  const std::string b = session(1000, 3, 2, false).to_text();
  // Find the lines that differ between the two texts.
  std::vector<std::string> la;
  std::vector<std::string> lb;
  for (const std::string* s : {&a, &b}) {
    std::vector<std::string>& out = s == &a ? la : lb;
    std::size_t start = 0;
    while (start < s->size()) {
      const std::size_t stop = s->find('\n', start);
      out.push_back(s->substr(start, stop - start));
      start = stop + 1;
    }
  }
  int differing = 0;
  std::string first_a;
  std::string first_b;
  for (std::size_t i = 0; i < la.size() && i < lb.size(); ++i)
    if (la[i] != lb[i]) {
      if (differing == 0) {
        first_a = la[i];
        first_b = lb[i];
      }
      ++differing;
    }
  std::cout << "  a session of 3 requests: " << la.size() << " lines; with a bug in the reading of request 2, " << differing << " line differs:\n    golden: " << first_a << "\n    now:    " << first_b
            << "\n\n";
  RC_CHECK(la.size() == 6);
  RC_CHECK(differing == 1);
  RC_CHECK(first_a != first_b);
}

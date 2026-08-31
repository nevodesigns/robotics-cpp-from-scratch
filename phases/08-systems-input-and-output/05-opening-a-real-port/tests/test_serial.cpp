#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/link.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

#if !defined(_WIN32)
#  include <cstdlib>
#  include <fcntl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace {

constexpr int kBaud = 115200;

// Every byte the terminal line discipline treats as something other than data:
// XON, XOFF, interrupt, suspend, end of file, carriage return, newline, erase.
const std::vector<std::uint8_t>& dangerous_bytes() {
  static const std::vector<std::uint8_t> bytes{
      0xAA, 0x04, 0x03, 0x11, 0x13, 0x0D, 0x0A, 0x1A, 0x7F, 0x00, 0xFF, 0xC4};
  return bytes;
}

// Bytes as a reader of a datasheet would write them.
std::string as_hex(const std::vector<std::uint8_t>& bytes) {
  std::ostringstream out;
  for (const std::uint8_t byte : bytes)
    out << ' ' << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(byte);
  return out.str();
}

}  // namespace

// ---------------------------------------------------------------------------
// Tests that need no port at all, and run everywhere.
// ---------------------------------------------------------------------------

RC_TEST("a port that has not been opened is closed, and says so") {
  SerialPort port;
  RC_CHECK(!port.is_open());
}

RC_TEST("reading or writing a closed port is an error, not silence") {
  // Silence would be indistinguishable from an idle link, and a program would
  // wait for ever on a port it never opened.
  SerialPort port;
  std::uint8_t buffer[8] = {};
  RC_CHECK(port.read(rc::span<std::uint8_t>(buffer, sizeof(buffer))) < 0);
  RC_CHECK(port.write(rc::span<const std::uint8_t>(buffer, sizeof(buffer))) < 0);
}

RC_TEST("closing twice is safe, and so is closing something never opened") {
  SerialPort port;
  port.close();
  port.close();
  RC_CHECK(!port.is_open());
}

RC_TEST("a port cannot be copied") {
  // Two objects holding one descriptor would each close it, and the second
  // close can shut a port something else has since opened.
  RC_CHECK(!std::is_copy_constructible<SerialPort>::value);
  RC_CHECK(!std::is_copy_assignable<SerialPort>::value);
}

RC_TEST("a device that does not exist is reported as not found") {
#if defined(_WIN32)
  const std::string missing = "COM255";
#else
  const std::string missing = "/dev/rc-no-such-serial-device";
#endif
  SerialPort port;
  RC_CHECK(port.open(missing, kBaud) == SerialError::NotFound);
  RC_CHECK(!port.is_open());
}

#if !defined(_WIN32)

// ---------------------------------------------------------------------------
// A pseudo terminal is a real terminal, configured by the same termios calls a
// serial port is configured by, so the whole implementation runs against it.
// ---------------------------------------------------------------------------

namespace {

class Pty {
 public:
  Pty() {
    master_ = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master_ < 0) return;
    if (::grantpt(master_) != 0 || ::unlockpt(master_) != 0) { give_up(); return; }

    // The master has to be non blocking too. Reading a blocking master after
    // its data has run out waits for a device that does not exist, and the
    // test suite hangs rather than failing, which is the worst way for a test
    // to be wrong.
    const int flags = ::fcntl(master_, F_GETFL, 0);
    if (flags < 0 || ::fcntl(master_, F_SETFL, flags | O_NONBLOCK) != 0) { give_up(); return; }
    const char* name = ::ptsname(master_);
    if (name == nullptr) { give_up(); return; }
    slave_name_ = name;
  }

  ~Pty() { if (master_ >= 0) ::close(master_); }

  Pty(const Pty&) = delete;
  Pty& operator=(const Pty&) = delete;

  bool ok() const { return master_ >= 0 && !slave_name_.empty(); }
  const std::string& slave_name() const { return slave_name_; }

  // What the device at the other end sends.
  void send(const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) return;
    const ssize_t written = ::write(master_, bytes.data(), bytes.size());
    (void)written;
  }

  // What the device at the other end received. Stops as soon as the expected
  // number of bytes has arrived, so a passing test costs one settle rather
  // than twenty.
  std::vector<std::uint8_t> receive(std::size_t expected) {
    std::vector<std::uint8_t> out;
    for (int attempt = 0; attempt < 30 && out.size() < expected; ++attempt) {
      std::uint8_t buffer[256];
      const ssize_t count = ::read(master_, buffer, sizeof(buffer));
      if (count > 0) for (ssize_t i = 0; i < count; ++i) out.push_back(buffer[i]);
      else settle();
    }
    return out;
  }

  // Serial data is not instant. Two milliseconds is far longer than a kernel
  // needs to move bytes between the two ends of a pseudo terminal, and short
  // enough that the whole suite stays quick.
  static void settle() { ::usleep(2000); }

 private:
  void give_up() { ::close(master_); master_ = -1; }

  int master_ = -1;
  std::string slave_name_;
};

// Reads until the expected number of bytes has arrived or the attempts run out.
std::vector<std::uint8_t> drain(SerialPort& port, std::size_t expected) {
  std::vector<std::uint8_t> out;
  for (int attempt = 0; attempt < 30 && out.size() < expected; ++attempt) {
    std::uint8_t buffer[256];
    const int count = port.read(rc::span<std::uint8_t>(buffer, sizeof(buffer)));
    if (count > 0) for (int i = 0; i < count; ++i) out.push_back(buffer[i]);
    else Pty::settle();
  }
  return out;
}

}  // namespace

RC_TEST("a pseudo terminal opens and configures like the serial port it stands in for") {
  Pty pty;
  RC_REQUIRE(pty.ok());

  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);
  RC_CHECK(port.is_open());
}

RC_TEST("a rate with no constant for it is refused rather than approximated") {
  Pty pty;
  RC_REQUIRE(pty.ok());

  SerialPort port;
  RC_CHECK(port.open(pty.slave_name(), 12345) == SerialError::BadBaud);
  RC_CHECK(!port.is_open());
}

RC_TEST("a path that exists and is not a terminal is reported as such") {
  // A regular file opens perfectly well, which is why the check has to be
  // explicit. It has to be a file the test can write, or the open fails on
  // permissions first and never reaches the question.
  const char* tmp = std::getenv("TMPDIR");
  const std::string path = std::string(tmp != nullptr ? tmp : "/tmp") + "/rc-08-05-not-a-tty";

  const int scratch = ::open(path.c_str(), O_RDWR | O_CREAT, 0600);
  RC_REQUIRE(scratch >= 0);
  ::close(scratch);

  SerialPort port;
  RC_CHECK(port.open(path, kBaud) == SerialError::NotATerminal);
  RC_CHECK(!port.is_open());
  ::unlink(path.c_str());
}

RC_TEST("an idle port reads zero, not an error") {
  // The contract Link depends on. A port that reports an error when nothing
  // has arrived gives a link that declares itself broken on an idle bus.
  Pty pty;
  RC_REQUIRE(pty.ok());
  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);

  std::uint8_t buffer[64];
  for (int i = 0; i < 20; ++i)
    RC_CHECK_EQ(port.read(rc::span<std::uint8_t>(buffer, sizeof(buffer))), 0);
}

RC_TEST("every byte the line discipline would have eaten arrives intact") {
  // The check that catches a port left in the terminal's default mode.
  Pty pty;
  RC_REQUIRE(pty.ok());
  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);

  const std::vector<std::uint8_t>& sent = dangerous_bytes();
  pty.send(sent);

  const std::vector<std::uint8_t> got = drain(port, sent.size());
  RC_REQUIRE_EQ(got.size(), sent.size());
  for (std::size_t i = 0; i < sent.size(); ++i)
    RC_CHECK_EQ(static_cast<int>(got[i]), static_cast<int>(sent[i]));
}

namespace {

// Opens the same device the SerialPort would, and deliberately does not
// configure it, which is the state an open without cfmakeraw leaves a port in.
int open_cooked(const std::string& device) {
  const int descriptor = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (descriptor < 0) return -1;
  termios defaults{};
  if (::tcgetattr(descriptor, &defaults) != 0) { ::close(descriptor); return -1; }
  if (::tcsetattr(descriptor, TCSANOW, &defaults) != 0) { ::close(descriptor); return -1; }
  return descriptor;
}

std::vector<std::uint8_t> read_for_a_while(int descriptor) {
  std::vector<std::uint8_t> out;
  for (int attempt = 0; attempt < 25; ++attempt) {
    std::uint8_t buffer[64];
    const ssize_t count = ::read(descriptor, buffer, sizeof(buffer));
    if (count > 0) for (ssize_t i = 0; i < count; ++i) out.push_back(buffer[i]);
    Pty::settle();
  }
  return out;
}

}  // namespace

RC_TEST("a port left at its defaults delivers nothing until a line ends") {
  // Not an assertion about the implementation: a measurement of what it is
  // avoiding. Canonical mode exists to let a person edit a line before the
  // program sees it, so a payload with no newline in it is held indefinitely.
  Pty pty;
  RC_REQUIRE(pty.ok());
  const int cooked = open_cooked(pty.slave_name());
  RC_REQUIRE(cooked >= 0);

  pty.send({0x68, 0x69});
  const std::vector<std::uint8_t> got = read_for_a_while(cooked);

  RC_CHECK_EQ(got.size(), static_cast<std::size_t>(0));
  ::close(cooked);
}

RC_TEST("a port left at its defaults eats the bytes it takes for flow control") {
  // A separate port from the previous test on purpose. Canonical mode holds a
  // partial line and delivers it joined to the next one, so measuring two
  // things through one port measures neither of them cleanly.
  Pty pty;
  RC_REQUIRE(pty.ok());
  const int cooked = open_cooked(pty.slave_name());
  RC_REQUIRE(cooked >= 0);

  pty.send({0x68, 0x11, 0x69, 0x0A});   // 0x11 is XON
  const std::vector<std::uint8_t> got = read_for_a_while(cooked);

  std::cout << "\n  sent 68 11 69 0A through a port left at its defaults, received"
            << as_hex(got) << "\n";

  // The line arrives, and the flow control byte is not in it.
  RC_REQUIRE(!got.empty());
  for (const std::uint8_t byte : got) RC_CHECK(byte != 0x11);
  RC_CHECK_EQ(got.size(), static_cast<std::size_t>(3));

  ::close(cooked);
}

RC_TEST("a port left at its defaults rewrites a carriage return") {
  Pty pty;
  RC_REQUIRE(pty.ok());
  const int cooked = open_cooked(pty.slave_name());
  RC_REQUIRE(cooked >= 0);

  pty.send({0x68, 0x69, 0x0D});          // ends with a carriage return
  const std::vector<std::uint8_t> got = read_for_a_while(cooked);

  RC_REQUIRE_EQ(got.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(got[2]), 0x0A);   // it arrived as a newline
  ::close(cooked);
}

RC_TEST("a frame whose length byte is an ordinary number survives") {
  // A four byte payload has a length field of 0x04, which is the end of file
  // character. Through a terminal in its default mode this frame is truncated
  // to its first byte, and nothing in the sending code mentions a length.
  Pty pty;
  RC_REQUIRE(pty.ok());
  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);

  const std::vector<std::uint8_t> frame{0xAA, 0x04, 0x20, 0x21, 0x22, 0x23, 0x5C};
  pty.send(frame);

  const std::vector<std::uint8_t> got = drain(port, frame.size());
  RC_REQUIRE_EQ(got.size(), frame.size());
  for (std::size_t i = 0; i < frame.size(); ++i)
    RC_CHECK_EQ(static_cast<int>(got[i]), static_cast<int>(frame[i]));
}

RC_TEST("what the port writes is what the device receives") {
  Pty pty;
  RC_REQUIRE(pty.ok());
  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);

  const std::vector<std::uint8_t> sent{0x01, 0x11, 0x13, 0x0A, 0x0D, 0xFE};
  const int wrote = port.write(rc::span<const std::uint8_t>(sent.data(), sent.size()));
  RC_REQUIRE_EQ(wrote, static_cast<int>(sent.size()));

  const std::vector<std::uint8_t> got = pty.receive(sent.size());
  RC_REQUIRE_EQ(got.size(), sent.size());
  for (std::size_t i = 0; i < sent.size(); ++i)
    RC_CHECK_EQ(static_cast<int>(got[i]), static_cast<int>(sent[i]));
}

RC_TEST("the descriptor is released when the port goes out of scope") {
  Pty pty;
  RC_REQUIRE(pty.ok());

  // Counting open descriptors before and after is what proves the destructor
  // did it, rather than trusting that it was written.
  const int before = ::dup(0);
  RC_REQUIRE(before >= 0);
  ::close(before);

  {
    SerialPort port;
    RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);
  }

  const int after = ::dup(0);
  RC_REQUIRE(after >= 0);
  ::close(after);

  // A leaked descriptor would push the next free number up.
  RC_CHECK_EQ(after, before);
}

RC_TEST("a moved from port is closed and the moved to port works") {
  Pty pty;
  RC_REQUIRE(pty.ok());

  SerialPort source;
  RC_REQUIRE(source.open(pty.slave_name(), kBaud) == SerialError::Ok);

  SerialPort destination = std::move(source);
  RC_CHECK(!source.is_open());
  RC_CHECK(destination.is_open());

  pty.send({0x42});
  const std::vector<std::uint8_t> got = drain(destination, 1);
  RC_REQUIRE_EQ(got.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(static_cast<int>(got[0]), 0x42);
}

RC_TEST("a whole link runs over a real port") {
  // The payoff. Everything from the four previous lessons, over a descriptor
  // the operating system opened, with no fake anywhere in the path.
  Pty pty;
  RC_REQUIRE(pty.ok());
  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);

  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 5000.0);

  // The device sends a framed, checksummed message carrying real fields.
  std::vector<std::uint8_t> body(11, 0);
  rc::io::ByteWriter encoder{rc::span<std::uint8_t>(body.data(), body.size())};
  encoder.put_u8(0x2A);
  encoder.put_u16(1000);
  encoder.put_i32(-12345);
  encoder.put_f32(2.5f);
  RC_REQUIRE(encoder.ok());

  std::vector<std::uint8_t> wire{rc::io::Link::kStartByte,
                                 static_cast<std::uint8_t>(body.size() + 1)};
  wire.insert(wire.end(), body.begin(), body.end());
  wire.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body.data(), body.size())));
  pty.send(wire);

  bool got = false;
  for (int attempt = 0; attempt < 40 && !got; ++attempt) {
    got = link.poll(static_cast<double>(attempt));
    if (!got) Pty::settle();
  }
  RC_REQUIRE(got);

  rc::io::ByteReader decoder{link.message()};
  std::uint8_t id = 0;
  std::uint16_t sequence = 0;
  std::int32_t offset = 0;
  float value = 0.0f;
  decoder.get_u8(id);
  decoder.get_u16(sequence);
  decoder.get_i32(offset);
  decoder.get_f32(value);

  RC_REQUIRE(decoder.ok());
  RC_CHECK_EQ(static_cast<int>(id), 0x2A);
  RC_CHECK_EQ(sequence, static_cast<std::uint16_t>(1000));
  RC_CHECK_EQ(offset, -12345);
  RC_CHECK_NEAR(static_cast<double>(value), 2.5, 1e-12);
  RC_CHECK_EQ(link.checksum_failures(), static_cast<std::size_t>(0));
}

RC_TEST("the link sends over a real port and the device receives a valid frame") {
  Pty pty;
  RC_REQUIRE(pty.ok());
  SerialPort port;
  RC_REQUIRE(port.open(pty.slave_name(), kBaud) == SerialError::Ok);

  std::vector<std::uint8_t> storage(64, 0);
  std::vector<std::uint8_t> scratch(64, 0);
  rc::io::Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), 5000.0);

  const std::vector<std::uint8_t> body{0x11, 0x13, 0x0A, 0x04};   // all awkward on purpose
  RC_REQUIRE(link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                       rc::span<std::uint8_t>(scratch.data(), scratch.size())));

  const std::vector<std::uint8_t> received = pty.receive(body.size() + 3);
  RC_REQUIRE_EQ(received.size(), body.size() + 3);
  RC_CHECK_EQ(static_cast<int>(received[0]), static_cast<int>(rc::io::Link::kStartByte));
  RC_CHECK_EQ(static_cast<int>(received[1]), static_cast<int>(body.size() + 1));
  for (std::size_t i = 0; i < body.size(); ++i)
    RC_CHECK_EQ(static_cast<int>(received[2 + i]), static_cast<int>(body[i]));
  RC_CHECK_EQ(static_cast<int>(received.back()),
              static_cast<int>(rc::io::crc8(rc::span<const std::uint8_t>(body.data(), body.size()))));
}

#else   // _WIN32

RC_TEST("a path that is not a communications device is reported as such") {
  // The furthest this platform can be taken without a driver. CreateFile
  // succeeds on an ordinary file and GetCommState then refuses it, which is
  // this platform's version of the isatty question.
  char directory[MAX_PATH] = {};
  RC_REQUIRE(::GetTempPathA(MAX_PATH, directory) != 0);
  const std::string path = std::string(directory) + "rc-08-05-not-a-port";

  const HANDLE scratch = ::CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
  RC_REQUIRE(scratch != INVALID_HANDLE_VALUE);
  ::CloseHandle(scratch);

  SerialPort port;
  RC_CHECK(port.open(path, kBaud) == SerialError::NotATerminal);
  RC_CHECK(!port.is_open());
  ::DeleteFileA(path.c_str());
}

RC_TEST("a port number above nine is asked for by path") {
  // COM10 and above cannot be opened by their bare name, so the prefix is not
  // an optional tidiness. The name is built even when no such port exists,
  // which is what this checks.
  RC_CHECK_EQ(windows_device_path("COM10"), std::string("\\\\.\\COM10"));
  RC_CHECK_EQ(windows_device_path("COM3"), std::string("\\\\.\\COM3"));
  RC_CHECK_EQ(windows_device_path("\\\\.\\COM7"), std::string("\\\\.\\COM7"));
}

RC_TEST("no port is opened on this platform, and that is recorded rather than hidden") {
  // Continuous integration cannot install the driver a virtual COM port needs,
  // so the implementation above is compiled and its error paths are run, and
  // its behaviour with a real device is not covered here. Saying so is the
  // point of this test: a silent gap is worse than a stated one.
  std::cout << "\n  windows: compiled and error paths exercised, no port opened.\n"
            << "  to cover the rest, connect a USB serial adapter's TX to its RX\n"
            << "  and run this suite against that COM port.\n";
  RC_CHECK(true);
}

#endif  // _WIN32

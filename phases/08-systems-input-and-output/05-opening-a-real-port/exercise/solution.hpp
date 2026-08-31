#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>

#if defined(_WIN32)
// Both of these before windows.h and not after. LEAN_AND_MEAN drops the parts
// of the header nothing here wants, and NOMINMAX stops it defining min and max
// as macros, which breaks std::min at every call site in the translation unit.
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

// Why an open can fail, in the terms the caller can act on. errno and
// GetLastError say the same things in two vocabularies, and the point of this
// enum is that the code above the port never has to learn either.
enum class SerialError {
  Ok,
  NotFound,           // no such device
  PermissionDenied,   // the device exists and you are not in its group
  Busy,               // something else has it
  NotATerminal,       // it opened, and it is not a serial device
  BadBaud,            // no constant for that rate
  ConfigureFailed,    // it is a serial device and would not take the settings
};

// A serial port, which is a file descriptor with settings attached.
//
// Opening it is the easy half. The settings are the half that decides whether
// binary data survives, and the default settings are wrong for binary data on
// both platforms.
class SerialPort : public rc::io::BytePort {
 public:
  SerialPort() = default;

  // Closing in the destructor, so the port is released on a normal return, an
  // early return, and while an exception unwinds. A port left open is not a
  // leak that a restart fixes: on Linux it stays claimed until the process
  // dies, and the next run reports it busy.
  ~SerialPort() override { close(); }

  // Copying would give two objects one descriptor and each would close it. The
  // second close can shut a port that something else has since opened, which is
  // a bug that reads as impossible at the place it goes wrong.
  SerialPort(const SerialPort&) = delete;
  SerialPort& operator=(const SerialPort&) = delete;

  SerialPort(SerialPort&& other) noexcept { steal_from(other); }

  SerialPort& operator=(SerialPort&& other) noexcept {
    if (this != &other) {
      close();
      steal_from(other);
    }
    return *this;
  }

  SerialError open(const std::string& device, int baud);

  void close() {
#if defined(_WIN32)
    if (handle_ != INVALID_HANDLE_VALUE) {
      ::CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (descriptor_ >= 0) {
      ::close(descriptor_);
      descriptor_ = -1;
    }
#endif
  }

  bool is_open() const {
#if defined(_WIN32)
    return handle_ != INVALID_HANDLE_VALUE;
#else
    return descriptor_ >= 0;
#endif
  }

  // Zero means nothing had arrived, which is the contract Link expects and the
  // reason this returns 0 rather than -1 when the port is merely idle.
  int read(rc::span<std::uint8_t> into) override;
  int write(rc::span<const std::uint8_t> from) override;

 private:
  void steal_from(SerialPort& other) {
#if defined(_WIN32)
    handle_ = other.handle_;
    other.handle_ = INVALID_HANDLE_VALUE;
#else
    descriptor_ = other.descriptor_;
    other.descriptor_ = -1;
#endif
  }

#if defined(_WIN32)
  HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
  int descriptor_ = -1;
#endif
};

#if !defined(_WIN32)

// The rate has to become one of a fixed set of constants. It is not a number
// the kernel scales, which is why 250000, a rate several servo protocols use,
// is not in this list on every system.
inline bool baud_constant(int baud, speed_t& out) {
  switch (baud) {
    case 9600:   out = B9600;   return true;
    case 19200:  out = B19200;  return true;
    case 38400:  out = B38400;  return true;
    case 57600:  out = B57600;  return true;
    case 115200: out = B115200; return true;
    case 230400: out = B230400; return true;
    default:     return false;
  }
}

inline SerialError SerialPort::open(const std::string& device, int baud) {
  close();

  // TODO: open the device and configure it for binary data.
  //
  // Open with O_RDWR, and with two more flags that each prevent a hang rather
  // than a corruption.
  //
  //   O_NOCTTY   do not let this become the process's controlling terminal, or
  //              a Ctrl-C byte arriving from the device signals your program.
  //   O_NONBLOCK do not wait for carrier detect, which a microcontroller will
  //              never assert, so an open without it hangs before your code
  //              has run a line.
  //
  // Map errno to a SerialError the caller can act on: ENOENT, EACCES, EBUSY.
  //
  // Then ask isatty. A path that exists and is not a serial device opens
  // perfectly well, and asking early turns a confusing silence into an answer.
  //
  // Then configure. tcgetattr, then cfmakeraw, which is the line that decides
  // whether binary data survives at all: without it the port stays in the mode
  // a terminal is in when a person is typing, and it will hold your data until
  // a newline, eat 0x11 and 0x13 as flow control, rewrite 0x0D, and let 0x7F
  // erase the byte before it.
  //
  // Then CLOCAL and CREAD, eight bits, no parity, one stop bit, and hardware
  // flow control off.
  //
  // Then VMIN and VTIME, both zero, so a read returns whatever is there this
  // instant including nothing. Anything else lets a device that has stopped
  // talking hold your control loop.
  //
  // Then cfsetispeed and cfsetospeed with the constant for the rate, and
  // tcsetattr.
  //
  // Then read the settings back and check them. tcsetattr reports success if
  // it applied any of what it was given, so a rate the hardware cannot do is
  // not an error, it is a different rate applied silently.
  (void)device;
  (void)baud;
  return SerialError::ConfigureFailed;
}

inline int SerialPort::read(rc::span<std::uint8_t> into) {
  if (descriptor_ < 0) return -1;

  // TODO: read what is there, and report nothing as nothing.
  //
  // EAGAIN, EWOULDBLOCK and EINTR all mean nothing happened rather than
  // something failed. Returning them as errors gives a link that declares
  // itself broken on an idle bus.
  (void)into;
  return -1;
}

inline int SerialPort::write(rc::span<const std::uint8_t> from) {
  if (descriptor_ < 0) return -1;

  // TODO: the same shape as read. Return what was accepted, which may be less
  // than was offered, since Link's write_all is what finishes the job.
  (void)from;
  return -1;
}

#else   // _WIN32

// COM1 to COM9 can be opened by name. COM10 and above cannot, because the bare
// name is parsed as a legacy device and the tenth one does not exist under that
// scheme. The prefix asks for the device by path instead and works for all of
// them, so it is applied always rather than only when it is needed.
inline std::string windows_device_path(const std::string& device) {
  if (device.size() >= 4 && device.compare(0, 4, "\\\\.\\") == 0) return device;
  return "\\\\.\\" + device;
}

inline SerialError SerialPort::open(const std::string& device, int baud) {
  close();

  // TODO: open the device and configure it for binary data.
  //
  // Build the path with windows_device_path, which is already written above.
  // COM10 and higher cannot be opened by their bare name.
  //
  // CreateFileA with GENERIC_READ | GENERIC_WRITE, a share mode of 0 so the
  // port is exclusive, and OPEN_EXISTING. Two programs holding one port each
  // receive a random half of the bytes, which looks like a device that has
  // become unreliable rather than like a mistake anybody made.
  //
  // Map GetLastError to a SerialError: ERROR_FILE_NOT_FOUND,
  // ERROR_PATH_NOT_FOUND, ERROR_ACCESS_DENIED, ERROR_SHARING_VIOLATION.
  //
  // Then GetCommState, which fails on a handle that is not a communications
  // device and is this platform's version of the isatty question.
  //
  // Then the DCB fields. BaudRate, ByteSize 8, NOPARITY, ONESTOPBIT, and
  // fBinary TRUE, which SetCommState insists on. Turn fOutX and fInX off:
  // they are XON and XOFF by another name and they eat 0x11 and 0x13 out of a
  // payload exactly as IXON does on the other platform. Turn off the hardware
  // handshaking, and set fAbortOnError FALSE, or one framing error stops the
  // port until somebody calls ClearCommError.
  //
  // Then COMMTIMEOUTS. ReadIntervalTimeout of MAXDWORD with both totals zero
  // is the documented way to ask for a read that returns immediately with
  // whatever has arrived. Left at their defaults, a read blocks for ever.
  (void)device;
  (void)baud;
  return SerialError::ConfigureFailed;
}

inline int SerialPort::read(rc::span<std::uint8_t> into) {
  if (handle_ == INVALID_HANDLE_VALUE) return -1;

  // TODO: ReadFile, and report a count of zero as zero rather than as failure.
  (void)into;
  return -1;
}

inline int SerialPort::write(rc::span<const std::uint8_t> from) {
  if (handle_ == INVALID_HANDLE_VALUE) return -1;

  // TODO: WriteFile, returning what it accepted.
  (void)from;
  return -1;
}

#endif  // _WIN32

#endif  // LESSON_SOLUTION_HPP

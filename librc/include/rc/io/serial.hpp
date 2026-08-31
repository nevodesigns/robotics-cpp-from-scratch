// rc/io/serial.hpp
//
// The serial port from lesson 08-05, graduated.
//
// The last piece: everything above this is platform independent and tested
// without hardware, and everything platform specific is here.
//
// Opening a port is the easy half. The settings are the half that decides
// whether binary data survives, and the defaults are wrong for binary data on
// both platforms. A port left as it arrives is in the state a terminal is in
// when a person is typing at it: it holds data until a newline, takes 0x11 and
// 0x13 as flow control, treats 0x03 and 0x1A as signals, lets 0x7F erase the
// byte before it, and rewrites 0x0D. Measured, a frame with a four byte payload
// is truncated to its first byte, because the length field is 0x04 and four is
// the end of file character.

#ifndef RC_IO_SERIAL_HPP
#define RC_IO_SERIAL_HPP

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

namespace rc {
namespace io {

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
class SerialPort : public BytePort {
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

  speed_t speed = B9600;
  if (!baud_constant(baud, speed)) return SerialError::BadBaud;

  // O_NOCTTY: do not let this become the controlling terminal of the process.
  // Without it, a Ctrl-C character arriving on the wire can signal your own
  // program, which is a spectacular way for a robot to stop.
  //
  // O_NONBLOCK: on a port with no carrier detect asserted, an open without
  // this waits for a modem signal that a microcontroller will never send, and
  // the program hangs before it has run a line of its own code.
  const int descriptor = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (descriptor < 0) {
    switch (errno) {
      case ENOENT: return SerialError::NotFound;
      case EACCES: return SerialError::PermissionDenied;
      case EBUSY:  return SerialError::Busy;
      default:     return SerialError::NotFound;
    }
  }

  // A path that exists and is not a serial device opens perfectly well. Asking
  // early turns a confusing silence into an answer.
  if (::isatty(descriptor) != 1) {
    ::close(descriptor);
    return SerialError::NotATerminal;
  }

  termios settings{};
  if (::tcgetattr(descriptor, &settings) != 0) {
    ::close(descriptor);
    return SerialError::ConfigureFailed;
  }

  // The line that decides everything.
  //
  // A terminal exists to carry text typed by a person, so by default it
  // buffers until a line is finished, echoes what it receives, rewrites
  // carriage returns, treats some bytes as flow control and others as signals,
  // and lets one of them erase the byte before it. Every one of those destroys
  // binary data, and the destruction depends on which byte values a message
  // happens to contain, which is why it looks intermittent.
  ::cfmakeraw(&settings);

  // CLOCAL: ignore modem control lines, since there is no modem.
  // CREAD: actually enable the receiver.
  settings.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);

  // Eight data bits, no parity, one stop bit, which is what almost every
  // device means when it says nothing about the frame format.
  settings.c_cflag &= static_cast<tcflag_t>(~CSIZE);
  settings.c_cflag |= static_cast<tcflag_t>(CS8);
  settings.c_cflag &= static_cast<tcflag_t>(~PARENB);
  settings.c_cflag &= static_cast<tcflag_t>(~CSTOPB);

#if defined(CRTSCTS)
  // Hardware flow control off. With it on and nothing driving the CTS line,
  // the port accepts writes and sends nothing, which looks exactly like a
  // device that is ignoring you.
  settings.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
#endif

  // How a read decides it is finished. Zero and zero means return whatever is
  // there this instant, including nothing, which is what a polled link wants.
  // A VMIN above zero makes read wait for that many bytes, and a control loop
  // waiting on a device that has stopped talking waits for ever.
  settings.c_cc[VMIN] = 0;
  settings.c_cc[VTIME] = 0;

  if (::cfsetispeed(&settings, speed) != 0 || ::cfsetospeed(&settings, speed) != 0) {
    ::close(descriptor);
    return SerialError::ConfigureFailed;
  }

  if (::tcsetattr(descriptor, TCSANOW, &settings) != 0) {
    ::close(descriptor);
    return SerialError::ConfigureFailed;
  }

  // tcsetattr reports success if it applied *any* of what it was given, so a
  // rate the device cannot do is not an error, it is a different rate. Read
  // the settings back and check the ones that matter.
  termios applied{};
  if (::tcgetattr(descriptor, &applied) != 0 ||
      ::cfgetispeed(&applied) != speed ||
      (applied.c_lflag & static_cast<tcflag_t>(ICANON)) != 0 ||
      (applied.c_iflag & static_cast<tcflag_t>(IXON)) != 0) {
    ::close(descriptor);
    return SerialError::ConfigureFailed;
  }

  descriptor_ = descriptor;
  return SerialError::Ok;
}

inline int SerialPort::read(rc::span<std::uint8_t> into) {
  if (descriptor_ < 0) return -1;
  if (into.size() == 0) return 0;

  const ssize_t count = ::read(descriptor_, into.data(), into.size());
  if (count >= 0) return static_cast<int>(count);

  // Nothing to read, and a signal arriving mid call, are both "nothing
  // happened" rather than failures. Reporting them as errors gives a link that
  // declares itself broken on an idle bus.
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
  return -1;
}

inline int SerialPort::write(rc::span<const std::uint8_t> from) {
  if (descriptor_ < 0) return -1;
  if (from.size() == 0) return 0;

  const ssize_t count = ::write(descriptor_, from.data(), from.size());
  if (count >= 0) return static_cast<int>(count);
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
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

  if (baud <= 0) return SerialError::BadBaud;

  const std::string path = windows_device_path(device);

  // The zero share mode is exclusive access. Two programs holding one port each
  // receive a random half of the bytes, which presents as a device that has
  // become unreliable rather than as a mistake anybody made.
  const HANDLE handle = ::CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                      0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    switch (::GetLastError()) {
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:  return SerialError::NotFound;
      case ERROR_ACCESS_DENIED:   return SerialError::Busy;
      case ERROR_SHARING_VIOLATION: return SerialError::Busy;
      default:                    return SerialError::NotFound;
    }
  }

  DCB settings{};
  settings.DCBlength = sizeof(settings);

  // Fails with an invalid parameter on a handle that is not a communications
  // device, which is this platform's version of the isatty question.
  if (::GetCommState(handle, &settings) == FALSE) {
    ::CloseHandle(handle);
    return SerialError::NotATerminal;
  }

  settings.BaudRate = static_cast<DWORD>(baud);
  settings.ByteSize = 8;
  settings.Parity = NOPARITY;
  settings.StopBits = ONESTOPBIT;

  // fBinary must be TRUE. Windows does not implement anything else and
  // SetCommState fails if it is FALSE, which is the tidiest possible way for a
  // platform to say that text mode on a serial port was a mistake.
  settings.fBinary = TRUE;

  // Software flow control off. fOutX and fInX are XON and XOFF by another
  // name, and they eat the bytes 0x11 and 0x13 out of a payload exactly as
  // IXON does on the other platform.
  settings.fOutX = FALSE;
  settings.fInX = FALSE;

  // Hardware flow control off, and the control lines held in a fixed state
  // rather than driven by the driver's own handshaking.
  settings.fOutxCtsFlow = FALSE;
  settings.fOutxDsrFlow = FALSE;
  settings.fDsrSensitivity = FALSE;
  settings.fDtrControl = DTR_CONTROL_DISABLE;
  settings.fRtsControl = RTS_CONTROL_DISABLE;

  // Without this, one parity or framing error stops the port until somebody
  // calls ClearCommError, and the symptom is a link that dies at the first
  // glitch and never returns.
  settings.fAbortOnError = FALSE;

  // No character is special. ErrorChar and EofChar replace or truncate a
  // payload byte, which is the same class of damage the line discipline does.
  settings.fErrorChar = FALSE;
  settings.fNull = FALSE;

  if (::SetCommState(handle, &settings) == FALSE) {
    ::CloseHandle(handle);
    return SerialError::ConfigureFailed;
  }

  // The documented combination for a read that returns immediately with
  // whatever has arrived: a maximum interval and no total timeout at all.
  // Leaving the timeouts at their defaults gives a read that blocks for ever.
  COMMTIMEOUTS timeouts{};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;

  if (::SetCommTimeouts(handle, &timeouts) == FALSE) {
    ::CloseHandle(handle);
    return SerialError::ConfigureFailed;
  }

  handle_ = handle;
  return SerialError::Ok;
}

inline int SerialPort::read(rc::span<std::uint8_t> into) {
  if (handle_ == INVALID_HANDLE_VALUE) return -1;
  if (into.size() == 0) return 0;

  DWORD count = 0;
  if (::ReadFile(handle_, into.data(), static_cast<DWORD>(into.size()),
                 &count, nullptr) == FALSE) {
    return -1;
  }
  return static_cast<int>(count);
}

inline int SerialPort::write(rc::span<const std::uint8_t> from) {
  if (handle_ == INVALID_HANDLE_VALUE) return -1;
  if (from.size() == 0) return 0;

  DWORD count = 0;
  if (::WriteFile(handle_, from.data(), static_cast<DWORD>(from.size()),
                  &count, nullptr) == FALSE) {
    return -1;
  }
  return static_cast<int>(count);
}

#endif  // _WIN32

}  // namespace io
}  // namespace rc

#endif  // RC_IO_SERIAL_HPP

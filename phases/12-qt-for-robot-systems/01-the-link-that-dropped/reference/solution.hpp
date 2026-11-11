#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

// A control station talks to a robot over a network, and a network is a thing that drops: the robot reboots, a cable is moved, a wireless link goes behind a wall. A client that
// gives up on the first failure is a station that has to be restarted. A client that retries at once, in a loop, is a station that fills the log and the robot's accept queue
// with attempts, and that spends its whole time connecting.
//
// The answer is to wait between attempts, and to wait longer after each failure, up to a cap, and to start again from the short wait once a connection has been made. And
// Qt has a detail that makes a client retry twice: when a connection that was up is closed by the other end, the socket reports an error AND that it disconnected, and a
// client that reacts to both has scheduled two attempts for one drop. A failed attempt reports only the error.
//
// And how long a failed attempt takes to be reported is the operating system's business: on Linux a refused connection to this machine is an error at once, and on the
// Windows runner of this repository it had not been reported after a second and a half. A host that does not answer at all is never reported. So the client keeps a limit
// of its own on an attempt, and counts an attempt that runs past it as a failure.

// The wait before attempt number `attempt` (0 for the first retry after a good connection): `base_ms`, doubled for each attempt, never more than `cap_ms`. A base that is not
// positive is no wait. A cap below the base is the base. Large attempt numbers do not overflow.
inline int next_delay_ms(int attempt, int base_ms, int cap_ms) {
  if (base_ms <= 0) return 0;
  if (cap_ms < base_ms) cap_ms = base_ms;
  long long delay = base_ms;
  for (int i = 0; i < attempt && delay < cap_ms; ++i) delay *= 2;
  return static_cast<int>(delay < cap_ms ? delay : cap_ms);
}

class LinkClient : public QObject {
  Q_OBJECT

 public:
  enum State { kStopped = 0, kConnecting = 1, kConnected = 2, kWaiting = 3 };

  LinkClient(const QHostAddress& host, quint16 port, int base_ms, int cap_ms, int connect_timeout_ms, QObject* parent = nullptr)
      : QObject(parent), host_(host), port_(port), base_ms_(base_ms), cap_ms_(cap_ms), connect_timeout_ms_(connect_timeout_ms) {
    retry_.setSingleShot(true);
    connect_timer_.setSingleShot(true);
    connect(&retry_, &QTimer::timeout, this, &LinkClient::attempt);
    connect(&connect_timer_, &QTimer::timeout, this, &LinkClient::on_connect_timeout);
    connect(&socket_, &QTcpSocket::connected, this, &LinkClient::on_connected);
    connect(&socket_, &QTcpSocket::disconnected, this, &LinkClient::schedule_retry);
    connect(&socket_, &QTcpSocket::errorOccurred, this, &LinkClient::schedule_retry);
    connect(&socket_, &QTcpSocket::readyRead, this, &LinkClient::on_ready_read);
  }

  void start() {
    if (state_ != kStopped) return;
    running_ = true;
    attempt();
  }
  // Ends the link and any retry that is waiting. Nothing is attempted again until start().
  void stop() {
    running_ = false;
    retry_.stop();
    connect_timer_.stop();
    socket_.abort();
    set_state(kStopped);
  }

  State state() const { return state_; }

  // Sends a line (the text and a newline) to the robot. Returns false, and sends nothing, unless the link is up.
  bool send_line(const QString& text) {
    if (state_ != kConnected) return false;
    socket_.write(text.toUtf8() + '\n');
    return true;
  }
  int attempts() const { return attempts_; }          // failures since the last good connection
  int connects() const { return connects_; }          // good connections, ever
  QList<int> delays() const { return delays_; }       // every wait that was scheduled, in order
  int base_ms() const { return base_ms_; }            // the first wait, and the one after every success
  int cap_ms() const { return cap_ms_; }              // the longest wait
  int connect_timeout_ms() const { return connect_timeout_ms_; }          // the longest an attempt may take
  QStringList lines() const { return lines_; }        // every complete line received

 signals:
  void state_changed(int state);
  void line_received(const QString& text);        // each complete line that arrives, as it arrives

 private slots:
  void attempt() {
    set_state(kConnecting);
    socket_.abort();
    socket_.connectToHost(host_, port_);
    connect_timer_.start(connect_timeout_ms_);
  }

  // An attempt that has run past its limit is a failure, however the operating system would have reported it, or not.
  void on_connect_timeout() {
    if (state_ != kConnecting) return;
    socket_.abort();
    schedule_retry();
  }

  void on_connected() {
    connect_timer_.stop();
    attempts_ = 0;                  // the next failure is a first failure again
    ++connects_;
    set_state(kConnected);
  }

  // Called for an error and for a disconnection, which a closed link reports together. Only the first of them, for a given drop, schedules anything.
  void schedule_retry() {
    if (!running_ || state_ == kWaiting || state_ == kStopped) return;
    connect_timer_.stop();
    set_state(kWaiting);
    const int delay = next_delay_ms(attempts_, base_ms_, cap_ms_);
    delays_.push_back(delay);
    ++attempts_;
    retry_.start(delay);
  }

  void on_ready_read() {
    buffer_.append(socket_.readAll());
    int end = buffer_.indexOf('\n');
    while (end >= 0) {
      lines_.append(QString::fromUtf8(buffer_.left(end)));
      emit line_received(lines_.last());
      buffer_.remove(0, end + 1);
      end = buffer_.indexOf('\n');
    }
  }

 private:
  void set_state(State s) {
    if (s == state_) return;
    state_ = s;
    emit state_changed(static_cast<int>(s));
  }

  QHostAddress host_;
  quint16 port_;
  int base_ms_;
  int cap_ms_;
  int connect_timeout_ms_;
  bool running_ = false;
  State state_ = kStopped;
  int attempts_ = 0;
  int connects_ = 0;
  QList<int> delays_;
  QStringList lines_;
  QByteArray buffer_;
  QTcpSocket socket_;
  QTimer retry_;
  QTimer connect_timer_;
};

#endif  // LESSON_SOLUTION_HPP

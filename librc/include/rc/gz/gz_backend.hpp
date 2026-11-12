// rc/gz/gz_backend.hpp
//
// The Gazebo Sim backend, from lesson 22-14: the second simulator of the field courses, behind the same Backend interface as
// the built in one.
//
// GzBackend writes the world of a scenario (rc/field/gz_world.hpp) to a file, starts a Gazebo Sim server on it as a child process
// (paused, headless, in a partition of its own so that two runs never hear each other), and talks to it over gz-transport: the
// command is a Twist published on /model/<name>/cmd_vel, the pose is read from /world/<name>/pose/info, and time is advanced
// with the world control service in steps of 1 ms, so that a run is as long as it was asked to be and not as long as the
// computer took. The server is stopped when the object is destroyed, and by the kernel if the program dies first.
//
// It is built only where Gazebo's transport and message libraries are installed (the lesson's tests define RC_HAVE_GZ), and
// only on Linux: it starts a process with fork and exec.
//
// Gazebo Sim is a separate program with its own idea of time and friction; what the two simulators do differently is the
// subject of the lesson after this one.

#ifndef RC_GZ_GZ_BACKEND_HPP
#define RC_GZ_GZ_BACKEND_HPP

#include <gz/msgs/boolean.pb.h>
#include <gz/msgs/pose_v.pb.h>
#include <gz/msgs/twist.pb.h>
#include <gz/msgs/world_control.pb.h>
#include <gz/transport/Node.hh>

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <rc/field/backend.hpp>
#include <rc/field/gz_world.hpp>
#include <string>
#include <thread>

#if defined(__linux__)
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace rc {
namespace gz {

class GzBackend : public rc::field::Backend {
 public:
  GzBackend() = default;
  GzBackend(const GzBackend&) = delete;
  GzBackend& operator=(const GzBackend&) = delete;
  ~GzBackend() override { stop_server(); }

  std::string name() const override { return "gazebo"; }

  bool start(const rc::field::FieldScenario& scenario, const rc::field::RobotModel& robot) override {
#if defined(__linux__)
    const std::string text = rc::field::to_world(scenario, robot);
    // A server that is already running this world is reset instead of started again, which takes a fraction of a second and not seconds.
    if (pid_ > 0 && node_ && text == world_text_) return reset_world();
    stop_server();
    static int counter = 0;
    ++counter;
    const std::string tag = std::to_string(static_cast<long>(::getpid())) + "_" + std::to_string(counter);
    const std::string partition = "rc_" + tag;
    world_path_ = "/tmp/rc_gz_" + tag + ".sdf";
    world_name_ = scenario.name.empty() ? "field" : scenario.name;
    robot_name_ = robot.name;
    world_text_ = text;
    {
      std::ofstream out(world_path_.c_str());
      out << text;
      if (!out) return false;
    }
    // The partition keeps this server and this program talking only to each other.
    ::setenv("GZ_PARTITION", partition.c_str(), 1);
    pid_ = ::fork();
    if (pid_ < 0) return false;
    if (pid_ == 0) {
      ::prctl(PR_SET_PDEATHSIG, SIGTERM);
      if (std::freopen("/dev/null", "w", stdout) == nullptr) ::_exit(1);
      if (std::freopen("/dev/null", "w", stderr) == nullptr) ::_exit(1);
      ::execlp("gz", "gz", "sim", "-s", world_path_.c_str(), static_cast<char*>(nullptr));
      ::_exit(127);
    }
    node_.reset(new ::gz::transport::Node());
    got_pose_ = false;
    pose_time_ = 0.0;
    if (!node_->Subscribe("/world/" + world_name_ + "/pose/info", &GzBackend::on_pose, this)) return false;
    twist_ = node_->Advertise<::gz::msgs::Twist>("/model/" + robot_name_ + "/cmd_vel");
    // The server takes a few seconds to load the world.
    std::unique_lock<std::mutex> lock(mutex_);
    const bool ready = changed_.wait_for(lock, std::chrono::seconds(60), [this]() { return got_pose_; });
    // The server has run a moment before it was seen; time is counted from the first pose.
    time_offset_ = pose_time_;
    return ready;
#else
    (void)scenario;
    (void)robot;
    return false;
#endif
  }

  void command(double v, double w) override {
    ::gz::msgs::Twist t;
    t.mutable_linear()->set_x(v);
    t.mutable_angular()->set_z(w);
    twist_.Publish(t);
    // Give the message time to reach the plugin before the world is stepped.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  void advance(double seconds) override {
    const long steps = std::lround(seconds / 0.001);
    if (steps <= 0) return;
    double target = 0.0;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      target = pose_time_ + static_cast<double>(steps) * 0.001;
    }
    ::gz::msgs::WorldControl request;
    ::gz::msgs::Boolean reply;
    bool result = false;
    // Without pause the world would go on running after the steps; with it, it runs exactly these and stops.
    request.set_pause(true);
    request.set_multi_step(static_cast<unsigned int>(steps));
    node_->Request("/world/" + world_name_ + "/control", request, 10000, reply, result);
    std::unique_lock<std::mutex> lock(mutex_);
    changed_.wait_for(lock, std::chrono::seconds(120), [this, target]() { return pose_time_ >= target - 1.5e-3; });
  }

  rc::field::BackendPose pose() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    rc::field::BackendPose p = pose_;
    p.t -= time_offset_;
    return p;
  }

 private:
  void on_pose(const ::gz::msgs::Pose_V& message) {
    for (int i = 0; i < message.pose_size(); ++i) {
      if (message.pose(i).name() != robot_name_) continue;
      const ::gz::msgs::Pose& p = message.pose(i);
      const double qx = p.orientation().x(), qy = p.orientation().y(), qz = p.orientation().z(), qw = p.orientation().w();
      std::lock_guard<std::mutex> lock(mutex_);
      pose_.x = p.position().x();
      pose_.y = p.position().y();
      pose_.theta = std::atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz));
      pose_time_ = static_cast<double>(message.header().stamp().sec()) + 1.0e-9 * static_cast<double>(message.header().stamp().nsec());
      pose_.t = pose_time_;
      got_pose_ = true;
      changed_.notify_all();
      return;
    }
  }

  // Puts the world back as it was loaded: the time at zero, the robot at its start, the command forgotten. The world stays paused.
  bool reset_world() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      got_pose_ = false;
    }
    ::gz::msgs::WorldControl request;
    ::gz::msgs::Boolean reply;
    bool result = false;
    request.set_pause(true);
    request.mutable_reset()->set_all(true);
    if (!node_->Request("/world/" + world_name_ + "/control", request, 10000, reply, result) || !result) return false;
    std::unique_lock<std::mutex> lock(mutex_);
    const bool ready = changed_.wait_for(lock, std::chrono::seconds(20), [this]() { return got_pose_ && pose_time_ < 0.01; });
    time_offset_ = 0.0;
    return ready;
  }

  void stop_server() {
#if defined(__linux__)
    node_.reset();
    if (pid_ > 0) {
      ::kill(pid_, SIGTERM);
      int status = 0;
      for (int i = 0; i < 100; ++i) {
        if (::waitpid(pid_, &status, WNOHANG) != 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (i == 60) ::kill(pid_, SIGKILL);
      }
      pid_ = -1;
    }
    if (!world_path_.empty()) {
      std::remove(world_path_.c_str());
      world_path_.clear();
      world_text_.clear();
    }
#endif
  }

  std::string world_path_;
  std::string world_name_;
  std::string world_text_;
  std::string robot_name_;
#if defined(__linux__)
  pid_t pid_ = -1;
#endif
  std::unique_ptr<::gz::transport::Node> node_;
  ::gz::transport::Node::Publisher twist_;
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  rc::field::BackendPose pose_;
  double pose_time_ = 0.0;
  double time_offset_ = 0.0;
  bool got_pose_ = false;
};

}  // namespace gz
}  // namespace rc

#endif

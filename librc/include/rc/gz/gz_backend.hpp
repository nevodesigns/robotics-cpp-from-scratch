// rc/gz/gz_backend.hpp
//
// The Gazebo Sim backend, from lesson 22-14: the second simulator of the field courses, behind the same Backend interface as
// the built in one.
//
// GzBackend writes the world of a scenario (rc/field/gz_world.hpp) to a file, starts a Gazebo Sim server on it as a child process
// (paused, headless, in a partition of its own so that two runs never hear each other), and talks to it over gz-transport: the
// command is a Twist published on /model/<name>/cmd_vel, the pose is read from the ground truth odometry the model publishes at every step, /model/<name>/pose_gt, and time is advanced
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
#include <gz/msgs/odometry.pb.h>
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
#include <fcntl.h>
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
    stop_server();
    const std::string text = rc::field::to_world(scenario, robot);
    static int counter = 0;
    ++counter;
    const std::string tag = std::to_string(static_cast<long>(::getpid())) + "_" + std::to_string(counter);
    const std::string partition = "rc_" + tag;
    world_path_ = "/tmp/rc_gz_" + tag + ".sdf";
    world_name_ = scenario.name.empty() ? "field" : scenario.name;
    robot_name_ = robot.name;
    {
      std::ofstream out(world_path_.c_str());
      out << text;
      if (!out) return false;
    }
    // The partition keeps this server and this program talking only to each other.
    ::setenv("GZ_PARTITION", partition.c_str(), 1);
    // Whatever is waiting in a buffer would otherwise be written twice, once by each process.
    std::fflush(nullptr);
    pid_ = ::fork();
    if (pid_ < 0) return false;
    if (pid_ == 0) {
      ::prctl(PR_SET_PDEATHSIG, SIGTERM);
      const int null_fd = ::open("/dev/null", O_WRONLY);
      if (null_fd < 0 || ::dup2(null_fd, 1) < 0 || ::dup2(null_fd, 2) < 0) ::_exit(1);
      ::execlp("gz", "gz", "sim", "-s", world_path_.c_str(), static_cast<char*>(nullptr));
      ::_exit(127);
    }
    node_.reset(new ::gz::transport::Node());
    got_pose_ = false;
    pose_time_ = 0.0;
    // The robot's own ground truth, which the model publishes at every step of simulated time.
    if (!node_->Subscribe("/model/" + robot_name_ + "/pose_gt", &GzBackend::on_odometry, this)) return false;
    twist_ = node_->Advertise<::gz::msgs::Twist>("/model/" + robot_name_ + "/cmd_vel");
    // The server takes a few seconds to load the world: ask it for five steps until it answers (the publisher is silent for the first), which also gives the first pose.
    return first_step(60.0);
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
    changed_.wait_for(lock, std::chrono::seconds(120), [this, target]() { return pose_time_ >= target - 1.0e-5; });
  }

  rc::field::BackendPose pose() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    rc::field::BackendPose p = pose_;
    p.t -= time_offset_;
    return p;
  }

 private:
  void on_odometry(const ::gz::msgs::Odometry& message) {
    const ::gz::msgs::Pose& p = message.pose();
    const double qx = p.orientation().x(), qy = p.orientation().y(), qz = p.orientation().z(), qw = p.orientation().w();
    std::lock_guard<std::mutex> lock(mutex_);
    pose_.x = p.position().x();
    pose_.y = p.position().y();
    pose_.theta = std::atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz));
    pose_time_ = static_cast<double>(message.header().stamp().sec()) + 1.0e-9 * static_cast<double>(message.header().stamp().nsec());
    pose_.t = pose_time_;
    got_pose_ = true;
    changed_.notify_all();
  }

  // Asks a paused world for a few steps, trying again until the server is up, and waits for the pose that step publishes. Time is counted from it.
  bool first_step(double timeout_seconds) {
    const auto start = std::chrono::steady_clock::now();
    bool sent = false;
    while (!sent && std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < timeout_seconds) {
      ::gz::msgs::WorldControl request;
      ::gz::msgs::Boolean reply;
      bool result = false;
      request.set_pause(true);
      request.set_multi_step(5);
      sent = node_->Request("/world/" + world_name_ + "/control", request, 1000, reply, result) && result;
    }
    if (!sent) return false;
    std::unique_lock<std::mutex> lock(mutex_);
    const bool ready = changed_.wait_for(lock, std::chrono::seconds(30), [this]() { return got_pose_ && pose_time_ < 0.05; });
    time_offset_ = pose_time_;
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
    }
#endif
  }

  std::string world_path_;
  std::string world_name_;
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

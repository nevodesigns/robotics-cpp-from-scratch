#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

struct Board {
  std::string name;
  double cores = 4.0;
  double ram_mb = 4096.0;
  double flash_mb = 16384.0;
  double throttle_factor = 0.6;    // the share of its speed the board keeps when it is hot enough to throttle
};

// What one program needs: the CPU it uses on average and at its peaks (in cores: 1.0 is one core fully busy), and the largest resident memory it reaches.
struct Proc {
  std::string name;
  double cpu_avg = 0.0;
  double cpu_peak = 0.0;
  double ram_mb = 0.0;
};

// The value below which a share q (0 to 1) of the samples lie, by the nearest rank: sort them, and take the one at position ceil(q * n) counting from 1 (at least the first, at most the last). Zero for no samples. q of 1 is the largest.
inline double percentile(std::vector<double> samples, double q) {
  // TODO: zero for none; sort; rank = ceil(q * n) with q kept in 0..1, then at least 1 and at most n; the sample at that rank counting from 1
  (void)samples;
  (void)q;
  return 0.0;
}

// A program's profile from measurements: cpu_avg is the mean of the CPU samples, cpu_peak the `q` percentile of them (q of 1 for the largest), and ram_mb the largest of the memory samples. No samples gives zeros.
inline Proc profile_from_samples(const std::string& name, const std::vector<double>& cpu, const std::vector<double>& ram_mb, double q) {
  // TODO: cpu_avg the mean of cpu, cpu_peak percentile(cpu, q), ram_mb the largest of ram_mb; zeros for no samples
  (void)name;
  (void)cpu;
  (void)ram_mb;
  (void)q;
  return Proc();
}

// How much of each resource may be used. The averages may not fill the CPU, the peaks of all the programs together (which seldom coincide, so more than the cores is not allowed but the whole of them is) may reach `cpu_peak_max` times the cores, the memory is kept clear of the point where the system starts killing programs, and flash keeps room to write.
struct Budget {
  double cpu_avg_max = 0.7;
  double cpu_peak_max = 1.25;
  double ram_max = 0.8;
  double flash_max = 0.9;
};

struct Finding {
  std::string resource;      // "cpu_avg", "cpu_peak", "ram" or "flash"
  double used = 0.0;
  double allowed = 0.0;
};

struct Fit {
  double cpu_avg = 0.0;
  double cpu_peak = 0.0;
  double ram_mb = 0.0;
  double flash_mb = 0.0;
  std::vector<Finding> findings;
  bool fits = false;
};

// Do these programs fit the board? Sum the programs' cpu_avg, cpu_peak and ram_mb. Flash is `slots` releases of release_mb each plus data_mb and log_mb. The board's cores count as cores * throttle_factor if `hot`.
// A Finding, in this order, for each resource that is over what is allowed: cpu_avg over cpu_avg_max * cores; cpu_peak over cpu_peak_max * cores; ram over ram_max * ram_mb; flash over flash_max * flash_mb. It fits if there are none.
inline Fit check_fit(const Board& b, const std::vector<Proc>& procs, double release_mb, int slots, double data_mb, double log_mb, const Budget& budget, bool hot) {
  // TODO: sum the programs; flash = slots * release_mb + data_mb + log_mb; cores = cores * throttle_factor if hot. A Finding for each of cpu_avg, cpu_peak, ram and flash (in that order) that is over its allowance; fits if none
  (void)b;
  (void)procs;
  (void)release_mb;
  (void)slots;
  (void)data_mb;
  (void)log_mb;
  (void)budget;
  (void)hot;
  return Fit();
}

// The largest release (megabytes) that `slots` copies of can be kept alongside the data and the logs: (flash_max * flash - data - logs) / slots. Zero if nothing is left or there are no slots.
inline double max_release_mb(const Board& b, int slots, double data_mb, double log_mb, const Budget& budget) {
  // TODO: zero for no slots; room = flash_max * flash - data - logs; zero if that is not above zero; else room / slots
  (void)b;
  (void)slots;
  (void)data_mb;
  (void)log_mb;
  (void)budget;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP

// rc/deploy/fit.hpp
//
// Whether the programs fit a board, from lesson 20-11, graduated.
//
// percentile is the nearest rank; profile_from_samples keeps a program's mean CPU, the CPU at a percentile (1 for the largest) and its largest memory. check_fit sums the programs, takes flash as slots times a release plus the data and the logs, and names each of CPU average, CPU peak, memory and flash (in that order) that is over its allowance (0.7 of the cores, 1.25 of the cores for all the peaks, 0.8 of the memory, 0.9 of the flash), with the board throttled to a share of its speed if it is hot. max_release_mb is the largest release that `slots` copies fit.
//
// A planner measured for 60 s on the bench had a peak of 0.40 cores and in an hour in the field 2.00. Six programs fit a 4 core, 4096 MB, 16 GB board (average 2.60 of 2.80, peaks 4.90 of 5.00, memory 2030 of 3277 MB, flash 8200 of 14746 MB); throttled to 60 percent they were over on CPU average and peak; a map server of 1500 MB took memory to 3530 MB of 3277. Two slots allow a release of 6273 MB and one slot 12546 MB.

#ifndef RC_DEPLOY_FIT_HPP
#define RC_DEPLOY_FIT_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace rc {
namespace deploy {

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
  if (samples.empty()) return 0.0;
  std::sort(samples.begin(), samples.end());
  const double n = static_cast<double>(samples.size());
  double rank = std::ceil(std::max(0.0, std::min(1.0, q)) * n);
  rank = std::max(1.0, std::min(n, rank));
  return samples[static_cast<std::size_t>(rank) - 1];
}

// A program's profile from measurements: cpu_avg is the mean of the CPU samples, cpu_peak the `q` percentile of them (q of 1 for the largest), and ram_mb the largest of the memory samples. No samples gives zeros.
inline Proc profile_from_samples(const std::string& name, const std::vector<double>& cpu, const std::vector<double>& ram_mb, double q) {
  Proc p;
  p.name = name;
  if (!cpu.empty()) {
    double sum = 0.0;
    for (double c : cpu) sum += c;
    p.cpu_avg = sum / static_cast<double>(cpu.size());
    p.cpu_peak = percentile(cpu, q);
  }
  for (double r : ram_mb) p.ram_mb = std::max(p.ram_mb, r);
  return p;
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
  Fit f;
  for (const Proc& p : procs) {
    f.cpu_avg += p.cpu_avg;
    f.cpu_peak += p.cpu_peak;
    f.ram_mb += p.ram_mb;
  }
  f.flash_mb = static_cast<double>(slots) * release_mb + data_mb + log_mb;
  const double cores = hot ? b.cores * b.throttle_factor : b.cores;
  if (f.cpu_avg > budget.cpu_avg_max * cores) f.findings.push_back({"cpu_avg", f.cpu_avg, budget.cpu_avg_max * cores});
  if (f.cpu_peak > budget.cpu_peak_max * cores) f.findings.push_back({"cpu_peak", f.cpu_peak, budget.cpu_peak_max * cores});
  if (f.ram_mb > budget.ram_max * b.ram_mb) f.findings.push_back({"ram", f.ram_mb, budget.ram_max * b.ram_mb});
  if (f.flash_mb > budget.flash_max * b.flash_mb) f.findings.push_back({"flash", f.flash_mb, budget.flash_max * b.flash_mb});
  f.fits = f.findings.empty();
  return f;
}

// The largest release (megabytes) that `slots` copies of can be kept alongside the data and the logs: (flash_max * flash - data - logs) / slots. Zero if nothing is left or there are no slots.
inline double max_release_mb(const Board& b, int slots, double data_mb, double log_mb, const Budget& budget) {
  if (slots < 1) return 0.0;
  const double room = budget.flash_max * b.flash_mb - data_mb - log_mb;
  return room > 0.0 ? room / static_cast<double>(slots) : 0.0;
}

}  // namespace deploy
}  // namespace rc

#endif  // RC_DEPLOY_FIT_HPP

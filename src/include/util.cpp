// TODO: credits to Clockwork

#include "util.h"
#include <algorithm>
#include <atomic>
#include <boost/filesystem.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <libgen.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <thread>

namespace asiohpn {



namespace util {
  
time_point epoch = hrt();
uint64_t epoch_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
std::uint64_t nanos(time_point t) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(t - epoch)
             .count() +
         epoch_time;
}

time_point hrt() { return std::chrono::steady_clock::now(); }
std::uint64_t now() { return nanos(hrt()); }

std::uint32_t get_random_identifier() {
  srand(time(0) + getpid());
  return rand();
}

std::vector<std::string> split(std::string string, char delimiter) {
  std::stringstream ss(string);
  std::vector<std::string> result;

  while (ss.good()) {
    std::string substr;
    getline(ss, substr, delimiter);
    result.push_back(substr);
  }
  return result;
}

uint64_t calculate_steady_clock_delta() {
  auto t1 = std::chrono::steady_clock::now();
  auto t2 = std::chrono::system_clock::now();
  uint64_t nanos_t1 = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          t1.time_since_epoch())
                          .count();
  uint64_t nanos_t2 = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          t2.time_since_epoch())
                          .count();
  //   CHECK(nanos_t2 > nanos_t1) << "Assumptions about steady clock aren't
  //   true";
  return nanos_t2 - nanos_t1;
}

uint64_t steady_clock_offset = calculate_steady_clock_delta();


struct path_leaf_string {
  std::string
  operator()(const boost::filesystem::directory_entry &entry) const {
    return entry.path().leaf().string();
  }
};

std::vector<std::string> listdir(std::string directory) {
  std::vector<std::string> filenames;
  boost::filesystem::path p(directory);
  boost::filesystem::directory_iterator start(p);
  boost::filesystem::directory_iterator end;
  std::transform(start, end, std::back_inserter(filenames), path_leaf_string());
  return filenames;
}

bool exists(std::string filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}

long filesize(std::string filename) {
  struct stat buffer;
  int rc = stat(filename.c_str(), &buffer);
  return rc == 0 ? buffer.st_size : -1;
}
} // namespace util
} // namespace asiohpn

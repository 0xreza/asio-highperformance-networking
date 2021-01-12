// TODO: credits to Clockwork

#ifndef _ASIOHPN_UTIL_H_
#define _ASIOHPN_UTIL_H_

#include "common.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace asiohpn {

typedef std::chrono::steady_clock::time_point time_point;

namespace util {

// High-resolution timer, current time in nanoseconds
std::uint64_t now();
time_point hrt();
std::uint64_t nanos(time_point t);

void intro();

std::vector<std::string> split(std::string string, char delimiter = ',');
std::uint32_t get_random_identifier();

std::uint64_t nanos(time_point t);
// std::string nowString();

std::vector<std::string> listdir(std::string directory);
bool exists(std::string filename);
long filesize(std::string filename);

// A hash function used to hash a pair of any kind
// Source:
// https://www.geeksforgeeks.org/how-to-create-an-unordered_map-of-pairs-in-c/
struct hash_pair {
  template <class T1, class T2>
  size_t operator()(const std::pair<T1, T2> &p) const {
    auto hash1 = std::hash<T1>{}(p.first);
    auto hash2 = std::hash<T2>{}(p.second);
    return hash1 ^ hash2;
  }
};

struct hash_tuple {
  template <class T1, class T2, class T3>
  size_t operator()(const std::tuple<T1, T2, T3> &t) const {
    auto hash1 = std::hash<T1>{}(std::get<0>(t));
    auto hash2 = std::hash<T2>{}(std::get<1>(t));
    auto hash3 = std::hash<T3>{}(std::get<2>(t));
    return hash1 ^ hash2 ^ hash3;
  }
};

template <typename T> class SlidingWindowT {
private:
  unsigned window_size;

  /* An order statistics tree is used to implement a wrapper around a C++
     set with the ability to know the ordinal number of an item in the set
     and also to get an item by its ordinal number from the set.
     The data structure I use is implemented in STL but only for GNU C++.
     Some sources are documented below:
       -- https://gcc.gnu.org/onlinedocs/libstdc++/ext/pb_ds/
       -- https://codeforces.com/blog/entry/11080
       --
     https://gcc.gnu.org/onlinedocs/libstdc++/ext/pb_ds/tree_based_containers.html
       --
     https://opensource.apple.com/source/llvmgcc42/llvmgcc42-2336.9/libstdc++-v3/testsuite/ext/pb_ds/example/tree_order_statistics.cc.auto.html
           --
     https://stackoverflow.com/questions/44238144/order-statistics-tree-using-gnu-pbds-for-multiset
       --
     https://www.geeksforgeeks.org/order-statistic-tree-using-fenwick-tree-bit/
     */

  typedef __gnu_pbds::tree<T, __gnu_pbds::null_type, std::less_equal<T>,
                           __gnu_pbds::rb_tree_tag,
                           __gnu_pbds::tree_order_statistics_node_update>
      OrderedMultiset;

  /* We maintain a list of data items (FIFO ordered) so that the latest
     and the oldest items can be easily tracked for insertion and removal.
     And we also maintain a parallel OrderedMultiset data structure where the
     items are stored in an order statistics tree so that querying, say, the
     99th percentile value is easy. We also maintain an upper bound on sliding
     window size. After the first few iterations, the number of data items
     is always equal to the upper bound. Thus, we have:
                  -- Invariant 1: q.size() == oms.size()
                  -- Invariant 2: q.size() <= window_size */
  std::list<T> q;
  OrderedMultiset oms;

public:
  SlidingWindowT() : window_size(100) {}
  SlidingWindowT(unsigned window_size) : window_size(window_size) {}

  /* Assumption: q.size() == oms.size() */
  unsigned get_size() { return q.size(); }

  /* Requirement: rank < oms.size() */
  T get_value(unsigned rank) { return (*(oms.find_by_order(rank))); }
  T get_percentile(float percentile) {
    float position = percentile * (q.size() - 1);
    unsigned up = ceil(position);
    unsigned down = floor(position);
    if (up == down)
      return get_value(up);
    return get_value(up) * (position - down) +
           get_value(down) * (up - position);
  }
  T get_min() { return get_value(0); }
  T get_max() { return get_value(q.size() - 1); }
  void insert(T latest) {
    q.push_back(latest);
    oms.insert(latest);
    if (q.size() > window_size) {
      uint64_t oldest = q.front();
      q.pop_front();
      auto it = oms.upper_bound(oldest);
      oms.erase(it); // Assumption: *it == oldest
    }
  }

  SlidingWindowT(unsigned window_size, T initial_value)
      : window_size(window_size) {
    insert(initial_value);
  }
};

class SlidingWindow : public SlidingWindowT<uint64_t> {
public:
  SlidingWindow() : SlidingWindowT(100) {}
  SlidingWindow(unsigned window_size) : SlidingWindowT(window_size) {}
};

#define DEBUG_PRINT(msg)                                                       \
  std::cout << __FILE__ << "::" << __LINE__ << "::" << __FUNCTION__ << " ";    \
  std::cout << msg << std::endl;
} // namespace util
} // namespace asiohpn

#endif

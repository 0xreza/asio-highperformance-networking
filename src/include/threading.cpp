#include "threading.h"
#include <algorithm>
#include <sched.h>
#include <sstream>
#include <thread>

#include "util.h"

namespace asiohpn {
namespace threading {

int scheduler = SCHED_FIFO;

void setPriority(int scheduler, int priority, pthread_t thId) {
  struct sched_param params;
  params.sched_priority = sched_get_priority_max(scheduler);
  int ret = pthread_setschedparam(thId, scheduler, &params);
  CHECK(ret == 0) << "Unable to set thread priority.  Don't forget to set "
                     "`rtprio` to unlimited in `limits.conf`.";

  int policy = 0;
  ret = pthread_getschedparam(thId, &policy, &params);
  CHECK(ret == 0) << "Unable to verify thread scheduler params";
  CHECK(policy == scheduler) << "Unable to verify thread scheduler params";
}

void setThreadToHighPriority(std::thread &thread) {
  setPriority(scheduler, sched_get_priority_max(scheduler),
              thread.native_handle());
}

void setThreadToLowPriority(std::thread &thread) {
  setPriority(scheduler, sched_get_priority_min(scheduler),
              thread.native_handle());
}

void initNetworkThread(std::thread &thread) { setThreadToHighPriority(thread); }

void initLoggerThread(std::thread &thread) { setThreadToLowPriority(thread); }

} // namespace threading
} // namespace asiohpn

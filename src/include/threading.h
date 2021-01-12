#ifndef _ASIOHPN_THREAD_H_
#define _ASIOHPN_THREAD_H_

#include <thread>
#include <vector>

namespace asiohpn {
namespace threading {

void setPriority(int scheduler, int priority, pthread_t thId);

void setThreadToHighPriority(std::thread &thread);
void setThreadToLowPriority(std::thread &thread);

void initNetworkThread(std::thread &thread);
void initLoggerThread(std::thread &thread);

} // namespace threading
} // namespace firehose

#endif

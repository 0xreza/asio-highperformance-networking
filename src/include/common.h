#ifndef _ASIO_HPN_COMMON_H_
#define _ASIO_HPN_COMMON_H_

#include "asio.hpp"
#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_vector.h"
#include "../../external/dmlc/logging.h"
#include <algorithm>
#include <arpa/inet.h>
#include <asio.hpp>
#include <atomic>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/serialization/string.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <ctype.h>
#include <deque>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace asiohpn {

const std::string default_localhost = "127.0.0.1";
const int tcp_rcv_buf_size = 33554432;
const int tcp_snd_buf_size = 33554432;

}

#endif
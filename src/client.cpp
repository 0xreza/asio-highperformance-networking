#include "api/client.h"

using namespace asiohpn;
using namespace asiohpn::client;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
  std::cout << "Client exiting ..." << std::endl;
  sleep(5);
  exit(signum);
}

void show_usage() {
  std::stringstream s;
  s << "\nUSAGE:\n";
  s << "  client [NODE_ID] [SERVER_IP:SERVER_PORT,...] [PUBLISH_RATE] "
       "[PAYLOAD_SIZE]\n\n";
  s << "\n";
  std::cout << s.str();
}

std::vector<std::string> server_address_string;
std::vector<std::pair<std::string, int>> server_addresses;

int main(int argc, char *argv[]) {

  if (argc < 5) {
    show_usage();
    return 1;
  }

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);

  uint32_t node_id = atol(argv[1]);
  std::vector<std::string> server_address_string;
  std::vector<std::pair<std::string, int>> server_addresses;
  if (argc > 2) {
    server_address_string = util::split(argv[2], ',');
    for (auto &address : server_address_string) {
      std::vector<std::string> temp = util::split(address, ':');
      server_addresses.push_back(
          std::make_pair(temp[0], atoi(temp[1].c_str())));
    }
  }

  std::string publish_rate_str(argv[3]);
  std::string payload_size_str(argv[4]);

  uint64_t publish_rate = std::stoull(argv[3], nullptr, 0);
  uint64_t payload_size = std::stoull(argv[4], nullptr, 0);

  std::default_random_engine generator;
  std::poisson_distribution<uint64_t> p_distribution((double)1000000000 /
                                                     publish_rate);

  std::cout << "Starting the Publisher ..." << std::endl;

  Client client(node_id);

  for (auto &server_address : server_addresses) {
    client.connect_to_server(server_address.first, server_address.second);
  }

  uint64_t global_id = 1;

  while (true) {

    auto start = std::chrono::high_resolution_clock::now();
    std::shared_ptr<Request> request = std::make_shared<Request>();
    request->header.sender_id = node_id;
    request->header.request_id = ++global_id;
    request->payload = std::string(payload_size, 'x'); // 1KB payload

    auto onSuccess = [](Response &result) {
      // std::cout << "publish -- on success" << std::endl;
    };
    client.send_request(request, onSuccess);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    uint64_t sleep_duration = p_distribution(generator) - duration.count();
    if (sleep_duration < 0) {
      sleep_duration = 0;
    }
    // std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_duration));
    std::this_thread::sleep_for(std::chrono::nanoseconds(2));
    // usleep(1);
  }

  client.join();
  return 0;
}

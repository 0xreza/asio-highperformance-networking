#include "api/server.h"

using namespace asiohpn;
using namespace asiohpn::server;

void signalHandler(int signum) {
  sleep(5);
  exit(signum);
}

void show_usage() {
  std::stringstream s;
  s << "\nUSAGE:\n";
  s << "  pump [NODE_ID] [PUBLISHERS_PORT] \n\n";
  std::cout << s.str();
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    show_usage();
    return 1;
  }

  uint64_t node_id = atol(argv[1]);
  int client_port = atoi(argv[2]);
  std::cout << "Starting the Server ..." << std::endl;
  Server server(client_port, node_id);
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  server.join();

  return 0;
}
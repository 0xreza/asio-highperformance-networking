#include "server.h"
#include "api_common.h"

namespace asiohpn {
namespace server {

using asio::ip::tcp;


void msg_req_rx::get(Request &request) {
  get_header(request.header, msg.header());
  request.timestamp = msg.timestamp();
  request.payload = msg.payload();
}

void msg_rsp_tx::set(Response &response) {
  set_header(response.header, msg.mutable_header());
  msg.set_payload(response.payload);
}


Server::Server(int client_port, uint32_t node_id)
    : node_id(node_id){
  connection_server = new ConnectionManager(client_port, this);
}

Server::~Server() {
  // TODO: delete connection and manager
}

void Server::on_request_receive(ClientConnection *connection,
                              Request request) {
  
}

void networkPrintThread(Server *server) {
  uint64_t last_print = util::now();
  uint64_t print_interval_nanos = 1000000000UL * 1;

  connection_stats client_previous_stats;
  while (true) {
    uint64_t now = util::now();
    if (last_print + print_interval_nanos > now) {
      usleep(100000);
      continue;
    }


    connection_stats client_stats;

    std::cout << "clients: " << server->client_connections.size() << std::endl;
    for (auto &client_connection : server->client_connections) {
      client_stats += client_connection->stats;
    }

    client_stats -= client_previous_stats;
    client_previous_stats = client_stats;

    float duration = (now - last_print) / 1000000000.0;
    client_stats /= duration;

    std::stringstream msg;
    msg << std::fixed << std::setprecision(1);
    
    msg << "Clients: ";
    msg << (client_stats.bytes_sent / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << client_stats.messages_sent << " msgs) snd, ";
    msg << (client_stats.bytes_received / (1024 * 1024.0)) << "MB/s ";
    msg << "(" << client_stats.messages_received << " msgs) rcv, ";

    msg << std::endl;

    std::cout << msg.str();

    last_print = now;
  }
}

void Server::join() {

  network_printer = std::thread(&networkPrintThread, this);
  threading::initLoggerThread(network_printer);
  connection_server->join();
}


//--- ClientConnection

ClientConnection::ClientConnection(asio::io_service &io_service, Server *server)
    : message_connection(io_service, *this), msg_tx_(this, *this),
      server(server), connected(false) {}

message_rx *ClientConnection::new_rx_message(message_connection *tcp_conn,
                                             uint64_t header_len,
                                             uint64_t body_len,
                                             uint64_t msg_type,
                                             uint64_t msg_id) {
  if (msg_type == m_req) {
    auto msg = new msg_req_rx();
    msg->set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void ClientConnection::ready() { connected.store(true); }

void ClientConnection::closed() {
  std::cout << "connection close handler ()" << std::endl;
}

void ClientConnection::aborted_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  delete req;
}

void ClientConnection::completed_receive(message_connection *tcp_conn,
                                         message_rx *req) {

  if (auto rsp = dynamic_cast<msg_req_rx *>(req)) {
    auto result = std::make_shared<Request>();
    rsp->get(*result);
    server->on_request_receive(this, *result);

  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void ClientConnection::completed_transmit(message_connection *tcp_conn,
                                          message_tx *req) {
  delete req;
}

void ClientConnection::aborted_transmit(message_connection *tcp_conn,
                                        message_tx *req) {
  delete req;
}

void ClientConnection::ack_request(Response &response) {
  auto tx = new msg_rsp_tx();
  tx->set(response);
  msg_tx_.send_message(tx);
}


//---- ConnectionManager

ConnectionManager::ConnectionManager(int port, Server *server)
    : io_service(),
      alive(true),
      network_thread(&ConnectionManager::run, this, port), server(server) {
  threading::initNetworkThread(network_thread);
}

void ConnectionManager::shutdown(bool awaitShutdown) {
  io_service.stop();
  if (awaitShutdown) {
    join();
  }
}

void ConnectionManager::join() {
  while (alive.load())
    ;
}

void ConnectionManager::run(int port) {
  try {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_service, endpoint);
    start_accept(&acceptor);
    std::cout << "Listening for publishers on " << endpoint << std::endl;
    asio::io_service::work work(io_service);
    io_service.run();
  } catch (std::exception &e) {
    std::cerr << "Exception in network thread: " << e.what();
    exit(0);
  } catch (const char *m) {
    std::cerr << "Exception in network thread: " << m;
    exit(0);
  }
  std::cout << "Server exiting" << std::endl;
  alive.store(false);
}

void ConnectionManager::start_accept(tcp::acceptor *acceptor) {
  auto connection = new ClientConnection(acceptor->get_io_service(), server);

  acceptor->async_accept(
      connection->get_socket(),
      boost::bind(&ConnectionManager::handle_accept, this, connection, acceptor,
                  asio::placeholders::error));
}

void ConnectionManager::handle_accept(ClientConnection *connection,
                                    tcp::acceptor *acceptor,
                                    const asio::error_code &error) {
  if (error) {
    throw std::runtime_error(error.message());
  }
  std::cout << "Connected to publisher" << std::endl;
  connection->established();
  server->client_connections.push_back(connection);
  start_accept(acceptor);
}


} // namespace server
} // namespace asiohpn
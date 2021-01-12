#include "client.h"
#include "api_common.h"

namespace asiohpn {
namespace client {


void msg_req_tx::set(Request &request) {
  set_header(request.header, msg.mutable_header());
  msg.set_timestamp(request.timestamp);
  msg.set_payload(request.payload);
}

void msg_rsp_rx::get(Response &response) {
  get_header(response.header, msg.header());
  response.payload = msg.payload();
}



Client::Client(uint32_t node_id) : node_id(node_id), request_id_seed(0) {
  
  connection_manager = new ConnectionServer(this);
  
}

Client::~Client() {
  // TODO: delete connection and manager
}

void Client::connect_to_server(std::string host, int port) {
  server_connections.push_back(
      connection_manager->connect(host, std::to_string(port)));
}

void Client::on_response_receive(
    Response response) {}

void Client::send_request(std::shared_ptr<Request> request,
                        std::function<void(Response &)> callback) {
  request->timestamp = util::now();
  server_connections[0]->send_request(request, callback);
}

void Client::join() { connection_manager->join(); }

//----- Connection -----
ServerConnection::ServerConnection(asio::io_service &io_service, Client *client)
    : message_connection(io_service, *this), client(client),
      msg_tx_(this, *this), alive(true), connected(false) {}

message_rx *ServerConnection::new_rx_message(message_connection *tcp_conn,
                                             uint64_t header_len,
                                             uint64_t body_len,
                                             uint64_t msg_type,
                                             uint64_t msg_id) {
  if (msg_type == m_rsp) {
    auto msg = new msg_rsp_rx();
    // msg->set  set_msg_id(msg_id);
    return msg;
  }
  std::cerr << "Unsupported msg_type " << msg_type << std::endl;
  return nullptr;
}

void ServerConnection::ready() { connected.store(true); }

void ServerConnection::closed() {
  std::cout << "connection closed!" << std::endl;
}

void ServerConnection::aborted_receive(message_connection *tcp_conn,
                                       message_rx *req) {
  delete req;
}

void ServerConnection::completed_receive(message_connection *tcp_conn,
                                         message_rx *req) {
  // uint64_t now = util::now();

  if (auto rsp = dynamic_cast<msg_rsp_rx *>(req)) {
    auto result = std::make_shared<Response>();
    rsp->get(*result);
    client->on_response_receive(*result);
  } else {
    std::cerr << "Received an unsupported message type" << std::endl;
  }

  delete req;
}

void ServerConnection::completed_transmit(message_connection *tcp_conn,
                                          message_tx *req) {
  delete req;
}

void ServerConnection::aborted_transmit(message_connection *tcp_conn,
                                        message_tx *req) {
  delete req;
}

void ServerConnection::send_request(std::shared_ptr<Request> request,
                                    std::function<void(Response &)> callback) {
  auto tx = new msg_req_tx();
  tx->set(*request);
  msg_tx_.send_message(tx);
}

ConnectionServer::ConnectionServer(Client *client)
    : io_service(), client(client),
      network_thread(&ConnectionServer::run, this), alive(true) {
  threading::initNetworkThread(network_thread);
}

void ConnectionServer::run() {

  try {
    asio::io_service::work work(io_service);
    io_service.run();
  } catch (std::exception &e) {
    alive.store(false);
    exit(0);
    CHECK(false) << "Exception in network thread: " << e.what();
  } catch (const char *m) {
    alive.store(false);
    exit(0);
    CHECK(false) << "Exception in network thread: " << m;
  }
  // }
}

void ConnectionServer::shutdown(bool awaitCompletion) {
  alive.store(false);
  io_service.stop();
  if (awaitCompletion) {
    join();
  }
}

void ConnectionServer::join() {
  while (alive.load())
    ;
  network_thread.join();
}

ServerConnection *ConnectionServer::connect(std::string host,
                                            std::string port) {
  try {
    ServerConnection *c = new ServerConnection(io_service, client);
    c->connect(host, port);
    std::cout << "Connecting to server @ " << host << ":" << port << std::endl;
    while (alive.load() && !c->connected.load())
      ; // If connection fails, alive sets to false
    std::cout << "Connection established" << std::endl;
    return c;
  } catch (std::exception &e) {
    alive.store(false);
    io_service.stop();
    std::cerr << "Exception in network thread: " << e.what();
    exit(0);
  } catch (const char *m) {
    alive.store(false);
    io_service.stop();
    std::cerr << "Exception in network thread: " << m;
    exit(0);
  }
  return nullptr;
}

} // namespace client
} // namespace asiohpn
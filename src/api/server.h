#ifndef _ASIOHPN_SERVER_API_H_
#define _ASIOHPN_SERVER_API_H_

#include "api_common.h"
#include "message.h"
#include "network.h"
#include "threading.h"

namespace asiohpn {
namespace server {

using asio::ip::tcp;



class msg_req_rx : public msg_protobuf_rx<m_req, RequestProto, Request> {
public:
  void get(Request &request);
};


class msg_rsp_tx : public msg_protobuf_tx<m_rsp, ResponseProto, Response> {
public:
  void set(Response &response);
};



class ClientConnection;
class ConnectionManager;

class Server {
public:
  bool print;
  uint64_t node_id;

  ConnectionManager *connection_server;

  std::vector<ClientConnection *> client_connections;
  std::thread network_printer;

  Server(int client_port, uint32_t node_id);
  ~Server();
  void on_request_receive(ClientConnection *connection, Request request);
  void join();
};

// ------- Server side of server - client connection
class ClientConnection : public message_connection, public message_handler {
private:
  message_sender msg_tx_;
  Server *server;

protected:
  virtual message_rx *new_rx_message(message_connection *tcp_conn,
                                     uint64_t header_len, uint64_t body_len,
                                     uint64_t msg_type, uint64_t msg_id);

  virtual void ready();
  virtual void closed();

  virtual void aborted_receive(message_connection *tcp_conn, message_rx *req);
  virtual void completed_receive(message_connection *tcp_conn, message_rx *req);
  virtual void completed_transmit(message_connection *tcp_conn,
                                  message_tx *req);
  virtual void aborted_transmit(message_connection *tcp_conn, message_tx *req);

public:
  std::atomic_bool connected;
  ClientConnection(asio::io_service &io_service, Server *server);
  void ack_request(Response &response);
};

//---- ConnectionServer

class ConnectionManager {
private:
  asio::io_service io_service;
  std::atomic_bool alive;
  std::thread network_thread;

public:
  Server *server;
  ConnectionManager(int port, Server *server);
  void shutdown(bool awaitShutdown);
  void join();
  void run(int port);

private:
  void start_accept(tcp::acceptor *acceptor);
  void handle_accept(ClientConnection *connection, tcp::acceptor *acceptor,
                     const asio::error_code &error);
};


} // namespace server
} // namespace asiohpn

#endif
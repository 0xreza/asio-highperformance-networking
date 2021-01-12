#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "api_common.h"
#include "message.h"
#include "network.h"
#include "threading.h"

namespace asiohpn {
namespace client {


class msg_req_tx : public msg_protobuf_tx<m_req, RequestProto, Request> {
public:
  void set(Request &request);
};

class msg_rsp_rx : public msg_protobuf_rx<m_rsp, ResponseProto, Response> {
public:
  void get(Response &response);
};


class ServerConnection;
class ConnectionServer;

class Client {
public:
  uint32_t node_id;
  bool print;
  std::atomic_int request_id_seed;
  ConnectionServer *connection_manager;
  std::vector<ServerConnection *> server_connections;

  Client(uint32_t node_id);
  ~Client();
  void connect_to_server(std::string host, int port);
  void send_request(std::shared_ptr<Request> request,
                    std::function<void(Response &)> callback);
  void on_response_receive(Response response);
  void join();
};

class ServerConnection : public message_connection, public message_handler {
private:
  Client *client;
  message_sender msg_tx_;

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
  std::atomic_bool alive;
  std::atomic_bool connected;
  ServerConnection(asio::io_service &io_service, Client *client);
  void send_request(std::shared_ptr<Request> request,
                    std::function<void(Response &)> callback);
};

class ConnectionServer {
private:
  asio::io_service io_service;
  Client *client;
  std::thread network_thread;
  std::atomic_bool alive;
  void run();

public:
  ConnectionServer(Client *client);
  void shutdown(bool awaitCompletion = false);
  void join();

  ServerConnection *connect(std::string host, std::string port);
};
} // namespace client
} // namespace asiohpn

#endif
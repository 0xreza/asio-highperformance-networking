#include "network.h"
namespace asiohpn {

message_sender::message_sender(message_connection *conn,
                               message_handler &handler)
    : socket_(conn->get_socket()), conn_(conn), req_(0), handler_(handler) {}

void message_sender::send_message(message_tx *req) {

  // auto start = std::chrono::high_resolution_clock::now();
  tx_queue_.push(req);
  // auto stop_1 = std::chrono::high_resolution_clock::now();
  conn_->io_service_.post(boost::bind(&message_sender::try_send, this));
  // auto stop_2 = std::chrono::high_resolution_clock::now();
  // auto duration_1 =
  // std::chrono::duration_cast<std::chrono::nanoseconds>(stop_1 - start); auto
  // duration_2 = std::chrono::duration_cast<std::chrono::nanoseconds>(stop_2 -
  // stop_1); std::cout << duration_1.count() << " -- " << duration_2.count() <<
  // std::endl;
}

void message_sender::try_send() {
  std::lock_guard<std::mutex> lock(queue_mutex);

  if (!req_)
    send_next_message();
}

void message_sender::send_next_message() {
  message_tx *req;
  if (!tx_queue_.try_pop(req))
    return;
  start_send(*req);
}

void message_sender::start_send(message_tx &req) {
  /* header length,  body length, message type, message id */
  pre_header[0] = req.get_tx_header_len();
  // std::cout << pre_header[0] << "\n";
  // pre_header[1] = req.get_tx_body_len();
  pre_header[1] = req.get_tx_msg_type();
  pre_header[2] = req.get_tx_req_id();

  req.serialize_tx_header(header_buf);

  // pre_header[4] = util::now();
  // pre_header[5] = handler_.local_delta_;

  conn_->stats.message_sent(pre_header[0] + 24);

  req_ = &req;
  asio::async_write(socket_, asio::buffer(pre_header),
                    boost::bind(&message_sender::handle_prehdr_sent, this,
                                asio::placeholders::error,
                                asio::placeholders::bytes_transferred));
}

void message_sender::handle_prehdr_sent(const asio::error_code &error,
                                        size_t bytes_transferred) {
  if (error) {
    abort_connection(error.message());
    return;
  }
  if (bytes_transferred != 24) {
    abort_connection("Invalid number of bytes sent for header lengths");
    return;
  }

  asio::async_write(socket_,
                    asio::buffer(header_buf, req_->get_tx_header_len()),
                    boost::bind(&message_sender::handle_hdr_sent, this,
                                asio::placeholders::error,
                                asio::placeholders::bytes_transferred));
}

void message_sender::handle_hdr_sent(const asio::error_code &error,
                                     size_t bytes_transferred) {
  if (error) {
    abort_connection(error.message());
    return;
  }
  if (bytes_transferred != req_->get_tx_header_len()) {
    abort_connection("Invalid number of bytes sent for header");
    return;
  }

  body_left = req_->get_tx_body_len();
  // next_body_seg();

  req_->tx_complete();
  handler_.completed_transmit(conn_, req_);
  std::lock_guard<std::mutex> lock(queue_mutex);
  req_ = 0;
  send_next_message();
  
}


void message_sender::abort_connection(const char *msg) { conn_->close(msg); }

message_receiver::message_receiver(message_connection *conn,
                                   message_handler &handler)
    : socket_(conn->get_socket()), conn_(conn), handler_(handler) {}

void message_receiver::start() { read_new_message(); }

void message_receiver::abort_connection(const char *msg) { conn_->close(msg); }

/* begin reading a new message */
void message_receiver::read_new_message() {
  /* begin by reading the pre-header */
  asio::async_read(socket_, asio::buffer(pre_header),
                   boost::bind(&message_receiver::handle_pre_read, this,
                               asio::placeholders::error,
                               asio::placeholders::bytes_transferred));
}

/* common pre header received */
void message_receiver::handle_pre_read(const asio::error_code &error,
                                       size_t bytes_transferred) {
  if (error) {
    abort_connection(error.message());
    return;
  }

  if (bytes_transferred != 24) {
    abort_connection("Invalid number of bytes read for header lengths");
    return;
  }

  if (pre_header[0] > max_header_len) {
    abort_connection("Specified header length larger than supported maximum");
    return;
  }

  // rx_begin_ = util::now();

  conn_->stats.message_received(pre_header[0] + 24);

  asio::async_read(socket_, asio::buffer(header_buf, pre_header[0]),
                   boost::bind(&message_receiver::handle_header_read, this,
                               asio::placeholders::error,
                               asio::placeholders::bytes_transferred));
}

/* header received */
void message_receiver::handle_header_read(const asio::error_code &error,
                                          size_t bytes_transferred) {
  if (error) {
    abort_connection(error.message());
    return;
  }

  if (bytes_transferred != pre_header[0]) {
    abort_connection("Header incomplete");
    return;
  }

  // int64_t delta = rx_begin_ - pre_header[4];
  // handler_.synchronize(delta, pre_header[5]);

  req_ = handler_.new_rx_message(conn_, pre_header[0], 0, pre_header[1],
                                 pre_header[2]);
  req_->header_received(header_buf, pre_header[0]);

  // body_left = 0; // pre_header[1];
  // next_body_seg();

  req_->rx_complete();
  handler_.completed_receive(conn_, req_);
  req_ = 0;
  read_new_message();
}


message_connection::message_connection(asio::io_service &io_service,
                                       message_handler &handler)
    : socket_(io_service), resolver_(io_service),
      msg_rx_(message_receiver(this, handler)), is_closed(ATOMIC_FLAG_INIT),
      io_service_(io_service) {}

/* establish outgoing connection */
void message_connection::connect(std::string server, std::string service) {
  asio::ip::tcp::resolver::query query(server, service);
  resolver_.async_resolve(query,
                          boost::bind(&message_connection::handle_resolved,
                                      this, asio::placeholders::error,
                                      asio::placeholders::iterator));
}

/* connection on socket established externally (e.g. through acceptor) */
void message_connection::established() {
  /* disable nagle */
  socket_.set_option(asio::ip::tcp::no_delay(true));
  socket_.set_option(
      asio::socket_base::receive_buffer_size(asiohpn::tcp_rcv_buf_size));
  socket_.set_option(
      asio::socket_base::send_buffer_size(asiohpn::tcp_snd_buf_size));

  msg_rx_.start();
  ready();
}

void message_connection::ready() {}

void message_connection::closed() {}

asio::ip::tcp::socket &message_connection::get_socket() { return socket_; }

void message_connection::handle_resolved(
    const asio::error_code &error,
    asio::ip::tcp::resolver::iterator endpoint_iterator) {
  if (error) {
    abort_connection(error.message());
    return;
  }

  asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
  socket_.async_connect(endpoint,
                        boost::bind(&message_connection::handle_established,
                                    this, asio::placeholders::error));
}

void message_connection::handle_established(const asio::error_code &error) {
  if (error) {
    abort_connection(error.message());
    return;
  }

  established();
}

void message_connection::close(const char *reason) {
  if (!is_closed.test_and_set()) {
    std::cout << "Closing connection " << this;
    if (reason) {
      std::cout << ", reason: " << reason;
    }
    std::cout << std::endl;
    socket_.cancel();
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both);
    socket_.close();
    this->closed();
  }
}

void message_connection::abort_connection(const char *msg) { close(msg); }

} // namespace firehose

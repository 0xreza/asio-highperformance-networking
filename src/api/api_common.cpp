#include "api_common.h"

namespace asiohpn {

void set_header(RequestHeader &request_header, RequestHeaderProto *proto) {
  proto->set_sender_id(request_header.sender_id);
  proto->set_request_id(request_header.request_id);
}

void set_header(ResponseHeader &response_header, ResponseHeaderProto *proto) {
  proto->set_request_id(response_header.request_id);
  proto->set_status(response_header.status);
  proto->set_message(response_header.message);
}

void get_header(RequestHeader &request_header,
                const RequestHeaderProto &proto) {
  request_header.sender_id = proto.sender_id();
  request_header.request_id = proto.request_id();
}

void get_header(ResponseHeader &response_header,
                const ResponseHeaderProto &proto) {
  response_header.request_id = proto.request_id();
  response_header.status = proto.status();
  response_header.message = proto.message();
}








} // namespace asiohpn

#ifndef _ASIOHPN_API_COMMON_H_
#define _ASIOHPN_API_COMMON_H_

#include "common.h"
#include "message.h"
#include <asiohpn.pb.h>

namespace asiohpn {

struct RequestHeader {
  uint32_t sender_id;
  uint64_t request_id;
};

struct Request {
  RequestHeader header;
  uint64_t timestamp;
  std::string payload;
};

struct ResponseHeader {
  uint64_t request_id;
  uint8_t status;
  std::string message;
};

struct Response {
  ResponseHeader header;
  std::string payload;
};


void set_header(RequestHeader &request_header, RequestHeaderProto *proto);
void set_header(ResponseHeader &response_header, ResponseHeaderProto *proto);
void get_header(RequestHeader &request_header, const RequestHeaderProto &proto);
void get_header(ResponseHeader &response_header, const ResponseHeaderProto &proto);








} // namespace asiohpn

#endif

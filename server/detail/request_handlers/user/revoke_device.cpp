#include <api/user/request/revoke_device.pb.h>
#include <api/user/response/revoke_device.pb.h>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::RevokeDevice& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  //

  //
  return {};
}

}  // namespace detail

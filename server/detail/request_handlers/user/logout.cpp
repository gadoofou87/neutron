#include <api/user/request/logout.pb.h>
#include <api/user/response/logout.pb.h>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::Logout& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  impl.client_manager.mark_as_unauthorized(client.id());

  //
  return {};
}

}  // namespace detail

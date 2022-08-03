#include <api/chat/event.pb.h>
#include <api/chat/request/create_group.pb.h>
#include <api/chat/response/create_group.pb.h>
#include <api/chat/type.pb.h>

#include <pqxx/pqxx>

#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "server_impl.hpp"
#include "utils/span/from.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::chat::request::CreateGroup& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  // auto& impl = ServerImpl::instance();

  auto chat_id = Helpers::Chat::create_chat(api::chat::GROUP);

  //

  api::chat::response::CreateGroup response;

  auto* response_result = response.mutable_result();

  response_result->set_id(chat_id);

  return response.SerializeAsString();
}

}  // namespace detail

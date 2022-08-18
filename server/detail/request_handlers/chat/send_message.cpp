#include <api/chat/event.pb.h>
#include <api/chat/event/new_message.pb.h>
#include <api/chat/request/send_message.pb.h>
#include <api/chat/response/send_message.pb.h>
#include <api/event.pb.h>

#include <pqxx/pqxx>

#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "server_impl.hpp"
#include "utils/span/from.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::chat::request::SendMessage& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  if (!Helpers::Chat::does_chat_exist(request.chat_id())) {
    api::chat::response::SendMessage response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::SendMessage_Error_Code_CHAT_NOT_FOUND);

    return response.SerializeAsString();
  }

  auto members_user_ids = Helpers::Chat::get_chat_members(request.chat_id());

  if (std::find(members_user_ids.begin(), members_user_ids.end(), client.user_id()) ==
      members_user_ids.end()) {
    api::chat::response::SendMessage response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::SendMessage_Error_Code_YOU_ARE_NOT_MEMBER);

    return response.SerializeAsString();
  }

  if (Helpers::Chat::is_deleted(request.chat_id())) {
    api::chat::response::SendMessage response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::SendMessage_Error_Code_CHAT_IS_DELETED);

    return response.SerializeAsString();
  }

  {
    api::chat::event::NewMessage inner_chat_event;

    inner_chat_event.mutable_message()->CopyFrom(request.message());

    auto chat_event = Helpers::Chat::create_event(
        request.chat_id(), client.user_id(), api::chat::Event_Type_NEW_MESSAGE, inner_chat_event);

    auto event = Helpers::create_event(api::Event_Type_CHAT, chat_event);

    auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
      //
      client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
    };

    for (const auto& user_id : members_user_ids) {
      impl.client_manager.do_for_each(user_id, do_notify);
    }
  }

  api::chat::response::SendMessage response;

  return response.SerializeAsString();
}

}  // namespace detail

#include <api/chat/event.pb.h>
#include <api/chat/event/chat_deleted.pb.h>
#include <api/chat/request/delete.pb.h>
#include <api/chat/response/delete.pb.h>
#include <api/chat/type.pb.h>
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
                                   const api::chat::request::Delete& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::Chat::does_chat_exist(request.id())) {
    api::chat::response::Delete response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::Delete_Error_Code_CHAT_NOT_FOUND);

    return response.SerializeAsString();
  }

  auto members_user_ids = Helpers::Chat::get_chat_members(request.id());

  if (std::find(members_user_ids.begin(), members_user_ids.end(), client.user_id()) ==
      members_user_ids.end()) {
    api::chat::response::Delete response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::Delete_Error_Code_YOU_ARE_NOT_MEMBER);

    return response.SerializeAsString();
  }

  if (!Helpers::Chat::is_chat_owner(request.id(), client.user_id())) {
    api::chat::response::Delete response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::Delete_Error_Code_INSUFFICIENT_RIGHTS);

    return response.SerializeAsString();
  }
  if (Helpers::Chat::is_deleted(request.id())) {
    api::chat::response::Delete response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::Delete_Error_Code_CHAT_IS_ALREADY_DELETED);

    return response.SerializeAsString();
  }

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "UPDATE chats "
        "SET deleted = true "
        "WHERE id = $1",
        request.id());

    transaction.commit();
  }

  {
    api::chat::event::ChatDeleted inner_chat_event;

    auto chat_event = Helpers::Chat::create_event(
        request.id(), client.user_id(), api::chat::Event_Type_CHAT_DELETED, inner_chat_event);

    auto event = Helpers::create_event(api::Event_Type_CHAT, chat_event);

    auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
      //
      client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
    };

    for (const auto& user_id : members_user_ids) {
      impl.client_manager.do_for_each(user_id, do_notify);
    }
  }

  api::chat::response::Delete response;

  return response.SerializeAsString();
}

}  // namespace detail

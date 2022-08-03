#include <api/chat/event.pb.h>
#include <api/chat/event/member_deleted.pb.h>
#include <api/chat/request/delete_member.pb.h>
#include <api/chat/response/delete_member.pb.h>
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
                                   const api::chat::request::DeleteMember& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::Chat::does_chat_exist(request.chat_id())) {
    api::chat::response::DeleteMember response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::DeleteMember_Error_Code_CHAT_NOT_FOUND);

    return response.SerializeAsString();
  }

  auto members_user_ids = Helpers::Chat::get_chat_members(request.chat_id());

  if (std::find(members_user_ids.begin(), members_user_ids.end(), client.user_id()) ==
      members_user_ids.end()) {
    api::chat::response::DeleteMember response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::DeleteMember_Error_Code_YOU_ARE_NOT_MEMBER);

    return response.SerializeAsString();
  }

  if (!Helpers::Chat::is_chat_owner(request.chat_id(), client.user_id())) {
    api::chat::response::DeleteMember response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::DeleteMember_Error_Code_INSUFFICIENT_RIGHTS);

    return response.SerializeAsString();
  }

  if (std::find(members_user_ids.begin(), members_user_ids.end(), request.user_id()) ==
      members_user_ids.end()) {
    api::chat::response::DeleteMember response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::DeleteMember_Error_Code_USER_IS_NOT_MEMBER);

    return response.SerializeAsString();
  }

  {
    pqxx::work transaction(connection);

    transaction.exec_params("DELETE FROM chats_members WHERE chat_id = $1 AND user_id = $2",
                            request.chat_id(), request.user_id());

    transaction.commit();
  }

  std::erase(members_user_ids, request.user_id());

  {
    api::chat::event::MemberDeleted inner_chat_event;

    inner_chat_event.set_user_id(request.user_id());

    auto chat_event =
        Helpers::Chat::create_event(request.chat_id(), client.user_id(),
                                    api::chat::Event_Type_MEMBER_DELETED, inner_chat_event);

    auto event = Helpers::create_event(api::Event_Type_CHAT, chat_event);

    auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
      //
      client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
    };

    for (const auto& user_id : members_user_ids) {
      impl.client_manager.do_for_each(user_id, do_notify);
    }
    impl.client_manager.do_for_each(request.user_id(), do_notify);
  }

  Helpers::Chat::rotate_keys(request.chat_id(), members_user_ids);

  api::chat::response::DeleteMember response;

  return response.SerializeAsString();
}

}  // namespace detail

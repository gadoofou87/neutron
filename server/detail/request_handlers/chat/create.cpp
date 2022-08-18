#include <api/chat/event.pb.h>
#include <api/chat/event/chat_created.pb.h>
#include <api/chat/request/create.pb.h>
#include <api/chat/response/create.pb.h>
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
                                   const api::chat::request::Create& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  uint64_t chat_id;

  {
    pqxx::work transaction(connection);

    auto result = transaction.exec_params1(
        "INSERT INTO chats "
        "RETURNING id");

    transaction.commit();

    chat_id = result[0].as<int64_t>();
  }

  {
    api::chat::event::ChatCreated inner_chat_event;

    auto chat_event = Helpers::Chat::create_event(
        chat_id, client.user_id(), api::chat::Event_Type_CHAT_CREATED, inner_chat_event);

    {
      Helpers::Chat::add_member(chat_id, client.user_id(), chat_event.id());

      Helpers::Chat::set_owner(chat_id, client.user_id());
    }

    auto event = Helpers::create_event(api::Event_Type_CHAT, chat_event);

    auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
      //
      client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
    };

    impl.client_manager.do_for_each(client.user_id(), do_notify);
  }

  api::chat::response::Create response;

  auto* response_result = response.mutable_result();

  response_result->set_id(chat_id);

  return response.SerializeAsString();
}

}  // namespace detail

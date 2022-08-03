#include <api/chat/event.pb.h>
#include <api/chat/request/sync.pb.h>
#include <api/chat/response/sync.pb.h>
#include <fmt/core.h>

#include <pqxx/pqxx>

#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "server_impl.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client, const api::chat::request::Sync& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::Chat::does_chat_exist(request.chat_id())) {
    api::chat::response::Sync response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::Sync_Error_Code_CHAT_NOT_FOUND);

    return response.SerializeAsString();
  }

  if (!Helpers::Chat::is_chat_member(request.chat_id(), client.user_id())) {
    api::chat::response::Sync response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::Sync_Error_Code_YOU_ARE_NOT_MEMBER);

    return response.SerializeAsString();
  }

  api::chat::response::Sync response;

  auto* response_result = response.mutable_result();

  {
    pqxx::read_transaction transaction(connection);

    pqxx::params params;
    pqxx::placeholders placeholders;
    std::stringstream query;

    query << fmt::format(
        "SELECT * FROM chats_events "
        "WHERE chat_id = {0} AND id >= "
        "( "
        " SELECT first_accessible_event_id FROM chats_members "
        " WHERE chat_id = {0} ",
        placeholders.view());

    params.append(request.chat_id()), placeholders.next();

    query << fmt::format(
        "AND user_id = {} "
        ") ",
        placeholders.view());

    params.append(client.user_id()), placeholders.next();

    if (request.has_last_event_id()) {
      query << fmt::format("AND id > {} ", placeholders.view());

      params.append(request.last_event_id()), placeholders.next();
    }

    query << fmt::format(
        "ORDER BY id ASC "
        "LIMIT {}",
        placeholders.view());

    params.append(request.count()), placeholders.next();

    auto result = transaction.exec_params(query.str(), params);

    transaction.commit();

    response_result->mutable_values()->Reserve(result.size());

    for (const auto& row : result) {
      auto* event = response_result->add_values()->mutable_event();

      event->set_id(row["id"].as<int64_t>());
      event->set_creation_timestamp(row["creation_timestamp"].as<int64_t>());
      event->set_chat_id(row["chat_id"].as<int64_t>());
      event->set_owner_user_id(row["owner_user_id"].as<int64_t>());
      event->set_type(static_cast<api::chat::Event_Type>(row["type"].as<int32_t>()));
      event->set_data(row["data"].as<std::string>());
    }
  }

  return response.SerializeAsString();
}

}  // namespace detail

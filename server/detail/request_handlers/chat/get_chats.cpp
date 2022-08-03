#include <api/chat/request/get_chats.pb.h>
#include <api/chat/response/get_chats.pb.h>
#include <fmt/core.h>

#include <pqxx/pqxx>

#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "server_impl.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::chat::request::GetChats& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  api::chat::response::GetChats response;

  {
    pqxx::read_transaction transaction(connection);

    pqxx::params params;
    pqxx::placeholders placeholders;
    std::stringstream query;

    query << fmt::format(
        "SELECT chat_id, last_event FROM chats_members, LATERAL "
        "( "
        " SELECT creation_timestamp FROM chats_events "
        " WHERE chats_events.chat_id = chats_members.chat_id "
        " ORDER BY creation_timestamp DESC "
        " LIMIT 1 "
        ") AS last_event "
        "WHERE user_id = {} ",
        placeholders.view());

    params.append(client.user_id()), placeholders.next();

    if (request.filter().has_type()) {
      query << fmt::format("AND type = {} ", placeholders.view());

      params.append(std::to_underlying(request.filter().type())), placeholders.next();
    }

    query << "ORDER BY event.creation_timestamp DESC";

    auto result = transaction.exec_params(query.str(), params);

    auto* response_result = response.mutable_result();

    response_result->mutable_values()->Reserve(result.size());

    for (const auto& row : result) {
      auto* value = response_result->add_values();

      value->set_chat_id(row["chat_id"].as<int64_t>());
      value->set_last_event_timestamp(row["last_event.creation_timestamp"].as<int64_t>());
    }

    transaction.commit();
  }

  return response.SerializeAsString();
}

}  // namespace detail

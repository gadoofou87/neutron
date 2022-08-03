#include <api/chat/request/get_members.pb.h>
#include <api/chat/response/get_members.pb.h>
#include <fmt/core.h>

#include <pqxx/pqxx>

#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "server_impl.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::chat::request::GetMembers& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::Chat::does_chat_exist(request.filter().chat_id())) {
    api::chat::response::GetMembers response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::GetMembers_Error_Code_CHAT_NOT_FOUND);

    return response.SerializeAsString();
  }
  if (!Helpers::Chat::is_chat_member(request.filter().chat_id(), client.user_id())) {
    api::chat::response::GetMembers response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::chat::response::GetMembers_Error_Code_YOU_ARE_NOT_MEMBER);

    return response.SerializeAsString();
  }

  api::chat::response::GetMembers response;

  auto* response_result = response.mutable_result();

  {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params(
        "SELECT user_id, owner FROM chats_members "
        "WHERE chat_id = $1",
        request.filter().chat_id());

    transaction.commit();

    response_result->mutable_values()->Reserve(result.size());

    for (const auto& row : result) {
      auto* value = response_result->add_values();

      value->set_user_id(row["user_id"].as<int64_t>());
      value->set_owner(row["owner"].as<bool>());
    }
  }

  return response.SerializeAsString();
}
}  // namespace detail

#include <api/user/connection.pb.h>
#include <api/user/request/get_connections.pb.h>
#include <api/user/response/get_connections.pb.h>
#include <fmt/core.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::GetConnections& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  api::user::response::GetConnections response;

  {
    pqxx::read_transaction transaction(connection);

    pqxx::params params;
    pqxx::placeholders placeholders;
    std::stringstream query;

    query << fmt::format(
        "SELECT * FROM users_connections "
        "WHERE (initiator_user_id = {0} OR responder_user_id = {0}) ",
        placeholders.view());

    params.append(client.user_id()), placeholders.next();

    if (request.filter().has_established()) {
      query << fmt::format("AND established = {} ", placeholders.view());

      params.append(request.filter().established()), placeholders.next();
    }

    auto result = transaction.exec_params(query.str(), params);

    transaction.commit();

    auto* response_result = response.mutable_result();

    for (const auto& row : result) {
      auto* connection = response_result->add_connections();

      connection->set_initiator_user_id(row["initiator_user_id"].as<int64_t>());
      connection->set_responder_user_id(row["responder_user_id"].as<int64_t>());
      connection->set_established(row["established"].as<bool>());
    }
  }

  return response.SerializeAsString();
}

}  // namespace detail

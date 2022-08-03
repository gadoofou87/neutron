#include <api/event.pb.h>
#include <api/user/connection.pb.h>
#include <api/user/event.pb.h>
#include <api/user/event/connection_created.pb.h>
#include <api/user/request/create_connection.pb.h>
#include <api/user/response/create_connection.pb.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "utils/span/from.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::CreateConnection& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::User::does_user_exist(request.user_id())) {
    api::user::response::CreateConnection response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::CreateConnection_Error_Code_USER_NOT_FOUND);

    return response.SerializeAsString();
  }

  auto initiator_user_id = client.user_id();
  auto responder_user_id = request.user_id();

  {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params(
        "SELECT true FROM users_connections "
        "WHERE (initiator_user_id = $1 AND responder_user_id = $2) OR (responder_user_id = $1 AND "
        "initiator_user_id = $2) "
        "LIMIT 1",
        client.user_id(), request.user_id());

    transaction.commit();

    if (!result.empty()) {
      api::user::response::CreateConnection response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::CreateConnection_Error_Code_ALREADY_PENDING);

      return response.SerializeAsString();
    }
  }

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "INSERT INTO users_connections (initiator_user_id, responder_user_id, established) "
        "VALUES ($1, $2, false)",
        initiator_user_id, responder_user_id);

    transaction.commit();
  }

  {
    api::user::event::ConnectionCreated inner_user_event;

    inner_user_event.set_initiator_user_id(initiator_user_id);
    inner_user_event.set_responder_user_id(responder_user_id);

    auto user_event =
        Helpers::User::create_event(api::user::Event_Type_CONNECTION_CREATED, inner_user_event);

    auto event = Helpers::create_event(api::Event_Type_USER, user_event);

    auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
      //
      client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
    };

    impl.client_manager.do_for_each(initiator_user_id, do_notify);
    impl.client_manager.do_for_each(responder_user_id, do_notify);
  }

  api::user::response::CreateConnection response;

  return response.SerializeAsString();
}

}  // namespace detail

#include <api/event.pb.h>
#include <api/user/connection.pb.h>
#include <api/user/event.pb.h>
#include <api/user/event/connection_deleted.pb.h>
#include <api/user/request/delete_connection.pb.h>
#include <api/user/response/delete_connection.pb.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "utils/span/from.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::DeleteConnection& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  uint64_t _db_initiator_user_id;
  uint64_t _db_responder_user_id;

  {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params(
        "SELECT initiator_user_id, responder_user_id FROM users_connections "
        "WHERE (initiator_user_id = $1 AND responder_user_id = $2) OR (responder_user_id = $1 AND "
        "initiator_user_id = $2) "
        "LIMIT 1",
        client.user_id(), request.user_id());

    transaction.commit();

    if (result.empty()) {
      api::user::response::DeleteConnection response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::DeleteConnection_Error_Code_NOT_FOUND);

      return response.SerializeAsString();
    }

    _db_initiator_user_id = result[0]["initiator_user_id"].as<int64_t>();
    _db_responder_user_id = result[0]["responder_user_id"].as<int64_t>();
  }

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "DELETE FROM users_connections "
        "WHERE initiator_user_id = $2 AND responder_user_id = $3",
        _db_initiator_user_id, _db_responder_user_id);

    transaction.commit();
  }

  {
    api::user::event::ConnectionDeleted inner_user_event;

    inner_user_event.set_initiator_user_id(_db_initiator_user_id);
    inner_user_event.set_responder_user_id(_db_responder_user_id);

    auto user_event =
        Helpers::User::create_event(api::user::Event_Type_CONNECTION_DELETED, inner_user_event);

    auto event = Helpers::create_event(api::Event_Type_USER, user_event);

    auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
      //
      client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
    };

    impl.client_manager.do_for_each(_db_initiator_user_id, do_notify);
    impl.client_manager.do_for_each(_db_responder_user_id, do_notify);
  }

  api::user::response::DeleteConnection response;

  return response.SerializeAsString();
}

}  // namespace detail

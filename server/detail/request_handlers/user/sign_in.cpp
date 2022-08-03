#include <api/user/request/sign_in.pb.h>
#include <api/user/response/sign_in.pb.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "crypto/argon2.hpp"
#include "crypto/falcon.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::SignIn& request) {
  if (client.is_authorized()) {
    api::user::response::SignIn response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::SignIn_Error_Code_ALREADY_SIGNED_IN);

    return response.SerializeAsString();
  }

  uint64_t user_id;
  uint32_t device_id;

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params(
        "SELECT id, encoded_password FROM users WHERE username = $1 LIMIT 1", request.username());

    transaction.commit();

    if (result.empty()) {
      api::user::response::SignIn response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::SignIn_Error_Code_USER_NOT_FOUND);

      return response.SerializeAsString();
    }

    auto encoded_password = result[0]["encoded_password"].view();

    if (!crypto::Argon2::verify(encoded_password, request.username(), crypto::Argon2::Type::id)) {
      api::user::response::SignIn response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::SignIn_Error_Code_INVALID_PASSWORD);

      return response.SerializeAsString();
    }

    user_id = result[0]["id"].as<int64_t>();
  }

  if (request.has_device_id()) {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params(
        "SELECT true FROM users_devices WHERE user_id = $1 AND id = $2", user_id, device_id);

    transaction.commit();

    if (result.empty()) {
      api::user::response::SignIn response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::SignIn_Error_Code_DEVICE_NOT_FOUND);

      return response.SerializeAsString();
    }

    device_id = request.device_id();
  } else {
    if (request.device_public_key().size() != crypto::Falcon512::PublicKeyLength) {
      api::user::response::SignIn response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::SignIn_Error_Code_INVALID_PUBLIC_KEY);

      return response.SerializeAsString();
    }

    pqxx::work transaction(connection);

    auto result = transaction.exec_params1(
        "INSERT INTO users_devices (user_id, public_key) "
        "VALUES ($1, $2) "
        "RETURNING id",
        user_id, pqxx::binary_cast(request.device_public_key()));

    transaction.commit();

    device_id = result[0].as<int32_t>();
  }

  impl.client_manager.mark_as_authorized(client.id(), user_id, device_id);

  api::user::response::SignIn response;

  auto* response_result = response.mutable_result();

  response_result->set_device_id(device_id);

  return response.SerializeAsString();
}

}  // namespace detail

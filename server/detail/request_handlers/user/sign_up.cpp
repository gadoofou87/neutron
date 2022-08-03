#include <api/user/request/sign_up.pb.h>
#include <api/user/response/sign_up.pb.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../request_handler.hpp"
#include "crypto/argon2.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const api::user::request::SignUp& request) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params("SELECT true FROM users WHERE username = $1 LIMIT 1",
                                          request.username());

    transaction.commit();

    if (!result.empty()) {
      api::user::response::SignUp response;

      auto* response_error = response.mutable_error();

      response_error->set_code(api::user::response::SignUp_Error_Code_USERNAME_TAKEN);

      return response.SerializeAsString();
    }
  }

  static const crypto::Argon2::Configuration configuration{
      .t_cost = 3, .m_cost = 1 << 12, .parallelism = 1, .saltlen = 8, .hashlen = 32};

  auto encoded_password =
      crypto::Argon2::hash(request.password(), crypto::Argon2::Type::id, configuration);

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "INSERT INTO users (username, encoded_password) "
        "VALUES ($1, $2)",
        request.username(), encoded_password);

    transaction.commit();
  }

  api::user::response::SignUp response;

  return response.SerializeAsString();
}

}  // namespace detail

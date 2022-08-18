#include <api/user/request/get_devices.pb.h>
#include <api/user/response/get_devices.pb.h>
#include <fmt/core.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::GetDevices& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::User::does_user_exist(request.filter().user_id())) {
    api::user::response::GetDevices response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::GetDevices_Error_Code_USER_NOT_FOUND);

    return response.SerializeAsString();
  }

  api::user::response::GetDevices response;

  {
    pqxx::read_transaction transaction(connection);

    pqxx::params params;
    pqxx::placeholders placeholders;
    std::stringstream query;

    query << fmt::format(
        "SELECT FROM users_devices "
        "WHERE user_id = {} ",
        placeholders.view());

    params.append(request.filter().user_id()), placeholders.next();

    if (request.filter().device_ids_size() != 0) {
      query << "AND id IN ( ";

      {
        query << fmt::format("{} ", placeholders.view());

        params.append(request.filter().device_ids(0)), placeholders.next();
      }
      for (int index = 1; index < request.filter().device_ids_size(); ++index) {
        query << fmt::format(", {} ", placeholders.view());

        params.append(request.filter().device_ids(index)), placeholders.next();
      }

      query << ") ";
    }
    if (request.filter().has_revoked()) {
      query << fmt::format("AND revoked = {} ", placeholders.view());

      params.append(request.filter().revoked()), placeholders.next();
    }

    transaction.commit();
  }

  return response.SerializeAsString();
}

}  // namespace detail

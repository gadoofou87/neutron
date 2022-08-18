#include <api/user/request/get_info.pb.h>
#include <api/user/response/get_info.pb.h>
#include <fmt/core.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::GetInfo& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  if (!Helpers::User::does_user_exist(request.user_id())) {
    api::user::response::GetInfo response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::GetInfo_Error_Code_USER_NOT_FOUND);

    return response.SerializeAsString();
  }

  api::user::response::GetInfo response;

  // auto* response_result = response.mutable_result();

  {
    pqxx::read_transaction transaction(connection);

    //

    transaction.commit();
  }

  return response.SerializeAsString();
}

}  // namespace detail

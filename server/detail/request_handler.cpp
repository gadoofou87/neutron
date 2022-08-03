#include "request_handler.hpp"

#include <api/chat/request.pb.h>
#include <api/exception_code.pb.h>
#include <api/request.pb.h>
#include <api/user/request.pb.h>
#include <api/user/request/create_connection.pb.h>
#include <api/user/request/delete_connection.pb.h>
#include <api/user/request/edit_password.pb.h>
#include <api/user/request/edit_photo.pb.h>
#include <api/user/request/get_connections.pb.h>
#include <api/user/request/get_info.pb.h>
#include <api/user/request/logout.pb.h>
#include <api/user/request/revoke_device.pb.h>
#include <api/user/request/sign_in.pb.h>
#include <api/user/request/sign_up.pb.h>
#include <api/user/request/upload_one_time_key.pb.h>

#include "client.hpp"
#include "utils/span/from.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client, const api::chat::Request& request) {
  (void)client;
  (void)request;

  throw InvalidDataException();
}

template <>
std::string RequestHandler::handle(const Client& client, const api::user::Request& request) {
  switch (request.type()) {
    case api::user::Request_Type_CREATE_CONNECTION: {
      api::user::request::CreateConnection inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_DELETE_CONNECTION: {
      api::user::request::DeleteConnection inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_EDIT_PASSWORD: {
      api::user::request::EditPassword inner_request;
      if (inner_request.ParseFromString(request.data())) {
        // return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_EDIT_PHOTO: {
      api::user::request::EditPhoto inner_request;
      if (inner_request.ParseFromString(request.data())) {
        // return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_GET_CONNECTIONS: {
      api::user::request::GetConnections inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_GET_INFO: {
      api::user::request::GetInfo inner_request;
      if (inner_request.ParseFromString(request.data())) {
        // return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_LOGOUT: {
      api::user::request::Logout inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_REVOKE_DEVICE: {
      api::user::request::RevokeDevice inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_SIGN_IN: {
      api::user::request::SignIn inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_SIGN_UP: {
      api::user::request::SignUp inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(inner_request);
      }
      break;
    }
    case api::user::Request_Type_UPLOAD_ONE_TIME_KEY: {
      api::user::request::UploadOneTimeKey inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::user::Request_Type_Request_Type_INT_MAX_SENTINEL_DO_NOT_USE_:
    case api::user::Request_Type_Request_Type_INT_MIN_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw InvalidDataException();
}

template <>
std::string RequestHandler::handle(const Client& client, const api::Request& request) {
  switch (request.type()) {
    case api::Request_Type_CHAT: {
      api::chat::Request inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::Request_Type_USER: {
      api::user::Request inner_request;
      if (inner_request.ParseFromString(request.data())) {
        return handle(client, inner_request);
      }
      break;
    }
    case api::Request_Type_Request_Type_INT_MAX_SENTINEL_DO_NOT_USE_:
    case api::Request_Type_Request_Type_INT_MIN_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw InvalidDataException();
}

void RequestHandler::handle(Client& client, size_t request_identifier,
                            const api::Request& request) {
  try {
    auto response_data = handle(client, request);

    client.communication().send_response(request_identifier,
                                         utils::span::from<const uint8_t>(response_data));
  } catch (...) {
    client.communication().send_exception(request_identifier,
                                          api::ExceptionCode::INTERNAL_SERVER_ERROR);
  }
}

}  // namespace detail

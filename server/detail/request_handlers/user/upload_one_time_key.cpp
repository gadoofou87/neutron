#include <api/user/request/upload_one_time_key.pb.h>
#include <api/user/response/upload_one_time_key.pb.h>

#include <pqxx/pqxx>

#include "../../../server_impl.hpp"
#include "../../client.hpp"
#include "../../request_handler.hpp"
#include "../helpers.hpp"
#include "crypto/falcon.hpp"
#include "crypto/sidhp434_compressed.hpp"
#include "utils/span/copy.hpp"
#include "utils/span/from.hpp"

namespace detail {

template <>
std::string RequestHandler::handle(const Client& client,
                                   const api::user::request::UploadOneTimeKey& request) {
  if (!client.is_authorized()) {
    throw UnauthorizedException();
  }

  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  std::array<uint8_t, crypto::Falcon512::PublicKeyLength> public_key;

  {
    pqxx::read_transaction transaction(connection);

    auto result = transaction.exec_params1(
        "SELECT public_key FROM users_devices "
        "WHERE user_id = $1 AND id = $2 "
        "LIMIT 1",
        client.user_id(), client.device_id());

    transaction.commit();

    auto _db_public_key = result[0].as<std::basic_string<std::byte>>();

    utils::span::copy<uint8_t, std::byte>(public_key, _db_public_key);
  }

  const auto& one_time_key = request.one_time_key();

  if (one_time_key.public_key_a().size() != crypto::SIDHp434_compressed::PublicKeyLength) {
    api::user::response::UploadOneTimeKey response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::UploadOneTimeKey_Error_Code_INVALID_PUBLIC_KEY_A);

    return response.SerializeAsString();
  }
  if (one_time_key.public_key_b().size() != crypto::SIDHp434_compressed::PublicKeyLength) {
    api::user::response::UploadOneTimeKey response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::UploadOneTimeKey_Error_Code_INVALID_PUBLIC_KEY_B);

    return response.SerializeAsString();
  }
  if (one_time_key.signature().size() != crypto::Falcon512::SignatureLength) {
    api::user::response::UploadOneTimeKey response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::UploadOneTimeKey_Error_Code_INVALID_SIGNATURE);

    return response.SerializeAsString();
  }

  std::array<uint8_t, crypto::SIDHp434_compressed::PublicKeyLength> xored_one_time_public_key;

  for (size_t i = 0; i < xored_one_time_public_key.size(); ++i) {
    xored_one_time_public_key[i] = one_time_key.public_key_a()[i] ^ one_time_key.public_key_b()[i];
  }

  if (!crypto::Falcon512::verify(utils::span::from<const uint8_t>(one_time_key.signature()),
                                 xored_one_time_public_key, public_key)) {
    api::user::response::UploadOneTimeKey response;

    auto* response_error = response.mutable_error();

    response_error->set_code(api::user::response::UploadOneTimeKey_Error_Code_INVALID_SIGNATURE);

    return response.SerializeAsString();
  }

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "INSERT INTO users_one_time_keys (user_id, device_id, public_key_a, public_key_b, "
        "signature) "
        "VALUES ($1, $2, $3, $4, $5)",
        client.user_id(), client.device_id(), pqxx::binary_cast(one_time_key.public_key_a()),
        pqxx::binary_cast(one_time_key.public_key_b()),
        pqxx::binary_cast(one_time_key.signature()));

    transaction.commit();
  }

  {
    pqxx::work transaction(connection);

    auto result = transaction.exec_params(
        "UPDATE chats_members "
        "SET pending_key_rotations = pending_key_rotations - 1 "
        "WHERE ctid = "
        "( "
        " SELECT ctid FROM chats_members "
        " WHERE user_id = $1 AND pending_key_rotations > 0 "
        ") "
        "RETURNING chat_id");

    transaction.commit();

    if (!result.empty()) {
      uint64_t _db_chat_id = result[0][0].as<int64_t>();

      Helpers::Chat::rotate_keys(_db_chat_id);
    }
  }

  api::user::response::UploadOneTimeKey response;

  return response.SerializeAsString();
}

}  // namespace detail

#include "helpers.hpp"

#include <api/chat/event.pb.h>
#include <api/chat/event/key_rotate.pb.h>
#include <api/chat/type.pb.h>
#include <api/event.pb.h>
#include <api/user/connection.pb.h>
#include <api/user/event.pb.h>
#include <api/user/one_time_key.pb.h>

#include <chrono>
#include <pqxx/pqxx>
#include <utility>

#include "../client.hpp"
#include "server_impl.hpp"
#include "utils/span/from.hpp"

namespace detail {

void RequestHandler::Helpers::Chat::add_member(uint64_t chat_id, uint64_t user_id,
                                               uint64_t first_accessible_event_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "INSERT INTO chats_members (chat_id, user_id, first_accessible_event_id) "
        "VALUES ($1, $2, $3)",
        chat_id, user_id, first_accessible_event_id);

    transaction.commit();
  }
}

api::chat::Event RequestHandler::Helpers::Chat::create_event(
    uint64_t chat_id, std::optional<uint64_t> owner_user_id, api::chat::Event_Type type,
    const google::protobuf::Message& message) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  auto creation_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

  auto data = message.SerializeAsString();

  pqxx::work transaction(connection);

  auto result = transaction.exec_params1(
      "INSERT INTO chats_events (creation_timestamp, chat_id, owner_user_id, type, data) "
      "VALUES ($1, $2, $3, $4, $5)"
      "RETURNING id",
      creation_timestamp, chat_id, owner_user_id, std::to_underlying(type),
      pqxx::binary_cast(data));

  transaction.commit();

  api::chat::Event event;

  event.set_id(result[0].as<int64_t>());
  event.set_creation_timestamp(creation_timestamp);
  event.set_chat_id(chat_id);

  if (owner_user_id.has_value()) {
    event.set_owner_user_id(*owner_user_id);
  }

  event.set_type(api::chat::Event_Type_MEMBER_ADDED);
  event.set_data(std::move(data));

  return event;
}

bool RequestHandler::Helpers::Chat::does_chat_exist(uint64_t chat_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params("SELECT true FROM CHATS WHERE id = $1", chat_id);

  transaction.commit();

  return !result.empty();
}

bool RequestHandler::Helpers::Chat::is_deleted(uint64_t chat_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params1("SELECT deleted FROM CHATS WHERE id = $1", chat_id);

  transaction.commit();

  return result[0].as<bool>();
}

bool RequestHandler::Helpers::Chat::is_chat_member(uint64_t chat_id, uint64_t user_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params(
      "SELECT true FROM chats_members "
      "WHERE chat_id = $1 AND user_id = $2",
      chat_id, user_id);

  transaction.commit();

  return !result.empty();
}

bool RequestHandler::Helpers::Chat::is_chat_owner(uint64_t chat_id, uint64_t user_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params1(
      "SELECT owner FROM chats_members "
      "WHERE chat_id = $1 AND user_id = $2",
      chat_id, user_id);

  transaction.commit();

  return result[0].as<bool>();
}

std::vector<uint64_t> RequestHandler::Helpers::Chat::get_chat_members(uint64_t chat_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result =
      transaction.exec_params("SELECT user_id FROM chats_members WHERE chat_id = $1", chat_id);

  transaction.commit();

  std::vector<uint64_t> user_ids;

  user_ids.reserve(result.size());

  for (const auto& row : result) {
    user_ids.push_back(row[0].as<int64_t>());
  }

  return user_ids;
}

api::chat::Type RequestHandler::Helpers::Chat::get_chat_type(uint64_t chat_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params1("SELECT type FROM chats WHERE id = $1", chat_id);

  transaction.commit();

  return static_cast<api::chat::Type>(result[0].as<int32_t>());
}

void RequestHandler::Helpers::Chat::rotate_keys(uint64_t chat_id) {
  return rotate_keys(chat_id, get_chat_members(chat_id));
}

void RequestHandler::Helpers::Chat::rotate_keys(uint64_t chat_id,
                                                const std::vector<uint64_t>& members_user_ids) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  api::chat::event::KeyRotate inner_chat_event;

  {
    pqxx::work transaction(connection);

    for (const auto& user_id : members_user_ids) {
      auto result =
          transaction.exec_params("SELECT id FROM users_devices WHERE user_id = $1", user_id);

      for (const auto& row : result) {
        uint32_t device_id = row[0].as<int32_t>();

        auto result = transaction.exec_params(
            "DELETE FROM users_one_time_keys "
            "WHERE ctid = "
            "("
            " SELECT ctid FROM users_one_time_keys "
            " WHERE user_id = $1 AND device_id = $2 "
            " LIMIT 1 "
            ") "
            "RETURNING public_key_a, public_key_b, signature",
            user_id, device_id);

        if (result.empty()) {
          transaction.exec_params(
              "UPDATE chats_members "
              "SET pending_key_rotations = pending_key_rotations + 1 "
              "WHERE chat_id = $1 AND user_id = $2",
              chat_id, user_id);

          // TODO: Отправить эвент пользователю?
        } else {
          const auto& row = result[0];

          auto* value = inner_chat_event.add_values();

          value->set_user_id(user_id);
          value->set_device_id(device_id);

          auto* one_time_key = value->mutable_one_time_key();

          one_time_key->set_public_key_a(row["public_key_a"].as<std::string>());
          one_time_key->set_public_key_b(row["public_key_b"].as<std::string>());
          one_time_key->set_signature(row["signature"].as<std::string>());
        }
      }
    }

    transaction.commit();
  }

  auto chat_event =
      Chat::create_event(chat_id, std::nullopt, api::chat::Event_Type_KEY_ROTATE, inner_chat_event);

  auto event = Helpers::create_event(api::Event_Type_CHAT, chat_event);

  auto do_notify = [&, event_data = event.SerializeAsString()](Client& client) {
    //
    client.communication().send_event({}, utils::span::from<const uint8_t>(event_data));
  };

  for (const auto& user_id : members_user_ids) {
    impl.client_manager.do_for_each(user_id, do_notify);
  }
}

void RequestHandler::Helpers::Chat::set_owner(uint64_t chat_id, uint64_t user_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  {
    pqxx::work transaction(connection);

    transaction.exec_params(
        "UPDATE chats_members "
        "SET owner = false "
        "WHERE chat_id = $1 ",
        chat_id);

    transaction.exec_params(
        "UPDATE chats_members "
        "SET owner = true "
        "WHERE chat_id = $1 ",
        chat_id);

    transaction.commit();
  }
}

}  // namespace detail

namespace detail {

api::Event RequestHandler::Helpers::User::create_event(api::user::Event_Type type,
                                                       const google::protobuf::Message& message) {
  api::Event event;

  event.set_type(api::Event_Type_USER);

  {
    api::user::Event inner_event;

    inner_event.set_type(type);
    inner_event.set_data(message.SerializeAsString());

    event.set_data(inner_event.SerializeAsString());
  }

  return event;
}

bool RequestHandler::Helpers::User::does_user_exist(uint64_t user_id) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params("SELECT true FROM users WHERE id = $1", user_id);

  transaction.commit();

  return !result.empty();
}

bool RequestHandler::Helpers::User::is_connection_established(uint64_t user_id_a,
                                                              uint64_t user_id_b) {
  auto& impl = ServerImpl::instance();

  auto& connection = impl.database_manager.connection();

  pqxx::read_transaction transaction(connection);

  auto result = transaction.exec_params(
      "SELECT true FROM users_connections "
      "WHERE "
      "("
      " (initiator_user_id = $1 AND responder_user_id = $2) "
      " OR "
      " (initiator_user_id = $2 AND responder_user_id = $1) "
      ") "
      "AND established = true "
      "LIMIT 1",
      user_id_a, user_id_b);

  transaction.commit();

  return !result.empty();
}

}  // namespace detail

namespace detail {

api::Event RequestHandler::Helpers::create_event(api::Event_Type type,
                                                 const google::protobuf::Message& message) {
  api::Event event;

  event.set_type(type);
  event.set_data(message.SerializeAsString());

  return event;
}

}  // namespace detail

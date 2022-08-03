#include "database_manager.hpp"

#include <pqxx/pqxx>

namespace detail {

pqxx::connection& DatabaseManager::connection() {
  static thread_local pqxx::connection connection(connection_string_);
  return connection;
}

const std::string& DatabaseManager::connection_string() const { return connection_string_; }

void DatabaseManager::create_tables() {
  pqxx::work transaction(connection());

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS chats ("
      " id      bigserial NOT NULL, "
      " type    integer   NOT NULL, "
      " deleted boolean   NOT NULL  DEFAULT FALSE, "
      " title   text, "
      " PRIMARY KEY (ID) "
      ")");

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS chats_events ("
      " id                 bigserial NOT NULL, "
      " creation_timestamp bigint    NOT NULL, "
      " chat_id            bigint    NOT NULL, "
      " owner_user_id      bigint    NOT NULL, "
      " type               integer   NOT NULL, "
      " data               bytea     NOT NULL, "
      " PRIMARY KEY (id, chat_id) "
      ")");

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS chats_members ("
      " chat_id                   bigint  NOT NULL, "
      " user_id                   bigint  NOT NULL, "
      " owner                     boolean NOT NULL, "
      " pending_key_rotations     integer NOT NULL  DEFAULT 0, "
      " first_accessible_event_id bigint  NOT NULL, "
      " PRIMARY KEY (chat_id, user_id) "
      ")");

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS users ("
      " id               bigserial NOT NULL, "
      " username         text      NOT NULL, "
      " encoded_password text      NOT NULL, "
      " PRIMARY KEY (id), UNIQUE (username) "
      ")");

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS users_connections ("
      " initiator_user_id bigint  NOT NULL, "
      " responder_user_id bigint  NOT NULL, "
      " established       boolean NOT NULL, "
      ")");

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS users_devices ("
      " id         serial NOT NULL, "
      " user_id    bigint NOT NULL, "
      " public_key bytea  NOT NULL, "
      " PRIMARY KEY (id, user_id) "
      ")");

  transaction.exec(
      "CREATE TABLE IF NOT EXISTS users_one_time_keys ("
      " user_id      bigint  NOT NULL, "
      " device_id    integer NOT NULL, "
      " public_key_a bytea   NOT NULL, "
      " public_key_b bytea   NOT NULL, "
      " signature    bytea   NOT NULL, "
      " PRIMARY KEY (user_id, device_id) "
      ")");

  transaction.commit();
}

void DatabaseManager::set_connection_string(std::string string) {
  connection_string_ = std::move(string);
}

}  // namespace detail

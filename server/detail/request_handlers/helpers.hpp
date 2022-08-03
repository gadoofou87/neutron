#pragma once

#include <string>
#include <unordered_set>

#include "../request_handler.hpp"

namespace api {

namespace chat {

class Event;
enum Event_Type : int;
enum Type : int;

}  // namespace chat

namespace user {

enum Event_Type : int;
class OneTimeKey;

}  // namespace user

class Event;
enum Event_Type : int;

}  // namespace api

namespace google {
namespace protobuf {

class Message;

}
}  // namespace google

namespace detail {

struct RequestHandler::Helpers {
  struct Chat {
    static uint64_t create_chat(api::chat::Type type);

    static api::chat::Event create_event(uint64_t chat_id, std::optional<uint64_t> owner_user_id,
                                         api::chat::Event_Type type,
                                         const google::protobuf::Message& message);

    static bool does_chat_exist(uint64_t chat_id);

    static bool is_chat_member(uint64_t chat_id, uint64_t user_id);

    static bool is_chat_owner(uint64_t chat_id, uint64_t user_id);

    static std::vector<uint64_t> get_chat_members(uint64_t chat_id);

    static api::chat::Type get_chat_type(uint64_t chat_id);

    static void rotate_keys(uint64_t chat_id);

    static void rotate_keys(uint64_t chat_id, const std::vector<uint64_t>& members_user_ids);
  };
  struct User {
    static api::Event create_event(api::user::Event_Type type,
                                   const google::protobuf::Message& message);

    static bool does_user_exist(uint64_t user_id);

    static bool is_connection_established(uint64_t user_id_a, uint64_t user_id_b);
  };

  static api::Event create_event(api::Event_Type type, const google::protobuf::Message& message);
};

}  // namespace detail

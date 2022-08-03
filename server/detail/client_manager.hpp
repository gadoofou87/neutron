#pragma once

#include <shared_mutex>
#include <unordered_map>

#include "client.hpp"
#include "utils/pair/hasher.hpp"

namespace detail {

class ClientManager {
 public:
  void add_unauthorized(std::shared_ptr<protocol::Connection>&& connection);

  void do_for_each(uint64_t user_id, const std::function<void(Client&)>& function,
                   const std::optional<uint64_t> exclude_client = std::nullopt) const;

  [[nodiscard]] std::shared_ptr<Client> find(uint64_t user_id, uint32_t device_id) const;

  void mark_as_authorized(uint64_t client_id, uint64_t user_id, uint32_t device_id);

  void mark_as_unauthorized(uint64_t client_id);

  void remove(uint64_t client_id);

 private:
  mutable std::shared_mutex mutex_;

  struct ClientList : std::list<std::shared_ptr<Client>> {
    uint64_t next_id;
  } client_list_;

  std::unordered_map<uint64_t, ClientList::iterator> client_id_map_;
  std::unordered_map<std::pair<uint64_t, uint32_t>, ClientList::iterator, utils::pair::hasher>
      client_user_device_id_map_;
  std::unordered_multimap<uint64_t, ClientList::iterator> client_user_id_map_;
};

}  // namespace detail

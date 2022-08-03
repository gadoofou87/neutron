#include "client_manager.hpp"

namespace detail {

void ClientManager::add_unauthorized(std::shared_ptr<protocol::Connection>&& connection) {
  ASSERT(connection != nullptr);

  std::unique_lock lock(mutex_);

  uint64_t id;

  do {
    id = client_list_.next_id++;
  } while (client_id_map_.contains(id));

  auto client_list_iterator =
      client_list_.insert(client_list_.end(), std::make_shared<Client>(id, std::move(connection)));

  auto [client_id_map_iterator, success] = client_id_map_.try_emplace(id, client_list_iterator);

  ASSERT(success);
}

void ClientManager::do_for_each(uint64_t user_id, const std::function<void(Client&)>& function,
                                const std::optional<uint64_t> exclude_client) const {
  std::shared_lock lock(mutex_);

  auto client_user_id_map_iterator_pair = client_user_id_map_.equal_range(user_id);

  for (auto iterator = client_user_id_map_iterator_pair.first;
       iterator != client_user_id_map_iterator_pair.second; ++iterator) {
    auto& client = *(*iterator->second);

    if (exclude_client != client.id()) {
      function(client);
    }
  }
}

std::shared_ptr<Client> ClientManager::find(uint64_t user_id, uint32_t device_id) const {
  std::shared_lock lock(mutex_);

  auto client_user_device_id_map_iterator =
      client_user_device_id_map_.find(std::make_pair(user_id, device_id));

  if (client_user_device_id_map_iterator == client_user_device_id_map_.end()) {
    return nullptr;
  }

  return *client_user_device_id_map_iterator->second;
}

void ClientManager::mark_as_authorized(uint64_t client_id, uint64_t user_id, uint32_t device_id) {
  std::unique_lock lock(mutex_);

  auto client_id_map_iterator = client_id_map_.find(client_id);

  if (client_id_map_iterator == client_id_map_.end()) {
    return;
  }

  auto client_list_iterator = client_id_map_iterator->second;

  auto& client = *client_list_iterator;

  if (client->authorized_) {
    return;
  }

  client->authorized_ = true;
  client->device_id_ = device_id;
  client->user_id_ = user_id;

  {
    auto [client_user_device_id_map_iterator, success] = client_user_device_id_map_.try_emplace(
        std::make_pair(user_id, device_id), client_list_iterator);

    ASSERT(success);
  }
  { client_user_id_map_.emplace(user_id, client_list_iterator); }
}

void ClientManager::mark_as_unauthorized(uint64_t client_id) {
  std::unique_lock lock(mutex_);

  auto client_id_map_iterator = client_id_map_.find(client_id);

  if (client_id_map_iterator == client_id_map_.end()) {
    return;
  }

  auto client_list_iterator = client_id_map_iterator->second;

  auto& client = *client_list_iterator;

  if (!client->authorized_) {
    return;
  }

  client->authorized_ = false;

  {
    auto size =
        client_user_device_id_map_.erase(std::make_pair(client->user_id_, client->device_id_));

    ASSERT(size == 1);
  }
  {
    auto size = client_user_id_map_.erase(client->user_id_);

    ASSERT(size == 1);
  }
}

void ClientManager::remove(uint64_t client_id) {
  std::unique_lock lock(mutex_);

  auto client_id_map_iterator = client_id_map_.find(client_id);

  if (client_id_map_iterator == client_id_map_.end()) {
    return;
  }

  auto client_list_iterator = client_id_map_iterator->second;

  auto& client = *client_list_iterator;

  if (client->is_authorized()) {
    {
      auto size =
          client_user_device_id_map_.erase(std::make_pair(client->user_id_, client->device_id_));

      ASSERT(size == 1);
    }
    {
      auto size = client_user_id_map_.erase(client->user_id_);

      ASSERT(size != 0);
    }
  }

  client_list_.erase(client_list_iterator);
}

}  // namespace detail

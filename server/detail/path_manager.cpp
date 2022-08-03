#include "path_manager.hpp"

#include "utils/debug/assert.hpp"

namespace detail {

template <>
std::filesystem::path PathManager::default_path<PathManager::Location::BaseDir>() const {
  return std::filesystem::current_path();
}

template <>
std::filesystem::path PathManager::default_path<PathManager::Location::FileDir>() const {
  return path<Location::BaseDir>() / "files";
}

template <>
std::filesystem::path PathManager::default_path<PathManager::Location::TempDir>() const {
  return std::filesystem::temp_directory_path();
}

template <PathManager::Location Type>
size_t PathManager::location_path_index() const {
  auto index = static_cast<size_t>(Type);
  ASSERT(index < location_paths_.size());
  return index;
}

template <PathManager::Location Type>
std::filesystem::path PathManager::path() const {
  if (const auto& path = location_paths_[location_path_index<Type>()]; path.has_value()) {
    return *path;
  } else {
    return default_path<Type>();
  }
}

template <PathManager::Location Type>
void PathManager::set_path(const std::filesystem::path& path) {
  location_paths_[location_path_index<Type>()] = path;
}

template std::filesystem::path PathManager::path<PathManager::Location::BaseDir>() const;

template std::filesystem::path PathManager::path<PathManager::Location::FileDir>() const;

template std::filesystem::path PathManager::path<PathManager::Location::TempDir>() const;

template void PathManager::set_path<PathManager::Location::BaseDir>(const std::filesystem::path&);

template void PathManager::set_path<PathManager::Location::FileDir>(const std::filesystem::path&);

template void PathManager::set_path<PathManager::Location::TempDir>(const std::filesystem::path&);

}  // namespace detail

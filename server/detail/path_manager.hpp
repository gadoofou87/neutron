#pragma once

#include <array>
#include <filesystem>
#include <optional>

namespace detail {

class PathManager {
 public:
  enum class Location { BaseDir, FileDir, TempDir };

 public:
  template <Location Type>
  [[nodiscard]] std::filesystem::path path() const;

  template <Location Type>
  void set_path(const std::filesystem::path& path);

 private:
  template <Location Type>
  [[nodiscard]] std::filesystem::path default_path() const;

  template <Location Type>
  [[nodiscard]] size_t location_path_index() const;

 private:
  std::array<std::optional<std::filesystem::path>, 4> location_paths_;
};

}  // namespace detail

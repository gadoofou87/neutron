#include <spdlog/spdlog.h>
#include <toml++/toml.h>

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <thread>

#include "crypto/sidhp434_compressed.hpp"
#include "server_impl.hpp"

using namespace detail;

void Server::start(int argc, char* argv[]) {
  auto& impl = ServerImpl::instance();

  if (impl.network_manager.is_accepting()) {
    return;
  }

  cxxopts::Options options("server");

  options.add_options()("base_dir", "Path to base directory", cxxopts::value<std::string>())(
      "c,config", "Path to configuration file", cxxopts::value<std::string>())(
      "h,help", "Print usage")("temp_dir", "Path to the temporary directory",
                               cxxopts::value<std::string>());

  auto options_parse_result = options.parse(argc, argv);

  if (options_parse_result.count("help") != 0) {
    std::cout << options.help() << std::endl;
    return;
  }

  if (options_parse_result.count("base_dir") != 0) {
    impl.path_manager.set_path<PathManager::Location::BaseDir>(
        options_parse_result["base_dir"].as<std::string>());
  }
  if (options_parse_result.count("temp_dir") != 0) {
    impl.path_manager.set_path<PathManager::Location::TempDir>(
        options_parse_result["temp_dir"].as<std::string>());
  }

  std::filesystem::path config_path;

  if (options_parse_result.count("config") != 0) {
    config_path = options_parse_result["config"].as<std::string>();
  } else {
    config_path = impl.path_manager.path<PathManager::Location::BaseDir>() / "config.toml";
  }

  if (!std::filesystem::exists(config_path)) {
    spdlog::error("Configuration file not found: {}", config_path.string());
    return;
  }

  auto config_parse_result = toml::parse_file(config_path.string());

  if (auto* database_url = config_parse_result["database_url"].as_string()) {
    impl.database_manager.set_connection_string(database_url->get());
  } else {
    impl.database_manager.set_connection_string(
        "user=postgres password=postgres dbname=neutron sslmode=disable");
  }

  impl.database_manager.create_tables();

  impl.database_manager.connection().close();

  protocol::Server::Configuration server_configuration;

  if (auto* address = config_parse_result["address"].as_string()) {
    server_configuration.local_endpoint.address(asio::ip::make_address(address->get()));
  } else {
    server_configuration.local_endpoint.address(asio::ip::address_v6::any());
  }

  if (auto* port = config_parse_result["port"].as_integer()) {
    server_configuration.local_endpoint.port(port->get());
  } else {
    server_configuration.local_endpoint.port(0);
  }

  if (auto* backlog = config_parse_result["backlog"].as_integer()) {
    server_configuration.backlog = backlog->get();
  } else {
    server_configuration.backlog = 0;
  }

  server_configuration.receive_buffer_size =
      config_parse_result["receive_buffer_size"].value<unsigned>();

  std::vector<uint8_t> public_key;
  std::vector<uint8_t> secret_key;

  auto public_key_path = impl.path_manager.path<PathManager::Location::BaseDir>() / "public.key";
  auto secret_key_path = impl.path_manager.path<PathManager::Location::BaseDir>() / "secret.key";

  if (std::filesystem::exists(public_key_path) && std::filesystem::exists(secret_key_path)) {
    {
      std::ifstream stream(public_key_path.string(), std::ios_base::binary);
      stream.seekg(0, std::ifstream::end);
      public_key.resize(stream.tellg());
      stream.seekg(0, std::ifstream::beg);
      stream.read(reinterpret_cast<char*>(public_key.data()), public_key.size());
    }
    {
      std::ifstream stream(secret_key_path.string(), std::ios_base::binary);
      stream.seekg(0, std::ifstream::end);
      secret_key.resize(stream.tellg());
      stream.seekg(0, std::ifstream::beg);
      stream.read(reinterpret_cast<char*>(secret_key.data()), secret_key.size());
    }
  } else {
    public_key.resize(crypto::SIDHp434_compressed::PublicKeyLength);
    secret_key.resize(crypto::SIDHp434_compressed::SecretKeyBLength);

    crypto::SIDHp434_compressed::generate_keypair_B(public_key, secret_key);

    {
      std::ofstream stream(public_key_path.string(), std::ios_base::binary);
      stream.write(reinterpret_cast<const char*>(public_key.data()), public_key.size());
    }
    {
      std::ofstream stream(secret_key_path.string(), std::ios_base::binary);
      stream.write(reinterpret_cast<const char*>(secret_key.data()), secret_key.size());
    }
  }

  server_configuration.secret_key = std::move(secret_key);

  impl.network_manager.start_accept(std::move(server_configuration));

  std::vector<std::thread> threads;
  std::generate_n(
      std::back_inserter(threads),
      config_parse_result["num_threads"].value_or<unsigned>(std::thread::hardware_concurrency()),
      [&]() { return std::thread([&]() { impl.io_context.run(); }); });
  std::for_each(threads.begin(), threads.end(), [](std::thread& thread) { thread.join(); });
}

ServerImpl::ServerImpl(Token) : network_manager(io_context) {}

ServerImpl::~ServerImpl() = default;

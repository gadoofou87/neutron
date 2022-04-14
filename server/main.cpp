#include <cxxopts.hpp>

int main(int argc, char *argv[]) {
  cxxopts::Options options("server");

  auto result = options.parse(argc, argv);

  return 0;
}

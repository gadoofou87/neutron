#include <boost/ut.hpp>
#include <vector>

#include "crypto/sidhp434_compressed.hpp"

int main() {
  std::vector<uint8_t> public_key_A(crypto::SIDHp434_compressed::PublicKeyLength);
  std::vector<uint8_t> public_key_B(crypto::SIDHp434_compressed::PublicKeyLength);
  std::vector<uint8_t> secret_key_A(crypto::SIDHp434_compressed::SecretKeyALength);
  std::vector<uint8_t> secret_key_B(crypto::SIDHp434_compressed::SecretKeyBLength);
  std::vector<uint8_t> shared_secret_A(crypto::SIDHp434_compressed::SharedSecretLength);
  std::vector<uint8_t> shared_secret_B(crypto::SIDHp434_compressed::SharedSecretLength);

  crypto::SIDHp434_compressed::generate_keypair_A(public_key_A, secret_key_A);
  crypto::SIDHp434_compressed::generate_keypair_B(public_key_B, secret_key_B);
  crypto::SIDHp434_compressed::agree_A(shared_secret_A, secret_key_A, public_key_B);
  crypto::SIDHp434_compressed::agree_B(shared_secret_B, secret_key_B, public_key_A);
  boost::ut::expect(shared_secret_A == shared_secret_B);

  return 0;
}

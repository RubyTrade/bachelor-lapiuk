#include <gtest/gtest.h>

#include <string>

#include "core/utils/crypto.hpp"

TEST(Crypto, SignEd25519WithInvalidKeyReturnsEmpty) {
  const std::string invalidPem = "-----BEGIN PRIVATE KEY-----\nNOPE\n-----END PRIVATE KEY-----\n";
  const std::string sig = Crypto::sign_ed25519(invalidPem, "hello");
  EXPECT_TRUE(sig.empty());
}

TEST(Crypto, SignRsaSha256WithInvalidKeyReturnsEmpty) {
  const std::string invalidPem = "-----BEGIN PRIVATE KEY-----\nNOPE\n-----END PRIVATE KEY-----\n";
  const std::string sig = Crypto::sign_rsa_sha256(invalidPem, "hello");
  EXPECT_TRUE(sig.empty());
}

TEST(Crypto, SignEd25519WithEmptyDataReturnsEmpty) {
  const std::string invalidPem = "-----BEGIN PRIVATE KEY-----\nNOPE\n-----END PRIVATE KEY-----\n";
  const std::string sig = Crypto::sign_ed25519(invalidPem, "");
  EXPECT_TRUE(sig.empty());
}

TEST(Crypto, SignRsaSha256WithEmptyDataReturnsEmpty) {
  const std::string invalidPem = "-----BEGIN PRIVATE KEY-----\nNOPE\n-----END PRIVATE KEY-----\n";
  const std::string sig = Crypto::sign_rsa_sha256(invalidPem, "");
  EXPECT_TRUE(sig.empty());
}

TEST(Crypto, SignEd25519WithEmptyKeyReturnsEmpty) {
  const std::string sig = Crypto::sign_ed25519("", "hello");
  EXPECT_TRUE(sig.empty());
}

TEST(Crypto, SignRsaSha256WithEmptyKeyReturnsEmpty) {
  const std::string sig = Crypto::sign_rsa_sha256("", "hello");
  EXPECT_TRUE(sig.empty());
}

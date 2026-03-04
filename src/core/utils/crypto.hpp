#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include "core/utils/log.hpp"
#include <string>
#include <vector>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

class Crypto {
public:
  // TODO: decide if it's needed
  static std::string sign_rsa_sha256(const std::string &private_key_pem,
                                     const std::string &data) {
    BIO *bio = BIO_new_mem_buf(private_key_pem.data(), private_key_pem.size());
    if (!bio) {
      Log::log_err("Failed to create BIO");
      return {};
    }

    EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    if (!pkey) {
      BIO_free(bio);
      Log::log_err("Failed to load private key");
      return {};
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
      EVP_PKEY_free(pkey);
      BIO_free(bio);
      Log::log_err("Failed to create EVP_MD_CTX");
      return {};
    }

    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      BIO_free(bio);
      Log::log_err("EVP_DigestSignInit failed");
      return {};
    }

    if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) <= 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      BIO_free(bio);
      Log::log_err("EVP_DigestSignUpdate failed");
      return {};
    }

    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sig_len) <= 0 || sig_len == 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      BIO_free(bio);
      Log::log_err("EVP_DigestSignFinal (get length) failed");
      return {};
    }

    std::vector<unsigned char> sig(sig_len);
    if (EVP_DigestSignFinal(ctx, sig.data(), &sig_len) <= 0 || sig_len == 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      BIO_free(bio);
      Log::log_err("EVP_DigestSignFinal (sign) failed");
      return {};
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    BIO_free(bio);

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new(BIO_s_mem());
    if (!b64 || !mem) {
      if (b64)
        BIO_free(b64);
      if (mem)
        BIO_free(mem);
      Log::log_err("Failed to create base64 BIO");
      return {};
    }
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, mem);
    if (BIO_write(b64, sig.data(), (int)sig_len) <= 0) {
      BIO_free_all(b64);
      Log::log_err("BIO_write failed");
      return {};
    }
    if (BIO_flush(b64) <= 0) {
      BIO_free_all(b64);
      Log::log_err("BIO_flush failed");
      return {};
    }

    BUF_MEM *buffer;
    BIO_get_mem_ptr(b64, &buffer);
    if (!buffer || !buffer->data || buffer->length == 0) {
      BIO_free_all(b64);
      Log::log_err("BIO_get_mem_ptr failed");
      return {};
    }

    std::string result(buffer->data, buffer->length);
    BIO_free_all(b64);

    return result;
  }

  static std::string sign_ed25519(const std::string &privkey_pem,
                                  const std::string &message) {
    BIO *bio = BIO_new_mem_buf(privkey_pem.data(), (int)privkey_pem.size());
    EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
      Log::log_err("Failed to load private key");
      return {};
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
      EVP_PKEY_free(pkey);
      Log::log_err("Failed to create EVP_MD_CTX");
      return {};
    }

    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) <= 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      Log::log_err("EVP_DigestSignInit failed");
      return {};
    }

    size_t siglen = 0;
    if (EVP_DigestSign(ctx, nullptr, &siglen,
                       reinterpret_cast<const unsigned char *>(message.data()),
                       message.size()) <= 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      Log::log_err("EVP_DigestSign (get length) failed");
      return {};
    }

    std::vector<unsigned char> signature(siglen);
    if (EVP_DigestSign(ctx, signature.data(), &siglen,
                       reinterpret_cast<const unsigned char *>(message.data()),
                       message.size()) <= 0) {
      EVP_MD_CTX_free(ctx);
      EVP_PKEY_free(pkey);
      Log::log_err("EVP_DigestSign (sign) failed");
      return {};
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return base64_encode(signature.data(), siglen);
  }

  static std::string sign_hmac_sha256(const std::string &secret_key,
                                      const std::string &message) {
    unsigned char *digest = HMAC(
        EVP_sha256(), secret_key.data(), secret_key.size(),
        reinterpret_cast<const unsigned char *>(message.data()), message.size(),
        nullptr, nullptr);

    if (!digest) {
      Log::log_err("HMAC_SHA256 signing failed");
      return {};
    }

    // Convert to hex string
    std::string result;
    result.reserve(64); // SHA256 produces 32 bytes = 64 hex chars
    char buf[3];
    for (int i = 0; i < 32; ++i) {
      snprintf(buf, sizeof(buf), "%02x", digest[i]);
      result += buf;
    }

    return result;
  }

private:
  static std::string base64_encode(const unsigned char *buffer, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);

    BIO_free_all(bio);

    return result;
  }
};

#endif // CRYPTO_HPP

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
    EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(ctx, data.data(), data.size());

    size_t sig_len = 0;
    EVP_DigestSignFinal(ctx, nullptr, &sig_len);

    std::vector<unsigned char> sig(sig_len);
    EVP_DigestSignFinal(ctx, sig.data(), &sig_len);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    BIO_free(bio);

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, mem);
    BIO_write(b64, sig.data(), sig_len);
    BIO_flush(b64);

    BUF_MEM *buffer;
    BIO_get_mem_ptr(b64, &buffer);

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

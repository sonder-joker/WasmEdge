// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/signatures/x25519.h - Rsa algorithm implement ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of rsa relative algorithm
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/signatures/options.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "openssl/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

template <int PadMode, int KeyBits, int ShaNid> class Rsa {
public:
  class Signature {
  public:
    Signature(std::shared_ptr<std::vector<uint8_t>> &&Data) noexcept
        : Data(std::move(Data)) {}

    static WasiCryptoExpect<Signature>
    import(Span<const uint8_t> Encoded,
           __wasi_signature_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_signature_encoding_e_t Encoding) const noexcept;

    const std::vector<uint8_t> &ref() const { return *Data; }

  private:
    std::shared_ptr<std::vector<uint8_t>> Data;
  };

  class SignState {
  public:
    SignState(EvpMdCtxPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) noexcept;

    WasiCryptoExpect<Signature> sign() noexcept;

  private:
    std::shared_ptr<EVP_MD_CTX> Ctx;
  };

  class VerificationState {
  public:
    VerificationState(EvpMdCtxPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    WasiCryptoExpect<void> update(Span<uint8_t const> Data) noexcept;

    WasiCryptoExpect<void> verify(const Signature &Sig) noexcept;

  private:
    std::shared_ptr<EVP_MD_CTX> Ctx;
  };

  class PublicKey {
  public:
    PublicKey(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<uint8_t const> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<void> verify() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<VerificationState> openVerificationState() const noexcept;

  private:
    static WasiCryptoExpect<PublicKey>
    importPem(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<PublicKey>
    importPkcs8(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPem() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };

  class KeyPair;

  class SecretKey {
  public:
    SecretKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<SecretKey>
    import(Span<const uint8_t> Encoded,
           __wasi_secretkey_encoding_e_t Encoding) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &Pk) const noexcept;

  private:
    static WasiCryptoExpect<SecretKey>
    importPem(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<SecretKey>
    importPkcs8(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPem() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };

  class KeyPair {
  public:
    KeyPair(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Encoded,
           __wasi_keypair_encoding_e_t Encoding) noexcept;

    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const Options> OptOptions) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept;

    WasiCryptoExpect<PublicKey> publicKey() const noexcept;

    WasiCryptoExpect<SecretKey> secretKey() const noexcept;

    WasiCryptoExpect<SignState> openSignState() const noexcept;

  private:
    static WasiCryptoExpect<KeyPair>
    importPem(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<KeyPair>
    importPkcs8(Span<uint8_t const> Encoded) noexcept;

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPem() const noexcept;

    WasiCryptoExpect<std::vector<uint8_t>> exportPkcs8() const noexcept;

    std::shared_ptr<EVP_PKEY> Ctx;
  };

private:
  static constexpr size_t getKeySize() { return KeyBits / 8; }

  static void *getShaCtx() {
    return static_cast<void *>(
        const_cast<EVP_MD *>(EVP_get_digestbynid(ShaNid)));
  }
};

using RSA_PKCS1_2048_SHA256 = Rsa<RSA_PKCS1_PADDING, 2048, NID_sha256>;
using RSA_PKCS1_2048_SHA384 = Rsa<RSA_PKCS1_PADDING, 2048, NID_sha384>;
using RSA_PKCS1_2048_SHA512 = Rsa<RSA_PKCS1_PADDING, 2048, NID_sha512>;

using RSA_PKCS1_3072_SHA384 = Rsa<RSA_PKCS1_PADDING, 3072, NID_sha384>;
using RSA_PKCS1_3072_SHA512 = Rsa<RSA_PKCS1_PADDING, 3072, NID_sha512>;

using RSA_PKCS1_4096_SHA512 = Rsa<RSA_PKCS1_PADDING, 4096, NID_sha512>;

using RSA_PSS_2048_SHA256 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha256>;
using RSA_PSS_2048_SHA384 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha384>;
using RSA_PSS_2048_SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha512>;

using RSA_PSS_3072_SHA384 = Rsa<RSA_PKCS1_PSS_PADDING, 3072, NID_sha384>;
using RSA_PSS_3072_SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 3072, NID_sha512>;

using RSA_PSS_4096_SHA512 = Rsa<RSA_PKCS1_PSS_PADDING, 4096, NID_sha512>;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/aeads/cipher.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "wasi_crypto/api.hpp"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {
using namespace std::literals;

template <int CipherNid>
constexpr size_t Cipher<CipherNid>::getKeySize() noexcept {
  switch (CipherNid) {
  case NID_aes_128_gcm:
    return 16;
  case NID_aes_256_gcm:
    return 32;
  case NID_chacha20_poly1305:
    return 32;
  }
}

template <int CipherNid>
WasiCryptoExpect<size_t> Cipher<CipherNid>::State::maxTagLen() const noexcept {
  return getTagSize();
}

template <int CipherNid>
constexpr size_t Cipher<CipherNid>::getTagSize() noexcept {
  return 16;
}

template <int CipherNid>
WasiCryptoExpect<typename Cipher<CipherNid>::Key>
Cipher<CipherNid>::Key::generate(OptionalRef<Options>) noexcept {
  return SecretKey::random(getKeySize());
}

template <int CipherNid>
WasiCryptoExpect<typename Cipher<CipherNid>::Key>
Cipher<CipherNid>::Key::import(Span<const uint8_t> Raw) noexcept {
  return std::make_shared<SecretKey>(Raw);
}

template <int CipherNid>
WasiCryptoExpect<typename Cipher<CipherNid>::State>
Cipher<CipherNid>::State::open(Key &Key,
                               OptionalRef<Options> OptOption) noexcept {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_NONCE_REQUIRED);

  std::array<uint8_t, NonceSize> Nonce;
  if (auto Res = OptOption->get("nonce"sv, Nonce); !Res) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
  } else {
    ensureOrReturn(*Res == NonceSize, __WASI_CRYPTO_ERRNO_INVALID_NONCE);
  }

  ensureOrReturn(getKeySize() == Key.ref().size(),
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  EvpCipherCtxPtr Ctx{EVP_CIPHER_CTX_new()};
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), EVP_get_cipherbynid(CipherNid),
                                    nullptr, Key.ref().data(), Nonce.data(),
                                    Mode::Unchanged));

  return State{std::move(Ctx), Nonce};
}

template <int CipherNid>
WasiCryptoExpect<size_t>
Cipher<CipherNid>::State::optionsGet(std::string_view Name,
                                     Span<uint8_t> Value) const noexcept {
  ensureOrReturn(Name == "nonce"sv, __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  ensureOrReturn(NonceSize <= Value.size(), __WASI_CRYPTO_ERRNO_OVERFLOW);
  /// Not work, alway return pointer to '0', so replace with a member
  // const uint8_t *Nonce = EVP_CIPHER_CTX_original_iv(Ctx.get());
  // std::copy(Nonce, Nonce + getNonceSize(), Value.begin());
  std::copy(Ctx->Nonce.begin(), Ctx->Nonce.end(), Value.begin());
  return NonceSize;
}

/// https://wiki.openssl.org/index.php/EVP_Authenticated_Encryption_and_Decryption
template <int CipherNid>
WasiCryptoExpect<void>
Cipher<CipherNid>::State::absorb(Span<const uint8_t> Data) noexcept {
  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  int ActualOutputSize;
  int DataSize = static_cast<int>(Data.size());
  opensslAssuming(EVP_CipherUpdate(Ctx->RawCtx.get(), nullptr,
                                   &ActualOutputSize, Data.data(), DataSize));
  ensureOrReturn(ActualOutputSize == DataSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return {};
}

template <int CipherNid>
WasiCryptoExpect<size_t>
Cipher<CipherNid>::State::encrypt(Span<uint8_t> Out,
                                  Span<const uint8_t> Data) noexcept {
  return encryptImpl(Out.first(Data.size()), Out.last(getTagSize()), Data);
}

template <int CipherNid>
WasiCryptoExpect<Tag>
Cipher<CipherNid>::State::encryptDetached(Span<uint8_t> Out,
                                          Span<const uint8_t> Data) noexcept {
  std::vector<uint8_t> Tag(getTagSize());
  if (auto Res = encryptImpl(Out, Tag, Data); !Res) {
    return WasiCryptoUnexpect(Res);
  }
  return Tag;
}

template <int CipherNid>
WasiCryptoExpect<size_t>
Cipher<CipherNid>::State::encryptImpl(Span<uint8_t> Out, Span<uint8_t> Tag,
                                      Span<const uint8_t> Data) noexcept {
  opensslAssuming(EVP_CipherInit_ex(Ctx->RawCtx.get(), nullptr, nullptr,
                                    nullptr, nullptr, Mode::Encrypt));

  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx->RawCtx.get(), Out.data(),
                                   &ActualOutSize, Data.data(),
                                   static_cast<int>(Data.size())));

  ensureOrReturn(static_cast<size_t>(ActualOutSize) == Out.size(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  ensureOrReturn(EVP_CipherFinal_ex(Ctx->RawCtx.get(), nullptr, &ActualOutSize),
                 __WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  ensureOrReturn(ActualOutSize == 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx->RawCtx.get(), EVP_CTRL_AEAD_GET_TAG,
                                      static_cast<int>(getTagSize()),
                                      Tag.data()));

  return Out.size() + getTagSize();
}

template <int CipherNid>
WasiCryptoExpect<size_t>
Cipher<CipherNid>::State::decrypt(Span<uint8_t> Out,
                                  Span<const uint8_t> Data) noexcept {
  return decryptImpl(Out, Data.first(Out.size()), Data.last(getTagSize()));
}

template <int CipherNid>
WasiCryptoExpect<size_t>
Cipher<CipherNid>::State::decryptDetached(Span<uint8_t> Out,
                                          Span<const uint8_t> Data,
                                          Span<const uint8_t> RawTag) noexcept {
  return decryptImpl(Out, Data, RawTag);
}

template <int CipherNid>
WasiCryptoExpect<size_t>
Cipher<CipherNid>::State::decryptImpl(Span<uint8_t> Out,
                                      Span<const uint8_t> Data,
                                      Span<const uint8_t> RawTag) noexcept {
  opensslAssuming(EVP_CipherInit_ex(Ctx->RawCtx.get(), nullptr, nullptr,
                                    nullptr, nullptr, Mode::Decrypt));
  int ActualOutSize;

  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  int DataSize = static_cast<int>(Data.size());
  opensslAssuming(EVP_CipherUpdate(Ctx->RawCtx.get(), Out.data(),
                                   &ActualOutSize, Data.data(), DataSize));
  ensureOrReturn(ActualOutSize == DataSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx->RawCtx.get(), EVP_CTRL_AEAD_SET_TAG,
                                      static_cast<int>(getTagSize()),
                                      const_cast<uint8_t *>(RawTag.data())));

  if (!EVP_CipherFinal_ex(Ctx->RawCtx.get(), nullptr, &ActualOutSize)) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_TAG);
  }
  ensureOrReturn(ActualOutSize == 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Out.size();
}

template class Cipher<NID_aes_128_gcm>;
template class Cipher<NID_aes_256_gcm>;
template class Cipher<NID_chacha20_poly1305>;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge

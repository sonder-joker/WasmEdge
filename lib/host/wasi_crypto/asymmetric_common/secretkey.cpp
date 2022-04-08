// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/asymmetric_common/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<SkVariant>
importSk(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<SkVariant> {
        using FactoryType = std::decay_t<decltype(Factory)>;
        return FactoryType::SecretKey::import(Encoded, Encoding);
      },
      Alg);
}

WasiCryptoExpect<std::vector<uint8_t>>
skExportData(const SkVariant &SkVariant,
             __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return std::visit(
      [Encoding](const auto &Sk) noexcept { return Sk.exportData(Encoding); },
      SkVariant);
}

WasiCryptoExpect<PkVariant> skPublicKey(const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Sk) noexcept {
        return Sk.publicKey().map(
            [](auto &&Pk) { return PkVariant{std::move(Pk)}; });
      },
      SkVariant);
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
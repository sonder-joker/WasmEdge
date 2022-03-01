// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<size_t>
Context::symmetricTagLen(__wasi_symmetric_tag_t TagHandle) noexcept {
  return SymmetricTagManager.get(TagHandle).map(&Symmetric::Tag::len);
}

WasiCryptoExpect<size_t>
Context::symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                          Span<uint8_t> Buf) noexcept {
  return SymmetricTagManager.get(TagHandle).and_then(
      [=](Symmetric::Tag &Tag) noexcept { return Tag.pull(Buf); });
}

WasiCryptoExpect<void>
Context::symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                            Span<const uint8_t> RawTag) noexcept {
  return SymmetricTagManager.get(TagHandle).and_then(
      [=](Symmetric::Tag &Tag) noexcept { return Tag.verify(RawTag); });
}

WasiCryptoExpect<void>
Context::symmetricTagClose(__wasi_symmetric_tag_t TagHandle) noexcept {
  return SymmetricTagManager.close(TagHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
Context::symmetricKeyExport(__wasi_symmetric_key_t KeyHandle) noexcept {
  return SymmetricKeyManager.get(KeyHandle)
      .map(Symmetric::keyExportData)
      .and_then([this](auto &&Data) noexcept {
        return ArrayOutputManger.registerManager(std::move(Data));
      });
}

WasiCryptoExpect<void>
Context::symmetricKeyClose(__wasi_symmetric_key_t SymmetricKey) noexcept {
  return SymmetricKeyManager.close(SymmetricKey);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge

// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/ctx.h"
#include "host/wasi/crypto/error.h"
#include "wasi/crypto/api.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

class ArrayOutput {
public:
  ArrayOutput(std::vector<uint8_t> Data);

  template <__wasi_size_t SizeType>
  WasiCryptoExpect<__wasi_size_t> pull(std::array<uint8_t, SizeType> &Buf) {
    if (Buf.size() < Data.size()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
    }
    std::copy(Buf.begin(), Buf.end(), Data);
    return Data.size();
  }

  WasiCryptoExpect<__wasi_size_t> len();

  WasiCryptoExpect<__wasi_array_output_t>
  registerInManger(HandleMangers Mangers, std::vector<uint8_t> InputData);

private:
  std::vector<uint8_t> Data;
};
} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
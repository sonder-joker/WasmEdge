// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/error.h"
#include "wasi/crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

class Options {
public:
  /// Create a new object to set non-default options.
  WasiCryptoExpect<__wasi_options_t>
  open(__wasi_algorithm_type_e_t AlogrithmType);

  /// Destroy an options object.
  WasiCryptoExpect<void> close(__wasi_options_t Handle);

  /// Set or update an option.
  WasiCryptoExpect<void> set(__wasi_options_t Handle, std::string_view Name,
                             uint8_t Value, __wasi_size_t ValueLen);

  /// Set or update an integer option.
  WasiCryptoExpect<void> set_u64(__wasi_options_t Handle, std::string_view Name,
                                 uint64_t Value);

private:
};

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
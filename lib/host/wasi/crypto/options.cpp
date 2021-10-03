// SPDX-License-Identifier: Apache-2.0

#include "host/wasi/crypto/options.h"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto{
WasiCryptoExpect<__wasi_options_t> open(__wasi_algorithm_type_e_t AlogrithmType) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPO RTED_OPTION);
}
} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
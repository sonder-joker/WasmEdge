// SPDX-License-Identifier: Apache-2.0

#include "host/wasi/crypto/array_output.h"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {
ArrayOutput::ArrayOutput(std::vector<uint8_t> Data) : Data{Data} {}

WasiCryptoExpect<__wasi_array_output_t>
ArrayOutput::registerInManger(HandleMangers Mangers,
                              std::vector<uint8_t> InputData) {
  ArrayOutput ArrayOutput{InputData};
  return Mangers.ArrayOutputManger.registerModule(ArrayOutput);
}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::len() { return Data.size(); }

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
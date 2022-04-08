// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/asymmetric_common/publickey.h - Asymmetric SecretKey --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric common SecretKey of wasi-crypto
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/registed.h"
#include "wasi_crypto/api.hpp"

#include <cstdint>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

using SkVariant = RegistedAlg::SkVariant;

WasiCryptoExpect<SkVariant>
importSk(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<std::vector<uint8_t>>
skExportData(const SkVariant &SkVariant,
             __wasi_secretkey_encoding_e_t Encoding) noexcept;

WasiCryptoExpect<PkVariant> skPublicKey(const SkVariant &SkVariant) noexcept;

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
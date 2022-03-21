// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "helper.h"
#include "host/wasi_crypto/symmetric/func.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Hash) {
  auto HashTest = [this](std::string_view Name,
                         const std::vector<uint8_t> &AbsorbData1,
                         const std::vector<uint8_t> &AbsorbData2,
                         const std::vector<uint8_t> &ExpectedSqueezeData1,
                         const std::vector<uint8_t> &ExpectedSqueezeData2,
                         const std::vector<uint8_t> &TruncatedSquueezeData) {
    WASI_CRYPTO_EXPECT_SUCCESS(
        StateHandle, symmetricStateOpen(Name, std::nullopt, std::nullopt));

    SCOPED_TRACE(Name);
    {
      // "data"
      std::vector<uint8_t> SqueezeContent(ExpectedSqueezeData1.size());
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(StateHandle, AbsorbData1));
      WASI_CRYPTO_EXPECT_TRUE(
          symmetricStateSqueeze(StateHandle, SqueezeContent));
      EXPECT_EQ(SqueezeContent, ExpectedSqueezeData1);
    }

    {
      // "datamore_data"
      std::vector<uint8_t> SqueezeContent(ExpectedSqueezeData2.size());
      WASI_CRYPTO_EXPECT_TRUE(symmetricStateAbsorb(StateHandle, AbsorbData2));
      WASI_CRYPTO_EXPECT_TRUE(
          symmetricStateSqueeze(StateHandle, SqueezeContent));
      EXPECT_EQ(SqueezeContent, ExpectedSqueezeData2);
    }

    {
      // smaller than the hash function output size. truncate output
      std::vector<uint8_t> SqueezeContent(TruncatedSquueezeData.size());
      WASI_CRYPTO_EXPECT_TRUE(
          symmetricStateSqueeze(StateHandle, SqueezeContent));
      EXPECT_EQ(SqueezeContent, TruncatedSquueezeData);
    }

    {
      // requested size exceeds return invalid_length.
      std::vector<uint8_t> SqueezeContent(ExpectedSqueezeData1.size() + 1);
      EXPECT_EQ(symmetricStateSqueeze(StateHandle, SqueezeContent).error(),
                __WASI_CRYPTO_ERRNO_INVALID_LENGTH);
    }

    {
      // some error case check
      EXPECT_EQ(symmetricKeyGenerate(Name, std::nullopt).error(),
                __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
      EXPECT_EQ(symmetricStateOpen(Name, InvaildHandle, std::nullopt).error(),
                __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
      EXPECT_EQ(symmetricStateOptionsGet(StateHandle, "foo"sv, {}).error(),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      EXPECT_EQ(symmetricStateOptionsGetU64(StateHandle, "foo"sv).error(),
                __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
      EXPECT_EQ(symmetricStateSqueezeTag(StateHandle).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateSqueezeKey(StateHandle, Name).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateMaxTagLen(StateHandle).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateEncrypt(StateHandle, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateEncryptDetached(StateHandle, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateDecrypt(StateHandle, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateDecryptDetached(StateHandle, {}, {}, {}).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
      EXPECT_EQ(symmetricStateRatchet(StateHandle).error(),
                __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    }

    // close
    EXPECT_EQ(symmetricStateClose(StateHandle).error(),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  };

  HashTest(
      "SHA-256"sv, "data"_u8, "more_data"_u8,
      "3a6eb0790f39ac87c94f3856b2dd2c5d110e6811602261a9a923d3bb23adc8b7"_u8v,
      "13c40eec22541a155e172010c7fd6ef654e4e138a0c20923f9a91062a27f57b6"_u8v,
      "13c40eec22541a155e172010c7fd6ef654e4e138a0c20923f9a91062a27f57"_u8v);
  HashTest(
      "SHA-512"sv, "data"_u8, "more_data"_u8,
      "77c7ce9a5d86bb386d443bb96390faa120633158699c8844c30b13ab0bf92760b7e4416aea397db91b4ac0e5dd56b8ef7e4b066162ab1fdc088319ce6defc876"_u8v,
      "78d0b55eeb3a07754f0967a6e960b5b7488b09ec4d2a62d832a45d80f814aef88e5414e2115165012ac592ff050651e956089a5aacd4ea52cf247c3cc2f6add2"_u8v,
      "78d0b55eeb3a07754f0967a6e960b5b7488b09ec4d2a62d832a45d80f814aef88e5414e2115165012ac592ff050651e956089a5aacd4ea52cf247c3cc2f6ad"_u8v);
  HashTest(
      "SHA-512/256"sv, "data"_u8, "more_data"_u8,
      "99902eaf90e92264667843cde66675ed94caa361634bad57874642aa364aa968"_u8v,
      "d1def71920a44d8b6c83b2eaa99379a16047cc82cec8d80689fbf02fbd062481"_u8v,
      "d1def71920a44d8b6c83b2eaa99379a16047cc82cec8d80689fbf02fbd0624"_u8v);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
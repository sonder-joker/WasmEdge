// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/utils/handles_manager.h - OptionalRef definition ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains definition of OptionalRef and some helper function
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/utils/error.h"

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

template <typename T> using OptionalRef = T *;

namespace detail {

template <typename> struct IsOptional : std::false_type {};
template <typename T> struct IsOptional<std::optional<T>> : std::true_type {
  using Type = T;
};

template <typename> struct IsExpected : std::false_type {};
template <typename T> struct IsExpected<WasiCryptoExpect<T>> : std::true_type {
  using Type = T;
};

template <typename> struct IsOptionalRef : std::false_type {};
template <typename T> struct IsOptionalRef<OptionalRef<T>> : std::true_type {
  using Type = T;
};

template <typename> struct IsExpectedOptionalRef : std::false_type {};
template <typename T>
struct IsExpectedOptionalRef<WasiCryptoExpect<OptionalRef<T>>>
    : std::true_type {
  using Type = T;
};

} // namespace detail

/// std::optional<T> -> (T -> WasiCrypto<R>) ->
/// WasiCryptoExpect<std::optional<R>>
template <
    typename O, typename F,
    typename = std::enable_if_t<detail::IsOptional<std::decay_t<O>>::value>>
inline auto mapAndTransposeOptional(O &&Optional, F &&Function) noexcept
    -> std::enable_if_t<
        detail::IsExpected<std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), *std::forward<O>(Optional)))>>::value,
        WasiCryptoExpect<std::optional<typename detail::IsExpected<std::decay_t<
            decltype(std::invoke(std::forward<F>(Function),
                                 *std::forward<O>(Optional)))>>::Type>>> {
  if (!Optional)
    return std::nullopt;

  return std::invoke(std::forward<F>(Function), *Optional);
}

/// std::optional<T> -> (T -> WasiCryptoExpect<OptionalRef<R>>) ->
/// WasiCryptoExpect<OptionalRef<R>>
template <
    typename O, typename F,
    typename = std::enable_if_t<detail::IsOptional<std::decay_t<O>>::value>>
inline auto transposeOptionalToRef(O &&Optional, F &&Function) noexcept
    -> WasiCryptoExpect<OptionalRef<typename detail::IsExpectedOptionalRef<
        std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), *std::forward<O>(Optional)))>>::Type>> {
  if (!Optional)
    return nullptr;

  return std::invoke(std::forward<F>(Function), *Optional);
}

/// OptionalRef<T> -> (T -> WasiCryptoExpect<OptionalRef<R>>) ->
/// WasiCryptoExpect<OptionalRef<R>>
template <
    typename O, typename F,
    typename = std::enable_if_t<detail::IsOptionalRef<std::decay_t<O>>::value>>
inline auto transposeOptionalRef(O &&Optional, F &&Function) noexcept
    -> WasiCryptoExpect<OptionalRef<typename detail::IsExpectedOptionalRef<
        std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), *std::forward<O>(Optional)))>>::Type>> {
  if (!Optional)
    return nullptr;

  return std::invoke(std::forward<F>(Function), *Optional);
}

/// std::optional<T> -> OptionalRef<T>
template <typename O, typename = std::enable_if_t<
                          detail::IsOptional<std::decay_t<O>>::value>>
inline auto asOptionalRef(O &&Optional) noexcept
    -> OptionalRef<typename detail::IsOptional<std::decay_t<O>>::Type> {
  if (!Optional)
    return nullptr;

  return &*Optional;
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
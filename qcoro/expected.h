#pragma once

#include "error.h"

namespace QCoro {

namespace detail {
#include "tl_expected_p.h"
} // namespace

template<typename E = QCoro::ErrorCode>
using Unexpected = detail::tl::unexpected<E>;

constexpr inline detail::tl::in_place_t inPlace = detail::tl::in_place;

template<typename E>
auto makeUnexpected(E &&e) {
    return detail::tl::make_unexpected(std::forward<E>(e));
}

template<typename T, typename E = QCoro::ErrorCode>
using Expected = detail::tl::expected<T, E>;

} // namespace QCoro

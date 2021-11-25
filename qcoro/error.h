#pragma once

#include <system_error>

#include <QString>
#include <QDebug>

namespace QCoro {

class ErrorCode;

class ErrorCategory {
public:
    constexpr ErrorCategory(const std::error_category &cat) noexcept
        : cat(cat) {}
    QString name() const {
        return QString::fromUtf8(cat.name());
    }

    operator const std::error_category &() noexcept {
        return cat;
    }

    std::error_condition defaultErrorCondition(int code) const noexcept {
        return cat.default_error_condition(code);
    }

    bool equivalent(int code, const std::error_condition &cond) const noexcept {
        return cat.equivalent(code, cond);
    }

    bool equivalent(const ErrorCode &code, int condition) const noexcept;

    bool equivalent(const std::error_code &code, int condition) const noexcept {
        return cat.equivalent(code, condition);
    }

    QString message(int condition) const {
        return QString::fromStdString(cat.message(condition));
    }

    bool operator==(const ErrorCategory &other) const noexcept {
        return cat == other.cat;
    }

    bool operator!=(const ErrorCategory &other) const noexcept {
        return cat != other.cat;
    }

    bool operator<(const ErrorCategory &other) const noexcept {
        return cat < other.cat;
    }

private:
    const std::error_category &cat;
};

class ErrorCode {
public:
    constexpr ErrorCode() noexcept = default;
    ErrorCode(std::error_code err) noexcept
        : err(err) {}

    ErrorCode(int ec, const std::error_category &ecat) noexcept
        : err(ec, ecat) {}

    ErrorCode(std::errc e) noexcept
        : err(std::make_error_code(e)) {}

    ErrorCode &operator=(std::errc e) noexcept {
        err = std::make_error_code(e);
        return *this;
    }

    void assign(int ec, const std::error_category &ecat) noexcept {
        err.assign(ec, ecat);
    }

    void clear() noexcept {
        err.clear();
    }

    int value() const noexcept {
        return err.value();
    }

    ErrorCategory category() const noexcept {
        return ErrorCategory{err.category()};
    }

    std::error_condition defaultErrorCondition() const noexcept {
        return err.default_error_condition();
    }

    QString message() const {
        return QString::fromStdString(err.message());
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(err);
    }

    operator std::error_code () const noexcept {
        return err;
    }

    operator const std::error_code & () const noexcept {
        return err;
    }

    bool operator==(const ErrorCode &other) const noexcept {
        return err == other.err;
    }

    bool operator!=(const ErrorCode &other) const noexcept {
        return err != other.err;
    }

    bool operator<(const ErrorCode &other) const noexcept {
        return err < other.err;
    }

private:
    std::error_code err = {};
};


inline bool ErrorCategory::equivalent(const ErrorCode &code, int condition) const noexcept {
    return cat.equivalent(code, condition);
}

} // namespace QCoro


inline QDebug operator<<(QDebug dbg, const QCoro::ErrorCode &ec) {
    dbg.nospace() << ec.category().name() << ": " << ec.message() << " (" << ec.value() << ")";
    return dbg;
}


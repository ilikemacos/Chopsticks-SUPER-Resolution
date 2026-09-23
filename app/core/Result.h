#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace ufx {

struct Error {
    std::string code;
    std::string message;
};

template <typename T>
class Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error err) : data_(std::move(err)) {}

    bool ok() const noexcept { return std::holds_alternative<T>(data_); }
    explicit operator bool() const noexcept { return ok(); }

    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }
    const Error& error() const { return std::get<Error>(data_); }

private:
    std::variant<T, Error> data_;
};

template <>
class Result<void> {
public:
    Result() = default;
    Result(Error err) : err_(std::move(err)) {}
    bool ok() const noexcept { return !err_.has_value(); }
    explicit operator bool() const noexcept { return ok(); }
    const Error& error() const { return *err_; }
private:
    std::optional<Error> err_;
};

}

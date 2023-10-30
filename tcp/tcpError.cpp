#include "tcp.h"

TCPError::TCPError(std::string message) : message{std::move(message)} {}

const char *TCPError::what() const noexcept {
    return message.c_str();
}
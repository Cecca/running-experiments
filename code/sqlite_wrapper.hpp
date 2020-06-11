#pragma once

#include <iostream>
#include "sqlite3.h"

//! A very minimal wrapper around the sqlite3 C api. Just provides RAII
//! encapsulation of resources and convenience methods for binding parameters
//! to prepared statements
namespace sqlite {
class Connection {
public:
  sqlite3 *handle;

public:
  Connection(std::string spec) noexcept(false) {
    switch (sqlite3_open(spec.c_str(), &this->handle)) {
    case SQLITE_OK:
      // OK, nop
      break;
    default:
      throw std::runtime_error(sqlite3_errmsg(this->handle));
    }
  }

  ~Connection() noexcept(false) {
    switch (sqlite3_close(this->handle)) {
    case SQLITE_OK:
      std::cout << "connection freed" << std::endl;
      break;
    default:
      throw std::runtime_error(sqlite3_errmsg(this->handle));
    }
  }
};

class Statement {
private:
  sqlite3_stmt *handle;

public:
  Statement(Connection &conn, std::string sql_str) noexcept(false) {
    switch (sqlite3_prepare_v2(conn.handle, sql_str.c_str(), -1, &this->handle,
                               NULL)) {
    case SQLITE_OK:
      // OK, nop
      break;
    default:
      throw std::runtime_error(sqlite3_errmsg(conn.handle));
    }
  }

  ~Statement() noexcept(false) {
    switch (sqlite3_finalize(this->handle)) {
    case SQLITE_OK:
      std::cout << "statement finalized" << std::endl;
      // OK, nop
      break;
    default:
      throw std::runtime_error("couldn't dispose of prepared statement");
    }
  }

  void bind_text(const std::string &placeholder,
                 const std::string &text) noexcept(false) {
    int rc = sqlite3_bind_text(
        this->handle,
        sqlite3_bind_parameter_index(this->handle, placeholder.c_str()),
        text.c_str(), text.length(), SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
      throw std::runtime_error("couldn't bind text");
    }
  }

  void bind_int(const std::string &placeholder, int value) noexcept(false) {
    int rc = sqlite3_bind_int(
        this->handle,
        sqlite3_bind_parameter_index(this->handle, placeholder.c_str()), value);
    if (rc != SQLITE_OK) {
      throw std::runtime_error("couldn't bind int");
    }
  }

  void bind_int64(const std::string &placeholder, uint64_t value) noexcept(false) {
    int rc = sqlite3_bind_int64(
        this->handle,
        sqlite3_bind_parameter_index(this->handle, placeholder.c_str()), value);
    if (rc != SQLITE_OK) {
      throw std::runtime_error("couldn't bind int64");
    }
  }

  void bind_double(const std::string &placeholder, double value) noexcept(false) {
    int rc = sqlite3_bind_double(
        this->handle,
        sqlite3_bind_parameter_index(this->handle, placeholder.c_str()), value);
    if (rc != SQLITE_OK) {
      throw std::runtime_error("couldn't bind double");
    }
  }

  int read_int(int index) noexcept(false) {
    return sqlite3_column_int(this->handle, index);
  }

  bool exec() noexcept(false) {
    switch (sqlite3_step(this->handle)) {
    case SQLITE_DONE:
      // We are done, reset the statement
      if (sqlite3_reset(this->handle) != SQLITE_OK) {
        throw std::runtime_error("couldn't reset the prepared statement");
      }
      return false;
    case SQLITE_ROW:
      // there are more things to be read, we can extract information
      return true;
    default:
      throw std::runtime_error("couldn't step the prepared statement");
    }
  }
};
} // namespace sqlite

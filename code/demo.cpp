#include "sqlite3.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>

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

sqlite::Connection db_connection() {
  return sqlite::Connection("file:demo-db.sqlite");
}

std::string datetime_now() {
  auto now = std::chrono::system_clock::now();
  auto epoch_seconds = std::chrono::system_clock::to_time_t(now);
  std::stringstream stream;
  stream << std::put_time(gmtime(&epoch_seconds), "%FT%T");
  auto truncated = std::chrono::system_clock::from_time_t(epoch_seconds);
  auto delta_us =
      std::chrono::duration_cast<std::chrono::microseconds>(now - truncated)
          .count();
  stream << "." << std::fixed << std::setw(6) << std::setfill('0') << delta_us;
  return stream.str();
}

int db_setup() {
  sqlite::Connection conn = db_connection();
  sqlite::Statement stmt(conn, "PRAGMA user_version;");
  stmt.exec();
  int version = stmt.read_int(0);
  std::cout << "database version " << version << std::endl;

  // Apply the changes defining each version. Note that the branches of the
  // switch will fall through: so all the changes are applied in order Version 0
  // stands for the uninitialized database
  switch (version + 1) {
  case 1: {
    std::cout << "applying initial schema definitions" << std::endl;
    sqlite::Statement create_table(conn,
                                   "CREATE TABLE results ("
                                   "  sha                 TEXT PRIMARY KEY,"
                                   "  date                TEXT,"
                                   "  dataset             TEXT,"
                                   "  dataset_version     INT,"
                                   "  algorithm           TEXT,"
                                   "  algorithm_version   INT,"
                                   "  parameters          TEXT,"
                                   "  running_time_ms     INT64"
                                   ");");
    create_table.exec();
    // Bump version number to the one of the initialized database
    sqlite::Statement bump(conn, "PRAGMA user_version = 1");
    bump.exec();
  }
  case 2: {
    std::cout << "results database schema up to date" << std::endl;
    break;
  }
  default: {
    std::cerr << "unsupported database version " << version << std::endl;
  }
  }

  return 0;
}

std::string sha256_string(std::string to_hash) {
  char outputBuffer[65];
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, to_hash.c_str(), to_hash.length());
  SHA256_Final(hash, &sha256);
  int i = 0;
  for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
  }
  return std::string(outputBuffer);
}

void record_result(std::string dataset, int dataset_version,
                   std::string algorithm, int algorithm_version,
                   std::string parameters, uint64_t running_time_ms) {
  std::string date = datetime_now();
  std::stringstream to_hash_stream;
  to_hash_stream << date << dataset << dataset_version << algorithm
                 << algorithm_version << parameters;
  std::string sha = sha256_string(to_hash_stream.str());

  sqlite::Connection conn = db_connection();
  sqlite::Statement stmt(
      conn,
      "INSERT INTO results"
      "  (sha, date, dataset, dataset_version, algorithm, algorithm_version, "
      "   parameters, running_time_ms)"
      "VALUES (:sha, :date, :dataset, :dataset_version, :algorithm, "
      "        :algorithm_version, :parameters, :running_time_ms);");

  stmt.bind_text(":sha", sha);
  stmt.bind_text(":date", date);
  stmt.bind_text(":dataset", dataset);
  stmt.bind_int(":dataset_version", dataset_version);
  stmt.bind_text(":algorithm", algorithm);
  stmt.bind_int(":algorithm_version", algorithm_version);
  stmt.bind_text(":parameters", parameters);
  stmt.bind_int64(":running_time_ms", running_time_ms);
  stmt.exec();
}

int main(int argc, char **argv) {
  // Bring the database schema to the most up to date version
  int rc = db_setup();

  record_result("test", 1, "algotest", 1, "params", 128);

  return rc;
}

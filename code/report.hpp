#pragma once

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <openssl/sha.h>
#include "sqlite_wrapper.hpp"

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


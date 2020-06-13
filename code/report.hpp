#pragma once

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "sqlite_wrapper.hpp"
#include "sha.hpp"

sqlite::Connection db_connection() {
  return sqlite::Connection("file:demo-db.sqlite");
}

std::string get_hostname() {
  char* h = getenv("HOST_HOSTNAME");
  if (h) {
    return std::string(h);
  } else {
    return std::string("NULL");
  }
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
                                   "  git_rev             TEXT,"
                                   "  hostname            TEXT,"
                                   "  date                TEXT,"
                                   "  dataset             TEXT,"
                                   "  dataset_version     INT,"
                                   "  algorithm           TEXT,"
                                   "  algorithm_version   INT,"
                                   "  parameters          TEXT,"
                                   "  running_time_ns     INT64"
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


bool contains_result(std::string dataset, int dataset_version,
                   std::string algorithm, int algorithm_version,
                   std::string parameters) {
  sqlite::Connection conn = db_connection();
  // We don't check for the GIT_REV, since a change in the git revision
  // might not imply a change in the dataset/algorithm version, hence
  // we don't want it to trigger a rerun
  sqlite::Statement stmt(
        conn,
        "SELECT COUNT(*) FROM results WHERE"
        " hostname = :hostname AND "
        " dataset = :dataset AND "
        " dataset_version = :dataset_version AND "
        " algorithm = :algorithm AND "
        " algorithm_version = :algorithm_version AND "
        " parameters = :parameters;");
  stmt.bind_text(":hostname", get_hostname());
  stmt.bind_text(":dataset", dataset);
  stmt.bind_int(":dataset_version", dataset_version);
  stmt.bind_text(":algorithm", algorithm);
  stmt.bind_int(":algorithm_version", algorithm_version);
  stmt.bind_text(":parameters", parameters);
  stmt.exec();

  return stmt.read_int(0) > 0;
}

void record_result(std::string dataset, int dataset_version,
                   std::string algorithm, int algorithm_version,
                   std::string parameters, uint64_t running_time_ns) {
  std::string hostname = get_hostname();
  std::string git_rev = GIT_REV;
  std::cout << "HERE" << git_rev << std::endl;
  std::string date = datetime_now();
  std::stringstream to_hash_stream;
  to_hash_stream << date << hostname << dataset << dataset_version << algorithm
                 << algorithm_version << parameters;
  std::string sha = sha256_string(to_hash_stream.str());

  sqlite::Connection conn = db_connection();
  sqlite::Statement stmt(
      conn,
      "INSERT INTO results"
      "  (sha, git_rev, hostname, date, dataset, dataset_version, algorithm, algorithm_version, "
      "   parameters, running_time_ns)"
      "VALUES (:sha, :git_rev, :hostname, :date, :dataset, :dataset_version, :algorithm, "
      "        :algorithm_version, :parameters, :running_time_ns);");

  stmt.bind_text(":sha", sha);
  stmt.bind_text(":git_rev", git_rev);
  stmt.bind_text(":hostname", hostname);
  stmt.bind_text(":date", date);
  stmt.bind_text(":dataset", dataset);
  stmt.bind_int(":dataset_version", dataset_version);
  stmt.bind_text(":algorithm", algorithm);
  stmt.bind_int(":algorithm_version", algorithm_version);
  stmt.bind_text(":parameters", parameters);
  stmt.bind_int64(":running_time_ns", running_time_ns);
  stmt.exec();
}


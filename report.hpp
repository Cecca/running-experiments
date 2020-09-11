#pragma once

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>
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
                                   "CREATE TABLE results_raw ("
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
    sqlite::Statement create_table2(conn,
                                    "CREATE TABLE nearest_neighbors ("
                                    "  sha                     TEXT,"
                                    "  query_index             INT,"
                                    "  nearest_neighbor_index  INT,"
                                    "  FOREIGN KEY(sha) REFERENCES results_raw(sha)"
                                    ")");
    create_table2.exec();
    // Bump version number to the one of the initialized database
    sqlite::Statement bump(conn, "PRAGMA user_version = 1");
    bump.exec();
  }
  case 2: {
    // TODO: we should select just the most recent one
    sqlite::Statement create_view(conn,
                                  "CREATE VIEW baseline AS "
                                  "SELECT dataset, dataset_version, query_index, nearest_neighbor_index as baseline_nn "
                                  "FROM (SELECT * FROM results_raw NATURAL JOIN "
                                  "    (SELECT sha, algorithm, MAX(algorithm_version) AS algorithm_version "
                                  "        FROM results_raw "
                                  "        WHERE algorithm == 'bruteforce'"
                                  "          AND parameters == 'method=simple;storage=float_aligned;')) "
                                  "NATURAL JOIN nearest_neighbors;"
                                  );
    create_view.exec();
    sqlite::Statement create_view2(conn,
      "CREATE VIEW results AS "
      "SELECT * FROM results_raw NATURAL JOIN "
      "   (SELECT sha, (1.0 - SUM(baseline_nn != nearest_neighbor_index) / (1.0 * COUNT(sha))) AS recall "
      "        FROM baseline NATURAL JOIN (SELECT sha, dataset, dataset_version, query_index, nearest_neighbor_index "
      "                                       FROM results_raw NATURAL JOIN nearest_neighbors) "
      "        GROUP BY sha);"
    );
    create_view2.exec();

    sqlite::Statement bump(conn, "PRAGMA user_version = 2");
    bump.exec();
  }
  case 3: {
    sqlite::Statement add_column_seed(conn, 
            "ALTER TABLE results_raw ADD COLUMN seed INT64 DEFAULT 0;");
    add_column_seed.exec();

    sqlite::Statement add_column_experiment_fn(conn, 
            "ALTER TABLE results_raw ADD COLUMN experiment_file TEXT DEFAULT \"not provided\";");
    add_column_experiment_fn.exec();

    sqlite::Statement bump(conn, "PRAGMA user_version = 3");
    bump.exec();
  }
  case 4: {
    sqlite::Statement add_column_components(conn, 
            "ALTER TABLE results_raw ADD COLUMN components TEXT DEFAULT '' NOT NULL;");
    add_column_components.exec();

    sqlite::Statement add_column_version_brute_force(conn, 
            "ALTER TABLE results_raw ADD COLUMN version_brute_force INT DEFAULT 0;");
    add_column_version_brute_force.exec();

    sqlite::Statement add_column_version_filter(conn, 
            "ALTER TABLE results_raw ADD COLUMN version_filter INT DEFAULT 0;");
    add_column_version_filter.exec();

    sqlite::Statement add_column_version_distance(conn, 
            "ALTER TABLE results_raw ADD COLUMN version_distance INT DEFAULT 0;");
    add_column_version_distance.exec();

    sqlite::Statement add_column_version_storage(conn, 
            "ALTER TABLE results_raw ADD COLUMN version_storage INT DEFAULT 0;");
    add_column_version_storage.exec();

    sqlite::Statement bump(conn, "PRAGMA user_version = 4");
    bump.exec();
  }
  case 5: {
    // The following view takes the maximum component version for each combination
    // of components, to be used for filtering in the `recent_results` view.
    sqlite::Statement create_view_versions(conn,
      "CREATE VIEW recent_versions AS SELECT components, "
      "    MAX(algorithm_version) AS algorithm_version, MAX(version_brute_force) AS version_brute_force, MAX(version_filter) AS version_filter, "
      "    MAX(version_storage) AS version_storage, MAX(version_distance) as version_distance "
      "FROM results_raw "
      "GROUP BY components;");
    create_view_versions.exec();

    // Filters results keeping only the ones using the most up to date component.
    // Begin a natural join, it uses all the columns from the `recent_versions` view.
    sqlite::Statement create_view_recent_results(conn,
      "CREATE VIEW recent_results AS SELECT * FROM results NATURAL JOIN recent_versions;");
    create_view_recent_results.exec();

    sqlite::Statement bump(conn, "PRAGMA user_version = 5");
    bump.exec();
  }
  case 6: {
    sqlite::Statement drop_view(conn, "DROP VIEW results;");
    drop_view.exec();
    sqlite::Statement drop_view2(conn, "DROP VIEW recent_results;");
    drop_view2.exec();

    sqlite::Statement create_view(conn,
      "CREATE VIEW results_with_recall AS "
      "SELECT * FROM results_raw NATURAL JOIN "
      "   (SELECT sha, (1.0 - SUM(baseline_nn != nearest_neighbor_index) / (1.0 * COUNT(sha))) AS recall "
      "        FROM baseline NATURAL JOIN (SELECT sha, dataset, dataset_version, query_index, nearest_neighbor_index "
      "                                       FROM results_raw NATURAL JOIN nearest_neighbors) "
      "        GROUP BY sha);"
    );
    create_view.exec();

    sqlite::Statement create_view_recent_results(conn,
      "CREATE VIEW results AS SELECT * FROM results_with_recall NATURAL JOIN recent_versions;");
    create_view_recent_results.exec();

    sqlite::Statement bump(conn, "PRAGMA user_version = 6");
    bump.exec();
    break;
  }
  case 7: {
    std::cout << "results database schema up to date" << std::endl;
    break;
  }
  default: {
    std::cerr << "unsupported database version " << version << std::endl;
  }
  }

  return 0;
}

// Checks whether an experiment has already been run. The `Version` information
// is used as follows. The `components` column reports which components are used
// by the run (including the implementation of distance and the implementation
// of storage): first we check against this column; if an experiment with the same
// components has been run, we check the `version_*` columns, which report the
// appropriate versions of the related component, with `0` meaning that the component
// is not used.
bool contains_result(std::string dataset, int dataset_version,
                   std::string algorithm,
                   Version version,
                   uint64_t seed, std::string parameters) {
  sqlite::Connection conn = db_connection();
  // We don't check for the GIT_REV, since a change in the git revision
  // might not imply a change in the dataset/algorithm version, hence
  // we don't want it to trigger a rerun
  sqlite::Statement stmt(
        conn,
        "SELECT COUNT(*) FROM results_raw WHERE"
        " hostname = :hostname AND "
        " dataset = :dataset AND "
        " dataset_version = :dataset_version AND "
        " algorithm = :algorithm AND "
        " algorithm_version = :algorithm_version AND "
        " components = :components AND "
        " version_brute_force = :version_brute_force AND "
        " version_filter = :version_filter AND "
        " version_distance = :version_distance AND "
        " version_storage = :version_storage AND "
        " seed = :seed AND "
        " parameters = :parameters;");
  stmt.bind_text(":hostname", get_hostname());
  stmt.bind_text(":dataset", dataset);
  stmt.bind_int(":dataset_version", dataset_version);
  stmt.bind_text(":algorithm", algorithm);
  stmt.bind_int(":algorithm_version", version.algo_version);
  stmt.bind_text(":components", version.components);
  stmt.bind_int(":version_brute_force", version.brute_force);
  stmt.bind_int(":version_filter", version.filter);
  stmt.bind_int(":version_storage", version.storage);
  stmt.bind_int(":version_distance", version.distance);
  stmt.bind_int64(":seed", seed);
  stmt.bind_text(":parameters", parameters);
  stmt.exec();

  return stmt.read_int(0) > 0;
}

void record_result(std::string dataset, int dataset_version,
                   std::string algorithm,
                   Version version,
                   std::string parameters, std::string experiment_file, 
                   uint64_t seed, uint64_t running_time_ns,
                   std::vector<size_t> nearest_neighbors) {
  std::string hostname = get_hostname();
  std::string git_rev = GIT_REV;
  std::string date = datetime_now();
  std::stringstream to_hash_stream;
  to_hash_stream << date << hostname << dataset << dataset_version << algorithm
                << version.components
                << version.algo_version << version.brute_force << version.filter 
                << version.storage << version.distance << parameters;
  std::string sha = sha256_string(to_hash_stream.str());

  sqlite::Connection conn = db_connection();
  sqlite::Statement stmt(
      conn,
      "INSERT INTO results_raw"
      "  (sha, git_rev, hostname, date, dataset, dataset_version, algorithm, algorithm_version, "
      "   components, version_brute_force, version_filter, version_storage, version_distance, "
      "   experiment_file, seed, parameters, running_time_ns)"
      "VALUES (:sha, :git_rev, :hostname, :date, :dataset, :dataset_version, "
      "        :algorithm, :algorithm_version, "
      "        :components, :version_brute_force, :version_filter, :version_storage, :version_distance, "
      "        :experiment_file, :seed, :parameters, :running_time_ns);");

  stmt.bind_text(":sha", sha);
  stmt.bind_text(":git_rev", git_rev);
  stmt.bind_text(":hostname", hostname);
  stmt.bind_text(":date", date);
  stmt.bind_text(":dataset", dataset);
  stmt.bind_int(":dataset_version", dataset_version);
  stmt.bind_text(":algorithm", algorithm);
  stmt.bind_int(":algorithm_version", version.algo_version);
  stmt.bind_text(":components", version.components);
  stmt.bind_int(":version_brute_force", version.brute_force);
  stmt.bind_int(":version_filter", version.filter);
  stmt.bind_int(":version_storage", version.storage);
  stmt.bind_int(":version_distance", version.distance);
  stmt.bind_text(":experiment_file", experiment_file);
  stmt.bind_text(":parameters", parameters);
  stmt.bind_int64(":seed", seed);
  stmt.bind_int64(":running_time_ns", running_time_ns);
  stmt.exec();

  sqlite::Statement stmt_nn(
    conn,
    "INSERT INTO nearest_neighbors"
    "  (sha, query_index, nearest_neighbor_index)"
    "VALUES (:sha, :query_index, :nearest_neighbor_index)"
  );
  for (size_t query_index=0; query_index < nearest_neighbors.size(); query_index++) {
    stmt_nn.bind_text(":sha", sha);
    stmt_nn.bind_int(":query_index", query_index);
    stmt_nn.bind_int(":nearest_neighbor_index", nearest_neighbors[query_index]);
    stmt_nn.exec();
  }
}


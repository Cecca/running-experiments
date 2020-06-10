#include "sqlite3.h"
#include <openssl/sha.h>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

sqlite3* db_connection() {
    sqlite3 *handle;
    sqlite3_open("file:demo-db.sqlite", &handle);
    return handle;
}

std::string datetime_now() {
    auto now = std::chrono::system_clock::now();
	auto epoch_seconds = std::chrono::system_clock::to_time_t(now);
	std::stringstream stream;
	stream << std::put_time(gmtime(&epoch_seconds), "%FT%T");
	auto truncated = std::chrono::system_clock::from_time_t(epoch_seconds);
	auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(now - truncated).count();
	stream << "." << std::fixed << std::setw(6) << std::setfill('0') << delta_us;
	return stream.str();
}

int db_setup() {
    sqlite3* handle = db_connection();
    sqlite3_stmt *stmt_handle;
    sqlite3_stmt *stmt_handle2;
    sqlite3_prepare_v2(handle, "PRAGMA user_version;", -1, &stmt_handle, NULL);
    int rc = sqlite3_step(stmt_handle);
    if (rc != SQLITE_ROW) {
        std::cerr << "not a sqlite row" << std::endl;
        return 1;
    }
    int version = sqlite3_column_int(stmt_handle, 0);
    std::cout << "database version " << version << std::endl;
    sqlite3_finalize(stmt_handle);

    // Apply the changes defining each version. Note that the branches of the switch
    // will fall through: so all the changes are applied in order
    // Version 0 stands for the uninitialized database
    const char *sql_str;
    switch (version + 1) {
        case 1: 
            std::cout << "applying initial schema definitions" << std::endl;
            sql_str = 
                "CREATE TABLE results ("
                "  sha                 TEXT PRIMARY KEY,"
                "  date                TEXT,"
                "  dataset             TEXT,"
                "  dataset_version     INT,"
                "  algorithm           TEXT,"
                "  algorithm_version   INT,"
                "  parameters          TEXT,"
                "  running_time_ms     INT64"
                ");";
            if (sqlite3_prepare_v2(handle, sql_str, -1, &stmt_handle, NULL) != SQLITE_OK) std::cerr << "couldn't step " << sqlite3_errmsg(handle) << std::endl;
            if (sqlite3_step(stmt_handle) != SQLITE_DONE) std::cerr << "couldn't step " << sqlite3_errmsg(handle) << std::endl;
            if (sqlite3_finalize(stmt_handle) != SQLITE_OK) std::cerr << "couldn't finalize " << sqlite3_errmsg(handle) << std::endl;
            // Bump version number to the one of the initialized database
            // sqlite3_prepare_v2(handle, "PRAGMA user_version = 1", -1, &stmt_handle2, NULL);
            // sqlite3_step(stmt_handle2);
            // if (sqlite3_finalize(stmt_handle2) != SQLITE_OK) std::cerr << "coludn't finalize " << sqlite3_errmsg(handle) << std::endl;
        case 2: 
            std::cout << "results database schema up to date" << std::endl;
            break;
        default: std::cerr << "unsupported database version " << version << std::endl;
    }

    if (sqlite3_close(handle) != SQLITE_OK) std::cerr << "couldn't close " << sqlite3_errmsg(handle) << std::endl;
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
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    return std::string (outputBuffer);
}

void record_result(std::string dataset, int dataset_version, std::string algorithm, int algorithm_version, std::string parameters, uint64_t running_time_ms) {
    std::string date = datetime_now();
    std::stringstream to_hash_stream;
    to_hash_stream << date << dataset << dataset_version << algorithm << algorithm_version << parameters;
    std::string sha = sha256_string(to_hash_stream.str());


    sqlite3* handle = db_connection();
    sqlite3_stmt *stmt_handle;
    const char *sql_str =
        "INSERT INTO results"
        "  (sha, date, dataset, dataset_version, algorithm, algorithm_version, parameters, running_time_ms)"
        "VALUES (:sha, :date, :dataset, :dataset_version, :algorithm, :algorithm_version, :parameters, :running_time_ms);";
    sqlite3_prepare_v2(handle, sql_str, -1, &stmt_handle, NULL);
    sqlite3_bind_text(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":sha"), sha.c_str(), sha.length(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":date"), date.c_str(), date.length(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":dataset"), dataset.c_str(), dataset.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":dataset_version"), dataset_version);
    sqlite3_bind_text(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":algorithm"), algorithm.c_str(), algorithm.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":algorithm_version"), algorithm_version);
    sqlite3_bind_text(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":parameters"), parameters.c_str(), parameters.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt_handle, sqlite3_bind_parameter_index(stmt_handle, ":running_time_ms"), running_time_ms);
    int rc = sqlite3_step(stmt_handle);
    if (rc != SQLITE_DONE) {
        std::cerr << "error inserting record" << std::endl << sqlite3_errmsg(handle) << std::endl;
    }
    sqlite3_reset(stmt_handle);
    sqlite3_finalize(stmt_handle);

    if (sqlite3_close(handle) != SQLITE_OK) std::cerr << sqlite3_errmsg(handle) << std::endl;
}

int main(int argc, char** argv) {
    // Bring the database schema to the most up to date version
    int rc = db_setup();

    // record_result("test", 1, "algotest", 1, "params", 128);

    return rc;
}

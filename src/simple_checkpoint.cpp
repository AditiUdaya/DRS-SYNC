#include "drs_sync/simple_checkpoint.hpp"
#include <iostream>

namespace drs_sync {

SimpleCheckpoint::SimpleCheckpoint(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database\n";
        return;
    }
    init_db();
}

SimpleCheckpoint::~SimpleCheckpoint() {
    if (db_) sqlite3_close(db_);
}

void SimpleCheckpoint::init_db() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS checkpoints (
            file_id TEXT PRIMARY KEY,
            last_chunk INTEGER,
            file_size INTEGER,
            updated_at INTEGER
        )
    )";
    
    char* err = nullptr;
    sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) {
        std::cerr << "DB init error: " << err << "\n";
        sqlite3_free(err);
    }
}

bool SimpleCheckpoint::save_progress(const std::string& file_id,
                                     uint32_t last_chunk,
                                     uint64_t file_size) {
    const char* sql = R"(
        INSERT OR REPLACE INTO checkpoints 
        (file_id, last_chunk, file_size, updated_at)
        VALUES (?, ?, ?, strftime('%s', 'now'))
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, file_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, last_chunk);
    sqlite3_bind_int64(stmt, 3, file_size);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool SimpleCheckpoint::load_progress(const std::string& file_id,
                                     uint32_t& last_chunk) {
    const char* sql = "SELECT last_chunk FROM checkpoints WHERE file_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, file_id.c_str(), -1, SQLITE_TRANSIENT);
    
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        last_chunk = sqlite3_column_int(stmt, 0);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found;
}

void SimpleCheckpoint::clear(const std::string& file_id) {
    const char* sql = "DELETE FROM checkpoints WHERE file_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, file_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

} // namespace drs_sync
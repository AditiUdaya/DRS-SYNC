#pragma once
#include <string>
#include <sqlite3.h>
#include <memory>

namespace drs_sync {

class SimpleCheckpoint {
public:
    SimpleCheckpoint(const std::string& db_path = "transfers.db");
    ~SimpleCheckpoint();
    
    // Save progress
    bool save_progress(const std::string& file_id, 
                      uint32_t last_chunk,
                      uint64_t file_size);
    
    // Load progress
    bool load_progress(const std::string& file_id,
                      uint32_t& last_chunk);
    
    // Clear completed transfer
    void clear(const std::string& file_id);
    
private:
    sqlite3* db_ = nullptr;
    void init_db();
};

} // namespace drs_sync
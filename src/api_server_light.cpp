#include "httplib.h"
#include "json.hpp"
#include "drs_sync/transfer_engine.hpp"
#include <memory>
#include <thread>
#include <iostream>

using json = nlohmann::json;
using namespace httplib;

class LightweightAPI {
public:
    LightweightAPI() 
        : io_context_(),
          engine_(std::make_shared<drs_sync::TransferEngine>(io_context_)) {
        
        // Start IO context thread
        io_thread_ = std::thread([this]() {
            io_context_.run();
        });
        
        setup_routes();
    }
    
    ~LightweightAPI() {
        io_context_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }
    
    void run(const std::string& host = "0.0.0.0", int port = 8080) {
        std::cout << "\n";
        std::cout << "🏎️  DRS-SYNC Lightweight API Server\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "📡 Listening on http://" << host << ":" << port << "\n";
        std::cout << "🎯 Endpoints:\n";
        std::cout << "   GET  /ping\n";
        std::cout << "   POST /manifest/create\n";
        std::cout << "   GET  /transfer/{id}/status\n";
        std::cout << "   POST /transfer/{id}/pause\n";
        std::cout << "   POST /transfer/{id}/resume\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        server_.listen(host.c_str(), port);
    }
    
private:
    void setup_routes() {
        // CORS middleware
        server_.set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type"}
        });
        
        // Handle OPTIONS for CORS preflight
        server_.Options(".*", [](const Request&, Response& res) {
            res.status = 200;
        });
        
        // Root endpoint - API information
        server_.Get("/", [](const Request&, Response& res) {
            json response = {
                {"service", "DRS-SYNC"},
                {"version", "1.0.0"},
                {"status", "running"},
                {"endpoints", {
                    {"GET /ping", "Health check endpoint"},
                    {"POST /manifest/create", "Create a new file transfer"},
                    {"GET /transfer/{id}/status", "Get transfer status"},
                    {"POST /transfer/{id}/pause", "Pause a transfer"},
                    {"POST /transfer/{id}/resume", "Resume a transfer"}
                }}
            };
            res.set_content(response.dump(2), "application/json");
        });
        
        // Health check
        server_.Get("/ping", [](const Request&, Response& res) {
            json response = {
                {"status", "ok"},
                {"service", "DRS-SYNC"},
                {"timestamp", std::time(nullptr)}
            };
            res.set_content(response.dump(2), "application/json");
        });
        
        // Test POST route
        server_.Post("/test/post", [](const Request& req, Response& res) {
            json response = {{"test", "POST works"}};
            res.set_content(response.dump(2), "application/json");
        });
        
        // Create new transfer
        server_.Post("/manifest/create", [this](const Request& req, Response& res) {
            try {
                auto j = json::parse(req.body);
                
                std::string filepath = j["filepath"];
                std::string destination = j["destination"];
                std::string priority_str = j.value("priority", "NORMAL");
                
                drs_sync::Priority priority = drs_sync::Priority::NORMAL;
                if (priority_str == "HIGH") priority = drs_sync::Priority::HIGH;
                else if (priority_str == "CRITICAL") priority = drs_sync::Priority::CRITICAL;
                
                // Parse destination (host:port)
                size_t colon = destination.find(':');
                if (colon == std::string::npos) {
                    throw std::runtime_error("Invalid destination format. Use host:port");
                }
                
                std::string host = destination.substr(0, colon);
                uint16_t port = std::stoi(destination.substr(colon + 1));
                
                asio::ip::udp::endpoint remote(
                    asio::ip::make_address(host), port);
                
                std::string file_id = engine_->start_transfer(filepath, remote, priority);
                
                json response = {
                    {"file_id", file_id},
                    {"status", "started"},
                    {"filepath", filepath},
                    {"priority", priority_str}
                };
                
                std::cout << "✅ Started transfer: " << file_id << " (" << priority_str << ")\n";
                
                res.set_content(response.dump(2), "application/json");
                
            } catch (const json::exception& e) {
                res.status = 400;
                json error = {
                    {"error", "Invalid JSON"},
                    {"message", e.what()}
                };
                res.set_content(error.dump(2), "application/json");
            } catch (const std::exception& e) {
                res.status = 500;
                json error = {
                    {"error", "Transfer failed"},
                    {"message", e.what()}
                };
                res.set_content(error.dump(2), "application/json");
            }
        });
        
        // Get transfer status
        server_.Get("/transfer/:id/status", [this](const Request& req, Response& res) {
            std::string file_id = req.path_params.at("id");
            
            try {
                auto stats = engine_->get_stats(file_id);
                
                json response = {
                    {"file_id", file_id},
                    {"bytes_sent", stats.bytes_sent.load()},
                    {"bytes_acked", stats.bytes_acked.load()},
                    {"chunks_sent", stats.chunks_sent.load()},
                    {"chunks_acked", stats.chunks_acked.load()},
                    {"retransmissions", stats.retransmissions.load()},
                    {"throughput_mbps", stats.throughput_mbps.load()},
                    {"completed", stats.completed.load()},
                    {"paused", stats.paused.load()}
                };
                
                res.set_content(response.dump(2), "application/json");
                
            } catch (const std::exception& e) {
                res.status = 404;
                json error = {
                    {"error", "Transfer not found"},
                    {"file_id", file_id}
                };
                res.set_content(error.dump(2), "application/json");
            }
        });
        
        // Pause transfer
        server_.Post("/transfer/:id/pause", [this](const Request& req, Response& res) {
            std::string file_id;
            try {
                if (req.path_params.find("id") != req.path_params.end()) {
                    file_id = req.path_params.at("id");
                } else {
                    // Fallback: try to extract from path manually
                    std::string path = req.path;
                    size_t start = path.find("/transfer/") + 9;
                    size_t end = path.find("/pause", start);
                    if (end != std::string::npos) {
                        file_id = path.substr(start, end - start);
                    } else {
                        throw std::runtime_error("Invalid file_id in path");
                    }
                }
                
                engine_->pause_transfer(file_id);
                
                json response = {
                    {"file_id", file_id},
                    {"status", "paused"}
                };
                
                std::cout << "⏸️  Paused transfer: " << file_id << "\n";
                
                res.set_content(response.dump(2), "application/json");
                
            } catch (const std::exception& e) {
                res.status = 404;
                json error = {
                    {"error", "Transfer not found"},
                    {"file_id", file_id.empty() ? "unknown" : file_id},
                    {"message", e.what()}
                };
                res.set_content(error.dump(2), "application/json");
            }
        });
        
        // Resume transfer
        server_.Post("/transfer/:id/resume", [this](const Request& req, Response& res) {
            std::string file_id;
            try {
                if (req.path_params.find("id") != req.path_params.end()) {
                    file_id = req.path_params.at("id");
                } else {
                    // Fallback: try to extract from path manually
                    std::string path = req.path;
                    size_t start = path.find("/transfer/") + 9;
                    size_t end = path.find("/resume", start);
                    if (end != std::string::npos) {
                        file_id = path.substr(start, end - start);
                    } else {
                        throw std::runtime_error("Invalid file_id in path");
                    }
                }
                
                engine_->resume_transfer(file_id);
                
                json response = {
                    {"file_id", file_id},
                    {"status", "resumed"}
                };
                
                std::cout << "▶️  Resumed transfer: " << file_id << "\n";
                
                res.set_content(response.dump(2), "application/json");
                
            } catch (const std::exception& e) {
                res.status = 404;
                json error = {
                    {"error", "Transfer not found"},
                    {"file_id", file_id.empty() ? "unknown" : file_id},
                    {"message", e.what()}
                };
                res.set_content(error.dump(2), "application/json");
            }
        });
        
        // 404 handler
        server_.set_error_handler([](const Request&, Response& res) {
            json error = {
                {"error", "Not Found"},
                {"message", "The requested endpoint does not exist"}
            };
            res.set_content(error.dump(2), "application/json");
        });
    }
    
    Server server_;
    asio::io_context io_context_;
    std::shared_ptr<drs_sync::TransferEngine> engine_;
    std::thread io_thread_;
};

int main() {
    try {
        LightweightAPI api;
        api.run("0.0.0.0", 8080);
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
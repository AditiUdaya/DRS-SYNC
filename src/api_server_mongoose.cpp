extern "C" {
#include "mongoose.h"
}
#include "drs_sync/transfer_engine.hpp"
#include <memory>
#include <thread>
#include <sstream>

static std::shared_ptr<drs_sync::TransferEngine> g_engine;

static void handle_ping(struct mg_connection *c, struct mg_http_message *hm) {
    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                  "{\"status\":\"ok\",\"timestamp\":%ld}", time(nullptr));
}

static void handle_create(struct mg_connection *c, struct mg_http_message *hm) {
    // Parse JSON body (simple manual parsing)
    char filepath[256] = {0};
    char destination[64] = {0};
    char priority[16] = "NORMAL";
    
    mg_http_get_var(&hm->body, "filepath", filepath, sizeof(filepath));
    mg_http_get_var(&hm->body, "destination", destination, sizeof(destination));
    mg_http_get_var(&hm->body, "priority", priority, sizeof(priority));
    
    // Parse destination
    std::string dest_str(destination);
    size_t colon = dest_str.find(':');
    std::string host = dest_str.substr(0, colon);
    uint16_t port = std::stoi(dest_str.substr(colon + 1));
    
    drs_sync::Priority prio = drs_sync::Priority::NORMAL;
    if (strcmp(priority, "HIGH") == 0) prio = drs_sync::Priority::HIGH;
    else if (strcmp(priority, "CRITICAL") == 0) prio = drs_sync::Priority::CRITICAL;
    
    asio::ip::udp::endpoint remote(asio::ip::make_address(host), port);
    std::string file_id = g_engine->start_transfer(filepath, remote, prio);
    
    char response[256];
    snprintf(response, sizeof(response),
             "{\"file_id\":\"%s\",\"status\":\"started\"}", file_id.c_str());
    
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response);
}

static void handle_status(struct mg_connection *c, struct mg_http_message *hm,
                         const char* file_id) {
    auto stats = g_engine->get_stats(file_id);
    
    char response[512];
    snprintf(response, sizeof(response),
             "{\"file_id\":\"%s\",\"bytes_sent\":%llu,\"bytes_acked\":%llu,"
             "\"chunks_sent\":%u,\"chunks_acked\":%u,\"throughput_mbps\":%.2f,"
             "\"completed\":%s,\"paused\":%s}",
             file_id,
             stats.bytes_sent.load(),
             stats.bytes_acked.load(),
             stats.chunks_sent.load(),
             stats.chunks_acked.load(),
             stats.throughput_mbps.load(),
             stats.completed.load() ? "true" : "false",
             stats.paused.load() ? "true" : "false");
    
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response);
}

static void event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        
        // CORS headers
        mg_printf(c, "HTTP/1.1 200 OK\r\n"
                     "Access-Control-Allow-Origin: *\r\n"
                     "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
                     "Access-Control-Allow-Headers: Content-Type\r\n");
        
        if (mg_match(hm->uri, mg_str("/ping"), nullptr)) {
            handle_ping(c, hm);
        }
        else if (mg_match(hm->uri, mg_str("/manifest/create"), nullptr)) {
            handle_create(c, hm);
        }
        else if (mg_match(hm->uri, mg_str("/transfer/*/status"), nullptr)) {
            char file_id[64] = {0};
            sscanf(hm->uri.buf, "/transfer/%[^/]/status", file_id);
            handle_status(c, hm, file_id);
        }
        else {
            mg_http_reply(c, 404, "", "Not Found\n");
        }
    }
}

int main() {
    struct mg_mgr mgr;
    
    // Create engine
    asio::io_context io_context;
    g_engine = std::make_shared<drs_sync::TransferEngine>(io_context);
    
    // Start IO thread
    std::thread io_thread([&io_context]() {
        io_context.run();
    });
    
    // Start HTTP server
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8080", event_handler, nullptr);
    
    std::cout << "🚀 DRS-SYNC Mongoose API Server\n";
    std::cout << "📡 Listening on http://0.0.0.0:8080\n";
    
    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }
    
    mg_mgr_free(&mgr);
    io_context.stop();
    io_thread.join();
    
    return 0;
}
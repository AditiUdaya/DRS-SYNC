#include <drogon/drogon.h>

int main() {
    drogon::app()
        .addListener("0.0.0.0", 8080)
        .registerHandler("/", [](const drogon::HttpRequestPtr &req,
                                 std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("Hello from Drogon!");
            callback(resp);
        })
        .run();
}

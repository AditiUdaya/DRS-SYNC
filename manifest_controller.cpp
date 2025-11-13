#include <drogon/HttpController.h>
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>   // <--- added
#include <json/json.h>
#include <fstream>
#include <filesystem>

using namespace drogon;

class ManifestController : public HttpController<ManifestController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ManifestController::createManifest, "/manifest/create", Post);
    ADD_METHOD_TO(ManifestController::getManifest, "/manifest/{1}", Get);
    METHOD_LIST_END

    void createManifest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto json = req->getJsonObject();
        if (!json || !json->isMember("filename"))
        {
            auto resp = HttpResponse::newHttpJsonResponse(
                Json::Value("Missing filename"));
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        std::string filename = (*json)["filename"].asString();

        // --- NEW UUID GENERATION HERE ---
        std::string file_id = drogon::utils::getUuid();

        uint64_t filesize = 0;
        std::filesystem::path p(filename);
        if (std::filesystem::exists(p))
            filesize = std::filesystem::file_size(p);

        uint64_t chunk_size = 65536;
        uint64_t total_chunks =
            filesize == 0 ? 0 : (filesize + chunk_size - 1) / chunk_size;

        Json::Value manifest;
        manifest["file_id"] = file_id;
        manifest["filename"] = filename;
        manifest["filesize"] = (Json::UInt64)filesize;
        manifest["chunk_size"] = (Json::UInt64)chunk_size;
        manifest["total_chunks"] = (Json::UInt64)total_chunks;
        manifest["last_acked_chunk"] = -1;
        manifest["priority"] = "standard";

        std::string path = "manifests/" + file_id + ".json";
        std::ofstream out(path);
        out << manifest.toStyledString();
        out.close();

        auto resp = HttpResponse::newHttpJsonResponse(manifest);
        callback(resp);
    }

    void getManifest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback,
        std::string file_id)
    {
        std::string path = "manifests/" + file_id + ".json";

        if (!std::filesystem::exists(path))
        {
            auto resp = HttpResponse::newHttpJsonResponse(
                Json::Value("Manifest not found"));
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        std::ifstream in(path);
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        in.close();

        Json::Value manifest;
        Json::Reader reader;
        reader.parse(content, manifest);

        auto resp = HttpResponse::newHttpJsonResponse(manifest);
        callback(resp);
    }
};

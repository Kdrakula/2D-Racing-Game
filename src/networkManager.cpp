#include "networkManager.hpp"
#include "gameConstants.hpp"
#include "utils.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <chrono>

NetworkManager::NetworkManager() : isOnline_(false), isPolling_(false) {
    if (const char* env_url = std::getenv("API_URL")) {
        activeUrl_ = env_url;
    } else if (!API_URLS.empty()) {
        activeUrl_ = API_URLS[0];
    }
    
    loadOfflineQueue();
}

NetworkManager::~NetworkManager() {
    stopPolling();
}

void NetworkManager::startPolling() {
    if (isPolling_) return;
    isPolling_ = true;
    pollingThread_ = std::thread([this]() {
        while (isPolling_) {
            checkConnection();
            
            // Wait 5 seconds before checking again
            for (int i = 0; i < 50 && isPolling_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}

void NetworkManager::stopPolling() {
    isPolling_ = false;
    if (pollingThread_.joinable()) {
        pollingThread_.join();
    }
}

void NetworkManager::checkConnection() {
    std::vector<std::string> urlsToTry;
    if (const char* env_url = std::getenv("API_URL")) {
        urlsToTry.push_back(env_url);
    }
    for (const auto& url : API_URLS) {
        if (std::find(urlsToTry.begin(), urlsToTry.end(), url) == urlsToTry.end()) {
            urlsToTry.push_back(url);
        }
    }

    bool found = false;
    for (const auto& url : urlsToTry) {
        httplib::Client cli(url);
        cli.set_connection_timeout(1); // 1 second timeout
        cli.set_read_timeout(1);

        if (auto res = cli.Get("/api/laptimes")) {
            if (res->status == 200) {
                std::lock_guard<std::mutex> lock(mtx_);
                activeUrl_ = url;
                if (!isOnline_) {
                    isOnline_ = true;
                    std::cout << "\n[NETWORK] Back online! Active server: " << activeUrl_ << std::endl;
                    syncOfflineQueue();
                }
                found = true;
                break;
            }
        }
    }

    if (!found) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (isOnline_) {
            isOnline_ = false;
            std::cout << "\n[NETWORK] Offline! Connection lost to all servers." << std::endl;
        }
    }
}

std::string NetworkManager::getActiveUrl() {
    std::lock_guard<std::mutex> lock(mtx_);
    return activeUrl_;
}

bool NetworkManager::isOnline() {
    return isOnline_;
}

std::string NetworkManager::getStatusString() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (isOnline_) {
        if (activeUrl_.find("100.71") != std::string::npos) return "Online (Tailscale)";
        if (activeUrl_.find("beaglebone") != std::string::npos) return "Online (BeagleBone)";
        if (activeUrl_.find("127.0.0.1") != std::string::npos || activeUrl_.find("localhost") != std::string::npos) return "Online (Local)";
        return "Online";
    }
    return "Offline";
}

void NetworkManager::queueOfflineLap(const std::string& player, const std::string& map_id, float time, const std::vector<uint8_t>& ghostData) {
    std::lock_guard<std::mutex> lock(mtx_);
    offlineQueue_.push_back({player, map_id, time, ghostData});
    saveOfflineQueue();
    std::cout << "[NETWORK] Saved lap offline for " << player << " (" << time << "s)" << std::endl;
}

void NetworkManager::syncOfflineQueue() {
    if (offlineQueue_.empty()) return;
    
    std::vector<OfflineLap> pendingLaps;
    {
        pendingLaps = offlineQueue_;
    }

    httplib::Client cli(activeUrl_);
    std::vector<OfflineLap> failedLaps;

    for (const auto& lap : pendingLaps) {
        nlohmann::json j;
        j["player"] = lap.player;
        j["map_id"] = lap.map_id;
        j["time"] = lap.time;
        if (!lap.ghostData.empty()) {
            j["ghost"] = base64_encode(lap.ghostData);
        }

        std::string payload = j.dump();
        if (auto res = cli.Post("/api/laptime", payload, "application/json")) {
            if (res->status == 200) {
                std::cout << "[NETWORK] Synced offline lap: " << lap.time << "s by " << lap.player << std::endl;
            } else {
                failedLaps.push_back(lap);
            }
        } else {
            failedLaps.push_back(lap);
            break; 
        }
    }

    {
        offlineQueue_ = failedLaps;
        saveOfflineQueue();
    }
}

void NetworkManager::loadOfflineQueue() {
    std::ifstream file(OFFLINE_QUEUE_FILE);
    if (!file.is_open()) return;

    try {
        nlohmann::json j;
        file >> j;
        for (const auto& item : j) {
            OfflineLap lap;
            lap.player = item.value("player", "");
            lap.map_id = item.value("map_id", "");
            lap.time = item.value("time", 0.0f);
            if (item.contains("ghost") && !item["ghost"].is_null()) {
                std::string b64 = item["ghost"];
                if (!b64.empty()) lap.ghostData = base64_decode(b64);
            }
            offlineQueue_.push_back(lap);
        }
        if (!offlineQueue_.empty()) {
            std::cout << "[NETWORK] Loaded " << offlineQueue_.size() << " offline laps." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[NETWORK] Failed to load offline queue: " << e.what() << std::endl;
    }
}

void NetworkManager::saveOfflineQueue() {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& lap : offlineQueue_) {
        nlohmann::json item;
        item["player"] = lap.player;
        item["map_id"] = lap.map_id;
        item["time"] = lap.time;
        if (!lap.ghostData.empty()) {
            item["ghost"] = base64_encode(lap.ghostData);
        }
        j.push_back(item);
    }

    std::ofstream file(OFFLINE_QUEUE_FILE);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

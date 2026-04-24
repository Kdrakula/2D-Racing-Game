#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

struct OfflineLap {
    std::string player;
    std::string map_id;
    float time;
    std::vector<uint8_t> ghostData;
};

class NetworkManager {
public:
    static NetworkManager& getInstance() {
        static NetworkManager instance;
        return instance;
    }

    // Delete copy and move constructors
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    void startPolling();
    void stopPolling();

    std::string getActiveUrl();
    bool isOnline();
    std::string getStatusString();

    void queueOfflineLap(const std::string& player, const std::string& map_id, float time, const std::vector<uint8_t>& ghostData);
    void syncOfflineQueue();

private:
    NetworkManager();
    ~NetworkManager();

    void checkConnection();
    void loadOfflineQueue();
    void saveOfflineQueue();

    std::string activeUrl_;
    std::atomic<bool> isOnline_;
    std::atomic<bool> isPolling_;
    
    std::thread pollingThread_;
    std::mutex mtx_;
    
    std::vector<OfflineLap> offlineQueue_;
    const std::string OFFLINE_QUEUE_FILE = "assets/offline_queue.json";
};

#endif // NETWORK_MANAGER_HPP

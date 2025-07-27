#include "../include/config_loader.hpp"
#include "../include/server/decode_utils.hpp"
#include "../include/server/cdr_writer.hpp"
#include "../include/server/http_api.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

class PgwServer {
public:
    explicit PgwServer(const ServerConfig& config);
    ~PgwServer();
    void run();

private:
    struct ClientTask {
        int sockfd;
        sockaddr_in client_addr;
        std::vector<uint8_t> buffer;
    };

    int setup_socket();
    int setup_epoll(int sockfd);
    void start_session_cleaner();
    void start_thread_pool(size_t num_threads);
    void handle_client(int sockfd, const sockaddr_in& client_addr, const std::vector<uint8_t>& buffer);
    void worker_thread();

    ServerConfig config_;
    CdrWriter cdr_writer_;
    BcdDecoder decoder_;
    std::unordered_set<std::string> blacklist_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> session_table_;
    std::mutex session_mutex_;
    std::atomic<bool> running_;
    std::thread cleaner_thread_;
    std::vector<std::thread> thread_pool_;
    std::queue<ClientTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cond_;
    std::atomic<bool> pool_running_;

    friend class PgwServerTest_HandleClient_BlacklistedImsiGetsRejected_Test;
    friend class PgwServerTest_HandleClient_NewImsiCreatesSession_Test;
    friend class PgwServerTest_HandleClient_RepeatedImsiSessionNotDuplicated_Test;
    friend class PgwServerTest_HandleClient_EmptyImsi_Test;
    friend class PgwServerTest_SessionCleanerRemovesExpiredSessions_Test;
    friend class PgwServerTest_SessionCleanerDoesNotRemoveActiveSessions_Test;

#ifdef UNIT_TEST
public:
    void test_handle_client(int sockfd, const sockaddr_in& addr, const std::vector<uint8_t>& buffer) {
        handle_client(sockfd, addr, buffer);
    }

    void test_start_session_cleaner() {
        start_session_cleaner();
    }

    std::unordered_map<std::string, std::chrono::steady_clock::time_point>& test_sessions() {
        return session_table_;
    }

    std::atomic<bool>& test_running() {
        return running_;
    }

    std::thread& test_cleaner_thread() {
        return cleaner_thread_;
    }

    int test_setup_socket() {
        return setup_socket();
    }

    int test_setup_epoll(int sockfd) {
        return setup_epoll(sockfd);
    }

    std::vector<std::thread>& test_thread_pool() {
        return thread_pool_;
    }

#endif
};
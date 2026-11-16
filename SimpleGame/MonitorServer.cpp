#include "stdafx.h"
#include "MonitorServer.h"
#include "Diagnostics.h"
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <atomic>
#include <cctype>
#include <chrono>
#include <iostream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>

namespace
{
    constexpr unsigned short Port = 8765;
    constexpr DWORD SocketTimeoutMs = 250;
    constexpr size_t MaxRequestBytes = 8192;
    const std::string Host = "127.0.0.1:" + std::to_string(Port);
    using Clock = std::chrono::steady_clock;

    void ReportFailure(const std::string& reason)
    {
        std::cerr << "[Monitor] Unavailable: " << reason << '\n';
        Diagnostics::Get().Event("monitor_unavailable " + reason);
    }

    std::string ReadRequest(SOCKET client)
    {
        std::string request;
        char buffer[1024];
        const auto deadline = Clock::now() + std::chrono::seconds(1);
        while (request.size() < MaxRequestBytes && Clock::now() < deadline)
        {
            const int received = recv(client, buffer, static_cast<int>(sizeof(buffer)), 0);
            if (received <= 0)
            {
                break;
            }
            request.append(buffer, received);
            const auto end = request.find("\r\n\r\n");
            if (end != std::string::npos && end + 4 <= MaxRequestBytes)
            {
                return request.substr(0, end + 2);
            }
        }
        return {};
    }

    bool LocalHostHeader(std::istream& headers)
    {
        std::string line;
        bool found = false;
        while (std::getline(headers, line))
        {
            const auto colon = line.find(':');
            if (colon == std::string::npos)
            {
                return false;
            }
            auto name = line.substr(0, colon);
            for (char& c : name)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (name == "host")
            {
                std::istringstream value(line.substr(colon + 1));
                std::string host, extra;
                if (found || !(value >> host) || host != Host || (value >> extra))
                {
                    return false;
                }
                found = true;
            }
        }
        return found;
    }

    bool Send(SOCKET client, const std::string& text, Clock::time_point deadline)
    {
        size_t sent = 0;
        while (sent < text.size() && Clock::now() < deadline)
        {
            const auto remaining = text.size() - sent;
            const int length = static_cast<int>(remaining < 16384 ? remaining : 16384);
            const int size = send(client, text.data() + sent, length, 0);
            if (size <= 0)
            {
                return false;
            }
            sent += size;
        }
        return sent == text.size();
    }

    void Reply(SOCKET client, const char* status, const char* type, const std::string& body)
    {
        const auto header = std::string("HTTP/1.1 ") + status + "\r\nContent-Type: " + type +
                            "\r\nContent-Length: " + std::to_string(body.size()) +
                            "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff"
                            "\r\nConnection: close\r\n\r\n";
        const auto deadline = Clock::now() + std::chrono::seconds(1);
        if (Send(client, header, deadline))
        {
            Send(client, body, deadline);
        }
    }
}

struct MonitorServer::State
{
    SOCKET listener = INVALID_SOCKET;
    bool socketsInitialized = false;
    std::atomic<bool> running{false};
    std::thread worker;
    std::string html;
    std::shared_ptr<const std::string> json;

    ~State()
    {
        running = false;
        if (worker.joinable())
        {
            worker.join();
        }
        if (listener != INVALID_SOCKET)
        {
            closesocket(listener);
        }
        if (socketsInitialized)
        {
            WSACleanup();
        }
    }

    void Serve(SOCKET client) const
    {
        const char* plain = "text/plain; charset=utf-8";
        std::istringstream headers(ReadRequest(client));
        std::string line;
        std::getline(headers, line);
        std::istringstream requestLine(line);
        std::string method, path, version, extra;
        if (!(requestLine >> method >> path >> version) || (requestLine >> extra) ||
            (version != "HTTP/1.1" && version != "HTTP/1.0") || !LocalHostHeader(headers))
        {
            Reply(client, "400 Bad Request", plain, "Invalid local monitor request.");
        }
        else if (method != "GET")
        {
            Reply(client, "405 Method Not Allowed", plain, "Only GET is supported.");
        }
        else if (path == "/")
        {
            Reply(client, "200 OK", "text/html; charset=utf-8", html);
        }
        else if (path == "/api/state")
        {
            const auto snapshot = std::atomic_load(&json);
            if (snapshot)
            {
                Reply(client, "200 OK", "application/json; charset=utf-8", *snapshot);
            }
            else
            {
                Reply(
                    client,
                    "503 Service Unavailable",
                    "application/json; charset=utf-8",
                    "{\"error\":\"snapshot_not_ready\"}");
            }
        }
        else
        {
            Reply(client, "404 Not Found", plain, "Unknown monitor endpoint.");
        }
    }

    void Run()
    {
        while (running)
        {
            fd_set readers;
            FD_ZERO(&readers);
            FD_SET(listener, &readers);
            timeval timeout{0, 150000};
            const int ready = select(0, &readers, nullptr, nullptr, &timeout);
            if (ready == SOCKET_ERROR)
            {
                running = false;
                break;
            }
            if (!ready || !running)
            {
                continue;
            }
            const SOCKET client = accept(listener, nullptr, nullptr);
            if (client == INVALID_SOCKET)
            {
                continue;
            }
            u_long nonBlocking = 0;
            const auto* milliseconds = reinterpret_cast<const char*>(&SocketTimeoutMs);
            if (ioctlsocket(client, FIONBIO, &nonBlocking) == 0 &&
                setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, milliseconds, sizeof(SocketTimeoutMs)) == 0 &&
                setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, milliseconds, sizeof(SocketTimeoutMs)) == 0)
            {
                Serve(client);
            }
            closesocket(client);
        }
    }
};

MonitorServer::MonitorServer() = default;
MonitorServer::~MonitorServer() = default;

void MonitorServer::Start()
{
    if (m_state)
    {
        return;
    }
    const auto directory = Diagnostics::ExecutableDirectory();
    if (directory.empty())
    {
        ReportFailure("executable directory unavailable");
        return;
    }
    const auto path = directory + L"Monitor\\index.html";
    std::ifstream page(path.c_str(), std::ios::binary);
    if (!page)
    {
        ReportFailure("Monitor/index.html missing beside executable");
        return;
    }
    auto state = std::make_unique<State>();
    state->html.assign(std::istreambuf_iterator<char>(page), std::istreambuf_iterator<char>());
    if (state->html.empty())
    {
        ReportFailure("Monitor/index.html is empty");
        return;
    }

    WSADATA data{};
    const int startup = WSAStartup(MAKEWORD(2, 2), &data);
    if (startup != 0)
    {
        ReportFailure("Winsock startup error=" + std::to_string(startup));
        return;
    }
    state->socketsInitialized = true;
    state->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(Port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    const BOOL exclusive = TRUE;
    u_long nonBlocking = 1;
    if (state->listener == INVALID_SOCKET ||
        setsockopt(
            state->listener,
            SOL_SOCKET,
            SO_EXCLUSIVEADDRUSE,
            reinterpret_cast<const char*>(&exclusive),
            sizeof(exclusive)) == SOCKET_ERROR ||
        bind(state->listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
        listen(state->listener, SOMAXCONN) == SOCKET_ERROR ||
        ioctlsocket(state->listener, FIONBIO, &nonBlocking) == SOCKET_ERROR)
    {
        ReportFailure("listen " + Host + " error=" + std::to_string(WSAGetLastError()));
        return;
    }
    state->running = true;
    try
    {
        state->worker = std::thread(&State::Run, state.get());
    }
    catch (const std::system_error& error)
    {
        ReportFailure(std::string("worker startup error=") + error.what());
        return;
    }
    m_state = std::move(state);
    std::cout << "[Monitor] http://" << Host << "/\n";
    Diagnostics::Get().Event("monitor_listen host=" + Host + " schema=1 interval_ms=1000");
}

void MonitorServer::Stop()
{
    m_state.reset();
}

bool MonitorServer::Running() const
{
    return m_state && m_state->running;
}

void MonitorServer::Publish(std::string json)
{
    if (m_state)
    {
        std::atomic_store(&m_state->json, std::make_shared<const std::string>(std::move(json)));
    }
}

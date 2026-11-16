#pragma once
#include <memory>
#include <string>

class MonitorServer
{
public:
    MonitorServer();
    ~MonitorServer();

    void Start();
    void Stop();
    bool Running() const;
    void Publish(std::string json);

private:
    struct State;
    std::unique_ptr<State> m_state;
};

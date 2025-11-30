#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>
#include <stdint.h>

class Timer
{
public:
    explicit Timer(uint64_t durationSeconds);

    void Start(uint64_t currentTimeMs);
    uint64_t Update(uint64_t currentTimeMs);
    void Stop(uint64_t currentTimeMs);
    bool HasStarted() const;
    bool IsRunning() const;
    bool IsFinished() const;

private:
    uint64_t durationMs;
    uint64_t accumulatedMs;
    uint64_t totalElapsedMs;
    uint64_t startTime;
    bool isRunning;
};

String formatTime(uint64_t ms);

#endif // TIMER_H
#include <Timer.h>

Timer::Timer(uint64_t durationSeconds)
    : durationMs(durationSeconds * 1000), isRunning(false),
      accumulatedMs(0), totalElapsedMs(0), startTime(0) {}

void Timer::Start(uint64_t currentTimeMs)
{
    if (!isRunning)
    {
        startTime = currentTimeMs;
        isRunning = true;
    }
}

uint64_t Timer::Update(uint64_t currentTimeMs)
{
    if (isRunning)
    {
        uint64_t sessionElapsed = currentTimeMs - startTime;
        totalElapsedMs = accumulatedMs + sessionElapsed;
    }
    int32_t remaining = durationMs - totalElapsedMs;
    return remaining > 0 ? remaining : 0;
}

void Timer::Stop(uint64_t currentTimeMs)
{
    if (isRunning)
    {
        uint64_t sessionElapsed = currentTimeMs - startTime;
        accumulatedMs += sessionElapsed;
        isRunning = false;
    }
}

bool Timer::HasStarted() const
{
    return isRunning || accumulatedMs > 0;
}

bool Timer::IsRunning() const
{
    return isRunning;
}

bool Timer::IsFinished() const
{
    return totalElapsedMs >= durationMs;
}

String formatTime(uint64_t ms) {
    if (ms >= 60000) {
        // Format as mm:ss
        uint16_t minutes = ms / 60000;
        uint16_t seconds = (ms % 60000) / 1000;
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%02u:%02u", minutes, seconds);
        return String(buffer);
    } else {
        // Format as ss.cc
        uint16_t seconds = ms / 1000;
        uint16_t centiseconds = (ms % 1000) / 10;
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%02u.%02u", seconds, centiseconds);
        return String(buffer);
    }
}

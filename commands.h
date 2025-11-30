#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

// Define command_t as alias for uint16_t
typedef uint16_t command_t;

// Command constants
static constexpr command_t COMMAND_NONE          = 0;
static constexpr command_t COMMAND_OPERATE_TIMER = 1;
static constexpr command_t COMMAND_START_TIMER   = 2;
static constexpr command_t COMMAND_STOP_TIMER    = 3;
static constexpr command_t COMMAND_RESET_TIMER   = 4;
static constexpr command_t COMMAND_HOME_SCORE    = 5;
static constexpr command_t COMMAND_AWAY_SCORE    = 6;
static constexpr command_t COMMAND_HOME_ADJUST   = 7;
static constexpr command_t COMMAND_AWAY_ADJUST   = 8;

#endif // COMMANDS_H
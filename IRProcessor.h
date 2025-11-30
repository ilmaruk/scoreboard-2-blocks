//
// Keep everything in an header file, to avoid problems with IRremote duplicate definitions
//
#ifndef IRPROCESSOR_H
#define IRPROCESSOR_H

#include <Arduino.h>
#include <IRremote.hpp>
#include <map>

#include "commands.h"

class IRProcessor {
public:
    using CommandMap = std::map<uint32_t, command_t>;

    IRProcessor(IRrecv& receiver, CommandMap commandMap)
        : receiver(receiver), commandMap(std::move(commandMap)) {}

    command_t Process() {
        command_t cmd = COMMAND_NONE;
        if (receiver.decode()) {
            if (!(receiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
                auto it = commandMap.find(receiver.decodedIRData.command);
                if (it != commandMap.end()) {
                    cmd = it->second;
                }
            }
            receiver.resume();
        }
        return cmd;
    }

private:
    IRrecv& receiver;
    CommandMap commandMap;
};

#endif // IRPROCESSOR_H
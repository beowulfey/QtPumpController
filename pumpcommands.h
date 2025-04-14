#ifndef PUMPCOMMANDS_H
#define PUMPCOMMANDS_H

enum class PumpCommand {
    Start,
    Stop,
    GetVersion,
    SetPhase,
    // These are prepended with FUN to represent phase functions
    SetFlowRate,
    SetFlowDirection,
    SetLinearRamp,
    SetRampTime,

};

#endif // PUMPCOMMANDS_H

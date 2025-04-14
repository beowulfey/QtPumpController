#ifndef PUMPCOMMANDS_H
#define PUMPCOMMANDS_H

#include <QString>

struct PumpPhase {
    int phaseNumber = 0;      // The phase number sent via PHN command
    QString function;         // "RAT", "LIN", "STP"
    double rate = 0.0;        // Flow rate in µL/min
    double volume;           // Optional — only used for "RAT"
    QString time;             // Optional — only used for "LIN"
    QString direction = "INF"; // "INF" or "WDR", default is "INF"
};

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

#ifndef PUMPCOMMANDS_H
#define PUMPCOMMANDS_H

#include <QString>

struct PumpPhase {
    int phaseNumber = 1;            // The phase number sent via PHN command
    QString function;               // "RAT", "LIN", "STP"
    double rate = 0.0;              // Flow rate in µL/min
    double volume;                  // Optional — only used for "RAT"
    QString time;                   // Optional — only used for "LIN". Hours:Minutes or Seconds:Tenths depending on phase number (n, n+1)
    QString direction = "INF";      // "INF" or "WDR", default is "INF"
};

enum class PumpCommand {
    Start,
    Stop,
    GetVersion,
    RateFunction,
    RampFunction,
    PauseFunction,
    StopFunction,
    SetPhase,
    SetFlowRate,
    SetVolume,
    SetFlowDirection,
    SetRampTime,
    SetVolUnits,
    SetPause

};

#endif // PUMPCOMMANDS_H

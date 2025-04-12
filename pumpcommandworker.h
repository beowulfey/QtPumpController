#ifndef PUMPCOMMANDWORKER_H
#define PUMPCOMMANDWORKER_H

// pumpcommandworker.h
#pragma once

#include <QObject>
#include <QQueue>
#include "pumpinterface.h"  // Forward declaration or include depending on structure

struct PumpCommand {
    int address;
    BasicCommand cmd;
    double value;
};

class PumpCommandWorker : public QObject {
    Q_OBJECT

public:
    explicit PumpCommandWorker(PumpInterface* interface, QObject* parent = nullptr);

public slots:
    void enqueueCommand(const PumpCommand& command);

private slots:
    void onResponseReceived(const QString& response);

private:
    void processNext();

    QQueue<PumpCommand> commandQueue;
    PumpInterface* pumpInterface;
    bool processing = false;
};

#endif // PUMPCOMMANDWORKER_H

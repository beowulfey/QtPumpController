#ifndef PUMPCOMMANDWORKER_H
#define PUMPCOMMANDWORKER_H

// pumpcommandworker.h
#pragma once

#include <QObject>
#include <QQueue>

// Forward declaration to avoid circular include
class PumpInterface;

#include "pumpcommands.h"  // Forward declaration or include depending on structure

struct PumpCommand {
    QString name;
    BasicCommand cmd;
    double value;
};

// Queues the commands used by PumpInterface for sending to pump.
// Needs testing to see if every command actually sends a response.


class PumpCommandWorker : public QObject {
    Q_OBJECT

public:
    explicit PumpCommandWorker(PumpInterface* interface, QObject* parent = nullptr);

signals:
    void pumpCommandReady(const QString& name, BasicCommand cmd, double value);


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

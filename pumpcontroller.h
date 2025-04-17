#ifndef PUMPCONTROLLER_H
#define PUMPCONTROLLER_H

#include <QMainWindow>
#include <QList>
#include <QTime>
#include "tablemodel.h"
#include "protocol.h"
#include "pumpcommands.h"
#include "pumpinterface.h"
#include "condinterface.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class PumpController;
}
QT_END_NAMESPACE



class PumpController : public QMainWindow
{
    Q_OBJECT

public:
    PumpController(QWidget *parent = nullptr);
    ~PumpController();



public slots:
    void writeToConsole(const QString& text, const QColor& color = QColor());
    void saveConsole();
    void clearConsole();

    void openCOMsDialog();
    void setCOMs(const QString& cond, const QString& pump);
    void confirmSettings();
    void settingsChanged();

    void addSegment();
    void rmSegment();
    void clearSegments();

    void updateProtocol(); // updates plot upon changes to TableModel

    // User button slots
    void startProtocol();
    void sendProtocol();
    void stopProtocol();

    void startPumps();
    void updatePumps();
    void stopPumps();

    void initiatePumps();
    void initiateCond();
    void receivePumpError(const QString& err);
    void receivePumpResponse(const QString& msg);
    void receiveCondMeasurement(CondReading reading);

    void timerTick();




    //void timerTick();


    //void resetPumps();
    //void updatePumps();
    //void startPump();
    //void stopPump();
    //void setCondMin();
    //void setCondMax();
    //void resetCond();
    //void receiveReading();
    //void condTimerTick();
    //void beginRecordCond();
    //void stopRecordCond();

    //void saveCond();


    //void stopProtocol();
    //void addSegment();
    //void rmSegment();
    //void clearSegments();
    //void updateProtocol();
    //void updatePumpProgram();


private:
    QTime startTime;
    Ui::PumpController *ui;
    //PumpCommandWorker *commandWorker;
    QString pumpComPort;
    QString condComPort;
    //float offset;
    TableModel *tableModel;
    QTimer *runTimer;
    QTimer *intervalTimer;
    int xPos;
    //QTimer *condTimer;
    Protocol *currProtocol;
    bool protocolChanged;
    PumpInterface *pumpInterface = nullptr;
    CondInterface *condInterface = nullptr;
    QVector<double>condReadings;
    QVector<double> condPreReadings;
    const int condPreSaveWindow = 120;


    QVector<QVector<PumpPhase>> generatePumpPhases(int startPhase, const QVector<QVector<double>>& segments) ;
    QVector<double> calculateFlowRates(double concentration) const;
};
#endif // PUMPCONTROLLER_H

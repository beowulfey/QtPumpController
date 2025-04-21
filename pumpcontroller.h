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
    void setCondFile();

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

    void resetCondPlot(); //currently unused



private:
    QTime startTime;
    QString experimentDirectory;
    QMap<QTime, QPair<QVector<double>, QVector<double>>> savedRuns;
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
    int condPreSaveWindow = 60;

    void updateCondPlot(); // called upon getting a new measurement
    void saveCurrentRun(); // stores conductivity plot to a QMap
    QVector<QVector<PumpPhase>> generatePumpPhases(int startPhase, const QVector<QVector<double>>& segments) ;
    QVector<double> calculateFlowRates(double concentration) const;
    QVector<double> generateRangeScaled(double start, double end, double step = 0.5);
};
#endif // PUMPCONTROLLER_H

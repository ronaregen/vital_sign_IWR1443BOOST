#ifndef VITALSIGN_H
#define VITALSIGN_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class VitalSign; }
QT_END_NAMESPACE

class VitalSign : public QMainWindow
{
    Q_OBJECT

public:
    VitalSign(QWidget *parent = nullptr);
    ~VitalSign();


private slots:
    void connectSerial();
    void disconnectSerial();

    void startSensing();
    void stopSensing();

    void getListSerial();

    void onSerialUARTReceived();
    void onSerialDATAReceived();

    bool checkFirst8Bytes(const QByteArray &data);

    void displayVitalSign(const QByteArray &data);


private:
    Ui::VitalSign *ui;
    QSerialPort serialUART, serialData;
};
#endif // VITALSIGN_H

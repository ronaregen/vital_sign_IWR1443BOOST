#include "vitalsign.h"
#include "ui_vitalsign.h"
#include <iostream>
#include <algorithm>
#include <cmath>


#include <QSerialPortInfo>

VitalSign::VitalSign(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VitalSign)
{
    ui->setupUi(this);

    // connect serial slot
    connect(&serialUART,SIGNAL(readyRead()),this,SLOT(onSerialUARTReceived()));
    connect(&serialData,SIGNAL(readyRead()),this,SLOT(onSerialDATAReceived()));

    // connect button to slot
    connect(ui->btnConnect, SIGNAL(clicked()), this, SLOT(connectSerial()));
    connect(ui->btnDisconnect, SIGNAL(clicked()), this, SLOT(disconnectSerial()));
    connect(ui->btnStart, SIGNAL(clicked()),this,SLOT(startSensing()));
    connect(ui->btnStop, SIGNAL(clicked()),this,SLOT(stopSensing()));
    connect(ui->btnReload,SIGNAL(clicked()),this,SLOT(getListSerial()));

    getListSerial();



}

VitalSign::~VitalSign()
{
    delete ui;

}

QStringList configCommand ={
    "flushCfg",
    "dfeDataOutputMode 1",
    "channelCfg 15 3 0",
    "adcCfg 2 1",
    "adcbufCfg 0 1 0 1",
    "profileCfg 0 77 7 6 57 0 0 70 1 200 4000 0 0 48",
    "chirpCfg 0 0 0 0 0 0 0 1",
    "frameCfg 0 0 2 0 50 1 0",
    "guiMonitor 0 0 0 0 1",
    "vitalSignsCfg 0.3 1.0 256 512 4 0.1 0.05 100000 100000",
    "motionDetection 1 20 3.0 0",
    "sensorStart",
};

bool startSensingBool = false;
bool stopSensingBool = false;
bool onConfigBool = false;
static int configIndex = 0;
uint32_t dataLen;

QByteArray bufferUART ={};
QByteArray bufferData = {};

//variable global for heart rate calculation
float heartrateDisplay[40]= {48};
float outSumEnergyBreathWfm_thresh = 3;

float rangeBinValueUpdated = 0;
float rangeBinValuePrev = 0;
float rangeBinValueThresh = 250;
float outSumEnergyHeartWfm_thresh =  0.05;
float thresh_HeartCM = 0.4;
float thresh_diffEst = 15;
float diffEst_heartRate = 0;
float outHeartNew_CM=0;
float outHeartPrev_CM=0;
float outBreathPrev_CM=0;
float outBreathNew_CM = 0;


void VitalSign::connectSerial()
{
    // setting properties serial for UART
    serialUART.setPortName(ui->cmbUartPort->currentText());
    serialUART.setBaudRate(ui->cmbUartBaudrate->currentText().toInt());

    // setting properties serial for DATA
    serialData.setPortName(ui->cmbDataPort->currentText());
    serialData.setBaudRate(ui->cmbDataBaudrate->currentText().toInt());

    // try connect serial
    if(serialUART.open(QIODevice::ReadWrite) && serialData.open(QIODevice::ReadWrite)){
        // disable combobox to prevent change during connection
        ui->cmbDataBaudrate->setEnabled(false);
        ui->cmbDataPort->setEnabled(false);
        ui->cmbUartBaudrate->setEnabled(false);
        ui->cmbUartPort->setEnabled(false);

        // disable connect button to prevent double click
        ui->btnConnect->setEnabled(false);

        // enable next action
        ui->btnDisconnect->setEnabled(true);
        ui->btnStart->setEnabled(true);

        // disable stop, because start aja belum
        ui->btnStop->setEnabled(false);
        ui->btnReload->setEnabled(false);

        // change status text
        ui->lblStatus->setText("connected");
    }else{
        serialUART.close();
        serialData.close();
        QMessageBox::critical(this, "Vital Sign", "open connection failed");
    }

}

void VitalSign::disconnectSerial()
{
    // close serial connection
    serialUART.close();
    serialData.close();

    // enable button connect
    ui->btnConnect->setEnabled(true);

    // enable kembali combobox
    ui->cmbDataBaudrate->setEnabled(true);
    ui->cmbDataPort->setEnabled(true);
    ui->cmbUartBaudrate->setEnabled(true);
    ui->cmbUartPort->setEnabled(true);

    // disable action button
    ui->btnDisconnect->setEnabled(false);
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(false);

    ui->btnReload->setEnabled(true);

    // change status text
    ui->lblStatus->setText("disconnected");
}

void VitalSign::startSensing()
{
    onConfigBool = true;
    startSensingBool = true;
    QString command = "sensorStop\n";
    serialUART.write(command.toUtf8());

    // set button enable
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);

    // change status text
    ui->lblStatus->setText("sensor start");
}

void VitalSign::stopSensing()
{
    QString stopCommand = "sensorStop\n";
    serialUART.write(stopCommand.toUtf8());
    // set button enable
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);

    // change status text
    ui->lblStatus->setText("sensor stop");
}

void VitalSign::getListSerial()
{
    // clear combobox before fill
    ui->cmbDataBaudrate->clear();
    ui->cmbDataPort->clear();
    ui->cmbUartBaudrate->clear();
    ui->cmbUartPort->clear();

    ui->lblBreathingRate->setText("");
    ui->lblHeartRate->setText("");

    if(!QSerialPortInfo::availablePorts().isEmpty()){
        // if device found
        // enable combobox
        ui->cmbDataBaudrate->setEnabled(true);
        ui->cmbDataPort->setEnabled(true);
        ui->cmbUartBaudrate->setEnabled(true);
        ui->cmbUartPort->setEnabled(true);

        // fill list serial
        Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
            ui->cmbUartPort->addItem(port.portName());
            ui->cmbDataPort->addItem(port.portName());
        }

        // fill baudrate
        ui->cmbUartBaudrate->addItem("115200");
        ui->cmbDataBaudrate->addItem("921600");

        ui->lblStatus->setText("mmWave sensor found");

        // enable button connect and disable all
        ui->btnConnect->setEnabled(true);
        ui->btnDisconnect->setEnabled(false);
        ui->btnStart->setEnabled(false);
        ui->btnStop->setEnabled(false);


    }else{
        // if no device found
        // disable combobox
        ui->cmbDataBaudrate->setEnabled(false);
        ui->cmbDataPort->setEnabled(false);
        ui->cmbUartBaudrate->setEnabled(false);
        ui->cmbUartPort->setEnabled(false);

        // disable all button
        ui->btnConnect->setEnabled(false);
        ui->btnDisconnect->setEnabled(false);
        ui->btnStart->setEnabled(false);
        ui->btnStop->setEnabled(false);

        ui->lblStatus->setText("no mmWave sensor found");
    }


}


// function handling serial
void VitalSign::onSerialUARTReceived()
{
    bufferUART += serialUART.readAll();
    QString textData = QString::fromUtf8(bufferUART);
    if(textData.right(1).at(0) == ">" ){
        ui->textEdit->append(textData);
        bufferUART.clear();
        if(startSensingBool && configIndex < configCommand.length()){
            QString cmd =configCommand[configIndex];
            cmd += "\n";
            serialUART.write(cmd.toUtf8());
            ui->lblStatus->setText(configCommand[configIndex]);
            configIndex++;
        }else{
            startSensingBool=false;
            configIndex = 0;
        }



    }

}

void VitalSign::onSerialDATAReceived()
{
    QByteArray dataNew = serialData.readAll();

    if(checkFirst8Bytes(dataNew)) // mengecek apakah permulaan data?
    {
        bufferData = dataNew; // masukan dataNew sebagai permulaan buffer
        dataLen = *reinterpret_cast<const uint32_t*>(dataNew.constData() + 12); // membaca panjang frame data (index 12)
    }else{
        bufferData += dataNew; // masukan dataNew sebagai lanjutan dari buffer
    }


    if(static_cast<uint32_t>(bufferData.length()) >= dataLen) // mengecek jika buffer telah memenuhi ketentuan panjang data
    {
        displayVitalSign(bufferData);
    }
}

bool VitalSign::checkFirst8Bytes(const QByteArray &data)
{
    if (data.size() >= 8)
    {
        // Mengecek apakah 8 byte pertama sesuai dengan ketentuan
        return (data.at(0) == 0x02 && data.at(1) == 0x01 &&
                data.at(2) == 0x04 && data.at(3) == 0x03 &&
                data.at(4) == 0x06 && data.at(5) == 0x05 &&
                data.at(6) == 0x08 && data.at(7) == 0x07);
    }
    return false;
}

void VitalSign::displayVitalSign(const QByteArray &data)
{
    // pengolahan heart rate
    QByteArray bheartRateEstPeak = data.mid(96,4);
    QByteArray bheartRateEstFFT = data.mid(76,4);
    QByteArray boutConfidenceMetric_Heart = data.mid(104,4);

    // data breath rate
    QByteArray bbreathRateEstFFT = data.mid(92,4);
    QByteArray boutConfidenceMetric_Breath = data.mid(100,4);
    QByteArray boutSumEnergyBreathWfm = data.mid(112,4);

    QByteArray brangeBinValue = data.mid(52,4);
//    QByteArray outSumEnergyHeartWfm = data.mid(0,4);


    float heartRateFinal=0;

    float heartRateEstPeak;
    float heartRateEstFFT;
    float outConfidenceMetric_Heart;

    float breathRateEstFFT;
    float outConfidenceMetric_Breath;
    float outSumEnergyBreathWfm;

    float rangeBinValue;

    float alpha = 0.5;
//    float outSumEnergyHeartWfm=0;

    memcpy(&heartRateEstPeak,bheartRateEstPeak.constData(),sizeof(float));
    memcpy(&heartRateEstFFT,bheartRateEstFFT.constData(),sizeof(float));
    memcpy(&outConfidenceMetric_Heart,boutConfidenceMetric_Heart.constData(),sizeof(float));

    memcpy(&outConfidenceMetric_Breath,boutConfidenceMetric_Breath.constData(),sizeof(float));
    memcpy(&breathRateEstFFT,bbreathRateEstFFT.constData(),sizeof(float));
    memcpy(&outSumEnergyBreathWfm,boutSumEnergyBreathWfm.constData(),sizeof(float));
    memcpy(&rangeBinValue,brangeBinValue.constData(),sizeof(float));

    outBreathPrev_CM = outBreathNew_CM;
    outBreathNew_CM = alpha*(outConfidenceMetric_Breath) + (1-alpha)*outBreathPrev_CM;

    outHeartPrev_CM = outHeartNew_CM;    
    outHeartNew_CM = alpha*(outConfidenceMetric_Heart) + (1-alpha)*outHeartPrev_CM;

//    rangeBinValuePrev = rangeBinValueUpdated;
//    alpha = 0.1;
//    rangeBinValueUpdated = alpha*(rangeBinValue) + (1-alpha)*rangeBinValuePrev;


    diffEst_heartRate = abs(heartRateEstFFT - heartRateEstPeak);
    if((outHeartNew_CM > thresh_HeartCM) || (diffEst_heartRate < thresh_diffEst)){
        heartRateFinal = heartRateEstFFT;
    }else{
        heartRateFinal = heartRateEstPeak;
    }

    for (int i = 0; i < 39; i++) {
        heartrateDisplay[i] = heartrateDisplay[i + 1];
    }
    heartrateDisplay[39] = heartRateFinal;

    // sorting heartrateDisplay untuk kemudian dilakukan median;
    std::sort(heartrateDisplay,heartrateDisplay+40);

    float median = (heartrateDisplay[19]+heartrateDisplay[20])/2;
    int intmedian = std::round(median);

    int breathRate = std::round(breathRateEstFFT);
    ui->lblHeartRate->setText(QString::number(intmedian));

    if(std::isnan(outSumEnergyBreathWfm)|| std::isinf(outSumEnergyBreathWfm))
    {
        outSumEnergyBreathWfm = 99;
    }

//    if((rangeBinValueUpdated < rangeBinValueThresh) ||(outSumEnergyBreathWfm < outSumEnergyBreathWfm_thresh)){
//        ui->lblBreathingRate->setText(QString::number(0));
////        ui->lblBreathingRate->setForegroundRole();
//    }else{
//        ui->lblBreathingRate->setText(QString::number(breathRate));
//    }
    ui->lblBreathingRate->setText(QString::number(breathRate));
    // circular shift heart rate final

//    memcpy(&fheartRateEst_FFT,heartRateEst_FFT.constData(),sizeof(float));


}

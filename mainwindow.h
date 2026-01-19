#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "modbusmaster.h"
#include "modbusslave.h"
#include "modbusregister.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum class ModbusMode{
        MASTER,
        SLAVE
    };

    enum class ConnectMode{
        TCP,
        RTU
    };

private slots:
    void on_rbtnModeMaster_clicked();

    void on_rbtnModeSlave_clicked();

    void on_btnTcp_clicked();

    void on_cbxCom_textActivated(const QString &arg1);

    void on_btnOpenCom_clicked();

    void on_btnConfirmConfig_clicked();

private:
    Ui::MainWindow *ui;
    ModbusMode mModbusMode = ModbusMode::MASTER;
    ConnectMode mConncetMode = ConnectMode::TCP;

    bool mListening = false;    // tcp 监听状态
    bool mConnecting = false;  // tcp / rtu 连接状态

    std::shared_ptr<ModbusMaster> mMaster;
    std::shared_ptr<ModbusSlave> mSlave;

    ModbusRegister mRegisterWin;

    int mSlaveId = 0;
    int mSlaveAddr = 0;
    int mSlaveRegisterCnt = 0;
    int mFuncode = 3;

    QTimer mFlushTimer;

private:
    void setMode(ModbusMode mode);
    void setConnectMode(ConnectMode mode);
    void flushComList();
    void setSlaveConfig();

    void writeRegister(int addr, int value);
    void flushDataTable();

    void setRegisterWinBtn();
};
#endif // MAINWINDOW_H

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

const char BTN_BACK_COLOR[] = "background-color: rgb(0, 120, 212);";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    flushComList();
    mRegisterWin.setParent(ui->grpRegister);
    connect(&mRegisterWin, &ModbusRegister::modifyValue, this, &MainWindow::writeRegister);
    mFlushTimer.start(50);
    connect(&mFlushTimer, &QTimer::timeout, this, &MainWindow::flushDataTable);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setMode(ModbusMode mode){
    mModbusMode = mode;
}


void MainWindow::on_rbtnModeMaster_clicked()
{
    setMode(ModbusMode::MASTER);
    ui->btnTcp->setText("连接");
}


void MainWindow::on_rbtnModeSlave_clicked()
{
    setMode(ModbusMode::SLAVE);
    ui->btnTcp->setText("监听");
}

void MainWindow::setConnectMode(ConnectMode mode){
    mConncetMode = mode;
    bool connected = mConnecting || mListening;
    if(connected){
        switch(mConncetMode){
        case ConnectMode::TCP:
            ui->tabRtu->setEnabled(false);
            break;
        case ConnectMode::RTU:
            ui->tabTcp->setEnabled(false);
            break;
        }
    }else{
        ui->tabTcp->setEnabled(true);
        ui->tabRtu->setEnabled(true);
    }

    QWidget *tcp[] = {ui->txtIp, ui->txtPort};
    QWidget *rtu[] = {ui->cbxCom, ui->cbxData, ui->cbxParity, ui->cbxStop, ui->txtBaud};

    for(auto ele:tcp){
        ele->setEnabled(!mListening);
    }

    for(auto ele:rtu){
        ele->setEnabled(!mConnecting);
    }

}


void MainWindow::on_btnTcp_clicked()
{
    if(mListening){
        // 已经连接
        mListening = false;
        if(mModbusMode == ModbusMode::MASTER){
            mMaster->close();
        }else{
            mSlave->close();
        }
        setConnectMode(ConnectMode::RTU);
        if(mModbusMode == ModbusMode::MASTER){
            ui->btnTcp->setText("连接");
        }else{
            ui->btnTcp->setText("监听");
        }
        ui->btnTcp->setStyleSheet("");
        ui->btnConfirmConfig->click();
        return;
    }
    // 未连接
    setSlaveConfig();
    if(mModbusMode == ModbusMode::MASTER){
        // 主站模式
        auto master = std::make_shared<ModbusMasterTcp>();
        master->setTarget(ui->txtIp->text().toStdString(), ui->txtPort->text().toUInt());
        master->setSlave(ui->txtSlaveId->text().toInt());
        if(master->open()){
            ui->btnConfirmConfig->click();
            mMaster = master;
            ui->btnTcp->setText("断开");
            ui->btnTcp->setStyleSheet(BTN_BACK_COLOR);
            mListening = true;
            setConnectMode(ConnectMode::TCP);
            setRegisterWinBtn();
        }
    }else{
        // 从站模式
        auto slave = std::make_shared<ModbusSlaveTCP>();
        slave->setLocalPort(ui->txtIp->text().toStdString(), ui->txtPort->text().toUInt());
        ModbusSlave::RegisterInfo registerInfo{0, 0, 0, 0};
        if(mFuncode == 3){
            registerInfo.holdRegister.addr = mSlaveAddr;
            registerInfo.holdRegister.size = mSlaveRegisterCnt;
        }else{
            registerInfo.inputRegister.addr = mSlaveAddr;
            registerInfo.inputRegister.size = mSlaveRegisterCnt;
        }
        slave->createRegisterMapping(registerInfo);
        if(slave->open()){
            ui->btnConfirmConfig->click();
            mSlave = slave;
            ui->btnTcp->setText("关闭");
            ui->btnTcp->setStyleSheet(BTN_BACK_COLOR);
            mListening = true;
            setConnectMode(ConnectMode::TCP);
            setRegisterWinBtn();
        }
    }
}

void MainWindow::flushComList(){
    const auto &coms = QSerialPortInfo::availablePorts();
    QStringList list;
    for(const auto &com : coms){
        list.append(com.portName());
    }
    list.sort();
    list.append("刷新");
    ui->cbxCom->clear();
    ui->cbxCom->addItems(list);
}


void MainWindow::on_cbxCom_textActivated(const QString &arg1)
{
    if(arg1 == "刷新"){
        flushComList();
    }
}


void MainWindow::on_btnOpenCom_clicked()
{
    if(mConnecting){
        mConnecting = false;
        if(mModbusMode == ModbusMode::MASTER){
            mMaster->close();
        }else{
            mSlave->close();
        }
        ui->btnOpenCom->setText("打开串口");
        ui->btnOpenCom->setStyleSheet("");
        setConnectMode(ConnectMode::RTU);
        ui->btnConfirmConfig->click();
        return;
    }

    if(mModbusMode == ModbusMode::MASTER){
        auto master = std::make_shared<ModbusMasterRtu>();
        master->setTarget(ui->cbxCom->currentText().toStdString(),
                         ui->txtBaud->text().toUInt(),
                         ui->cbxParity->currentText().toStdString().at(0),
                         ui->cbxData->currentText().toUInt(),
                         ui->cbxStop->currentText().toInt()
                         );
        master->setSlave(ui->txtSlaveId->text().toInt());
        if(master->open()){
            ui->btnConfirmConfig->click();
            mMaster = master;
            ui->btnOpenCom->setText("关闭串口");
            ui->btnOpenCom->setStyleSheet(BTN_BACK_COLOR);
            mConnecting = true;
            setConnectMode(ConnectMode::RTU);
            setRegisterWinBtn();
        }
    }else{
        auto slave = std::make_shared<ModbusSlaveRTU>();
        slave->setTarget(ui->cbxCom->currentText().toStdString(), ui->txtBaud->text().toUInt(),
                        ui->cbxParity->currentText().toStdString().at(0),
                        ui->cbxData->currentText().toUInt(), ui->cbxStop->currentText().toUInt());
        slave->setSlave(ui->txtSlaveId->text().toUInt());
        setSlaveConfig();
        ModbusSlave::RegisterInfo registerInfo{0,0,0,0};
        if(mFuncode == 3){
            registerInfo.holdRegister.addr = mSlaveAddr;
            registerInfo.holdRegister.size = mSlaveRegisterCnt;
        }else{
            registerInfo.inputRegister.addr = mSlaveAddr;
            registerInfo.inputRegister.size = mSlaveRegisterCnt;
        }
        slave->createRegisterMapping(registerInfo);
        if(slave->open()){
            ui->btnConfirmConfig->click();
            mSlave = slave;
            ui->btnOpenCom->setText("关闭串口");
            ui->btnOpenCom->setStyleSheet(BTN_BACK_COLOR);
            mConnecting = true;
            setConnectMode(ConnectMode::RTU);
            setRegisterWinBtn();
        }
    }
}

void MainWindow::setSlaveConfig(){
    mSlaveId = ui->txtSlaveId->text().toInt();
    mSlaveAddr = ui->txtAddr->text().toInt();

    mSlaveRegisterCnt = ui->txtRegisterCnt->text().toInt();
    if(mSlaveRegisterCnt > 100){
        mSlaveRegisterCnt = 100;
        ui->txtRegisterCnt->setText("100");
    }
    if(mSlaveRegisterCnt < 0){
        mSlaveRegisterCnt = 0;
        ui->txtRegisterCnt->setText("0");
    }
    mFuncode = ui->cbxFuncode->currentIndex() + 3;
}

void MainWindow::on_btnConfirmConfig_clicked()
{
    QWidget *configs[] = {ui->txtSlaveId, ui->cbxFuncode, ui->txtAddr, ui->txtRegisterCnt};
    if(ui->btnConfirmConfig->isChecked()){
        setSlaveConfig();
        mRegisterWin.setAddrAndCount(mSlaveAddr, mSlaveRegisterCnt);
    }
    for(auto conf : configs){
        conf->setEnabled(!ui->btnConfirmConfig->isChecked());
    }
}

void MainWindow::writeRegister(int addr, int value){
    uint16_t val = value;
    if(addr < mSlaveAddr && addr >= mSlaveAddr + mSlaveRegisterCnt){
        return;
    }
    if(mModbusMode == ModbusMode::MASTER){
        if(mMaster){
            mMaster->writeRegister(addr, {val});
        }
    }else{
        if(mSlave){
            switch (mFuncode) {
            case 3:
                mSlave->writeHoldRegister(addr, {val});
                break;
            case 4:
                mSlave->writeInputRegister(addr, {val});
                break;
            }
        }
    }
}

void MainWindow::flushDataTable(){
    if(!mConnecting && !mListening){
        return;
    }
    auto flushData = [this](const std::vector<uint16_t> &regs){
        mRegisterWin.setValues(mSlaveAddr, regs);
    };
    if(mModbusMode == ModbusMode::MASTER){
        if(mFuncode == 3){
            std::vector<uint16_t> regvals = mMaster->readHoldRegister(mSlaveAddr, mSlaveRegisterCnt);
            flushData(regvals);
        }else{
            std::vector<uint16_t> regvals = mMaster->readInputRegister(mSlaveAddr, mSlaveRegisterCnt);
            flushData(regvals);
        }
    }else{
        if(mFuncode == 3){
            const uint16_t *registers = mSlave->getHoldRegisters();
            std::vector<uint16_t> regs(registers, registers + mSlaveRegisterCnt);
            flushData(regs);
        }else{
            const uint16_t *registers = mSlave->getInputRegisters();
            std::vector<uint16_t> regs(registers, registers + mSlaveRegisterCnt);
            flushData(regs);
        }
    }
}

void MainWindow::setRegisterWinBtn(){
    if(mModbusMode == ModbusMode::MASTER && mFuncode == 4){
        mRegisterWin.disenableAllInput();
    }
}

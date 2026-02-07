#include "modbusregister.h"

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>

constexpr int WIN_HEIGHT = 440;
constexpr int WIN_WIDTH = 910;

ModbusRegister::ModbusRegister() {
    this->resize(WIN_WIDTH, WIN_HEIGHT);
    init();
}


void ModbusRegister::setAddrAndCount(int addr, int count){
    int startCol = (addr / 10) * 10;
    int base = 10;
    if(mDisplayMode == DisplayMode::HEX){
        base = 16;
    }
    for(int i = 1; i < 11; i++){
        mValues[0][i] = startCol + (i-1) * 10;
        mValues[i][0] = i - 1;
        mTable[0][i]->setText(QString::number(mValues[0][i], base));
        mTable[i][0]->setText(QString::number(mValues[i][0], base));
    }

    int rangeMin = addr;
    int rangeMax = addr + count;
    for(int row = 1; row < 11; row++){
        for(int col = 1; col < 11; col++){
            int addr = mValues[0][col] + mValues[row][0];
            if(addr >= rangeMin && addr < rangeMax){
                mTable[row][col]->setEnabled(true);
            }else{
                mTable[row][col]->setEnabled(false);
            }
        }
    }
}

void ModbusRegister::flushTable(){

    for(int row = 1; row < 11; row ++){
        for(int col = 1; col < 11; col++){
            QString val;
            if(mDisplayMode == DisplayMode::HEX){
                val = QString::asprintf("%04X", mValues[row][col]);
            }else{
                val = QString::number(mValues[row][col]);
            }
            mTable[row][col]->setText(val);
        }
    }
}


void ModbusRegister::createWidget(){
    const int eleWidth = 80;
    const int eleHeight = 35;
    const int x_offset = 15;
    const int y_offset = 20;
    for(int row = 0; row < 11; row++){
        for(int col = 0; col < 11; col++){
            mTable[row][col] = new QPushButton(this);
            if(col == 0 || row == 0){
                mTable[row][col]->setEnabled(false);
                mTable[row][col]->setStyleSheet("background-color: transparent;");
            }
            mTable[row][col]->setGeometry(eleWidth * col + x_offset, eleHeight * row + y_offset, eleWidth, eleHeight);
            connect(mTable[row][col], &QPushButton::clicked, this, [this, row, col]{
                int addr = mValues[0][col] + mValues[row][0];
                mEditAddrVal.setText(QString::number(addr));
                mTxtEditValue.setText("");
                mEditWin.show();
            });
            mValues[row][col] = 0;
        }
    }
    mTable[0][0]->setText("OFF/ADDR");
}


void ModbusRegister::init(){
    createWidget();
    setAddrAndCount(0, 100);
    createEditWin();
}

void ModbusRegister::setValues(int addr, const std::vector<uint16_t> &values){
    int startcol = addr / 10;
    for(uint16_t val : values){
        int row = addr % 10 + 1;
        int col = addr / 10 - startcol + 1;
        mValues[row][col] = val;
        addr++;
    }
    flushTable();
}

void ModbusRegister::createEditWin(){
    mEditWin.setParent(this);
    mEditWin.setGeometry(100,100, 200, 130);
    mEditWin.setStyleSheet("background-color: #555; border: solid 1px blue; border-radius: 10px;");

    mEditAddr.setParent(&mEditWin);
    mEditAddr.setText("Addr: ");
    mEditAddr.setGeometry(30, 10, 40, 30);

    mEditAddrVal.setParent(&mEditWin);
    mEditAddrVal.setText("1000");
    mEditAddrVal.setGeometry(80, 10, 60, 30);

    mLabEditValue.setParent(&mEditWin);
    mLabEditValue.setText("Value:");
    mLabEditValue.setGeometry(30, 50, 40, 30);

    static const QString STYLE_SHEET_BACK {"background-color: white; color: black; border: 1px #444 solid; border-radius: 2px;"};

    mTxtEditValue.setParent(&mEditWin);
    mTxtEditValue.setGeometry(80, 52, 80, 26);
    mTxtEditValue.setStyleSheet(STYLE_SHEET_BACK);

    mBtnEditCancel.setParent(&mEditWin);
    mBtnEditCancel.setGeometry(30, 90, 60, 26);
    mBtnEditCancel.setText("取消");
    mBtnEditCancel.setStyleSheet(STYLE_SHEET_BACK);

    mBtnEditConfirm.setParent(&mEditWin);
    mBtnEditConfirm.setGeometry(100, 90, 60, 26);
    mBtnEditConfirm.setText("确定");
    mBtnEditConfirm.setStyleSheet(STYLE_SHEET_BACK);

    connect(&mBtnEditCancel, &QPushButton::clicked, this, [this]{
        mTxtEditValue.clear();
        mEditWin.hide();
    });

    connect(&mBtnEditConfirm, &QPushButton::clicked, this, [this]{
        int addr = mEditAddrVal.text().toUInt();
        QString valstr = mTxtEditValue.text().toLower();
        int value = 0;
        if(valstr.startsWith("0x")){
            value = valstr.mid(2).toUInt(nullptr, 16);
        }else{
            value = valstr.toUInt();
        }
        if(value > 65536){
            mTxtEditValue.setStyleSheet(STYLE_SHEET_BACK + "color: red;");
            return;
        }
        mTxtEditValue.setStyleSheet(STYLE_SHEET_BACK);

        emit modifyValue(addr, value);
        mEditWin.hide();
    });

    mEditWin.hide();
}

void ModbusRegister::disenableAllInput(){
    for(auto &row : mTable){
        for(auto &ele : row){
            ele->setEnabled(false);
        }
    }
}

void ModbusRegister::setDisplayMode(DisplayMode mode){
    mDisplayMode = mode;
}


void ModbusRegister::keyReleaseEvent(QKeyEvent *event){
    if(mEditWin.isHidden()){
        return;
    }

    int key = event->key();
    if(key == Qt::Key_Escape){
        mBtnEditCancel.click();
    }else if(key == Qt::Key_Return){
        mBtnEditConfirm.click();
    }
}

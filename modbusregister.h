#ifndef MODBUSREGISTER_H
#define MODBUSREGISTER_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

class ModbusRegister : public QWidget
{
    Q_OBJECT
public:
    ModbusRegister();
    void setAddrAndCount(int addr, int count);
    void setValues(int addr, const std::vector<uint16_t> &values);
    void disenableAllInput();

signals:
    void modifyValue(int addr, int value);

private:
    void flushTable();
    void createWidget();
    void init();
    void createEditWin();

private:
    std::array<std::array<QPushButton*, 11>, 11> mTable;
    int mValues[11][11];

    QWidget mEditWin;
    QLabel mEditAddr;
    QLabel mEditAddrVal;
    QLabel mLabEditValue;
    QLineEdit mTxtEditValue;
    QPushButton mBtnEditCancel;
    QPushButton mBtnEditConfirm;

};

#endif // MODBUSREGISTER_H

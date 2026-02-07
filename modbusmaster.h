#ifndef MODBUSMASTER_H
#define MODBUSMASTER_H

#include "modbus.h"
#include <string>
#include <regex>
#include <uchar.h>
#include <array>

class ModbusMaster
{
public:
    ModbusMaster();
    void setSlave(int slaveId);

    uint16_t readHoldRegister(unsigned int addr);
    std::vector<uint16_t> readHoldRegister(unsigned int addr, unsigned int len);

    uint16_t readInputRegister(unsigned int addr);
    std::vector<uint16_t> readInputRegister(unsigned int addr, unsigned int len);

    void writeRegister(int addr, const std::vector<uint16_t> &valus);

    virtual bool open() = 0;
    void close();

    virtual bool connected() const;

protected:
    static constexpr int UNSET_SLAVE_ID = -1;
    std::shared_ptr<modbus_t> mHandle = nullptr;
    int mSlaveId = UNSET_SLAVE_ID;
private:
    bool checkConnect();
};

class ModbusMasterTcp : public ModbusMaster
{
public:
    void setTarget(const std::string ip, uint16_t port);
    bool open() override;
private:
    std::string mIp;
    unsigned int mPort;
};

class ModbusMasterRtu : public ModbusMaster
{
public:
    void setTarget(const std::string &com, uint32_t baud, char parity = 'N', int dataBits = 8, int stopBits = 1);
    bool open() override;
private:
    std::string mCom;
    uint32_t mBaud;
    char mParity = 'N';
    int mDataBits = 9;
    int mStopBits = 1;
};

#endif // MODBUSMASTER_H

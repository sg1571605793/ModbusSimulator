#ifndef MODBUSSLAVE_H
#define MODBUSSLAVE_H

#include <memory>
#include <uchar.h>
#include <vector>
#include <thread>

#include "modbus.h"
#include <winsock2.h>

class ModbusSlave
{
public:
    struct RegisterInfo
    {
        struct
        {
            int addr;
            int size;
        } holdRegister, inputRegister;
    };

    enum class AddrType : uint8_t
    {
        HOLD_REGISTER,
        INPUT_REGISTER
    };

public:
    ModbusSlave();

    virtual bool open() = 0;
    void close();

    void setSlave(int slaveId);

    bool createRegisterMapping(const RegisterInfo &info);

    uint16_t readHoldRegister(unsigned int addr);
    std::vector<uint16_t> readHoldRegister(unsigned int addr, unsigned int len);

    uint16_t readInputRegister(unsigned int addr);
    std::vector<uint16_t> readInputRegister(unsigned int addr, unsigned int len);

    void writeHoldRegister(unsigned int addr, const std::vector<uint16_t> &values);
    void writeInputRegister(unsigned int addr, const std::vector<uint16_t> &values);

    const uint16_t* getHoldRegisters() const;
    const uint16_t* getInputRegisters() const;

protected:
    static constexpr int UNSET_SLAVE_ID = -1;
    std::shared_ptr<modbus_t> mHandle;
    int mSlaveId = UNSET_SLAVE_ID;

    std::shared_ptr<modbus_mapping_t> mMapping;
    RegisterInfo mRegisterInfo;

protected:
    bool legalAddress(int addr, AddrType type);
};

class ModbusSlaveTCP : public ModbusSlave
{
public:
    ModbusSlaveTCP() = default;

    bool open() override;
    void setLocalPort(const std::string &ip, int port);

private:
    std::string mIp;
    int mPort;

    static constexpr int LISTEN_LIST_LEN = 5;
    std::unique_ptr<std::thread> mListenThread;
    mutable std::mutex mMappingMutex;
    int mSockServ = -1;

    bool mFinish = false;

private:
    void tcpListen();
    void handleClient(int client);
};

class ModbusSlaveRTU : public ModbusSlave
{
public:
    ModbusSlaveRTU() = default;

    bool open() override;
    void setTarget(const std::string &com, int baud, char parity, int databits, int stopbits);

private:
    std::string mCom;
    int mBaud;
    char mParity;
    int mDataBits;
    int mStopBits;

    bool mFinish = false;

    std::unique_ptr<std::thread> mReplyMaster;

private:
    void replyMaster();
};

#endif // MODBUSSLAVE_H

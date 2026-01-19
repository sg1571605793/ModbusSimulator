#include "modbusmaster.h"

ModbusMaster::ModbusMaster() {}

void ModbusMaster::setSlave(int slaveId)
{
    mSlaveId = slaveId;
}

uint16_t ModbusMaster::readHoldRegister(unsigned int addr)
{
    if (!mHandle)
    {
        return 0;
    }
    uint16_t res;
    if (modbus_read_registers(mHandle.get(), addr, 1, &res) == -1)
    {
        return 0;
    }
    else
    {
        return res;
    }
}

std::vector<uint16_t> ModbusMaster::readHoldRegister(unsigned int addr, unsigned int len)
{
    if (!mHandle)
    {
        return {};
    }
    std::vector<uint16_t> res(len);
    if (modbus_read_registers(mHandle.get(), addr, len, res.data()) == -1)
    {
        return {};
    }
    else
    {
        return res;
    }
}

uint16_t ModbusMaster::readInputRegister(unsigned int addr)
{
    if (!mHandle)
    {
        return 0;
    }
    uint16_t res;
    if (modbus_read_input_registers(mHandle.get(), addr, 1, &res) == -1)
    {
        return 0;
    }
    else
    {
        return res;
    }
}

std::vector<uint16_t> ModbusMaster::readInputRegister(unsigned int addr, unsigned int len)
{
    if (!mHandle)
    {
        return {};
    }
    std::vector<uint16_t> res(len);
    if (modbus_read_input_registers(mHandle.get(), addr, len, res.data()) == -1)
    {
        return {};
    }
    else
    {
        return res;
    }
}

void ModbusMaster::close()
{
    mHandle.reset();
}

void ModbusMasterTcp::setTarget(const std::string ip, uint16_t port)
{
    mIp = ip;
    mPort = port;
}

bool ModbusMasterTcp::open()
{
    modbus_t *ctx = modbus_new_tcp(mIp.c_str(), mPort);
    if (!ctx)
    {
        return false;
    }
    modbus_set_slave(ctx, mSlaveId);
    int res = modbus_connect(ctx);
    if (res == -1)
    {
        return false;
    }
    mHandle.reset(ctx, [](modbus_t *ctx)
                  {modbus_close(ctx); modbus_free(ctx); });
    return true;
}

void ModbusMasterRtu::setTarget(const std::string &com, uint32_t baud, char parity, int dataBits, int stopBits)
{
    mCom = com;
    mBaud = baud;
    mParity = parity;
    mDataBits = dataBits;
    mStopBits = stopBits;
}

bool ModbusMasterRtu::open()
{
    modbus_t *ctx = modbus_new_rtu(mCom.c_str(), mBaud, mParity, mDataBits, mStopBits);
    if (!ctx)
    {
        return false;
    }
    modbus_set_slave(ctx, mSlaveId);
    int res = modbus_connect(ctx);
    if (res == -1)
    {
        return false;
    }
    mHandle.reset(ctx, [](modbus_t *ctx)
                  {modbus_close(ctx); modbus_free(ctx); });
    return true;
}

void ModbusMaster::writeRegister(int addr, const std::vector<uint16_t> &values){
    if(mHandle){
        modbus_write_registers(mHandle.get(), addr, values.size(), values.data());
    }
}

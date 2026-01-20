#include "modbusslave.h"
#include "modbus.h"

#include <thread>
#include <memory>
#include <iostream>
#include <windows.h>

constexpr int TIME_OUT = 500;

ModbusSlave::ModbusSlave() {}

void ModbusSlave::close()
{
    mHandle.reset();
    mMapping.reset();
}

bool ModbusSlave::createRegisterMapping(const RegisterInfo &info)
{
    modbus_mapping_t *mapping = modbus_mapping_new_start_address(
        0, 0,
        0, 0,
        info.holdRegister.addr, info.holdRegister.size,
        info.inputRegister.addr, info.inputRegister.size);

    if (!mapping)
    {
        return false;
    }

    memset(mapping->tab_input_registers, 0, info.inputRegister.size * 2);
    memset(mapping->tab_registers, 0, info.holdRegister.size * 2);

    mRegisterInfo = info;

    mMapping.reset(mapping, [](modbus_mapping_t *mapping)
                   { modbus_mapping_free(mapping); });
    return true;
}

void ModbusSlave::setSlave(int slaveId)
{
    mSlaveId = slaveId;
}

uint16_t ModbusSlave::readHoldRegister(unsigned int addr)
{
    if (!mHandle || !mMapping)
    {
        return 0;
    }
    if (legalAddress(addr, AddrType::HOLD_REGISTER))
    {
        return mMapping->tab_registers[addr - mMapping->start_registers];
    }
    return 0;
}

std::vector<uint16_t> ModbusSlave::readHoldRegister(unsigned int addr, unsigned int len)
{
    if (!mHandle || !mMapping)
    {
        return {};
    }
    if (legalAddress(addr, AddrType::HOLD_REGISTER) && legalAddress(addr + len - 1, AddrType::HOLD_REGISTER))
    {
        int offset = addr - mMapping->start_registers;
        return std::vector<uint16_t>(mMapping->tab_registers + offset, mMapping->tab_registers + offset + len);
    }
    else
    {
        return {};
    }
}

uint16_t ModbusSlave::readInputRegister(unsigned int addr)
{
    if (!mHandle || !mMapping)
    {
        return 0;
    }
    if (legalAddress(addr, AddrType::INPUT_REGISTER))
    {
        return mMapping->tab_input_registers[addr - mMapping->start_input_registers];
    }
    return 0;
}

std::vector<uint16_t> ModbusSlave::readInputRegister(unsigned int addr, unsigned int len)
{
    if (!mHandle || !mMapping)
    {
        return {};
    }
    if (legalAddress(addr, AddrType::INPUT_REGISTER) && legalAddress(addr + len - 1, AddrType::INPUT_REGISTER))
    {
        int offset = addr - mMapping->start_input_registers;
        return std::vector<uint16_t>(mMapping->tab_input_registers + offset, mMapping->tab_input_registers + offset + len);
    }
    else
    {
        return {};
    }
}

void ModbusSlave::writeHoldRegister(unsigned int addr, const std::vector<uint16_t> &values)
{
    if (!mHandle || !mMapping)
    {
        return;
    }
    if (legalAddress(addr, AddrType::HOLD_REGISTER))
    {
        int offset = addr - mMapping->start_registers;
        std::copy(values.begin(), values.end(), mMapping->tab_registers + offset);
    }
}

void ModbusSlave::writeInputRegister(unsigned int addr, const std::vector<uint16_t> &values)
{
    if (!mHandle || !mMapping)
    {
        return;
    }
    if (legalAddress(addr, AddrType::INPUT_REGISTER))
    {
        int offset = addr - mMapping->start_input_registers;
        std::copy(values.begin(), values.end(), mMapping->tab_input_registers + offset);
    }
}

bool ModbusSlave::legalAddress(int addr, AddrType type)
{
    int startAddr = 0;
    int endAddr = 0;
    switch (type)
    {
    case AddrType::HOLD_REGISTER:
        startAddr = mRegisterInfo.holdRegister.addr;
        endAddr = startAddr + mRegisterInfo.holdRegister.size;
        break;
    case AddrType::INPUT_REGISTER:
        startAddr = mRegisterInfo.inputRegister.addr;
        endAddr = startAddr + mRegisterInfo.inputRegister.size;
        break;
    }
    return (addr >= startAddr && addr < endAddr);
}

bool ModbusSlaveTCP::open()
{
    modbus_t *handle = modbus_new_tcp(mIp.c_str(), mPort);
    if (!handle)
    {
        return false;
    }

    mSockServ = modbus_tcp_listen(handle, LISTEN_LIST_LEN);
    if (mSockServ == -1)
    {
        return false;
    }

    modbus_set_slave(handle, mSlaveId);
    mListenThread = std::make_unique<std::thread>(&ModbusSlaveTCP::tcpListen, this);
    mHandle.reset(handle, [this](modbus_t *handle)
                  { mFinish = true;
                    if(mListenThread && mListenThread->joinable()){
                        mListenThread->join();
                    }
                    mListenThread.release();
                    if(mSockServ != INVALID_SOCKET){
                        closesocket(mSockServ);
                        mSockServ = INVALID_SOCKET;
                    }
                    modbus_free(handle); });
    return true;
}

void ModbusSlaveTCP::tcpListen()
{
    u_long mode = 1;
    if (ioctlsocket(mSockServ, FIONBIO, &mode) != 0)
    {
        return;
    }

    std::vector<std::unique_ptr<std::thread>> masters;
    while (!mFinish)
    {
        int sock_client = modbus_tcp_accept(mHandle.get(), &mSockServ);
        if (sock_client == -1)
        {
            Sleep(50);
            continue;
        }

        masters.emplace_back(std::make_unique<std::thread>(&ModbusSlaveTCP::handleClient, this, sock_client));
    }

    for (auto &master : masters)
    {
        master->join();
    }
}

void ModbusSlaveTCP::handleClient(int sock_client)
{
    // 创建新的modbus句柄用于此客户端
    modbus_t *client_handle = modbus_new_tcp(nullptr, 0);
    if (!client_handle)
    {
        closesocket(sock_client);
        return;
    }

    // 设置从机ID
    if (mSlaveId >= 0)
    {
        modbus_set_slave(client_handle, mSlaveId);
    }

    // 设置此客户端的套接字
    modbus_set_socket(client_handle, sock_client);

    // modbus_set_indication_timeout(client_handle, 0, TIME_OUT * 1000);

    while (!mFinish)
    {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

        // 接收查询请求
        int rc = modbus_receive(client_handle, query);
        if (rc == -1)
        {
            // 如果是超时或断开连接，退出循环
            break;
        }

        // 加锁以确保对共享映射的安全访问
        {
            std::lock_guard<std::mutex> lock(mMappingMutex);
            // 处理并回复请求
            int reply_rc = modbus_reply(client_handle, query, rc, mMapping.get());
            if (reply_rc == -1)
            {
                break;
            }
        }
    }

    // 清理资源
    modbus_close(client_handle);
    modbus_free(client_handle);
    closesocket(sock_client);
}

void ModbusSlaveTCP::setLocalPort(const std::string &ip, int port)
{
    mIp = ip;
    mPort = port;
}

void ModbusSlaveRTU::setTarget(const std::string &com, int baud, char parity, int databits, int stopbits)
{
    mCom = com;
    mBaud = baud;
    mParity = parity;
    mDataBits = databits;
    mStopBits = stopbits;
}

bool ModbusSlaveRTU::open()
{
    modbus_t *ctx = modbus_new_rtu(mCom.c_str(), mBaud, mParity, mDataBits, mStopBits);
    if (!ctx)
    {
        return false;
    }

    if (modbus_connect(ctx) == -1)
    {
        modbus_free(ctx);
        return false;
    }

    modbus_set_slave(ctx, mSlaveId);

    mReplyMaster = std::make_unique<std::thread>(&ModbusSlaveRTU::replyMaster, this);
    mHandle.reset(ctx, [this](modbus_t *ctx)
                  { 
                    mFinish = true;
                    mReplyMaster->join();
                    mReplyMaster.release();
                    modbus_close(ctx);
                    modbus_free(ctx); });

    return true;
}

void ModbusSlaveRTU::replyMaster()
{
    // 设置短的响应超时时间，这样可以快速检测是否有数据到达
    uint32_t response_timeout = TIME_OUT * 1000;

    modbus_set_indication_timeout(mHandle.get(), 0, response_timeout);

    while (!mFinish)
    {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

        // 非阻塞接收查询请求（通过设置短超时时间实现）
        int rc = modbus_receive(mHandle.get(), query);
        if (rc == -1)
        {
            // 如果是因为超时（没有数据到达），继续循环
            // 在Windows上，libmodbus使用超时机制而不是真正的非阻塞I/O
            Sleep(10); // 短暂休眠，避免CPU占用过高
            continue;
        }

        // 处理并回复请求
        int reply_rc = modbus_reply(mHandle.get(), query, rc, mMapping.get());
        if (reply_rc == -1)
        {
            continue; // 继续监听下一个请求
        }
    }
}

const uint16_t* ModbusSlave::getHoldRegisters() const{
    if(mMapping){
        return mMapping->tab_registers;
    }
    return nullptr;
}

const uint16_t* ModbusSlave::getInputRegisters() const{
    if(mMapping){
        return mMapping->tab_input_registers;
    }
    return nullptr;
}

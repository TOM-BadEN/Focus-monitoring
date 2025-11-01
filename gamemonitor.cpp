#include "gamemonitor.hpp"
#include <cstring>
#include "ipc_SaltyNX.h"

GameForegroundMonitor::GameForegroundMonitor() {
    // 步骤1：初始化成员变量
    m_NxFpsBlock = nullptr;
    m_Initialized = false;
    memset(&m_SharedMem, 0, sizeof(SharedMemory));
    
    // 步骤2：连接SaltySD服务
    Handle port = 0;
    Result rc = svcConnectToNamedPort(&port, "SaltySD");
    if (R_FAILED(rc)) return;
    
    // 步骤3：准备IPC请求
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 reserved[4];
    } *raw;
    
    raw = (decltype(raw))ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;
    
    // 发送请求
    rc = ipcDispatch(port);
    if (R_FAILED(rc)) {
        svcCloseHandle(port);
        return;
    }
    
    // 步骤4：解析响应并获取句柄
    IpcParsedCommand r;
    ipcParse(&r);
    
    struct {
        u64 magic;
        u64 result;
        u64 reserved[2];
    } *resp = (decltype(resp))r.Raw;
    
    rc = resp->result;
    if (rc != 0) {
        svcCloseHandle(port);
        return;
    }
    
    // 从响应中获取共享内存句柄
    Handle shmemHandle = r.Handles[0];
    svcCloseHandle(port);
    
    // 步骤5：加载并映射共享内存
    shmemLoadRemote(&m_SharedMem, shmemHandle, 0x1000, Perm_Rw);
    
    rc = shmemMap(&m_SharedMem);
    if (R_FAILED(rc)) {
        return;  // 映射共享内存失败
    }
    
    // 初始化成功（共享内存已映射，数据块在运行时搜索）
    m_Initialized = true;
}

GameForegroundMonitor::~GameForegroundMonitor() {
    // 清理共享内存
    if (m_Initialized) shmemClose(&m_SharedMem);
}

bool GameForegroundMonitor::IsInitialized() const {
    return m_Initialized;
}

bool GameForegroundMonitor::SearchNxFpsBlock() {
    uintptr_t base = (uintptr_t)shmemGetAddr(&m_SharedMem);
    m_NxFpsBlock = nullptr;  // 先重置
    
    for (size_t offset = 0; offset < 0x1000; offset += 4) {
        // 边界检查：确保不会越界
        if (offset + sizeof(NxFpsSharedBlock) >= 0x1000) {
            break;
        }
        
        uint32_t* magic = (uint32_t*)(base + offset);
        if (*magic == 0x465053) {  // "FPS"
            // 找到了！指向整个结构体
            m_NxFpsBlock = (NxFpsSharedBlock*)(base + offset);
            return true;
        }
    }
    // 找不到，m_NxFpsBlock 保持 nullptr
    return false;
}

// 获取游戏焦点状态
uint8_t GameForegroundMonitor::GetFocusState() const {
    if (m_NxFpsBlock == nullptr) {
        return 0;  // 数据块未找到，返回未初始化状态
    }
    return m_NxFpsBlock->currentFocusState;
}

// 重置焦点状态
void GameForegroundMonitor::ResetFocusState() {
    if (m_NxFpsBlock != nullptr) {
        m_NxFpsBlock->currentFocusState = 0;
    }
}

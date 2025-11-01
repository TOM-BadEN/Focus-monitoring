#pragma once
#include <switch.h>

// NxFps共享内存数据块结构（SaltyNX修改版）
struct resolutionCalls {
    uint16_t width;
    uint16_t height;
    uint16_t calls;
};

struct NxFpsSharedBlock {
    uint32_t MAGIC;
    uint8_t FPS;
    float FPSavg;
    bool pluginActive;
    uint8_t FPSlocked;
    uint8_t FPSmode;
    uint8_t ZeroSync;
    uint8_t patchApplied;
    uint8_t API;
    uint32_t FPSticks[10];
    uint8_t Buffers;
    uint8_t SetBuffers;
    uint8_t ActiveBuffers;
    uint8_t SetActiveBuffers;
    union {
        struct {
            bool handheld: 1;
            bool docked: 1;
            unsigned int reserved: 6;
        } __attribute__((packed)) ds;  // 关键！必须和SaltyNX一致
        uint8_t general;
    } displaySync;
    resolutionCalls renderCalls[8];
    resolutionCalls viewportCalls[8];
    bool forceOriginalRefreshRate;
    bool dontForce60InDocked;
    bool forceSuspend;
    uint8_t currentRefreshRate;
    float readSpeedPerSecond;
    uint8_t FPSlockedDocked;
    uint64_t frameNumber;
    int8_t expectedSetBuffers;
    uint8_t currentFocusState;  // 游戏焦点状态（SaltyNX官方字段）
} __attribute__((packed));

static_assert(sizeof(NxFpsSharedBlock) == 175, "NxFpsSharedBlock size must be 175 bytes");

class GameForegroundMonitor {
public:
    GameForegroundMonitor();
    ~GameForegroundMonitor();
    
    // 检查是否初始化成功
    bool IsInitialized() const;
    
    // 搜索NxFps数据块（游戏启动时调用，返回是否找到）
    bool SearchNxFpsBlock();
    
    // 获取游戏焦点状态（0=未初始化, 1=前台, 2=小程序, 3=HOME/休眠）
    uint8_t GetFocusState() const;
    
    // 重置焦点状态（游戏退出时调用）
    void ResetFocusState();
    
private:
    SharedMemory m_SharedMem;         // SaltyNX共享内存对象
    NxFpsSharedBlock* m_NxFpsBlock;   // 指向NxFps数据块的指针
    bool m_Initialized;               // 初始化是否成功
};


﻿#pragma once

#ifdef BASE_ADDRESS
const HMODULE MODULE_HANDLE = GetModuleHandle(nullptr);

#define ASLR(address) \
    ((size_t)MODULE_HANDLE + (size_t)address - (size_t)BASE_ADDRESS)
#endif

#define FUNCTION_PTR(returnType, callingConvention, function, location, ...) \
    returnType (callingConvention *function)(__VA_ARGS__) = (returnType(callingConvention*)(__VA_ARGS__))(location)

#define PROC_ADDRESS(libraryName, procName) \
    GetProcAddress(LoadLibrary(TEXT(libraryName)), procName)

#define HOOK(returnType, callingConvention, functionName, location, ...) \
    typedef returnType callingConvention functionName(__VA_ARGS__); \
    functionName* original##functionName = (functionName*)(location); \
    returnType callingConvention implOf##functionName(__VA_ARGS__)

#define INSTALL_HOOK(functionName) \
    { \
        DetourTransactionBegin(); \
        DetourUpdateThread(GetCurrentThread()); \
        DetourAttach((void**)&original##functionName, implOf##functionName); \
        DetourTransactionCommit(); \
    }

#define VTABLE_HOOK(returnType, callingConvention, className, functionName, ...) \
    typedef returnType callingConvention functionName(className* This, __VA_ARGS__); \
    functionName* original##functionName; \
    returnType callingConvention implOf##functionName(className* This, __VA_ARGS__)

#define INSTALL_VTABLE_HOOK(object, functionName, functionIndex) \
    { \
        void** addr = &(*(void***)object)[functionIndex]; \
        if (*addr != implOf##functionName) \
        { \
            original##functionName = (functionName*)*addr; \
            DWORD oldProtect; \
            VirtualProtect(addr, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect); \
            *addr = implOf##functionName; \
            VirtualProtect(addr, sizeof(void*), oldProtect, NULL); \
        } \
    }

#define WRITE_MEMORY(location, type, ...) \
    { \
        const type data[] = { __VA_ARGS__ }; \
        DWORD oldProtect; \
        VirtualProtect((void*)(location), sizeof(data), PAGE_EXECUTE_READWRITE, &oldProtect); \
        memcpy((void*)(location), data, sizeof(data)); \
        VirtualProtect((void*)(location), sizeof(data), oldProtect, NULL); \
    }

#define WRITE_JUMP(location, function) \
    { \
        WRITE_MEMORY(location, uint8_t, 0xE9); \
        WRITE_MEMORY(location + 1, uint32_t, (uint32_t)(function) - (size_t)(location) - 5); \
    }

#define WRITE_NOP(location, count) \
    { \
        DWORD oldProtect; \
        VirtualProtect((void*)(location), (size_t)(count), PAGE_EXECUTE_READWRITE, &oldProtect); \
        for (size_t i = 0; i < (size_t)(count); i++) \
            *((uint8_t*)(location) + i) = 0x90; \
        VirtualProtect((void*)(location), (size_t)(count), oldProtect, NULL); \
    }
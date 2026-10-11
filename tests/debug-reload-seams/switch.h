#pragma once
#include <cstddef>
#include <cstdint>

using Result = std::uint32_t;
constexpr bool R_FAILED(Result value) {
    return value != 0;
}
enum AppletType { AppletType_Application,
                  AppletType_SystemApplication,
                  AppletType_LibraryApplet,
                  AppletType_None };
struct Thread {};

AppletType appletGetAppletType();
bool hosversionAtLeast(unsigned major, unsigned minor, unsigned patch);
bool envHasNextLoad();
Result envSetNextLoad(const char* path, const char* argv);
Result appletRestartProgram(const void* argument, std::size_t size);
Result socketInitializeDefault();
void socketExit();
Result threadCreate(Thread*, void (*)(void*), void*, void*, std::size_t, int, int);
Result threadStart(Thread*);
Result threadWaitForExit(Thread*);
Result threadClose(Thread*);

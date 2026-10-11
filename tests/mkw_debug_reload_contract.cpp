// Compile the real Switch-specific protocol/SDK branch. Network writes use an
// in-memory transport; configuration failures stop before any socket creation.
#include <switch.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
AppletType type = AppletType_Application;
bool modern = true;
Result restart_result = 0;
unsigned restarts = 0, socket_initializations = 0;
bool parsed_program_reload = false;
std::string wire;
} // namespace

AppletType appletGetAppletType() {
    return type;
}
bool hosversionAtLeast(unsigned major, unsigned minor, unsigned patch) {
    assert(major == 5 && minor == 0 && patch == 0);
    return modern;
}
Result appletRestartProgram(const void* argument, std::size_t size) {
    assert(argument == nullptr && size == 0);
    ++restarts;
    return restart_result;
}
Result socketInitializeDefault();
void socketExit() {
    std::abort();
}
Result threadCreate(Thread*, void (*)(void*), void*, void*, std::size_t, int, int) {
    std::abort();
}
Result threadStart(Thread*) {
    std::abort();
}
void threadWaitForExit(Thread*) {
    std::abort();
}
void threadClose(Thread*) {
    std::abort();
}
int reload_test_select(int, fd_set*, fd_set*, fd_set*, timeval*) {
    return 1;
}
ssize_t reload_test_send(int fd, const void* bytes, std::size_t size, int) {
    assert(fd == 17);
    wire.append(static_cast<const char*>(bytes), size);
    return static_cast<ssize_t>(size);
}
#define select reload_test_select
#define send reload_test_send
#include "../source/mkw_debug_server.cpp"
#undef send
#undef select

Result socketInitializeDefault() {
    ++socket_initializations;
    parsed_program_reload = mkw::debug::reload_program_opt_in;
    return 1; // No host socket may be created by the configuration tests.
}

namespace {
void Configuration(const char* extra, bool expected) {
    mkw::debug::stop();
    const auto config = std::string(mkw::debug::DefaultRoot) + "/Config/debug.ini";
    std::filesystem::create_directories(std::filesystem::path(config).parent_path());
    std::ofstream(config) << "debug_server=1\n"
                          << extra;
    const auto previous = socket_initializations;
    assert(!mkw::debug::start()); // Mock SDK initialization intentionally fails.
    assert(socket_initializations == previous + 1);
    assert(parsed_program_reload == expected);
    assert(!mkw::debug::can_reload());
    mkw::debug::stop();
    assert(!mkw::debug::reload_program_opt_in);
}
void Reload(bool enabled, AppletType applet, bool version, Result result, bool accepted) {
    mkw::debug::running = true;
    mkw::debug::reload_program_opt_in = enabled;
    type = applet;
    modern = version;
    restart_result = result;
    restarts = 0;
    wire.clear();
    mkw::debug::Client client;
    client.fd = 17;
    assert(mkw::debug::can_reload() == accepted);
    const auto description = mkw::debug::info();
    assert(description.find(accepted ? "reload_supported=1\n" : "reload_supported=0\n") != std::string::npos);
    const auto keep_session = mkw::debug::command(client, "reload");
    assert(restarts == (accepted ? 1u : 0u));
    if (!accepted) {
        assert(keep_session && wire.starts_with("err "));
        assert(wire.find("restart requested") == std::string::npos);
        if (!enabled) {
            assert(wire.find("debug_reload_program=1") != std::string::npos);
            assert(wire.find("host game") != std::string::npos);
        }
    } else {
        assert(!keep_session && wire.starts_with("ok 18\nrestart requested\n"));
        if (result)
            assert(wire.find("err ") != std::string::npos && wire.find("restart failed: 0x00001234") != std::string::npos);
        else
            assert(wire == "ok 18\nrestart requested\n");
    }
    mkw::debug::running = false;
}
} // namespace

int main() {
    static_assert(!mkw::debug::program_restart_allowed(false, false, false));
    static_assert(!mkw::debug::program_restart_allowed(false, false, true));
    static_assert(!mkw::debug::program_restart_allowed(false, true, false));
    static_assert(!mkw::debug::program_restart_allowed(false, true, true));
    static_assert(!mkw::debug::program_restart_allowed(true, false, false));
    static_assert(!mkw::debug::program_restart_allowed(true, false, true));
    static_assert(!mkw::debug::program_restart_allowed(true, true, false));
    static_assert(mkw::debug::program_restart_allowed(true, true, true));
    assert(!mkw::debug::reload_program_opt_in);
    Configuration("", false);
    Configuration("debug_reload_program=0\n", false);
    Configuration("debug_reload_program=true\n", false);
    Configuration("# debug_reload_program=1\n", false);
    Configuration("debug_reload_program=1\n", true);
    Configuration("debug_reload_program=1\ndebug_reload_program=0\n", false);
    Configuration("debug_reload_program=0\n debug_reload_program = 1 # forwarder opt-in\n", true);
    std::filesystem::remove(std::string(mkw::debug::DefaultRoot) + "/Config/debug.ini");
    mkw::debug::reload_program_opt_in = true;
    assert(!mkw::debug::start() && !mkw::debug::reload_program_opt_in);
    Reload(false, AppletType_Application, true, 0, false); // hbmenu host title.
    Reload(false, AppletType_SystemApplication, true, 0, false);
    Reload(true, AppletType_LibraryApplet, true, 0, false);
    Reload(true, AppletType_Application, false, 0, false);
    Reload(true, AppletType_Application, true, 0, true);
    Reload(true, AppletType_SystemApplication, true, 0, true);
    Reload(true, AppletType_Application, true, 0x1234, true);
    std::puts("PASS: Switch program reload requires explicit forwarder opt-in; Application alone never authorizes SDK restart");
}

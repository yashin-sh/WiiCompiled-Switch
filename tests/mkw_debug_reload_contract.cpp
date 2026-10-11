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
Result restart_result = 0, next_load_result = 0;
bool loader = false, owner_phase = false, send_ok = true;
unsigned next_loads = 0;
std::string next_path, next_argv;
bool allow_thread_calls = false;
Result join_result = 0, close_result = 0;
unsigned joins = 0, closes = 0, sockets_exits = 0;
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
    assert(owner_phase);
    ++restarts;
    return restart_result;
}
bool envHasNextLoad() {
    return loader;
}
Result envSetNextLoad(const char* path, const char* argv) {
    assert(owner_phase);
    ++next_loads;
    next_path = path;
    next_argv = argv;
    return next_load_result;
}
Result socketInitializeDefault();
void socketExit() {
    assert(allow_thread_calls);
    ++sockets_exits;
}
Result threadCreate(Thread*, void (*)(void*), void*, void*, std::size_t, int, int) {
    std::abort();
}
Result threadStart(Thread*) {
    std::abort();
}
Result threadWaitForExit(Thread*) {
    assert(allow_thread_calls);
    ++joins;
    return join_result;
}
Result threadClose(Thread*) {
    assert(allow_thread_calls);
    ++closes;
    return close_result;
}
int reload_test_select(int, fd_set*, fd_set*, fd_set*, timeval*) {
    return 1;
}
ssize_t reload_test_send(int fd, const void* bytes, std::size_t size, int) {
    assert(fd == 17);
    if (!send_ok)
        return 0;
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
    assert(!mkw::debug::reload_owner_available);
}
void Reload(bool enabled, AppletType applet, bool version, Result result, bool accepted, bool next_loader = false) {
    mkw::debug::running = true;
    mkw::debug::set_reload_owner_available(true);
    mkw::debug::reload_program_opt_in = enabled;
    mkw::debug::pending_reload = mkw::debug::ReloadMethod::Unavailable;
    mkw::debug::reload_prepared = false;
    mkw::debug::nro = mkw::debug::DefaultNro;
    type = applet;
    modern = version;
    loader = next_loader;
    restart_result = next_load_result = result;
    owner_phase = false;
    restarts = next_loads = 0;
    wire.clear();
    mkw::debug::Client client;
    client.fd = 17;
    assert(mkw::debug::can_reload() == accepted);
    const auto description = mkw::debug::info();
    assert(description.find(accepted ? "reload_supported=1\n" : "reload_supported=0\n") != std::string::npos);
    const auto keep_session = mkw::debug::command(client, "reload");
    assert(!restarts && !next_loads); // Worker only acknowledges and publishes.
    if (!accepted) {
        assert(keep_session && wire.starts_with("err "));
        assert(wire.find("restart requested") == std::string::npos);
        assert(!mkw::debug::reload_requested());
        assert(!mkw::debug::prepare_reload_on_owner());
    } else {
        assert(!keep_session && wire.starts_with("ok ") && wire.find("restart requested\n") != std::string::npos);
        assert(wire.find("method=hbloader\n") != std::string::npos);
        assert(mkw::debug::reload_requested());
        wire.clear();
        assert(mkw::debug::command(client, "reload") && wire.starts_with("err "));
        wire.clear();
        assert(mkw::debug::command(client, "deploy 128 00000000") && wire.starts_with("err "));
        owner_phase = true;
        assert(mkw::debug::prepare_reload_on_owner() == !result);
        assert(!restarts && next_loads == 1u);
        if (next_loader)
            assert(next_path == mkw::debug::nro && next_argv == next_path && next_path.size() < 512 && next_argv.size() < 2048);
        assert(!mkw::debug::reload_requested());
        assert(!mkw::debug::prepare_reload_on_owner());
        assert(mkw::debug::reload_error == result);
        assert(mkw::debug::enabled()); // Preparation failure cannot terminate NRO.
        if (result) {
            wire.clear();
            assert(mkw::debug::command(client, "ping") && wire.find("pong") != std::string::npos);
        }
    }
    mkw::debug::running = false;
    mkw::debug::set_reload_owner_available(false);
}

void StopFailures() {
    allow_thread_calls = true;
    joins = closes = sockets_exits = 0;
    mkw::debug::running = true;
    mkw::debug::thread_ready = mkw::debug::thread_started = true;
    mkw::debug::thread_joined = false;
    mkw::debug::sockets_ready = true;
    mkw::debug::set_reload_owner_available(true);
    join_result = 0x101;
    close_result = 0;
    assert(!mkw::debug::stop());
    assert(joins == 1 && !closes && !sockets_exits);
    assert(!mkw::debug::enabled() && mkw::debug::thread_ready && mkw::debug::thread_started);
    assert(!mkw::debug::thread_joined && mkw::debug::sockets_ready);
    // Failed cleanup cannot be replaced with a fresh thread/configuration.
    assert(!mkw::debug::start());
    assert(joins == 2 && !closes && !sockets_exits);
    join_result = 0;
    close_result = 0x202;
    assert(!mkw::debug::stop());
    assert(joins == 3 && closes == 1 && !sockets_exits);
    assert(mkw::debug::thread_ready && mkw::debug::thread_started && mkw::debug::thread_joined);
    assert(mkw::debug::sockets_ready && mkw::debug::reload_error == close_result);
    close_result = 0;
    assert(mkw::debug::stop());
    assert(joins == 3 && closes == 2 && sockets_exits == 1); // Retry close, never rejoin.
    assert(!mkw::debug::thread_ready && !mkw::debug::thread_started && !mkw::debug::thread_joined);
    assert(!mkw::debug::sockets_ready && !mkw::debug::reload_owner_available);
    assert(mkw::debug::stop() && joins == 3 && closes == 2 && sockets_exits == 1);
    // A created thread whose start failed has no running body to join.
    mkw::debug::thread_ready = true;
    mkw::debug::thread_started = mkw::debug::thread_joined = false;
    close_result = 0x303;
    assert(!mkw::debug::stop() && joins == 3 && closes == 3);
    assert(mkw::debug::thread_ready && !mkw::debug::thread_started);
    close_result = 0;
    assert(mkw::debug::stop() && joins == 3 && closes == 4 && !mkw::debug::thread_ready);
    allow_thread_calls = false;
}

} // namespace

int main() {
    static_assert(mkw::debug::choose_reload_method(false, false) == mkw::debug::ReloadMethod::Unavailable);
    static_assert(mkw::debug::choose_reload_method(false, true) == mkw::debug::ReloadMethod::Unavailable);
    static_assert(mkw::debug::choose_reload_method(true, false) == mkw::debug::ReloadMethod::Unavailable);
    static_assert(mkw::debug::choose_reload_method(true, true) == mkw::debug::ReloadMethod::HbloaderNextLoad);
    assert(!mkw::debug::reload_program_opt_in);
    std::filesystem::create_directories(std::filesystem::path(mkw::debug::DefaultNro).parent_path());
    std::string target(128, '\0');
    target.replace(16, 4, "NRO0");
    target[24] = static_cast<char>(128);
    std::ofstream(mkw::debug::DefaultNro, std::ios::binary).write(target.data(), target.size());
    mkw::debug::set_launch_path(mkw::debug::DefaultNro);
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
    mkw::debug::nro = mkw::debug::DefaultNro;
    mkw::debug::running = true;
    loader = true;
    mkw::debug::reload_program_opt_in = true;
    mkw::debug::Client blocked;
    blocked.fd = 17;
    assert(!mkw::debug::can_reload()); // SDK capability is not owner readiness.
    wire.clear();
    assert(mkw::debug::command(blocked, "reload") && wire.starts_with("err "));
    assert(!mkw::debug::reload_requested() && !restarts && !next_loads);
    mkw::debug::set_reload_owner_available(true);
    for (const auto* suffix : {".part", ".rollback"}) {
        std::ofstream(mkw::debug::nro + suffix) << "unfinished";
        wire.clear();
        assert(mkw::debug::command(blocked, "reload") && wire.starts_with("err "));
        assert(!mkw::debug::reload_requested());
        std::filesystem::remove(mkw::debug::nro + suffix);
    }
    send_ok = false;
    assert(!mkw::debug::command(blocked, "reload") && !mkw::debug::reload_requested());
    send_ok = true;
    loader = false;
    mkw::debug::running = false;
    mkw::debug::set_reload_owner_available(false);
    Reload(false, AppletType_Application, true, 0, false); // hbmenu host title.
    Reload(false, AppletType_SystemApplication, true, 0, false);
    Reload(true, AppletType_LibraryApplet, true, 0, false);
    Reload(true, AppletType_Application, false, 0, false);
    Reload(true, AppletType_Application, true, 0, false);
    Reload(true, AppletType_SystemApplication, true, 0, false);
    Reload(true, AppletType_Application, true, 0x1234, false);
    Reload(false, AppletType_Application, true, 0, true, true);
    Reload(false, AppletType_LibraryApplet, false, 0, true, true);
    Reload(true, AppletType_Application, true, 0, true, true); // Program flag cannot enable SDK restart.
    Reload(false, AppletType_Application, true, 0x1234, true, true);
    mkw::debug::stop();
    mkw::debug::set_launch_path(mkw::debug::DefaultNro);
    Configuration("debug_reload_program=0\n", false);
    const auto identity = mkw::debug::runtime_nro_sha256;
    const auto session = mkw::debug::runtime_session;
    assert(identity.size() == 64 && mkw::debug::runtime_nro_size == 128);
    std::ofstream(mkw::debug::DefaultNro, std::ios::binary) << "replaced candidate";
    assert(mkw::debug::runtime_nro_sha256 == identity); // Never live hash SD after upload.
    Configuration("debug_reload_program=0\n", false);
    assert(mkw::debug::runtime_nro_sha256 == identity && mkw::debug::runtime_session == session);
    mkw::debug::set_launch_path(nullptr);
    StopFailures();
    std::puts("PASS: Switch reload queues owner-only hbloader next-load; program restart never invokes the SDK; failed worker joins/closes remain owned");
}

#include <host_context.h>

#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <new>

extern "C" void mkw_switch_co_switch(void** target_sp, void** source_sp);
extern "C" void* mkw_switch_co_init(void* stack_top, void (*entry)(void*), void* argument);
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace HostContext {
namespace {

struct Context {
    void* saved_stack_pointer = nullptr;
    void* stack = nullptr;
    std::size_t stack_size = 0;
    bool owns_stack = false;
};

// WiiCompiled confines guest cooperative scheduling to the host thread that
// initialized the scheduler. Do not use TLS here: manually switched stacks on
// Horizon should not depend on libc TLS implementation details.
Context* g_current = nullptr;

constexpr std::size_t kStackAlignment = 0x1000;

std::size_t align_stack_size(std::size_t value) {
    return (value + kStackAlignment - 1) & ~(kStackAlignment - 1);
}

} // namespace

bool InitializeScheduler(Handle* scheduler) {
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_SCHEDULER_INIT");
    if (!scheduler || g_current) {
        mkw_switch_set_fast_track_stage("HOST_CONTEXT_SCHEDULER_INVALID");
        return false;
    }

    auto* context = new (std::nothrow) Context();
    if (!context) {
        mkw_switch_set_fast_track_stage("HOST_CONTEXT_SCHEDULER_ALLOC_FAILED");
        return false;
    }

    g_current = context;
    *scheduler = context;
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_SCHEDULER_READY");
    return true;
}

void ShutdownScheduler(Handle scheduler) {
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_SCHEDULER_SHUTDOWN");
    auto* context = static_cast<Context*>(scheduler);
    if (!context) {
        return;
    }

    if (g_current == context) {
        g_current = nullptr;
    }
    if (context->owns_stack && context->stack) {
        std::free(context->stack);
    }
    delete context;
}

Handle Create(std::size_t stackSize, Entry entry, void* argument) {
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_CREATE");
    if (!g_current || !entry || stackSize == 0) {
        mkw_switch_set_fast_track_stage("HOST_CONTEXT_CREATE_INVALID");
        return nullptr;
    }

    auto* context = new (std::nothrow) Context();
    if (!context) {
        mkw_switch_set_fast_track_stage("HOST_CONTEXT_CREATE_ALLOC_FAILED");
        return nullptr;
    }

    context->stack_size = align_stack_size(stackSize);
    context->stack = memalign(kStackAlignment, context->stack_size);
    if (!context->stack) {
        delete context;
        mkw_switch_set_fast_track_stage("HOST_CONTEXT_STACK_ALLOC_FAILED");
        return nullptr;
    }
    context->owns_stack = true;
    std::memset(context->stack, 0, context->stack_size);

    auto* stack_top = static_cast<unsigned char*>(context->stack) + context->stack_size;
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_CO_INIT");
    context->saved_stack_pointer = mkw_switch_co_init(stack_top, entry, argument);
    if (!context->saved_stack_pointer) {
        std::free(context->stack);
        delete context;
        mkw_switch_set_fast_track_stage("HOST_CONTEXT_CO_INIT_FAILED");
        return nullptr;
    }
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_WORKER_READY");
    return context;
}

void Destroy(Handle context) {
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_DESTROY");
    auto* native_context = static_cast<Context*>(context);
    if (!native_context || native_context == g_current) {
        return;
    }
    if (native_context->owns_stack && native_context->stack) {
        std::free(native_context->stack);
    }
    delete native_context;
}

bool IsCurrent(Handle context) {
    return context != nullptr && context == g_current;
}

void Switch(Handle target) {
    auto* destination = static_cast<Context*>(target);
    Context* source = g_current;
    if (!destination || !source || destination == source) {
        return;
    }

    mkw_switch_set_fast_track_stage("HOST_CONTEXT_SWITCH_ENTER");
    g_current = destination;
    mkw_switch_co_switch(&destination->saved_stack_pointer, &source->saved_stack_pointer);
    g_current = source;
    mkw_switch_set_fast_track_stage("HOST_CONTEXT_SWITCH_RETURNED");
}

} // namespace HostContext

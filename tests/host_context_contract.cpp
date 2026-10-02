#include "switch_host_context_ext.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>

namespace {
std::size_t allocationCalls = 0;
std::size_t allocationSize = 0;
std::size_t initializationCalls = 0;
void* allocation = nullptr;
void* expectedArgument = nullptr;
bool failAllocation = false;
bool failInitialization = false;

void Worker(void*) {
    // The allocation-only contract must never execute a coroutine entry.
    assert(false);
}
} // namespace

extern "C" void* __real_memalign(std::size_t alignment, std::size_t size);
extern "C" void* __wrap_memalign(std::size_t alignment, std::size_t size) {
    ++allocationCalls;
    assert(alignment == 4096u);
    allocationSize = size;
    if (failAllocation) {
        return nullptr;
    }
    allocation = __real_memalign(alignment, size);
    return allocation;
}

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}
extern "C" void mkw_switch_co_switch(void**, void**) {
    // No AArch64 context switch is exercised by this host contract.
    assert(false);
}
extern "C" void* mkw_switch_co_init(void* stackTop, void (*entry)(void*), void* argument) {
    ++initializationCalls;
    assert(entry == Worker);
    assert(argument == expectedArgument);
    assert(allocationSize >= 4096u);
    const auto* begin = static_cast<unsigned char*>(allocation);
    assert(stackTop == begin + allocationSize);
    assert(std::all_of(begin, begin + allocationSize, [](unsigned char byte) { return byte == 0; }));
    if (failInitialization) {
        return nullptr;
    }
    return static_cast<unsigned char*>(stackTop) - 240u;
}

int main() {
    assert(!HostContext::Current());
    assert(!HostContext::InitializeScheduler(nullptr));
    assert(!HostContext::Create(4096u, Worker, nullptr));

    HostContext::Handle scheduler = nullptr;
    assert(HostContext::InitializeScheduler(&scheduler));
    assert(scheduler && HostContext::Current() == scheduler);
    assert(HostContext::IsCurrent(scheduler));
    assert(!HostContext::IsCurrent(nullptr));
    HostContext::Handle duplicate = nullptr;
    assert(!HostContext::InitializeScheduler(&duplicate));
    assert(!duplicate);
    HostContext::Destroy(scheduler);
    assert(HostContext::Current() == scheduler);
    HostContext::Destroy(nullptr);
    HostContext::Switch(nullptr);

    constexpr auto maximum = std::numeric_limits<std::size_t>::max();
    for (const auto size : {std::size_t{0}, maximum, maximum - 1u, maximum - 4094u}) {
        assert(!HostContext::Create(size, Worker, nullptr));
    }
    assert(!HostContext::Create(4096u, nullptr, nullptr));
    assert(allocationCalls == 0u && initializationCalls == 0u);

    int argument = 0;
    expectedArgument = &argument;
    for (const auto size : {std::size_t{1}, std::size_t{256u * 1024u}}) {
        const auto worker = HostContext::Create(size, Worker, &argument);
        assert(worker && worker != scheduler);
        assert(!HostContext::IsCurrent(worker));
        assert(allocationSize == (size == 1u ? 4096u : size));
        assert(HostContext::Current() == scheduler);
        HostContext::Destroy(worker);
    }
    assert(allocationCalls == 2u && initializationCalls == 2u);

    failAllocation = true;
    assert(!HostContext::Create(4096u, Worker, &argument));
    failAllocation = false;
    assert(allocationCalls == 3u && initializationCalls == 2u);
    failInitialization = true;
    assert(!HostContext::Create(4096u, Worker, &argument));
    failInitialization = false;
    assert(allocationCalls == 4u && initializationCalls == 3u);

    HostContext::ShutdownScheduler(nullptr);
    assert(HostContext::Current() == scheduler);
    HostContext::ShutdownScheduler(scheduler);
    assert(!HostContext::Current());
    assert(!HostContext::Create(4096u, Worker, &argument));
    assert(HostContext::InitializeScheduler(&scheduler));
    HostContext::ShutdownScheduler(scheduler);
    assert(!HostContext::Current());
}

#include "mkw_gpu_shutdown_guard.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

struct nvk_device {
    unsigned tag;
};
struct nvk_queue {
    unsigned tag;
};
struct nvk_upload_queue {
    unsigned tag;
};
extern "C" bool __wrap_nvk_queue_try_destroy(nvk_device*, nvk_queue*);
extern "C" bool __wrap_nvk_upload_queue_try_finish(nvk_device*, nvk_upload_queue*);
namespace {
nvk_device device{7};
nvk_queue queue{9};
nvk_upload_queue upload{11};
unsigned queue_calls = 0, upload_calls = 0;
bool queue_ok = true, upload_ok = true;
} // namespace
extern "C" bool __real_nvk_queue_try_destroy(nvk_device* d, nvk_queue* q) {
    assert(d == &device && q == &queue);
    ++queue_calls;
    return queue_ok;
}
extern "C" bool __real_nvk_upload_queue_try_finish(nvk_device* d, nvk_upload_queue* q) {
    assert(d == &device && q == &upload);
    ++upload_calls;
    return upload_ok;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    assert(!mkw_switch_gpu_shutdown_failed());
    assert(__wrap_nvk_queue_try_destroy(&device, &queue));
    assert(__wrap_nvk_upload_queue_try_finish(&device, &upload));
    assert(queue_calls == 1 && upload_calls == 1 && !mkw_switch_gpu_shutdown_failed());
    if (std::strcmp(argv[1], "queue") == 0) {
        queue_ok = false;
        assert(!__wrap_nvk_queue_try_destroy(&device, &queue));
        queue_ok = true;
    } else {
        assert(std::strcmp(argv[1], "upload") == 0);
        upload_ok = false;
        assert(!__wrap_nvk_upload_queue_try_finish(&device, &upload));
        upload_ok = true;
    }
    assert(mkw_switch_gpu_shutdown_failed());
    assert(__wrap_nvk_queue_try_destroy(&device, &queue));
    assert(__wrap_nvk_upload_queue_try_finish(&device, &upload));
    assert(mkw_switch_gpu_shutdown_failed());
    assert(queue_calls + upload_calls == 5);
    std::puts("PASS: real NVK teardown wrappers preserve pointer/return ABI, delegate once and retain failure across later successful cleanup");
}

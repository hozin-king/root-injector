#include "payload.h"

#include <android/log.h>
#include <pthread.h>

#define LOG_TAG "RootInjectorPayload"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {

void* worker_thread(void*) {
    LOGI("payload worker thread alive inside target process");

    // --- UNKNOWN: scripting backend hookup, intentionally not implemented ---
    // Which block belongs here depends on two things I don't have yet: the
    // target game's Unity scripting backend, and what the feature should
    // actually do (ESP, god mode, a specific value edit, a menu). See the
    // README's open question.
    //
    //   Mono:   dlopen the Mono runtime library (libmonobdwgc-2.0.so on
    //           newer Unity, libmono.so on older) from this process's own
    //           maps, then mono_get_root_domain / mono_thread_attach before
    //           touching any other Mono API from this thread.
    //   IL2CPP: dlopen libil2cpp.so from this process's own maps, then
    //           either call il2cpp_* exports directly if this build still
    //           exports them, or resolve structure offsets from an
    //           Il2CppDumper run against this build's global-metadata.dat
    //           and GameAssembly.dll/libil2cpp.so.
    //
    // Until then this thread only proves the injection path works and
    // exits -- check logcat for this tag inside the target process to
    // confirm dlopen landed here at all.

    return nullptr;
}

}  // namespace

void payload_init() {
    LOGI("payload_init: injected into target process");
    pthread_t t;
    pthread_create(&t, nullptr, worker_thread, nullptr);
    pthread_detach(t);
}

__attribute__((constructor)) static void on_load() {
    payload_init();
}

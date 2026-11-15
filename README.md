# Root Injector

Root-based injector for Unity games on rooted Android (arm64-v8a only).
Three pieces:

- **app** (Kotlin) -- UI to check root, find a target PID by process/package
  name, and trigger injection. Talks to a persistent `su` shell.
- **injector** (C++, `cpp/injector/`) -- a standalone native binary, run as
  root via `su -c`, that ptrace-attaches to the target PID, resolves
  `dlopen`/`mmap` inside it by offset matching against this process's own
  `libdl.so`/`libc.so` mappings, allocates scratch memory in the target,
  writes the payload path into it, and calls `dlopen` remotely. Standard
  ptrace + breakpoint-trampoline injection technique, nothing game-specific.
- **payload** (C++, `cpp/payload/`) -- the library that ends up loaded
  inside the target process. Right now it only logs that it got there and
  spawns a thread; there is no Mono/IL2CPP hookup yet, see "Open question"
  below.

## What's deliberately not here yet

No ESP, no god mode, no value edits, no Mono/IL2CPP structure walking.
Those all need game-specific data (offsets, which backend, what the
feature should actually do) that I don't have. The payload stub has the
exact point marked where that code goes.

## Build

```
./gradlew assembleDebug
```

Open in Android Studio first if you don't have a Gradle wrapper jar locally
-- Android Studio will generate `gradlew`/`gradlew.bat` and the wrapper jar
on sync using the `gradle-wrapper.properties` already in this project.

Output: `app/build/outputs/apk/debug/app-debug.apk`

## Packaging check (do this before anything else)

`libinjector.so` is really a PIE executable with its output suffix forced
to `.so` so the Android Gradle Plugin bundles it next to the real shared
library instead of dropping it -- this is the one part of this project I'm
least certain survives unmodified across AGP/CMake versions. Verify:

```
unzip -l app/build/outputs/apk/debug/app-debug.apk | grep lib/arm64-v8a
```

You should see both `libinjector.so` and `libpayload.so` with non-trivial
sizes (tens of KB, not 0). If `libinjector.so` is missing, AGP didn't pick
up the renamed executable on your version. Fallback: copy it manually --
after a build it also exists at
`app/build/intermediates/cmake/debug/obj/arm64-v8a/libinjector.so` -- into
`app/src/main/jniLibs/arm64-v8a/libinjector.so` and rebuild; `jniLibs` is
always packaged regardless of what the native build step did.

## Usage

1. Install the APK on a rooted arm64 device, grant the su prompt when it
   appears.
2. Check root.
3. Type the target game's package name, Find PID (uses `pidof`; this
   assumes the game runs single-process under its own package name, true
   for most Unity games).
4. Inject. Watch the in-app log for the injector's exit code, and
   `adb logcat -s RootInjector:* RootInjectorPayload:*` for the detailed
   trail, including the payload's own log line once it's running inside
   the target.

## Things to verify on your actual device/build, in order

1. **Packaging** -- see above.
2. **SELinux** -- if `PTRACE_ATTACH` fails with `EPERM` even though `id -u`
   is 0, run `getenforce`. Some ROMs/root solutions need SELinux permissive,
   or a policy patch, for cross-app ptrace even as root.
3. **Linker namespace** -- if attach succeeds but `dlopen` returns `NULL`
   inside the target, this is very likely Android's per-process linker
   namespace rejecting the path rather than a missing file (introduced
   around Android 8). Next step if this happens: switch the remote call
   from `dlopen` to `android_dlopen_ext` with a namespace handle valid for
   the target process -- I did not implement this yet since it's extra
   complexity that may not be needed depending on the device/root setup.
4. **Bionic layout** -- `libdl.so`/`libc.so` live at different real paths
   depending on Android version (`/system/lib64/` vs
   `/apex/com.android.runtime/lib64/bionic/`). The substring match in
   `proc_maps.cpp` doesn't care which directory, but if resolution still
   fails, confirm with `adb shell cat /proc/<pid>/maps | grep libdl`.

## Open question before I can add the actual feature

Two things decide what goes in the payload's marked TODO:

- **Mono or IL2CPP?** Determines which runtime library to hook and how
  (direct `il2cpp_*`/`mono_*` calls vs. an Il2CppDumper-derived offset
  config).
- **What should it actually do once loaded?** ESP, god mode, a specific
  value edit, a full menu -- name the feature (and the game, if you want
  me to start pulling real offsets from a dump) and I'll build the hook on
  top of this.

## Status

- Files delivered: everything under `app/`, plus this README and the root
  Gradle files, as listed above.
- Build status: not built here -- I don't have an NDK/Android toolchain or
  a device in this environment. Everything above follows the standard
  ptrace + breakpoint-trampoline Android injection technique, but it has
  not been compiled or run.
- Checks to run first: the packaging check above, then root + SELinux on
  your actual device.
- What must be verified in the game: that `dlopen` actually succeeds
  against the target process (namespace check above) once you have a real
  Unity process to point it at.

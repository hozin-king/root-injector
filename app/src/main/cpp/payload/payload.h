#pragma once

// Entry point run once when the payload .so is loaded into the target
// process. Kept separate from the constructor attribute in
// payload_main.cpp so the actual hook setup logic isn't tangled with it.
void payload_init();

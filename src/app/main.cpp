#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

import epoch.app;

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    return epoch::app::run_application(static_cast<void*>(instance));
}
#else
int main() {
    return epoch::app::run_application(nullptr);
}
#endif

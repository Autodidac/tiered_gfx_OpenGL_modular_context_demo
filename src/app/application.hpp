#pragma once
// Private compatibility shim. The application surface is the epoch.app module.
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.app.hpp"
#else
import epoch.app;
#endif
#pragma once

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <cstdint>
#endif

namespace par_beatdown
{

const char *version();

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
std::uint32_t tracker_library_version();
#endif

} // namespace par_beatdown

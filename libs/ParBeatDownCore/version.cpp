#include "ParBeatDownCore/version.h"

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <libopenmpt/libopenmpt.hpp>
#endif

namespace ParBeatDownCore
{

const char *version()
{
    return "0.1.0";
}

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
std::uint32_t tracker_library_version()
{
    return openmpt::get_library_version();
}
#endif

} // namespace ParBeatDownCore

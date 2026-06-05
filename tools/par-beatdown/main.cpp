#include <ParBeatdown/Version.h>

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <ParBeatdown/TrackerTimeline.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#endif

int main(int argc, char *argv[])
{
#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
    if (argc == 4 && std::string{argv[2]} == "-o")
    {
        try
        {
            std::ofstream output{argv[3], std::ios::binary};
            if (!output)
            {
                std::cerr << "could not open output file: " << argv[3] << "\n";
                return 1;
            }

            output << par_beatdown::tracker_timeline_from_file(argv[1]);
            return output ? 0 : 1;
        }
        catch (const std::exception &exception)
        {
            std::cerr << exception.what() << "\n";
            return 1;
        }
    }
#else
    (void) argc;
    (void) argv;
#endif

    return par_beatdown::version()[0] == '\0' ? 1 : 0;
}

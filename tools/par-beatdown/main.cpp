#include <ParBeatdown/Version.h>

int main()
{
    return par_beatdown::version()[0] == '\0' ? 1 : 0;
}

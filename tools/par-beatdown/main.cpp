#include <ParBeatDownCore/version.h>

int main()
{
    return ParBeatDownCore::version()[0] == '\0' ? 1 : 0;
}

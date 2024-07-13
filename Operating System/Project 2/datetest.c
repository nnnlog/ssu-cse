#include "types.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) {
    struct rtcdate dt;
    date(&dt);
    printf(1, "Current time : %d-%d-%d %d:%d:%d\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    exit();
}

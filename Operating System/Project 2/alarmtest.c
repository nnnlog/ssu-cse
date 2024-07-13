#include "types.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) {
    int seconds;
    struct rtcdate dt;

    if (argc <= 1) exit();

    seconds = atoi(argv[1]);

    alarm(seconds);

    date(&dt);
    printf(1, "SSU_Alarm Start\n");
    printf(1, "Current time : %d-%d-%d %d:%d:%d\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    while(1);

    exit();
}

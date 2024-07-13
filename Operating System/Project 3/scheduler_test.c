#include "types.h"
#include "stat.h"
#include "user.h"

#ifdef EVAL
#define TC 10
#define PNUM 3
int arr[TC][PNUM][2] = {{{81,28},{90,168},{13,194},},{{91,45},{63,62},{10,110},},{{28,38},{55,199},{95,200},},{{96,194},{16,146},{97,197},},{{95,22},{49,160},{80,60},},{{15,1},{42,23},{91,128},},{{79,176},{95,101},{65,160},},{{4,73},{85,43},{93,137},},{{68,80},{76,149},{74,95},},{{39,85},{65,35},{17,61},},};
#else
#define PNUM 3
int arr[PNUM][2] = {
    {1, 110},
    {10, 60},
    {11, 60},
};
//int arr[PNUM][2] = {
//    {1, 110},
//    {22, 200},
//    {11, 250},
//};
//int arr[PNUM][2] = {
//    {1, 300},
//    {22, 600},
//    {34, 600},
//};
#endif

void scheduler_func() {
  printf(1, "start scheduler_test\n");
#ifdef EVAL
  for (int tc = 0; tc < TC; tc++) {
    printf(1, "turn around time\tresponse time\n");
#endif
  for (int i = 0; i < PNUM; i++) {
    int pid = fork();
    if (!pid) {
#ifdef EVAL
      set_sche_info(arr[tc][i][0], arr[tc][i][1]);
#else
      set_sche_info(arr[i][0], arr[i][1]);
#endif
      while (1);
    }
  }
  for (int i = 0; i < PNUM; i++) wait();
#ifdef EVAL
  }
#endif
  printf(1, "end of scheduler_test\n");
}

int main() {
  scheduler_func();
  exit();
}

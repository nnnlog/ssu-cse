#include <bits/stdc++.h>

/*
OS 스케쥴러 성능 비교

P_NUM 샘플 값을 3, 20, 40, 60에 대해서 수행

우선순위/틱을 임의 생성 방법 (각 경우에 대해서 10번씩 수행)
1. 무작위
2. 틱 감소
3. 틱 증가

위 5가지 경우에 대해서 성능을 측정함

성능 측정 방법
- average/total wait tick
- average/total response tick
 */

using namespace std;

int main() {
  mt19937 mt; // fixed seed
  uniform_int_distribution<int> prior(1, 99);
  uniform_int_distribution<int> tick(1, 200);

  int TC = 10;
  int PNUM = 60; // 3, 20, 40, 60

  cout << "#define TC " << TC << "\n";
  cout << "#define PNUM " << PNUM << "\n";
  cout << "int arr[TC][PNUM][2] = {\n";
  for (int tc = 0; tc < TC; tc++) {
    cout << "{";
    for (int i = 0; i < PNUM; i++) {
      cout << "{" << prior(mt) << "," << tick(mt) << "},";
    }
    cout << "},\n";
  }
  cout << "};\n";
}

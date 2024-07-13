import subprocess
import sys

for i in range(0, 7):
    print(i)
    # subprocess.run(f"python hw2_1.py tests_yes/{i}.jpg", stdout=sys.stdout, stderr=sys.stderr)
    subprocess.run(f"python hw2_1.py tests_no/{i}.jpg", stdout=sys.stdout, stderr=sys.stderr)

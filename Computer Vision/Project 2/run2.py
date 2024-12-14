import subprocess
import sys

for i in range(0, 15):
    print(i)
    # subprocess.run(f"python hw2_2.py etc_img/realistic_fail/{i}.jpg", stdout=sys.stdout, stderr=sys.stderr)
    # subprocess.run(f"python hw2_2.py etc_img/realistic/{i}.jpg", stdout=sys.stdout, stderr=sys.stderr)
    # p = subprocess.Popen(["python3", "run1.py", f"tests_no/{i}.jpg",], stdout=sys.stdout, stderr=sys.stderr)
    p = subprocess.Popen(["python", "hw2_2.py", f"tests_yes/{i}.jpg",], stdout=sys.stdout, stderr=sys.stderr)
    p.wait()
    # subprocess.run(f"python hw2_2.py tests_no/{i}.jpg", stdout=sys.stdout, stderr=sys.stderr)

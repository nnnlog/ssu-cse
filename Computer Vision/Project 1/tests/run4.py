import subprocess
import sys

# for i in range(16, -1, -1):
#     subprocess.run(f"python hw1_4.py sample/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)
# for i in range(17):
#     subprocess.run(f"python hw1_4.py sample/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)
# for i in range(6):
#     subprocess.run(f"python hw1_4.py sample2/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)


for i in range(6):
    subprocess.run(f"python hw1_4.py dataset_4/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)
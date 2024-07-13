import subprocess
import sys

# for i in range(16, -1, -1):
#     subprocess.run(f"python hw1_3.py sample/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)
# for i in range(17):
#     subprocess.run(f"python hw1_3.py sample/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)
# for i in range(6):
#     subprocess.run(f"python hw1_3.py sample2/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)

for i in range(20):
    subprocess.run(f"python hw1_3.py dataset_13/img{"_" + str(i) if i else ""}.png", stdout=sys.stdout, stderr=sys.stderr)

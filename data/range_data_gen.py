import random
import subprocess

with open("../data/test", "w") as f:
    f.writelines([f"INSERT {x}\n" for x in random.sample(range(1, 101), 100)])
    f.writelines([f"RANGE {i} {i + 1}\n" for i in range(1, 101)])

subprocess.run(["make"], capture_output=False, shell=True)
output = subprocess.run(["./BPTree", "test"], capture_output=True, shell=False)

values = [x.split() for x in str(output.stdout)[2:-1].split("\\n")][:-1]
accesses_btree, accesses_heap = zip(*values)

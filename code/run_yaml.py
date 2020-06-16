from itertools import product

import os
import sys
import yaml

if len(sys.argv) != 3:
    print("Usage: " + sys.argv[0] + " <executable> <yaml>")
    exit(1)

sn = sys.argv[0]
executable_fn = sys.argv[1]
fn = sys.argv[2]

if __name__ == "__main__":
    with open(fn) as f:
        exp_file = yaml.load(f)

    datasets = exp_file['environment']['dataset']
    seed = exp_file['environment']['seed']
    force = exp_file['environment']['force']

    tasks = []

    for k in exp_file['experiments']:
        if "sketches" in k and k["sketches"]:
            tasks.extend(product(datasets, k["storage"],
                                 k["method"], [True], k["recall"]))
        else:
            tasks.extend(product(datasets, k["storage"],
                                 k["method"], [False], [1.0]))

    for ds, storage, method, sketches, recall in tasks:
        cmd = executable_fn
        cmd += " --dataset " + ds
        cmd += " --storage " + storage
        cmd += " --method "  + method
        cmd += " --seed %d" % seed
        cmd += " --experiment-file " + fn
        if sketches:
            cmd += " --filter --recall %f" % recall
        if force:
            cmd += " --force"

        print("Running " + cmd)
        os.system(cmd)









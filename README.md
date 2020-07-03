# Source code for "Running experiments with confidence and sanity"

This document details how to run the code and connects the statements made in [our paper](to_come) to the actual code base.

## Installing and Running the Code

The code is meant to be run through docker. You can see details of the docker container here: https://github.com/Cecca/running-experiments/blob/master/Dockerfile. 
A complete run of the experiments presented in the paper looks like this:
```bash
./dockerrun make # compiles the code
./dockerrun python3 run_yaml.py ./demo experiments/sketch-test.yaml # runs the experiment
./dockerrun python3 evaluation/report.py # creates a running time and recall plot
./dockerrun jupyter nbconvert --to html --execute evaluation/evaluation.ipynb # executes the jupyter notebook, creates the figures used in the paper
./dockerrun make r-analysis # generats an interactive html website for result analysis
```

## Code Structure

The toy project implementation is header-only and can be found in `toy_project/`. The code that presents a solution to the challenges presented in [the paper]() can be found in the root directory. We quickly discuss how the code connects to our challenges, that we summarize as follows.

### Challenges

- **(C1) Feedback Loops-by-Design.** Implementations and tools support the iterative nature of an experimental study.
- **(C2) Economic Execution.** Exactly those experiments that change through code changes have to be re-run, but nothing else. 
  Moreover, only the changing parts of the experimental evaluation should be recomputed.
- **(C3) Versioning.** The ability to go back in time and compare old results to more recent ones, finding regressions or bugs; the workflow is \emph{append-only}.
- **(C4) Machine Independence.** Code and tools are designed in a way that allow them to run in a general setting.
- **(C5) Reproducibility-by-Design.** We strive for an automatic workflow that processes an experimental setup into measurements used for evaluating the experiment. Results to be included into a publication should not require manual work to transform these measurements into tables and plots.

Working on the experimental evaluation, we moved from a naïve brute-force implementation, to supporting sketches (which made computation of recall necessary), to running the experiments through an experimental file, to versioning of single components after a bug in the inner product avx computation was found (see #7), to running experiments with a fixed seed. 

To support such feedback loops, the database migration shown in https://github.com/Cecca/running-experiments/blob/master/report.hpp#L47-L157 gives a detailed
history of these changes. To keep track of the most recent results, an [SQL statement](https://github.com/Cecca/running-experiments/blob/master/report.hpp#L138-L146) gives an easy way to just select most recent versions, as used in https://github.com/Cecca/running-experiments/blob/master/evaluation/report.py#L19  and the other scripts in `evaluation/`.

It further allows to [skip](https://github.com/Cecca/running-experiments/blob/master/demo.cpp#L185-L189) experiments that have [already been carried out](https://github.com/Cecca/running-experiments/blob/master/report.hpp#L177-L215). For example, https://github.com/Cecca/running-experiments/commit/c724bac05f5d1bc117105eecba9f3319640dd368 fixed a serious bug in the distance computation code and increasing the version number allowed to just carry out all experiments in
https://github.com/Cecca/running-experiments/blob/master/experiments/sketch-test.yaml that were affected by this change.



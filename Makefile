# GIT_SHA := $(shell git rev-parse --short HEAD)
# GIT_DIRTY := $(shell git diff --exit-code > /dev/null || echo "*")
CXXFLAGS=-std=c++17 -Wall -O3 -march=native -I/usr/include/hdf5/serial 
LINKFLAGS=-lsqlite3 -lssl -lcrypto -lhdf5_cpp -lhdf5 -L/usr/lib/x86_64-linux-gnu/hdf5/serial/

demo: demo.cpp *.hpp toy_project/*.hpp
	${CXX} ${CXXFLAGS} -DGIT_REV='"${GIT_REV}"' demo.cpp -o demo ${LINKFLAGS}

.PHONY: clean
clean:
	${RM} demo

.PHONY: r-analysis
r-analysis:
	R -e 'rmarkdown::render("evaluation/evaluation.Rmd", output_file="evaluation-r.html")'


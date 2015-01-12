#!/bin/bash

#valgrind --suppressions=glog_dtor.sup --leak-check=full --show-leak-kinds=all ./../pilosim -O ../out/ -S 4 -H 2 -C 2 ../tests/square_full/topology.yaml --events ../tests/square_full/failures.yaml

valgrind --suppressions=glog_dtor.sup --leak-check=full --show-leak-kinds=all ./../pilosim -O ../out/ -S 8 -H 4 -C 4 ../tests/linkstatereq/topology.yaml --events ../tests/linkstatereq/failures.yaml

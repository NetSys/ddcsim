#!/bin/bash

# TODO some of the in parameters need to be synchornized with the simulator
# TODO allow user to set out params
# TODO set directory prefix for in params and out params

# In Parameters

RAW_BANDUSE=bandwidth_usage.txt
BANDUSE_PLOT_TEMPLATE=bandwidth_usage.gnuplot.template

HRTBT_LOG_PREFIX=log_
HRTBT_LOG_SUFFIX=.txt
HRTBT_PLOT_TEMPLATE=heartbeats.gnuplot.template

# Out Parameters

BANDUSE_PLOT=bandwidth_usage.png
HRTBT_PLOT_PREFIX=heartbeats_
HRTBT_PLOT_SUFFIX=.png

# Generate time series of total bandwidth usage

BANDUSE=bandwidth_usage.txt.tmp
python convert_bandwidth_usage.py < ${RAW_BANDUSE} > ${BANDUSE}
m4 --define=IN_FILE=${BANDUSE} ${BANDUSE_PLOT_TEMPLATE} | gnuplot  > ${BANDUSE_PLOT}
rm ${BANDUSE}

# Generate time series of bandwidth consumed by hearbeats per node

HRTBT_LOG_NAMES=${HRTBT_LOG_PREFIX}*${HRTBT_LOG_SUFFIX}
i=0
for f in $( ls ${HRTBT_LOG_NAMES} ); do
    m4 --define=IN_FILE=${f} --define=NODE=${i} ${HRTBT_PLOT_TEMPLATE} | \
	gnuplot > ${HRTBT_PLOT_PREFIX}${i}${HRTBT_PLOT_SUFFIX}
    ((++i))
done

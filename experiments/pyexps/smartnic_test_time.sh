#!/bin/bash

# Record start time
start_time=$(date +%s.%N)

# Run command here
# python3 run.py --verbose --force pyexps/smartnic_test_2nics.py > out/time.txt  # Experiment completed in 28.361418800 seconds.
python3 run.py --verbose --force pyexps/smartnic_test_5nics.py > out/time.txt  # Experiment completed in 74.283931704 seconds..
# Record end time
end_time=$(date +%s.%N)

# Calculate elapsed time
elapsed_time=$(echo "$end_time - $start_time" | bc)

echo "Experiment completed in $elapsed_time seconds."

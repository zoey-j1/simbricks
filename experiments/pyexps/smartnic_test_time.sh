#!/bin/bash

# Record start time
start_time=$(date +%s.%N)

# Run command here
# python3 run.py --verbose --force pyexps/smartnic_test.py > out/time.txt  # Experiment completed in 22.939597608 seconds.
# python3 run.py --verbose --force pyexps/smartnic_test_2nics.py > out/time.txt  # Experiment completed in 28.361418800 seconds.
# python3 run.py --verbose --force pyexps/smartnic_test_3nics.py > out/time.txt  # Experiment completed in 41.334653330 seconds.
# python3 run.py --verbose --force pyexps/smartnic_test_4nics.py > out/time.txt  # Experiment completed in 48.031642780 seconds.
# python3 run.py --verbose --force pyexps/smartnic_test_5nics.py > out/time.txt  # Experiment completed in 74.283931704 seconds.

python3 run.py --verbose --force pyexps/smartnic_test.py > out/time.txt  # 200 request per second time per request 1182 microseconds.
                                                                         # 400 request per second time per request 1213 microseconds.
                                                                         # 600 request per second time per request 2136 microseconds.
                                                                         # 800 request per second time per request 4550 microseconds.
                                                                         # 1000 request per second time per request 15067 microseconds.

# Record end time
end_time=$(date +%s.%N)

# Calculate elapsed time
elapsed_time=$(echo "$end_time - $start_time" | bc)

echo "Experiment completed in $elapsed_time seconds."

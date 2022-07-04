
#!/bin/bash

#make build

for pidx in $(seq 1 $1)
do
    echo run the $pidx with thread, each GB
    ./main  &
done

wait

echo finished!


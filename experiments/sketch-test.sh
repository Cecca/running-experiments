FN=$(basename $0)
SEED=4132
DATASET=glove-100-angular

 ./demo --dataset $DATASET --storage float_aligned --experiment-file $FN --seed $SEED # produce baseline
 ./demo --dataset $DATASET --storage i16_aligned --method simple --experiment-file $FN --seed $SEED
 ./demo --dataset $DATASET --storage i16_aligned --method avx2 --experiment-file $FN --seed $SEED
for recall in 0.001 0.1 0.2 0.5 0.7 0.9 0.99; do
    for alignment in i16_aligned float_aligned; do
        for method in simple avx2; do
             ./demo --dataset $DATASET --filter --storage i16_aligned --recall $recall --method $method --experiment-file $FN --seed $SEED
        done
    done
done



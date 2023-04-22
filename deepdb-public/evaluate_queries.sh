#export NUMEXPR_MAX_THREADS=32
dataset="geotweet"
qid=7
for partnum in 5 
do
for freqtype in ""
do
  for querytype in "subset" "superset"
  do
    for settype in ""
    do
python3 maqp.py --evaluate_cardinalities \
  --dataset $dataset \
  --database_name=$dataset \
  --partnum=$partnum \
  --target_path ../color_partition_${partnum}/deepdb/query_${qid}${freqtype}${settype}_${querytype}.csv \
  --ensemble_location ../color_partition_${partnum}/deepdb/ensemble_single_${dataset}_1000000.pkl \
  --query_file_location ../color_partition_${partnum}/${dataset}_query_${qid}${freqtype}${settype}_trans_settrie_${querytype}_deepdb.sql \
  --ground_truth_file_location ../color_partition_${partnum}/${dataset}_query_${qid}${freqtype}${settype}_trans_settrie_${querytype}_deepdbcard.csv \
  --pairwise_rdc_path ../color_partition_${partnum}/deepdb/pairwise_rdc.pkl
  done
    done
  done
done

export NUMEXPR_MAX_THREADS=32
dataset="wiki"
qid=1
for partnum in 10
do
for keepnum in 40000
do
  for freqtype in "" "_lowfreq" "_highfreq" 
  do
    for settype in "" 
    do
      for querytype in "subset" "superset"
      do
python3 maqp.py --evaluate_cardinalities \
  --dataset $dataset \
  --database_name=$dataset \
  --partnum=$partnum \
  --keepnum=$keepnum \
  --target_path ../partition_${partnum}_${keepnum}/deepdb/query_${qid}${freqtype}${settype}_${querytype}.csv \
  --ensemble_location ../partition_${partnum}_${keepnum}/deepdb/ensemble_single_${dataset}_1000000.pkl \
  --query_file_location ../partition_${partnum}_${keepnum}/${dataset}_query_${qid}${freqtype}${settype}_trans_settrie_${querytype}_deepdb.sql \
  --percentage_file_location ../partition_${partnum}_${keepnum}/${dataset}_query_${qid}${freqtype}${settype}_trans_settrie_${querytype}_approx_deepdbpercent.csv \
  --ground_truth_file_location ../partition_${partnum}_${keepnum}/${dataset}_query_${qid}${freqtype}${settype}_trans_settrie_${querytype}_deepdbcard.csv \
  --pairwise_rdc_path ../partition_${partnum}_${keepnum}/deepdb/pairwise_rdc.pkl
  done
      done
    done
  done
done

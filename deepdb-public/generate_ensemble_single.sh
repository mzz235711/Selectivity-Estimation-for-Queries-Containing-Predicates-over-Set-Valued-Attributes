dataset="gn"
for partnum in 3 5 10 20
do
python3 maqp.py --generate_ensemble \
  --dataset ${dataset} \
  --partnum ${partnum} \
  --keepnum -1 \
  --samples_per_spn 1000000 \
  --ensemble_strategy single \
  --hdf_path ../color_partition_${partnum}/deepdb \
  --ensemble_path ../color_partition_${partnum}/deepdb \
  --post_sampling_factor 10 \
	--rdc_threshold 0.3 \
  --pairwise_rdc_path ../color_partition_${partnum}/deepdb/pairwise_rdc.pkl
done
        

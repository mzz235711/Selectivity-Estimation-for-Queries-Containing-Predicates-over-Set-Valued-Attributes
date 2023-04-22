#export NUMEXPR_MAX_THREADS=32
dataset="geotweet"
for partnum in 5 
do
for keepnum in 5000
do
python3 maqp.py --generate_ensemble \
  --dataset $dataset \
  --partnum $partnum \
  --keepnum $keepnum \
  --samples_per_spn 1000000 \
  --ensemble_strategy single \
  --hdf_path ../partition_${partnum}_${keepnum}/deepdb \
  --ensemble_path ../partition_${partnum}_${keepnum}/deepdb \
  --post_sampling_factor 10 \
	--rdc_threshold 0.3 \
  --pairwise_rdc_path ../partition_${partnum}_${keepnum}/deepdb/pairwise_rdc.pkl
done
done
        

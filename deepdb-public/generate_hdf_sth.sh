dataset="geotweet"
for partnum in 5 
do
for keepnum in 5000
do
python3 maqp.py --generate_hdf \
    --dataset $dataset \
    --csv_seperator , \
    --csv_path ../dataset/$dataset \
    --hdf_path ../partition_${partnum}_${keepnum}/deepdb \
    --max_rows_per_hdf_file 100000000 \
    --partnum ${partnum} \
    --keepnum ${keepnum}
done
done

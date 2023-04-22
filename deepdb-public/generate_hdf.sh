dataset="gn"
for partnum in 3 5 10 20
do
python3 maqp.py --generate_hdf \
    --dataset ${dataset} \
    --csv_seperator , \
    --csv_path ../dataset/${dataset} \
    --hdf_path ../color_partition_${partnum}/deepdb \
    --max_rows_per_hdf_file 100000000 \
    --partnum ${partnum} \
    --keepnum -1 
done

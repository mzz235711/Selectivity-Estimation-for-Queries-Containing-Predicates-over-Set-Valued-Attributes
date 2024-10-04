partnum=$1
keepnum=$2
mkdir partition_${partnum}_${keepnum}
#g++ color_based_dis_part.cc -O3 -o color_based_dis_part
#./color_based_dis_part $partnum
cp color_partition_${partnum}/part.txt partition_${partnum}_${keepnum}/part.txt
g++ freq_set_map.cc -O3 -o freq_set_map
./freq_set_map ${partnum} ${keepnum}
g++ settrie_trans_query_hist.cc -O3 -o settrie_trans_query_hist
./settrie_trans_query_hist $partnum $keepnum 0 0 0
./settrie_trans_query_hist $partnum $keepnum 1 0 0 


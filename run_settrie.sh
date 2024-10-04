partnum=$1
mkdir color_partition_${partnum}
g++ color_based_dis_part.cc -O3 -o color_based_dis_part
./color_based_dis_part $partnum
g++ set_map.cc -O3 -o set_map
./set_map ${partnum}
g++ settrie_trans_query.cc -O3 -o settrie_trans_query
./settrie_trans_query $partnum 0 0 0
./settrie_trans_query $partnum 1 0 0 


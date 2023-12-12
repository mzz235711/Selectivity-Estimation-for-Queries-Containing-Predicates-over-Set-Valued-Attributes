g++ generate_idx.cc -O3 -o generate_idx
./generate_idx
g++ generate_graph.cc -O3 -o generate_graph
./generate_graph
mkdir graph_color
python3 graph_color_partition.py

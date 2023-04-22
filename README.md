# Selectivity-estimation-for-querying-set-valued-data
We use a sample dataset from Twitter which contains 10000 tuples. The running codes mainly contains 4 steps.
**Preprocessing**. Preprocess partitioning with graph coloring based method.
**ST**. Convert the data and query with ST method.
**STH**. Convert the data and query with ST-hist method.
**Estimation**. Estimate the converted query on Postgres, Neurocard or DeepDB.

##### Preprocessing
To partition the elements with graph coloring based method, you could run:
```bash
bash preprocessing.sh
```
This command will generate indices for elements with high-frequency-first order first (`generate_idx.cc`). Then generate graph based on the set-valued column (`generate_graph.cc`), and partition the graph with graph coloring method (`graph_color_partition.py`). The partition result is stored in the folder `./graph_color`.

##### ST
After preprocessing, you could run the following command to convert the set-valued data into 5 numeric subcolumns and convert the queries with **ST** method:
```bash
bash run_settrie.sh 5
```
It first greedy partition the elements based on graph coloring partition result (`color_based_dis_part.cc`). Then construct settrie based on the partitioning (`set_map.cc`). Finally convert the queries based on the settrie (`settrie_trans_query.cc`). The partition result, settrie and converted query are stored in the folder `./color_partition_5`. You can change the parameter of this command to change the number of subcolumns.

##### STH
Alternatively, you could run the following command to convert the set-valued data into 5 numeric subcolumns and convert the queries with **STH** method by keeping 500 nodes on each settrie:
```bash
bash run_settrie_hist.sh 5 500
```
The partition is same as the partition in **ST**, so in the script we directly copy the partition. If you don't partition first, please use `color_based_dis_part.cc` to partition the elements first. Then construct histogram based settrie based on the partitioning (`freq_set_map.cc`). Finally convert the queries based on the settrie (`settrie_trans_query_hist.cc`). The partition result, settrie and converted query are stored in the folder `./partition_5_500`. You can change the parameter of this command to change the number of subcolumns and the number of kept nodes on each settrie.



##### Estimation
If you want to do estimation with **ST** on any estimator, you should run the following command first to convert the dataset:
```bash
python3 clique_trans_table.py --partnum 5
```
Alternatively, if you want to do estimation with **STH**, please modify the line 20 in `clique_trans_table.py` first to change the folder, then run the following command:
```bash
python3 clique_trans_table.py --partnum 5 --keepnum 500
```
To estimate with Postgres, you can modify `generate_sql.py` and run it to populate the converted dataset into Postgres. You need the package `psycopg2` to run it. Then you can modify the folder path and parameters in `postgres_est_new.py` and run it to estimate.

If you want to estimate with Neurocard, we have modified the source code of Neurocard to support our method. You can refer to [Neurocard](https://github.com/neurocard/neurocard) to insert new table and run queries. We also provide the running scripts, but you need to modify some folder path and parameters in `datasets.py` and `run.py`.

If you want to estimate with Neurocard, we have modified the source code of DeepDB to support our method. You can refer to [DeepDB](https://github.com/DataManagementLab/deepdb-public) to insert new table and run queries. We also provide the running scripts, but you need to modify some folder path and parameters in `maqp.py` and `schemas/geotweet/schema.py`.

#### Incremental ST and STH
The incremental **ST** and **STH** are in the folder `incremental`. The running procedure are similar with the base methods.

import numpy as np
import psycopg2
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument('--partnum', default=5, type=int)
parser.add_argument('--keepnum', default=5000, type=int)
args = parser.parse_args()

current_path = os.getcwd()
conn = psycopg2.connect(database='geotweet', port='5433', user='anonymous')
conn.autocommit = True
cursor = conn.cursor()
partition_num = args.partnum 
keep_num = args.keepnum
cursor.execute("drop table if exists geotweet_settrie_{};".format(partition_num, keep_num))
conn.commit()
sql = ""
sql += "create table geotweet_settrie_{}(".format(partition_num, keep_num)
sql += "createtime integer,"
sql += "lat double precision,"
sql += "lon double precision,"
sql += "country character varying,"
for i in range(partition_num):
    sql += "tags{} integer,".format(i)
sql += "tags_lfword integer);"
cursor.execute(sql)
conn.commit()
sql = "copy geotweet_settrie_{} from \'{}/color_partition_{}/geotweet_approx_clique.csv\' DELIMITER \',\' HEADER CSV;".format(partition_num, current_path, partition_num)
cursor.execute(sql)
conn.commit()

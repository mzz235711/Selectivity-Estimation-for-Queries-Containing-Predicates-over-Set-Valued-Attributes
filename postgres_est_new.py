import psycopg2
import time
import numpy as np
import argparse
import os
import csv
import sys
import json

csv.field_size_limit(sys.maxsize)
parser = argparse.ArgumentParser()
parser.add_argument('--dataset', default='geotweet', type=str)
parser.add_argument('--partnum', default=5, type=int)
parser.add_argument('--keepnum', default=500, type=int)
parser.add_argument('--querytype', default='superset', type=str)
parser.add_argument('--hist', default=0, type=int)
parser.add_argument('--freqtype', default='', type=str)
parser.add_argument('--settype', default='', type=str)
parser.add_argument('--avi', default=True, type=bool)
args = parser.parse_args()

dataset = args.dataset
conn = psycopg2.connect(database='geotweet', port='5433', user='anonymous')
conn.autocommit = True
cursor = conn.cursor()
partition_num = args.partnum 
keep_num = args.keepnum
query_type = args.querytype
hist = args.hist
freq_type = args.freqtype
set_type = args.settype
avi = args.avi
qid = 1 
catcols = ['geotweet.country']
setcol = 'tags'
offset = 5 
setcols = []
if hist == 0:
    tablename = '{}_settrie_{}'.format(dataset, partition_num)
else:
    tablename = '{}_settrie_{}_{}'.format(dataset, partition_num, keep_num)
for i in range(partition_num):
    setcols.append("geotweet.tags{}".format(i))
tables = [tablename]
if avi:
    query = "drop statistics if exists {}_stts".format(tablename)
    cursor.execute(query)
    conn.commit()
else: 
    query = "create statistics if not exists {}_stts (dependencies) on ".format(tablename)
    for i in range(partition_num):
        query += setcol + str(i) + ", "
    query = query[:-2] + " from " + tablename + ";"
    cursor.execute(query)
    conn.commit()
for t in tables:
    cursor.execute('analyze ' + t + ';')
    conn.commit()
histogram = []
histogram_nums = []
common_values = []
common_freqs = []
#common_freqs = []
hist_freqs = []
correlation = {}
col2id = {}
nullval = [0] * partition_num
for i in range(partition_num):
    col2id["tags" + str(i)] = i + offset
for i in range(partition_num):
    query = "select histogram_bounds from pg_stats where tablename='{}' and attname='{}{}'".format(tablename, setcol, i)
    cursor.execute(query)
    result = cursor.fetchall()
    result = result[0][0].split('{')[1].split('}')[0]
    bounds = [int(i) for i in result.split(',')]
    histogram.append(np.array(bounds, dtype=int))
    buckets_num = len(bounds) - 1
    histogram_nums.append(buckets_num)
    query = "select most_common_vals, most_common_freqs from pg_stats where tablename='{}' and attname='{}{}'".format(tablename, setcol, i)
    cursor.execute(query)
    result = cursor.fetchall()
    common_value = result[0][0].split('{')[1].split('}')[0]
    common_value = [int(i) for i in common_value.split(',')]
    indices = np.argsort(common_value)
    common_freq = result[0][1]
    common_values.append(np.array(common_value, dtype=int)[indices])
    common_freqs.append(np.array(common_freq, dtype=float)[indices])
    #common_freqs.append({common_value[i]:common_freq[i] for i in range(len(common_value))})
    hist_freq = np.sum(common_freq)
    hist_freqs.append(hist_freq)
if avi is False:
    query = "select dependencies from pg_stats_ext where tablename='{}'".format(tablename)
    cursor.execute(query)
    result = cursor.fetchall()[0][0]
    correlation = json.loads(result)
#for freq_str in frequencies:
#    print(freq_str)
#    k = freq_str.split(':')[0]
#    val = float(freq_str.split(':')[1])
#    correlation[k] = val
#print(frequencies)
for i in range(partition_num):
    query = "select max(tags{}) from {};".format(i, tablename)
    cursor.execute(query)
    result = cursor.fetchall()
    nullval[i] = result[0][0]

print("query_type: ", query_type)
print("freq_type: ", freq_type)
print("set_type: ", set_type)
if hist == 0:
    sql_file = open('./color_partition_{}/{}_query_{}{}{}_trans_settrie_{}.sql'.format(partition_num, dataset, qid, freq_type, set_type, query_type), 'r')
    csv_file = open('./color_partition_{}/{}_query_{}{}{}_trans_settrie_{}_postgres.csv'.format(partition_num, dataset, qid, freq_type, set_type, query_type), 'r')
else:
    sql_file = open('./partition_{}_{}/{}_query_{}{}{}_trans_settrie_{}_approx.sql'.format(partition_num, keep_num, dataset, qid, freq_type, set_type, query_type), 'r')
    csv_file = open('./partition_{}_{}/{}_query_{}{}{}_trans_settrie_{}_approx_postgres.csv'.format(partition_num, keep_num, dataset, qid, freq_type, set_type, query_type), 'r')
#sql_file = open('./gn_query_1_{}.sql'.format(query_type), 'r')
#sql_file = open('./gn_query_1_{}.sql'.format(query_type), 'r')
#csv_file = open('./gn_query_1_{}.csv'.format(query_type), 'r')
sqls = sql_file.readlines()
#csvs = csv_file.readlines()
dur_time = 0
errors = []
max_time = 0
min_time = 2e30 
each_time = []
total_card = 2283724
large_num = 0
if hist == 0:
    if not os.path.exists('./color_partition_{}/result'.format(partition_num)):
        os.makedirs('./color_partition_{}/result'.format(partition_num))
    result_file = open('./color_partition_{}/result/{}_query_5{}{}_{}_result_postgres.csv'.format(partition_num, dataset, freq_type, set_type, query_type), 'w')
else:
    if not os.path.exists('./partition_{}_{}/result'.format(partition_num, keep_num)):
        os.makedirs('./partition_{}_{}/result'.format(partition_num, keep_num))
    result_file = open('./partition_{}_{}/result/{}_query_5{}{}_{}_result_postgres.csv'.format(partition_num, keep_num, dataset, freq_type, set_type, query_type), 'w')
csvlines = list(list(rec) for rec in csv.reader(csv_file, delimiter='#'))
for i in range(len(csvlines)):
    percent = 1
    nullcard = 0
    start_time = time.time()
    '''
    sql = sqls[i]
    sql_all_null = sql.split(';')
    sql = sql_all_null[0] + ';'
    if hist == 1:
        percent = float(sql_all_null[1])
    if query_type == "subset":
        nullsql = sql_all_null[-2] + ";"
        query = nullsql.replace('count(*)', '*')
        query = 'explain ' + query
        cursor.execute(query)
        res = cursor.fetchall()
        nullcard = int(res[0][0].split('=')[2].split(' ')[0])
    '''
    csvline = csvlines[i]
    true_card = int(csvline[-1])
    sql = ""
    if hist == 1:
        percent = float(csvline[3])
    sql = "explain select * from {} as geotweet where ".format(tablename)
    predicates = csvline[2]
    predicates = csv.reader([predicates])
    predicates = next(predicates)
    set_sels = np.array([1] * partition_num, dtype=float)
    allcol_nullsel = 1
    allcol_nonullsel = 0 
    final_selectivity = 1
    # Set column which already met
    current_setid = []
    for i in range(0, len(predicates), 3):
        col = predicates[i]
        op = predicates[i + 1]
        val = predicates[i + 2]
        if col in catcols:
            sql += (col + op + '\'' + val + '\' and ')
        elif col not in setcols:
            sql += (col + op + val + " and ")
        else:
            #print(col)
            val = val[1:-1]
            setid = int(col[13:] )
            nullselectivity = 0
            nonullselectivity = 0
            vals = [int(j) for j in val.split(',')]
            #forsearch = []
            #for v in common_freqs[setid]:
            #    for j in range(0, len(vals), 2):
            #        if v >= vals[j] and v < vals[j + 1]:
            #            selectivity += common_freqs[setid][v]
            #            break
                #if v in common_freqs[setid]:
                #    selectivity += common_freqs[setid][v]
                #else:
                #    forsearch.append(v)
            searchadd = np.array([0] * histogram_nums[setid], dtype = int)
            search_index = np.minimum((np.searchsorted(histogram[setid], vals) - 1), histogram_nums[setid] - 1)
            common_idx = 0 
            for j in range(0, len(search_index), 2):
                lb = search_index[j]
                ub = search_index[j + 1]
                cv = common_values[setid][common_idx]
                while cv < vals[j] and common_idx < len(common_values[setid]) :
                    common_idx += 1
                    cv = common_values[setid][common_idx]
                if cv >= vals[j] and cv < vals[j + 1]:
                    if (cv == nullval[setid]):
                        nullselectivity += common_freqs[setid][common_idx]
                    else:
                        nonullselectivity += common_freqs[setid][common_idx]
                #for k, v in enumerate(common_values[setid]):
                #    if v >= vals[j] and v < vals[j + 1]:
                #        selectivity += common_freqs[setid][k]
                if search_index[j] == search_index[j + 1]:
                    searchadd[lb] += (vals[j + 1] - vals[j])
                else:
                    #assert histogram[setid][lb + 1] >= vals[j]
                    searchadd[lb] += (histogram[setid][lb + 1] - vals[j])
                    for idx in range(lb + 1, ub):
                        searchadd [idx] += histogram[setid][idx + 1] - histogram[setid][idx]
                    #assert vals[j + 1] >= histogram[setid][ub]
                    searchadd[ub] += (vals[j + 1] - histogram[setid][ub])
            #for idx in search_index:
            #    searchadd[idx] += 1
            #print(selectivity)
            #print(searchadd)
            #print(histogram[setid][1:] - histogram[setid][:-1])
            #print(histogram[setid])
            #exit()
            nonullselectivity += np.sum(searchadd / (histogram[setid][1:] - histogram[setid][:-1])) / histogram_nums[setid]
            if avi is False:
                if len(current_setid) == 0:
                    allcol_nonullsel = nonullselectivity
                    allcol_nullsel = nullselectivity
                else:
                    corr_key = str(current_setid[0] + offset)
                    for i in range(1, len(current_setid)):
                        corr_key += ", {}".format(current_setid[i] + offset)
                    corr_key += (" => " + str(setid + offset))
                    corr_value = correlation[corr_key]
                    allcol_nonullsel = allcol_nonullsel * corr_value + allcol_nullsel * nonullselectivity
                    allcol_nullsel *= nullselectivity
            else:
                final_selectivity *= (nonullselectivity + nullselectivity)
                allcol_nullsel *= nullselectivity
            
            current_setid.append(setid)
            current_setid.sort()
            #print(selectivity)
            #set_sels[setid] *= selectivity
    #print(set_sels)
    #query = sql.replace('count(*)', '*')
    #query = 'explain ' + query
    #query = sql
    #cursor.execute(query)
    if avi is False:
        final_selectivity = allcol_nonullsel + allcol_nullsel
        #final_selectivity = allcol_nonullsel
    #else:
        #final_selectivity -= allcol_nullsel
    if sql[-2] == 'd':
        sql = sql[:-5]
    else:
        sql = sql[:-7]
    cursor.execute(sql)
    res = cursor.fetchall()
    end_time = time.time()
    dur_time += (end_time - start_time)
    if (end_time - start_time) > max_time:
        max_time = end_time - start_time
    if (end_time - start_time) < min_time:
        min_time = end_time - start_time
    #avg_lenth = len(sqls[i].split(','))
    each_time.append((end_time - start_time) * 1000)
    est_card = int(int(res[0][0].split('=')[2].split(' ')[0]) * percent * final_selectivity)
    #print(est_card, nullcard, est_card - nullcard)
    #est_card -= nullcard
    est_card = max(1, est_card)
    true_card = max(1, true_card)
    err = max(true_card / est_card, est_card / true_card)
    errors.append(err)
    if true_card > est_card:
        large_num += 1
    result_file.write(str(est_card) + '\n')
    #print(est_card, true_card)
    #print((end_time - start_time) * 1000)
result_file.close()
errors.sort()
print('max: ', np.max(errors))
print('99th: ', np.quantile(errors, 0.99))
print('95th: ', np.quantile(errors, 0.95))
print('median: ', np.quantile(errors, 0.5))
print('mean: ', np.mean(errors))
print('time_ms: ',dur_time * 1000 / len(sqls) )
print("max time_ms: ", max_time * 1000)
print("min time_ms: ", min_time * 1000)
print("large_num: ", large_num)
each_time = np.array(each_time)
each_time = np.sort(each_time)
np.set_printoptions(suppress=True, precision=4)
#np.savetxt("test_time_origin.csv", each_time, fmt='%.04f')
statefile = open("result.txt", "a")
statefile.write('dataset: {}\n'.format(dataset))
statefile.write('query_type: {}\n'.format(query_type))
statefile.write('freq_type: {}\n'.format(freq_type))
statefile.write('set_type: {}\n'.format(set_type))
statefile.write('partition_num: {}\n'.format(partition_num))
statefile.write('keep_num: {}\n'.format(keep_num))
statefile.write('hist: {}\n'.format(hist))
statefile.write('max: {}\n'.format(np.max(errors)))
statefile.write('99th: {}\n'.format(np.quantile(errors, 0.99)))
statefile.write('95th: {}\n'.format(np.quantile(errors, 0.95)))
statefile.write('median: {}\n'.format(np.quantile(errors, 0.5)))
statefile.write('mean: {}\n'.format(np.mean(errors)))
statefile.write('time_ms: {}\n\n'.format(dur_time * 1000 / len(sqls)))



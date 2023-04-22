"""Utility functions."""

import ast
from collections import defaultdict
import csv
import sys
import common


def _get_table_dict(tables):
    table_dict = {}
    for t in tables:
        split = t.split(' ')
        if len(split) > 1:
            # Alias -> full table name.
            table_dict[split[1]] = split[0]
        else:
            # Just full table name.
            table_dict[split[0]] = split[0]
    return table_dict


def _get_join_dict(joins, table_dict, use_alias_keys):
    join_dict = defaultdict(set)
    for j in joins:
        ops = j.split('=')
        op1 = ops[0].split('.')
        op2 = ops[1].split('.')
        t1, k1 = op1[0], op1[1]
        t2, k2 = op2[0], op2[1]
        if not use_alias_keys:
            t1 = table_dict[t1]
            t2 = table_dict[t2]
        join_dict[t1].add(k1)
        join_dict[t2].add(k2)
    return join_dict


def _try_parse_literal(s):
    try:
        ret = ast.literal_eval(s)
        # IN needs a tuple operand
        # String equality needs a string operand
        if isinstance(ret, tuple) or isinstance(ret, str):
            return ret
        return s
    except:
        return s


def _get_predicate_dict(predicates, table_dict):
    predicates = [predicates[x:x + 3] for x in range(0, len(predicates), 3)]
    predicate_dict = {}
    for p in predicates:
        split_p = p[0].split('.')
        table_name = table_dict[split_p[0]]
        if table_name not in predicate_dict:
            predicate_dict[table_name] = {}
            predicate_dict[table_name]['cols'] = []
            predicate_dict[table_name]['ops'] = []
            predicate_dict[table_name]['vals'] = []
        predicate_dict[table_name]['cols'].append(split_p[1])
        predicate_dict[table_name]['ops'].append(p[1])
        predicate_dict[table_name]['vals'].append(_try_parse_literal(p[2]))
    return predicate_dict

def LoadPercentage(file_path):
    if file_path:
        percentage_file = open(file_path, 'r')
        percentlines = percentage_file.readlines()
        percents = []
        for p in percentlines:
            ps = p.split(';')
            percent = {}
            for name_p in ps:
                name = name_p.split(':')[0]
                perc = name_p.split(':')[1]
                percent[name] = [float(i) for i in perc.split(',')]
            percents.append(percent)
    else:
        percents = None
    return percents

def JobToQuery(csv_file, use_alias_keys=True):
    """Parses custom #-delimited query csv.

    `use_alias_keys` only applies to the 2nd return value.
    If use_alias_keys is true, join_dict will use aliases (t, mi) as keys;
    otherwise it uses real table names (title, movie_index).

    Converts into (tables, join dict, predicate dict, true cardinality).  Only
    works for single equivalency class.
    """
    maxInt = sys.maxsize

    while True:
        # decrease the maxInt value by factor 10
        # as long as the OverflowError occurs.
    
        try:
            csv.field_size_limit(maxInt)
            break
        except OverflowError:
            maxInt = int(maxInt/10)
    queries = []
    with open(csv_file) as f:
        data_raw = list(list(rec) for rec in csv.reader(f, delimiter='#'))
        for i, row in enumerate(data_raw):
            #print("XXXXXXXXXXXXXXXXXXXX ", i)
            reader = csv.reader(row)  # comma-separated
            table_dict = _get_table_dict(next(reader))
            join_dict = _get_join_dict(next(reader), table_dict, use_alias_keys)
            predicate_dict = _get_predicate_dict(next(reader), table_dict)
            true_cardinality = int(next(reader)[0])
            queries.append((list(table_dict.values()), join_dict,
                            predicate_dict, true_cardinality))

        return queries

def ParseQuery(table, col, op, val, concat_table, query_cols, query_ops, query_vals, pad='ZZZ'):
    if table != 'ukpoi' and table != 'gn' and table != 'geotweet' and table != 'bookc' and table != 'livejournal' and table != 'wiki':
        col_name = common.JoinTableAndColumnNames(table, col)
    else:
        col_name = col
    if col_name in concat_table.word2col:
        word2col = concat_table.word2col[col_name]
        query_keywords = val.split('|')
        if op == '>=':
            colsid = []
            for w in query_keywords:
                if w not in word2col:
                    continue
                wid = word2col[w]
                if wid in colsid:
                    continue
                colsid.append(wid)
                w_column_name = col_name + str(wid)
                #column = concat_table.columns[word2col[w]]
                column = concat_table.columns[concat_table.ColumnIndex(w_column_name)]
                cast_fn = column.all_distinct_values.dtype.type
                query_cols.append(column)
                query_ops.append('=')
                query_vals.append(cast_fn(w))
            #w_column_name = col_name + str(0)
            #column = concat_table.columns[concat_table.ColumnIndex(w_column_name)]
            #cast_fn = column.all_distinct_values.dtype.type
            #query_cols.append(column)
            #query_ops.append('=')
            #query_vals.append(cast_fn('zzz'))

        elif op == '<=' or 'IN':
            colnum = concat_table.keyword_column_num[col_name]
            colsid = [[] for _ in range(concat_table.keyword_column_num[col_name])]
            for w in query_keywords:
                if w not in word2col:
                    continue
                wid = word2col[w]
                colsid[wid].append(w)
            for i in range(colnum):
                w_column_name = col_name + str(i)
                column = concat_table.columns[concat_table.ColumnIndex(w_column_name)]
                if pad in column.all_distinct_values:
                    colsid[i].append(pad)
                #column = concat_table.columns[i]
                cast_fn = column.all_distinct_values.dtype.type
                query_cols.append(column)
                if len(colsid[i]) == 1:
                    query_ops.append('=')
                    query_vals.append(cast_fn(colsid[i][0]))
                else:
                    query_ops.append('IN')
                    query_vals.append(type(colsid[i])(map(cast_fn, colsid[i])))
            lfword_column_name = col_name + '_lfword'
            column = concat_table.columns[concat_table.ColumnIndex(lfword_column_name)]
            query_cols.append(column)
            query_ops.append('=')
            query_vals.append('0')


    else:
        if table != 'ukpoi' and table != 'gn' and table != 'geotweet' and table != 'bookc' and table != 'livejournal' and table != 'wiki':
            column = concat_table.columns[concat_table.TableColumnIndex(table, col)]
        else:
            column = concat_table.columns[concat_table.ColumnIndex(col)]
        cast_fn = column.all_distinct_values.dtype.type
        query_cols.append(column)
        query_ops.append(op)
        if isinstance(val, (list, set, tuple)):
            qv = type(val)(map(cast_fn, val))
        else:
            qv = cast_fn(val)
        query_vals.append(qv)





def UnpackQueries(concat_table, queries):
    """Converts custom query representation to (cols, ops, vals)."""
    converted = []
    true_cards = []
    for q in queries:
        tables, join_dict, predicate_dict, true_cardinality = q
        # All predicates in a query (an AND of these).
        query_cols, query_ops, query_vals = [], [], []

        skip = False
        # A naive impl of "is join graph subset of another join" check.
        if tables[0] != 'ukpoi' and tables[0] != 'gn' and tables[0] != 'geotweet' and tables[0] != 'bookc' and tables[0] != 'livejournal' and tables[0] != 'wiki':
            for table in tables:
                if table not in concat_table.table_names:
                    print('skipping query')
                    skip = True
                    break
                # Add the indicators.
                idx = concat_table.ColumnIndex('__in_{}'.format(table))
                query_cols.append(concat_table.columns[idx])
                query_ops.append('=')
                query_vals.append(1)

        if skip:
            continue

        for table, preds in predicate_dict.items():
            cols = preds['cols']
            ops = preds['ops']
            vals = preds['vals']
            assert len(cols) == len(ops) and len(ops) == len(vals)
            for c, o, v in zip(cols, ops, vals):
                ParseQuery(table, c, o, v, concat_table, query_cols, query_ops, query_vals)
                #column = concat_table.columns[concat_table.TableColumnIndex(
                #    table, c)]
                #query_cols.append(column)
                #query_ops.append(o)
                # Cast v into the correct column dtype.
                #cast_fn = column.all_distinct_values.dtype.type
                # If v is a collection, cast its elements.
                #if isinstance(v, (list, set, tuple)):
                #    qv = type(v)(map(cast_fn, v))
                #else:
                #    qv = cast_fn(v)
                #query_vals.append(qv)

        converted.append((query_cols, query_ops, query_vals))
        true_cards.append(true_cardinality)
        '''
        csvline = 'geotweet_trans_20##'
        sqlline = 'select count(*) from geotweet_trans_20 where '
        for i in range(len(query_cols)):
            if query_ops[i] != 'IN':
                csvline += ('geotweet_trans_20.' + query_cols[i].name + ',' + query_ops[i] + ',' + str(query_vals[i]) + ',')
                if type(query_vals[i]) is str:
                    sqlline += ('geotweet_trans_20.' + query_cols[i].name + query_ops[i] + '\'' + query_vals[i] + '\' AND ')
                else:
                    sqlline += ('geotweet_trans_20.' + query_cols[i].name + query_ops[i] + str(query_vals[i]) + ' AND ')
            else:
                csvline += ('geotweet_trans_20.' + query_cols[i].name + ',' + query_ops[i] + ',')
                sqlline += (query_cols[i].name + ' ' + query_ops[i] + ' ' + '(')
                for w in query_vals[i]:
                    csvline += (w + '|')
                    sqlline += ('\'' + w + '\',')
                csvline = csvline[:-1] + ','
                sqlline = sqlline[:-1] + ') AND '
        csvline = csvline[:-1] + '#' + str(true_cardinality) + '\n'
        sqlline = sqlline[:-5] + ';\n'
        csv_file.write(csvline)
        sql_file.write(sqlline)
        '''
    return converted, true_cards


def InvertOrder(order):
    if order is None:
        return None
    # 'order'[i] maps nat_i -> position of nat_i
    # Inverse: position -> natural idx.  This it the "true" ordering -- it's how
    # heuristic orders are generated + (less crucially) how Transformer works.
    nin = len(order)
    inv_ordering = [None] * nin
    for natural_idx in range(nin):
        inv_ordering[order[natural_idx]] = natural_idx
    return inv_ordering


def HumanFormat(num):
    magnitude = 0
    while abs(num) >= 1000:
        magnitude += 1
        num /= 1000.0
    return '%.2f%s' % (num, ['', 'K', 'M', 'G', 'T', 'P'][magnitude])

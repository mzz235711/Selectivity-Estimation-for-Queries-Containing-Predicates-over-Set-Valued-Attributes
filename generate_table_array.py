import pandas as pd
import re
import psycopg2 as pg
conn = pg.connect(dbname='geotweet', user='anonymous', port='5433')
conn.set_session(autocommit=True)
cursor = conn.cursor()
df = './dataset/geotweet/geotweet.csv'
cols=['createtime', 'lat', 'lon','country', 'tags']
df = pd.read_csv(df, usecols=cols, delimiter=',')
df.fillna('', inplace=True)
#lines = df['title'].value_counts(dropna=False).index.values
#texts = []
#for i, line in enumerate(lines):
#    line = line.lower()
#    lines[i] = line
#    text = re.findall('[a-zA-Z]+', line)
#    texts.append(text)
#outsql = open('create_gn.sql', 'w')
#outsql.write('create extension hstore;\n')
#cursor.execute('create extension hstore;')
sqlstr = 'drop table if exists geotweet;'
cursor.execute(sqlstr)
sqlstr = 'CREATE TABLE geotweet (createtime integer, lat double precision, lon double precision, country char varying(50), tags text[]);'
#outsql.write(sqlstr)
cursor.execute(sqlstr)
col_num = len(df.columns)
for i in range(len(df)):
    sqlstr = 'insert into geotweet values('
    for j in range(col_num):
        col = df.columns[j]
        if col != 'tags':
            if isinstance(df[col][i], str):
                if df[col][i] == '':
                    sqlstr += 'NULL,'
                else:
                    sqlstr += ('\'' + df[col][i] + '\',')
            else:
                if df[col][i] == '':
                    sqlstr += 'NULL,'
                else:
                    sqlstr += (str(df[col][i]) + ',')
        else:
            sqlstr += '\'{'
            line = df[col][i]
            line = line.lower()
            #text = re.findall('[a-zA-Z]+', line)
            text = line.split(',')
            for word in text:
                sqlstr += ('"' + word + '",')
            if len(text) == 0:
                sqlstr = sqlstr + '}\','
            else:
                sqlstr = sqlstr[:-1] + '}\','
    sqlstr = sqlstr[:-1] + ');'
    #print(sqlstr)
    #break
    cursor.execute(sqlstr)





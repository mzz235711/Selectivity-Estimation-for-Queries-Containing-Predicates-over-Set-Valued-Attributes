from ensemble_compilation.graph_representation import SchemaGraph, Table

def gen_gn_schema(csv_path, partnum=20, keepnum=-1):
    schema = SchemaGraph()
    if keepnum == -2:
        partnum = 114
    attributes = []
    attributes.extend(['FEATURE_CLASS','STATE_ALPHA','COUNTY_NAME','PRIM_LAT_DEC','PRIM_LONG_DEC','ELEV_IN_M'])
    for i in range(partnum):
        attributes.append('FEATURE_NAME' + str(i))
    attributes.append('FEATURE_NAME_lfword')
    if keepnum == -1:
        csvfile = '../color_partition_{}/gn_approx_clique.csv'.format(partnum)
    elif keepnum == -2:
        csvfile = '../graph_color/gn_approx_clique.csv'
    else:
        csvfile = '../partition_{}_{}/gn_approx_clique.csv'.format(partnum, keepnum)
    schema.add_table(Table('gn', attributes=attributes, csv_file_location=csvfile,
                     table_size=2283724))
    return schema

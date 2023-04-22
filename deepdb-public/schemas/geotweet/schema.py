from ensemble_compilation.graph_representation import SchemaGraph, Table

def gen_geotweet_schema(csv_path, partnum=10, keepnum=-1):
    schema = SchemaGraph()
    if keepnum == -2:
        partnum = 832
    attributes = ['createtime', 'lat', 'lon', 'country']
    for i in range(partnum):
        attributes.append('tags' + str(i))
    attributes.append('tags_lfword')
    if keepnum == -1:
        csvfile = '../color_partition_{}/geotweet_approx_clique.csv'.format(partnum)
    elif keepnum == -2:
        csvfile = '../geotweet/new_idx_related/graph_color/geotweet_approx_clique.csv'
    else:
        csvfile = '../geotweet/new_idx_related/partition_{}_{}/geotweet_approx_clique.csv'.format(partnum, keepnum)
    schema.add_table(Table("geotweet", attributes=attributes, csv_file_location=csvfile,
                     table_size=7655912))
    return schema

from ensemble_compilation.graph_representation import SchemaGraph, Table

def gen_wiki_schema(csv_path, partnum=10, keepnum=-1):
    schema = SchemaGraph()
    if keepnum == -2:
        partnum = 1231
    attributes = []
    for i in range(partnum):
        attributes.append('abstract' + str(i))
    attributes.append('abstract_lfword')
    if keepnum == -1:
        csvfile = '../color_partition_{}/wiki_approx_clique.csv'.format(partnum)
    elif keepnum == -2:
        csvfile = '../graph_color/wiki_approx_clique.csv'
    else:
        csvfile = '../partition_{}_{}/wiki_approx_clique.csv'.format(partnum, keepnum)
    schema.add_table(Table("wiki", attributes=attributes, csv_file_location=csvfile,
                     table_size=5313758))
    return schema

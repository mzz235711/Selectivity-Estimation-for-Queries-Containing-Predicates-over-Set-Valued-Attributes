import networkx as nx
import numpy as np

g = nx.Graph()
idxfile = open("idx.csv", "r")
idxlines =idxfile.readlines()
node_num = len(idxlines)
for i in range(node_num):
    g.add_node(i)
graphfile = open("geotweet.mtx", "r")
lines = graphfile.readlines()
for i in range(2, len(lines)):
    line = lines[i]
    u = int(line.split(" ")[0])
    v = int(line.split(" ")[1])
    g.add_edge(u, v)
part = nx.greedy_color(g) 
nodes = list(g.nodes)
partfile = open("graph_color/part.txt", "w")
partnum = 0
for v in nodes:
    partfile.write("{}\n".format(part[v]))
    if (part[v] > partnum):
        partnum = part[v]
print("partnum: {}".format(partnum + 1))


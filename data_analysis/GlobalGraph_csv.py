import math
import networkx as nx
import matplotlib.pyplot as plt

def printgraph():
    graph_reader = open("../../LCL_data/cycle/graph_.csv")
    graph_lines = graph_reader.readlines(400_000_000)
    Head = []
    Tails= []
    for line in graph_lines:
        numlist = line.split()
        Head.append(int(numlist[0]))
        if(len(numlist)>1):
            Tails.append(numlist[1])
        else:
            Tails.append("")

    size = len(Head)
    G = nx.DiGraph()
    for index in range(0, size):
        head = Head[index]
        tails = Tails[index].split(",")
        for tail in tails:
            if tail != "":
                G.add_edges_from([(head, int(tail))])
    pos = nx.circular_layout(G)
    for e in G.edges():
        G[e[0]][e[1]]['color'] = 'black'
    loops = list(nx.simple_cycles(G))
    loop_nums = len(loops)
    dim = math.ceil(math.sqrt(loop_nums))
    #### print Global Graph
    plt.subplot(dim+1,1,1)
    plt.title("Global Graph")
    lnodes= set()
    for loop in loops:
        for i in range(0, len(loop) - 1):
            lnodes.add(loop[i])
            G[loop[i]][loop[i + 1]]['color'] = '#F6B352'
        G[loop[len(loop) - 1]][loop[0]]['color'] = '#F6B352'
        lnodes.add(loop[len(loop)-1])
    # Store in a list to use for drawing
    edge_color_list = [G[e[0]][e[1]]['color'] for e in G.edges()]
    node_colors = ["#F6B352" if n in lnodes else "#30A9DE" for n in G.nodes()]
    nx.draw(G, pos, with_labels=True, edge_color=edge_color_list,node_color=node_colors, node_size=500)
    #### print loop
    for index in range(0,loop_nums):
        plt.subplot(dim+1, dim, index + dim+1)
        plt.title("Loop is:" + repr(loops[index]))
        print("len is: " + repr(len(loops[index])))
        print("Loop is:" + repr(loops[index]))

        G=nx.DiGraph()
        for i in range(0,len(loops[index])-1):
            G.add_edge(loops[index][i],loops[index][i+1])
        G.add_edge(loops[index][len(loops[index])-1],loops[index][0])
        nx.draw(G,pos,with_labels=True,node_color="#F6B352",edge_color="#F6B352",node_size=500)

printgraph()
print("here")
plt.show()

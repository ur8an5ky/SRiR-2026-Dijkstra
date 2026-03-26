import networkx as nx
import matplotlib.pyplot as plt
import os

def load_matrix(filepath):
    """Loads an adjacency matrix from a text file (tab-separated values)."""
    matrix = []
    with open(filepath, 'r') as file:
        for line in file:
            # Ignore empty lines
            if line.strip():
                # Split by tabs and cast to int
                row = [int(val) for val in line.strip().split('\t')]
                matrix.append(row)
    return matrix

def build_graph(matrix):
    """Builds a directed graph based on the adjacency matrix."""
    # We use DiGraph because we established the graph is directed
    G = nx.DiGraph()
    
    num_nodes = len(matrix)
    # Add nodes (from 0 to num_nodes - 1)
    G.add_nodes_from(range(num_nodes))
    
    # Add edges
    for i in range(num_nodes):
        for j in range(num_nodes):
            weight = matrix[i][j]
            # -1 indicates no edge
            if weight != -1:
                G.add_edge(i, j, weight=weight)
                
    return G

def save_graph_image(G, output_filepath):
    """Draws the graph and saves it to a PNG file."""
    # Set image size
    plt.figure(figsize=(8, 6))
    
    # Use spring_layout for reasonably nice node positioning
    # Set seed so the graph looks the same on every run
    pos = nx.spring_layout(G, seed=42)
    
    # 1. Draw nodes
    nx.draw_networkx_nodes(G, pos, node_size=700, node_color='lightblue', edgecolors='black')
    
    # 2. Draw node labels (numbers 0, 1, 2...)
    nx.draw_networkx_labels(G, pos, font_size=12, font_weight='bold')
    
    # 3. Draw edges (directed arrows)
    nx.draw_networkx_edges(G, pos, arrowstyle='-|>', arrowsize=20, edge_color='gray')
    
    # 4. Draw edge weights
    edge_labels = nx.get_edge_attributes(G, 'weight')
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, font_color='red', font_size=10)
    
    # Hide Matplotlib axes
    plt.axis('off')
    
    # Save to file
    plt.savefig(output_filepath, format='png', bbox_inches='tight')
    print(f"Graph successfully generated and saved as: {output_filepath}")
    
    # Clear the plot buffer so nothing overlaps on subsequent calls
    plt.close()

def main():
    # File paths (assuming execution from the main repository folder)
    input_file = '../1_MPI/data/matrix.txt'
    output_file = '../1_MPI/data/graph_output.png'
    
    if not os.path.exists(input_file):
        print(f"Error: File {input_file} not found!")
        return
        
    print("Loading matrix...")
    matrix = load_matrix(input_file)
    
    print("Building graph...")
    graph = build_graph(matrix)
    
    print("Generating and saving image...")
    save_graph_image(graph, output_file)
    
if __name__ == "__main__":
    main()
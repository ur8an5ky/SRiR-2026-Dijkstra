import argparse
import random
import os

def generate_symmetric_matrix(size, density=0.4, max_weight=20):
    """Generates a random symmetric adjacency matrix for an undirected graph."""
    matrix = [[-1 for _ in range(size)] for _ in range(size)]
    
    for i in range(size):
        for j in range(i + 1, size):
            if random.random() < density:
                weight = random.randint(1, max_weight)
                matrix[i][j] = weight
                matrix[j][i] = weight
                
    return matrix

def save_matrix(matrix, filepath):
    """Saves the matrix to a text file with tab-separated values."""
    # Ensure the directory exists
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    
    with open(filepath, 'w') as file:
        for row in matrix:
            file.write('\t'.join(map(str, row)) + '\n')

def main():
    parser = argparse.ArgumentParser(description="Generate a random symmetric adjacency matrix.")
    parser.add_argument("size", type=int, help="Dimension of the matrix (number of nodes)")
    parser.add_argument("--density", type=float, default=0.4, help="Edge density (0.0 to 1.0)")
    
    args = parser.parse_args()
    
    # Get the directory where the script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Create a local folder for tests in the same location
    output_dir = os.path.join(script_dir, 'test_matrices')
    output_file = os.path.join(output_dir, f'matrix_{args.size}x{args.size}.txt')
    
    print(f"Generating {args.size}x{args.size} matrix (density: {args.density})...")
    matrix = generate_symmetric_matrix(args.size, args.density)
    
    save_matrix(matrix, output_file)
    print(f"Success! Matrix saved to: {output_file}")

if __name__ == "__main__":
    main()
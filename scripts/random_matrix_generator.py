import argparse
import random
import os

def generate_connected_symmetric_matrix(size, density=0.4, max_weight=20):
    """Generates a random connected symmetric adjacency matrix.

    Strategy:
      1. Build a random spanning tree (n-1 edges) — guarantees connectivity.
      2. Add extra random edges until we reach the target edge count
         implied by `density`.
    """
    matrix = [[-1 for _ in range(size)] for _ in range(size)]

    if size <= 1:
        return matrix

    def add_edge(i, j):
        w = random.randint(1, max_weight)
        matrix[i][j] = w
        matrix[j][i] = w

    nodes = list(range(size))
    random.shuffle(nodes)

    for k in range(1, size):
        parent = nodes[random.randint(0, k - 1)]
        child  = nodes[k]
        add_edge(parent, child)

    max_edges    = size * (size - 1) // 2
    target_edges = int(round(density * max_edges))
    target_edges = max(target_edges, size - 1)

    current_edges = size - 1
    if current_edges >= target_edges:
        return matrix

    candidates = [
        (i, j)
        for i in range(size)
        for j in range(i + 1, size)
        if matrix[i][j] == -1
    ]
    random.shuffle(candidates)

    needed = target_edges - current_edges
    for i, j in candidates[:needed]:
        add_edge(i, j)

    return matrix


def save_matrix(matrix, filepath):
    """Saves the matrix to a text file with tab-separated values."""
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, 'w') as file:
        for row in matrix:
            file.write('\t'.join(map(str, row)) + '\n')


def main():
    parser = argparse.ArgumentParser(
        description="Generate a random connected symmetric adjacency matrix."
    )
    parser.add_argument("size", type=int, help="Dimension of the matrix (number of nodes)")
    parser.add_argument("--density", type=float, default=0.4, help="Edge density (0.0 to 1.0)")
    parser.add_argument("--seed", type=int, default=None, help="Random seed (optional, for reproducibility)")

    args = parser.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    if not 0.0 <= args.density <= 1.0:
        parser.error("--density must be between 0.0 and 1.0")
    if args.size < 1:
        parser.error("size must be at least 1")

    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, '../1_MPI/data')
    output_file = os.path.join(output_dir, f'matrix_{args.size}.txt')

    print(f"Generating connected {args.size}x{args.size} matrix (density: {args.density})...")
    matrix = generate_connected_symmetric_matrix(args.size, args.density)

    save_matrix(matrix, output_file)
    print(f"Success! Matrix saved to: {output_file}")


if __name__ == "__main__":
    main()
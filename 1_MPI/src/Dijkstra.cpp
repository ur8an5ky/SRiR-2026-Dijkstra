#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "Edge.hpp"

void ImportGraph(std::string, std::vector<int>&, std::unordered_map<Edge, int>&);

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        std::cerr << "Too litle arguments, usage: ./Dijkstra <pathToGraphFile>" << std::endl;
        return 1;
    }

    std::vector<int> V; 
    std::unordered_map<Edge, int> E;
    // std::string filePath{argv[1]};
    ImportGraph(argv[1], V, E);

    for (const auto& [edge, value] : E) {
        std::cout << "(" << edge.a << ", " << edge.b << ") -> " << value << "\n";
    }

    return 0;
}

void ImportGraph(std::string filePath, std::vector<int>& V, std::unordered_map<Edge, int>& E)
{
    std::cout << "Importing graph from file: " << filePath << std::endl; 
    std::ifstream file(filePath);
    std::string line;
    int i, j;
    i = j = 0;
    while (std::getline(file, line))
    {
        V.push_back(i);
        std::stringstream lineStream(line);
        std::string valStr;
        while (std::getline(lineStream, valStr, '\t'))
        {
            int val = std::atoi(valStr.c_str());
            if (val > 0)
                E.insert({Edge{i, j}, val});

            std::cout << val << "\t";
            j++;
        }
        std::cout << std::endl;
        j = 0;
        i++;
    }
}


void DijkstraSingleSourceSP()
{

}
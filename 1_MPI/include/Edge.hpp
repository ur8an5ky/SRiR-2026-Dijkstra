#pragma once
#include <algorithm>

struct Edge{
    int a, b;  
    Edge(int x, int y) 
    {
        a = std::min(x, y);
        b = std::max(x, y);
    }
    
    bool operator==(const Edge& other) const
    {
        return a == other.a && b == other.b;
    }
};

namespace std {
template<>
struct hash<Edge> {
    size_t operator()(const Edge& e) const {
        return hash<int>()(e.a) ^ (hash<int>()(e.b) << 1);
    }
};
}


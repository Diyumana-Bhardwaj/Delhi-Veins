// g++ main.cpp -o metro_backend -std=c++17 -pthread -lws2_32 -I./include
#include <iostream>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <set>
#include <algorithm>


using namespace std;

struct Connection {
    string station;
    double distance;
    string lineColor;
};

class MetroGraph {
public:
    unordered_map<string, vector<Connection>> adjList;
    unordered_map<string, int> stationToId;
    vector<string> idToStation;
    vector<vector<pair<int, double>>> adjInt;

    void trim(string &s) {
        s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) { return !isspace(ch); }));
        s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), s.end());
    }

    void addEdge(string station1, string station2, double distance, string lineColor) {
        trim(station1);
        trim(station2);
        trim(lineColor);

        if (!station1.empty() && !station2.empty() && station1 != station2) {
            adjList[station1].push_back({station2, distance, lineColor});
            adjList[station2].push_back({station1, distance, lineColor});
        }
    }

    void loadFromFile(string filename) {
        ifstream file(filename);
        string line, station1, station2, lineColor;
        double distance;

        if (!file.is_open()) {
            cout << "Error opening file!" << endl;
            return;
        }

        getline(file, line); // Skip header

        while (getline(file, line)) {
            stringstream ss(line);
            getline(ss, station1, ',');
            getline(ss, station2, ',');
            getline(ss, lineColor, ',');
            ss >> distance;

            addEdge(station1, station2, distance, lineColor);
        }

        file.close();
    }

    void buildIntegerGraph() {
        int id = 0;

        // Assign ID to every station
        for (auto& station : adjList) {
            stationToId[station.first] = id;
            idToStation.push_back(station.first);
            id++;
        }

        // Resize adjacency list
        adjInt.resize(id);

        // Fill integer adjacency list
        for (auto& station : adjList) {
            int fromId = stationToId[station.first];

            for (auto& neighbor : station.second) {
                int toId = stationToId[neighbor.station];
                adjInt[fromId].push_back({toId, neighbor.distance});
            }
        }
    }


    double findShortestPathJSON(string source, string destination) {
        if (adjList.find(source) == adjList.end() || adjList.find(destination) == adjList.end()) {
            cout << "Error: One or both stations not found!" << endl;
            return -1;
        }

        unordered_map<string, double> minDistance;
        unordered_map<string, string> prevStation;
        unordered_map<string, string> lineForPrev;
        priority_queue<pair<double, string>, vector<pair<double, string>>, greater<>> pq;

        for (auto& station : adjList) {
            minDistance[station.first] = numeric_limits<double>::infinity();
        }

        minDistance[source] = 0;
        pq.push({0, source});

        while (!pq.empty()) {
            string current = pq.top().second;
            double currDist = pq.top().first;
            pq.pop();

            if (current == destination) break;

            for (auto& neighbor : adjList[current]) {
                double newDist = currDist + neighbor.distance;
                if (newDist < minDistance[neighbor.station]) {
                    minDistance[neighbor.station] = newDist;
                    prevStation[neighbor.station] = current;
                    lineForPrev[neighbor.station] = neighbor.lineColor;
                    pq.push({newDist, neighbor.station});
                }
            }
        }

        if (prevStation.find(destination) == prevStation.end()) {
            cout<< "Error reconstructing path!" << endl;
            return -1;
        }

        vector<string> path;
        vector<string> lines;
        string track = destination;

        while (track != source) {
            path.push_back(track);
            if (prevStation.count(track)) {
                lines.push_back(lineForPrev[track]);
                track = prevStation[track];
            } else {
                cout << "Error reconstructing path!" << endl;
                return -1;
            }
        }

        path.push_back(source);
        reverse(path.begin(), path.end());
        reverse(lines.begin(), lines.end());

        return minDistance[destination];
    }

    double findMinimumExchangesJSON(string source, string destination) {
        if (adjList.find(source) == adjList.end() || adjList.find(destination) == adjList.end()) {
            cout << "Error: One or both stations not found!" << endl;
            return -1;
        }

        struct Node {
            string station;
            int lineChanges;
            double distance;
            string lineColor;
        };

        struct Compare {
            bool operator()(const Node &a, const Node &b) {
                if (a.lineChanges == b.lineChanges)
                    return a.distance > b.distance;
                return a.lineChanges > b.lineChanges;
            }
        };

        priority_queue<Node, vector<Node>, Compare> pq;
        unordered_map<string, pair<int, double>> bestPath;
        unordered_map<string, tuple<string, string>> prevNode;

        pq.push({source, 0, 0.0, ""});
        bestPath[source] = {0, 0.0};

        while (!pq.empty()) {
            Node current = pq.top();
            pq.pop();

            if (current.station == destination) break;

            for (auto &neighbor : adjList[current.station]) {
                int newLineChanges = current.lineChanges + ((current.lineColor == "" || current.lineColor == neighbor.lineColor) ? 0 : 1);
                double newDistance = current.distance + neighbor.distance;

                if (bestPath.find(neighbor.station) == bestPath.end() ||
                    newLineChanges < bestPath[neighbor.station].first ||
                    (newLineChanges == bestPath[neighbor.station].first && newDistance < bestPath[neighbor.station].second)) {

                    bestPath[neighbor.station] = {newLineChanges, newDistance};
                    prevNode[neighbor.station] = make_tuple(current.station, neighbor.lineColor);
                    pq.push({neighbor.station, newLineChanges, newDistance, neighbor.lineColor});
                }
            }
        }

        if (bestPath.find(destination) == bestPath.end()) {
            cout << "Error: No path found!" << endl;
            return -1;
        }

        if (source != destination && prevNode.find(destination) == prevNode.end()) {
            cout << "Error reconstructing path!" << endl;
            return -1;
        }

        vector<string> path;
        vector<string> lines;
        string current = destination;

        while (current != source) {
            path.push_back(current);
            if (prevNode.count(current)) {
                lines.push_back(get<1>(prevNode[current]));
                current = get<0>(prevNode[current]);
            } else {
                cout << "Error reconstructing path!" << endl;
                return -1;
            }
        }

        path.push_back(source);
        reverse(path.begin(), path.end());
        reverse(lines.begin(), lines.end());


        return bestPath[destination].second;
    }

    double findShortestPath_Int(string source, string destination) {
        int sourceId = stationToId[source];
        int destinationId = stationToId[destination];
        int n = adjInt.size();
        vector<double> dist(n, numeric_limits<double>::infinity());
        priority_queue<pair<double, int>, vector<pair<double, int>>, greater<>> pq;

        dist[sourceId] = 0;
        pq.push({0, sourceId});

        while (!pq.empty()) {
            auto [currDist, current] = pq.top();
            pq.pop();

            if (currDist > dist[current]) continue;
            if (current == destinationId) break;

            for (auto& [neighborId, weight] : adjInt[current]) {
                double newDist = currDist + weight;
                if (newDist < dist[neighborId]) {
                    dist[neighborId] = newDist;
                    pq.push({newDist, neighborId});
                }
            }
        }

        return dist[destinationId];
    }

};

#include <chrono>

int main() {
    MetroGraph metro;
    metro.loadFromFile("Delhi_Metro_Lines.csv");
    metro.buildIntegerGraph();

    string source = "Rithala";
    string destination = "Rajiv Chowk";


    // 🔥 Warm up
    for (int i = 0; i < 1000; i++) {
        metro.findShortestPathJSON(source, destination);
    }

    // 🔥 Measure
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; i++) {
        metro.findShortestPathJSON(source, destination);
    }

    auto end = std::chrono::high_resolution_clock::now();

    double total =
        std::chrono::duration<double, std::milli>(end - start).count();

    cout << "Total time: " << total << " ms\n";
    cout << "Average time: " << total / 10000 << " ms\n";

    return 0;
}

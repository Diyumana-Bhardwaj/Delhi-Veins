#include <iostream>
#include <unordered_map>
#include <vector>
#include <list>
#include <tuple>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <algorithm>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <mutex>

size_t cacheCapacity = 1000;

using json = nlohmann::json;
using namespace std;

mutex cacheMutex;

struct Connection {
    string station;
    double distance;
    string lineColor;
};

struct Edge {
    int to;
    double weight;
    int lineId;
};


class MetroGraph {
public:
    unordered_map<string, vector<Connection>> adjList;
    unordered_map<string, int> stationToId;
    vector<string> idToStation;
    vector<vector<Edge>> adjInt;
    unordered_map<string, int> lineToId;
    vector<string> idToLine;
    list<string> lruList;  // Most recent at front
    unordered_map<string, pair<json, list<string>::iterator>> routeCache;

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
        stationToId.clear();
        idToStation.clear();
        adjInt.clear();
        lineToId.clear();
        idToLine.clear();


        int stationId = 0;
        int lineIdCounter = 0;

        for (auto& station : adjList) {
            stationToId[station.first] = stationId++;
            idToStation.push_back(station.first);
        }

        adjInt.resize(stationId);

        for (auto& station : adjList) {
            int fromId = stationToId[station.first];

            for (auto& neighbor : station.second) {
                if (!lineToId.count(neighbor.lineColor)) {
                    lineToId[neighbor.lineColor] = lineIdCounter++;
                    idToLine.push_back(neighbor.lineColor);
                }
                int toId = stationToId[neighbor.station];
                int lineId = lineToId[neighbor.lineColor];

                adjInt[fromId].push_back({toId, neighbor.distance, lineId});
            }
        }
        adjList.clear();
        adjList.rehash(0);

    }

    json findShortestPathOptimized(const string& source, const string& destination) {
        string cacheKey = "shortest|" + source + "|" + destination;

        {
            lock_guard<mutex> lock(cacheMutex);

            auto it = routeCache.find(cacheKey);
            if (it != routeCache.end()) {
                // Move key to front (most recently used)
                lruList.erase(it->second.second);
                lruList.push_front(cacheKey);
                it->second.second = lruList.begin();

                return it->second.first;
            }
        }



        json result;

        if (!stationToId.count(source) || !stationToId.count(destination)) {
            result["error"] = "Error: One or both stations not found!";
            return result;
        }

        int sourceId = stationToId[source];
        int destId = stationToId[destination];

        int n = adjInt.size();
        thread_local vector<double> dist;
        thread_local vector<int> parent;

        if (dist.size() != n) {
            dist.resize(n);
            parent.resize(n);
        }

        fill(dist.begin(), dist.end(), numeric_limits<double>::infinity());
        fill(parent.begin(), parent.end(), -1);


        priority_queue<pair<double,int>, vector<pair<double,int>>, greater<>> pq;

        dist[sourceId] = 0;
        pq.push({0, sourceId});

        while (!pq.empty()) {
            auto [currDist, u] = pq.top();
            pq.pop();

            if (currDist > dist[u]) continue;
            if (u == destId) break;

            for (auto& edge : adjInt[u]) {
                int v = edge.to;
                double newDist = currDist + edge.weight;

                if (newDist < dist[v]) {
                    dist[v] = newDist;
                    parent[v] = u;
                    pq.push({newDist, v});
                }
            }
        }

        if (dist[destId] == numeric_limits<double>::infinity()) {
            result["error"] = "Error: No path found!";
            return result;
        }

        vector<string> path;
        for (int at = destId; at != -1; at = parent[at]) {
            path.push_back(idToStation[at]);
        }
        reverse(path.begin(), path.end());

        result["path"] = path;
        result["total_distance"] = dist[destId];

        {
            lock_guard<mutex> lock(cacheMutex);

            // If already exists (rare but safe)
            if (routeCache.count(cacheKey)) {
                lruList.erase(routeCache[cacheKey].second);
            }
            else if (routeCache.size() >= cacheCapacity) {
                // Remove least recently used
                string lruKey = lruList.back();
                lruList.pop_back();
                routeCache.erase(lruKey);
            }

            lruList.push_front(cacheKey);
            routeCache[cacheKey] = {result, lruList.begin()};
        }


        return result;
    }


    json findMinimumExchangesOptimized(const string& source, const string& destination) {
        string cacheKey = "exchange|" + source + "|" + destination;
        {
            lock_guard<mutex> lock(cacheMutex);

            auto it = routeCache.find(cacheKey);
            if (it != routeCache.end()) {
                // Move key to front (most recently used)
                lruList.erase(it->second.second);
                lruList.push_front(cacheKey);
                it->second.second = lruList.begin();

                return it->second.first;
            }
        }



        json result;

        if (!stationToId.count(source) || !stationToId.count(destination)) {
            result["error"] = "Error: One or both stations not found!";
            return result;
        }

        int sourceId = stationToId[source];
        int destId = stationToId[destination];

        struct Node {
            int station;
            int lineChanges;
            double distance;
            int lineId;
        };

        struct Compare {
            bool operator()(const Node& a, const Node& b) {
                if (a.lineChanges == b.lineChanges)
                    return a.distance > b.distance;
                return a.lineChanges > b.lineChanges;
            }
        };

        int n = adjInt.size();
        vector<pair<int,double>> best(n, {INT_MAX, numeric_limits<double>::infinity()});
        vector<int> parent(n, -1);

        priority_queue<Node, vector<Node>, Compare> pq;

        pq.push({sourceId, 0, 0.0, -1});
        best[sourceId] = {0, 0.0};

        while (!pq.empty()) {
            Node current = pq.top();
            pq.pop();

            if (current.lineChanges > best[current.station].first ||
                (current.lineChanges == best[current.station].first &&
                current.distance > best[current.station].second))
                continue;

            if (current.station == destId) break;

            for (auto& edge : adjInt[current.station]) {

                int newLineChanges = current.lineChanges +
                    ((current.lineId == -1 || current.lineId == edge.lineId) ? 0 : 1);

                double newDistance = current.distance + edge.weight;

                if (newLineChanges < best[edge.to].first ||
                (newLineChanges == best[edge.to].first &&
                    newDistance < best[edge.to].second)) {

                    best[edge.to] = {newLineChanges, newDistance};
                    parent[edge.to] = current.station;

                    pq.push({edge.to, newLineChanges, newDistance, edge.lineId});
                }
            }
        }

        if (best[destId].first == INT_MAX) {
            result["error"] = "Error: No path found!";
            return result;
        }

        vector<string> path;
        for (int at = destId; at != -1; at = parent[at]) {
            path.push_back(idToStation[at]);
        }
        reverse(path.begin(), path.end());

        result["path"] = path;
        result["total_line_changes"] = best[destId].first;
        result["total_distance"] = best[destId].second;

        {
            lock_guard<mutex> lock(cacheMutex);

            // If already exists (rare but safe)
            if (routeCache.count(cacheKey)) {
                lruList.erase(routeCache[cacheKey].second);
            }
            else if (routeCache.size() >= cacheCapacity) {
                // Remove least recently used
                string lruKey = lruList.back();
                lruList.pop_back();
                routeCache.erase(lruKey);
            }

            lruList.push_front(cacheKey);
            routeCache[cacheKey] = {result, lruList.begin()};
        }

        return result;
    }

};

int main() {
    MetroGraph metro;
    metro.loadFromFile("public/dataset/Delhi_Metro_Lines.csv");
    metro.buildIntegerGraph();

    httplib::Server svr;
    svr.set_mount_point("/", "./public");


    svr.Get("/shortest_path", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("source") && req.has_param("destination")) {
            string source = req.get_param_value("source");
            string destination = req.get_param_value("destination");
            json result = metro.findShortestPathOptimized(source, destination);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(result.dump(4), "application/json");
        } else {
            res.status = 400;
            res.set_content("Missing parameters", "text/plain");
        }
    });

    svr.Get("/min_exchanges", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("source") && req.has_param("destination")) {
            string source = req.get_param_value("source");
            string destination = req.get_param_value("destination");
            json result = metro.findMinimumExchangesOptimized(source, destination);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(result.dump(4), "application/json");
        } else {
            res.status = 400;
            res.set_content("Missing parameters", "text/plain");
        }
    });

    cout << "Server listening on http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
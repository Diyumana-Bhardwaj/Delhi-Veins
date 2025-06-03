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
#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

struct Connection {
    string station;
    double distance;
    string lineColor;
};

class MetroGraph {
public:
    unordered_map<string, vector<Connection>> adjList;

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

    json findShortestPathJSON(string source, string destination) {
        json result;
        if (adjList.find(source) == adjList.end() || adjList.find(destination) == adjList.end()) {
            result["error"] = "Error: One or both stations not found!";
            return result;
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
            result["error"] = "Error: No path found!";
            return result;
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
                result["error"] = "Error reconstructing path!";
                return result;
            }
        }

        path.push_back(source);
        reverse(path.begin(), path.end());
        reverse(lines.begin(), lines.end());

        result["path"] = path;
        result["total_distance"] = minDistance[destination];
        result["lines"] = lines;
        return result;
    }

    json findMinimumExchangesJSON(string source, string destination) {
        json result;
        if (adjList.find(source) == adjList.end() || adjList.find(destination) == adjList.end()) {
            result["error"] = "Error: One or both stations not found!";
            return result;
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
            result["error"] = "Error: No path found!";
            return result;
        }

        if (source != destination && prevNode.find(destination) == prevNode.end()) {
            result["error"] = "Error reconstructing path!";
            return result;
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
                result["error"] = "Error reconstructing path!";
                return result;
            }
        }

        path.push_back(source);
        reverse(path.begin(), path.end());
        reverse(lines.begin(), lines.end());

        result["path"] = path;
        result["total_line_changes"] = bestPath[destination].first;
        result["total_distance"] = bestPath[destination].second;
        result["lines"] = lines;
        return result;
    }
};

int main() {
    MetroGraph metro;
    metro.loadFromFile("Delhi_Metro_Lines.csv");

    httplib::Server svr;

    svr.Get("/shortest_path", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("source") && req.has_param("destination")) {
            string source = req.get_param_value("source");
            string destination = req.get_param_value("destination");
            json result = metro.findShortestPathJSON(source, destination);
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
            json result = metro.findMinimumExchangesJSON(source, destination);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(result.dump(4), "application/json");
        } else {
            res.status = 400;
            res.set_content("Missing parameters", "text/plain");
        }
    });

    cout << "Server listening on http://localhost:8080" << endl;
    svr.listen("localhost", 8080);

    return 0;}
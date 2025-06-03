// g++ test.cpp -o test.exe -I C:/SFML-2.6.1/include -L C:/SFML-2.6.1/lib -lsfml-graphics -lsfml-window -lsfml-system delhimetroline.csv metro_coordinates_dot_csv fonts/OpenSans-Regular.ttf fonts/OpenSans-Bold.ttf
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <unordered_map>
#include <queue>
#include <limits>
#include <set>
#include <algorithm>

using namespace std;

// --- Data Structures ---
struct Station {
    std::string name;
    float x;
    float y;
    sf::Color color;
    sf::CircleShape circle;
};

struct Connection {
    std::string station;
    double distance;
    std::string lineColor;
};

// --- Helper Functions ---
sf::Color getColorFromName(const std::string& colorName) {
    static std::map<std::string, sf::Color> colorMap = {
        {"Blue", sf::Color::Blue},
        {"Red", sf::Color::Red},
        {"Yellow", sf::Color::Yellow},
        {"Green", sf::Color::Green},
        {"Violet", sf::Color(148, 0, 211)},
        {"Magenta", sf::Color::Magenta},
        {"Orange", sf::Color(255, 140, 0)},
        {"Pink", sf::Color(255, 105, 180)}
    };
    return colorMap[colorName];
}

// --- Metro Graph Class (Logic) ---
class MetroGraph {
public:
    std::unordered_map<std::string, std::vector<Connection>> adjList;

    void addEdge(std::string station1, std::string station2, double distance, std::string lineColor) {
        if (!station1.empty() && !station2.empty() && station1 != station2){
            adjList[station1].push_back({station2, distance, lineColor});
            adjList[station2].push_back({station1, distance, lineColor}); // Bidirectional
        }
    }

    void loadFromFile(std::string filename) {
        std::ifstream file(filename);
        std::string line, station1, station2, lineColor;
        double distance;

        if (!file.is_open()) {
            std::cerr << "Error opening connection file!" << std::endl;
            return;
        }

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::getline(ss, station1, ',');
            std::getline(ss, station2, ',');
            std::getline(ss, lineColor, ',');
            ss >> distance;
            addEdge(station1, station2, distance, lineColor);
        }
        file.close();
    }

    std::pair<std::vector<std::string>, double> findShortestPath(std::string source, std::string destination) {
        if (adjList.find(source) == adjList.end() || adjList.find(destination) == adjList.end()) {
            std::cerr << "Error: One or both stations not found for shortest path!\n";
            return {{}, std::numeric_limits<double>::infinity()};
        }
        std::unordered_map<std::string, double> minDistance;
        std::unordered_map<std::string, std::string> prevStation;
        std::priority_queue<std::pair<double, std::string>, std::vector<std::pair<double, std::string>>, std::greater<>> pq;

        for (auto& station : adjList) {
            minDistance[station.first] = std::numeric_limits<double>::infinity();
        }
        minDistance[source] = 0;
        pq.push({0, source});

        while (!pq.empty()) {
            std::string current = pq.top().second;
            double currDist = pq.top().first;
            pq.pop();

            if (current == destination) break;

            for (auto& neighbor : adjList[current]) {
                double newDist = minDistance[current] + neighbor.distance;
                if (newDist < minDistance[neighbor.station]) {
                    minDistance[neighbor.station] = newDist;
                    prevStation[neighbor.station] = current;
                    pq.push({newDist, neighbor.station});
                }
            }
        }

        if (prevStation.find(destination) == prevStation.end()) {
            std::cerr << "Error: No shortest path found from " << source << " to " << destination << "!\n";
            return {{}, std::numeric_limits<double>::infinity()};
        }

        std::vector<std::string> path;
        std::string track = destination;
        while (track != source) {
            path.push_back(track);
            track = prevStation[track];
        }
        path.push_back(source);
        std::reverse(path.begin(), path.end());

        return {path, minDistance[destination]};
    }

    std::pair<std::vector<std::string>, int> findMinimumExchanges(std::string source, std::string destination) {
        if (adjList.find(source) == adjList.end() || adjList.find(destination) == adjList.end()) {
            std::cerr << "Error: One or both stations not found for minimum exchanges!\n";
            return {{}, -1};
        }

        struct Node {
            std::string station;
            int lineChanges;
            double distance;
            std::string lineColor;
            std::vector<std::string> path;
        };

        struct Compare {
            bool operator()(const Node& a, const Node& b) {
                if (a.lineChanges == b.lineChanges) {
                    return a.distance > b.distance;
                }
                return a.lineChanges > b.lineChanges;
            }
        };

        std::priority_queue<Node, std::vector<Node>, Compare> pq;
        pq.push({source, 0, 0.0, "", {source}});
        std::map<std::string, std::pair<int, double>> visited; // {station, {exchanges, distance}}
        visited[source] = {0, 0.0};

        while (!pq.empty()) {
            Node current = pq.top();
            pq.pop();

            if (current.station == destination) {
                return {current.path, current.lineChanges};
            }

            for (const auto& neighborConn : adjList[current.station]) {
                int newLineChanges = current.lineChanges + (current.lineColor.empty() || current.lineColor == neighborConn.lineColor ? 0 : 1);
                double newDistance = current.distance + neighborConn.distance;
                std::vector<std::string> newPath = current.path;
                newPath.push_back(neighborConn.station);

                if (visited.find(neighborConn.station) == visited.end() || newLineChanges < visited[neighborConn.station].first || (newLineChanges == visited[neighborConn.station].first && newDistance < visited[neighborConn.station].second)) {
                    visited[neighborConn.station] = {newLineChanges, newDistance};
                    pq.push({neighborConn.station, newLineChanges, newDistance, neighborConn.lineColor, newPath});
                }
            }
        }

        std::cerr << "Error: No path with minimum exchanges found from " << source << " to " << destination << "!\n";
        return {{}, -1};
    }
};

// --- GUI Application Class ---
class MetroApp {
public:
    MetroApp(int width, int height) : window(sf::VideoMode(width, height), "Delhi Metro App"), metroGraph() {
        if (!font.loadFromFile("fonts/OpenSans-Regular.ttf")) {
            std::cerr << "Error loading font!" << std::endl;
        }
        if (!boldFont.loadFromFile("fonts/OpenSans-Bold.ttf")) {
            std::cerr << "Error loading bold font!" << std::endl;
        }
        metroGraph.loadFromFile("Delhi_Metro_Lines.csv");
        loadStationData("metro_coordinates.csv");
        setupWelcomePage();
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                else if (currentPage == Page::WELCOME) {
                    handleWelcomePageEvents(event);
                }
                else if (currentPage == Page::MAP) {
                    handleMapPageEvents(event);
                }
            }

            window.clear(sf::Color::White);
            if (currentPage == Page::WELCOME) {
                drawWelcomePage();
            }
            else if (currentPage == Page::MAP) {
                drawMapPage();
            }
            window.display();
        }
    }

private:
    enum Page { WELCOME, MAP };
    Page currentPage = Page::WELCOME;

    sf::RenderWindow window;
    MetroGraph metroGraph;
    std::vector<Station> stations;
    std::map<std::string, Station> stationsMap;
    sf::Font font;
    sf::Font boldFont;

    // Welcome Page Elements
    sf::Text welcomeText;
    sf::Text sourceLabel;
    sf::Text destinationLabel;
    //sf::String sourceInputText;
    sf::Text sourceInput;
    //sf::RectangleShape sourceInputBox;
    //sf::String destinationInputText;
    sf::Text destinationInput;
    //sf::RectangleShape destinationInputBox;
    sf::RectangleShape searchButton;
    sf::Text searchButtonText;
    sf::FloatRect searchButtonBounds;
    std::string sourceStation;
    std::string destinationStation;
    //bool isSourceSelected = false;
    //bool isDestinationSelected = false;

    // New Dropdown Menu elements
    sf::RectangleShape sourceDropdownButton;
    sf::RectangleShape destinationDropdownButton;
    sf::Text sourceDropdownText;
    sf::Text destinationDropdownText;
    bool isSourceDropdownOpen = false;
    bool isDestinationDropdownOpen = false;
    std::vector<sf::Text> sourceStationList;
    std::vector<sf::Text> destinationStationList;
    sf::FloatRect sourceDropdownBounds;
    sf::FloatRect destinationDropdownBounds;
    sf::RectangleShape sourceDropdownListRect;
    sf::RectangleShape destinationDropdownListRect;
    sf::Vector2f sourceDropdownListPosition;
    sf::Vector2f destinationDropdownListPosition;
    float sourceListScrollOffset = 0.0f;
    float destinationListScrollOffset = 0.0f;
    bool isDraggingSourceList = false;
    bool isDraggingDestinationList = false;
    float sourceDragStartY = 0.0f;
    float destinationDragStartY = 0.0f;
    float sourceScrollBarHeight = 200.0f;
    float destinationScrollBarHeight = 200.0f;
    sf::RectangleShape sourceScrollBar;
    sf::RectangleShape destinationScrollBar;
    float sourceScrollPercent = 0.0f;
    float destinationScrollPercent = 0.0f;

    // Map Page Elements
    float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f;
    std::pair<std::vector<std::string>, double> shortestPathResult;
    std::pair<std::vector<std::string>, int> minExchangePathResult;
    sf::RectangleShape backButton;
    sf::Text backButtonText;
    sf::FloatRect backButtonBounds;
    sf::Text shortestPathText;
    sf::Text minExchangesText;
    int hoveredStationIndex = -1;
    sf::Text hoverText;
    sf::RectangleShape hoverBox;
    sf::Vector2f mousePosition;

    void loadStationData(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            Station station;

            std::getline(ss, station.name, ',');
            std::getline(ss, token, ',');
            station.x = std::stof(token);
            std::getline(ss, token, ',');
            station.y = std::stof(token);
            std::getline(ss, token, ',');
            station.color = getColorFromName(token);
            station.circle.setRadius(6);
            station.circle.setFillColor(station.color);

            if (stations.empty()) {
                minX = station.x;
                maxX = station.x;
                minY = station.y;
                maxY = station.y;
            }
            else {
                minX = std::min(minX, station.x);
                maxX = std::max(maxX, station.x);
                minY = std::min(minY, station.y);
                maxY = std::max(maxY, station.y);
            }
            stations.push_back(station);
            stationsMap[station.name] = station;
        }
    }

    void setupWelcomePage() {
        welcomeText.setFont(boldFont);
        welcomeText.setString("Welcome to Delhi Metro App!");
        welcomeText.setCharacterSize(30);
        welcomeText.setFillColor(sf::Color::Black);
        welcomeText.setPosition({ static_cast<float>(window.getSize().x) / 2 - welcomeText.getLocalBounds().width / 2, 100 });

        sourceLabel.setFont(font);
        sourceLabel.setString("Source Station:");
        sourceLabel.setCharacterSize(20);
        sourceLabel.setFillColor(sf::Color::Black);
        sourceLabel.setPosition({ 100, 200 });

        // Source Dropdown Button
        sourceDropdownButton.setSize({ 300, 30 });
        sourceDropdownButton.setFillColor(sf::Color::White);
        sourceDropdownButton.setOutlineColor(sf::Color::Black);
        sourceDropdownButton.setOutlineThickness(1);
        sourceDropdownButton.setPosition({ 100, 230 });
        sourceDropdownBounds = sourceDropdownButton.getGlobalBounds();

        sourceDropdownText.setFont(font);
        sourceDropdownText.setString("Select Source Station");
        sourceDropdownText.setCharacterSize(20);
        sourceDropdownText.setFillColor(sf::Color::Black);
        sourceDropdownText.setPosition({ 105, 235 });

        // Source Station List
        for (size_t i = 0; i < stations.size(); ++i) {
            sf::Text stationText;
            stationText.setFont(font);
            stationText.setString(stations[i].name);
            stationText.setCharacterSize(20);
            stationText.setFillColor(sf::Color::Black);
            stationText.setPosition({ 100, 270 + i * 30 }); // Adjust Y as needed
            sourceStationList.push_back(stationText);
        }

        destinationLabel.setFont(font);
        destinationLabel.setString("Destination Station:");
        destinationLabel.setCharacterSize(20);
        destinationLabel.setFillColor(sf::Color::Black);
        destinationLabel.setPosition({ 100, 280 });

        // Destination Dropdown Button
        destinationDropdownButton.setSize({ 300, 30 });
        destinationDropdownButton.setFillColor(sf::Color::White);
        destinationDropdownButton.setOutlineColor(sf::Color::Black);
        destinationDropdownButton.setOutlineThickness(1);
        destinationDropdownButton.setPosition({ 100, 310 });
        destinationDropdownBounds = destinationDropdownButton.getGlobalBounds();

        destinationDropdownText.setFont(font);
        destinationDropdownText.setString("Select Destination Station");
        destinationDropdownText.setCharacterSize(20);
        destinationDropdownText.setFillColor(sf::Color::Black);
        destinationDropdownText.setPosition({ 105, 315 });

        // Destination Station List
        for (size_t i = 0; i < stations.size(); ++i) {
            sf::Text stationText;
            stationText.setFont(font);
            stationText.setString(stations[i].name);
            stationText.setCharacterSize(20);
            stationText.setFillColor(sf::Color::Black);
            stationText.setPosition({ 100, 350 + i * 30 }); // Adjust Y as needed
            destinationStationList.push_back(stationText);
        }

        searchButton.setSize({ 150, 40 });
        searchButton.setFillColor(sf::Color::Green);
        searchButton.setPosition({ 100, 370 });
        searchButtonBounds = searchButton.getGlobalBounds();

        searchButtonText.setFont(boldFont);
        searchButtonText.setString("Search");
        searchButtonText.setCharacterSize(20);
        searchButtonText.setFillColor(sf::Color::White);
        searchButtonText.setPosition({ searchButton.getPosition().x + searchButton.getSize().x / 2 - searchButtonText.getLocalBounds().width / 2,
                                        searchButton.getPosition().y + searchButton.getSize().y / 2 - searchButtonText.getLocalBounds().height / 2 - 3 });
    }

    void drawWelcomePage() {
        window.draw(welcomeText);
        window.draw(sourceLabel);
        window.draw(sourceDropdownButton);
        window.draw(sourceDropdownText);
        //window.draw(sourceInputBox);
        //window.draw(sourceInput);
        window.draw(destinationLabel);
        window.draw(destinationDropdownButton);
        window.draw(destinationDropdownText);
        //window.draw(destinationInputBox);
        //window.draw(destinationInput);
        window.draw(searchButton);
        window.draw(searchButtonText);

        // Draw dropdown lists if open
        if (isSourceDropdownOpen) {
            for (const auto& stationText : sourceStationList) {
                window.draw(stationText);
            }
        }
        if (isDestinationDropdownOpen) {
            for (const auto& stationText : destinationStationList) {
                window.draw(stationText);
            }
        }
    }

    void setupMapPage() {
        scaleX = (static_cast<float>(window.getSize().x) - 100) / (maxX - minX);
        scaleY = (static_cast<float>(window.getSize().y) - 150) / (maxY - minY);

        backButton.setSize({ 150, 40 });
        backButton.setFillColor(sf::Color::Blue);
        backButton.setPosition({ static_cast<float>(window.getSize().x) - 170, static_cast<float>(window.getSize().y) - 60 });
        backButtonBounds = backButton.getGlobalBounds();

        backButtonText.setFont(boldFont);
        backButtonText.setString("Back");
        backButtonText.setCharacterSize(20);
        backButtonText.setFillColor(sf::Color::White);
        backButtonText.setPosition({ backButton.getPosition().x + backButton.getSize().x / 2 - backButtonText.getLocalBounds().width / 2,
                                        backButton.getPosition().y + backButton.getSize().y / 2 - backButtonText.getLocalBounds().height / 2 - 3 });

        std::string shortestPathStr = "Shortest Path: ";
        if (!shortestPathResult.first.empty()) {
            for (size_t i = 0; i < shortestPathResult.first.size(); ++i) {
                shortestPathStr += shortestPathResult.first[i];
                if (i < shortestPathResult.first.size() - 1) {
                    shortestPathStr += " -> ";
                }
            }
            shortestPathStr += "\nDistance: " + std::to_string(static_cast<int>(shortestPathResult.second * 100) / 100.0) + " km";
        }
        else {
            shortestPathStr += "No path found";
        }
        shortestPathText.setFont(font);
        shortestPathText.setString(shortestPathStr);
        shortestPathText.setCharacterSize(18);
        shortestPathText.setFillColor(sf::Color::Black);
        shortestPathText.setPosition({ 50, static_cast<float>(window.getSize().y) - 100 });

        std::string minExchangesStr = "Min Exchanges: ";
        if (minExchangePathResult.second != -1) {
            minExchangesStr += std::to_string(minExchangePathResult.second) + " exchanges\nPath: ";
            for (size_t i = 0; i < minExchangePathResult.first.size(); ++i) {
                minExchangesStr += minExchangePathResult.first[i];
                if (i < minExchangePathResult.first.size() - 1) {
                    minExchangesStr += " -> ";
                }
            }
        }
        else {
            minExchangesStr += "No path found";
        }
        minExchangesText.setFont(font);
        minExchangesText.setString(minExchangesStr);
        minExchangesText.setCharacterSize(18);
        minExchangesText.setFillColor(sf::Color::Black);
        minExchangesText.setPosition({ 50, static_cast<float>(window.getSize().y) - 130 });

        hoverText.setFont(boldFont);
        hoverText.setCharacterSize(14);
        hoverText.setFillColor(sf::Color::Black);

        hoverBox.setFillColor(sf::Color(255, 255, 255, 200));
        hoverBox.setOutlineColor(sf::Color::Black);
        hoverBox.setOutlineThickness(1.0f);
    }

    void drawMapPage() {
        // Draw Connections
        for (const auto& pair : metroGraph.adjList) {
            const std::string& station1Name = pair.first;
            if (stationsMap.count(station1Name)) {
                const Station& station1 = stationsMap.at(station1Name);
                float x1 = 50 + (station1.x - minX) * scaleX;
                float y1 = static_cast<float>(window.getSize().y) - 150 - (station1.y - minY) * scaleY;

                for (const auto& conn : pair.second) {
                    const std::string& station2Name = conn.station;
                    if (stationsMap.count(station2Name)) {
                        const Station& station2 = stationsMap.at(station2Name);
                        float x2 = 50 + (station2.x - minX) * scaleX;
                        float y2 = static_cast<float>(window.getSize().y) - 150 - (station2.y - minY) * scaleY;

                        sf::Vertex line[] = {
                            sf::Vertex(sf::Vector2f(x1, y1), sf::Color(128, 128, 128)), // Grey
                            sf::Vertex(sf::Vector2f(x2, y2), sf::Color(128, 128, 128))  // Grey
                        };
                        window.draw(line, 2, sf::Lines);

                        // Draw colored border
                        sf::Vertex borderLine[] = {
                            sf::Vertex(sf::Vector2f(x1, y1), getColorFromName(conn.lineColor)),
                            sf::Vertex(sf::Vector2f(x2, y2), getColorFromName(conn.lineColor))
                        };
                        window.draw(borderLine, 2, sf::Lines);
                    }
                }
            }
        }

        // Draw Stations
        for (auto& station : stations) { // Iterate with non-const reference
            float plotX = 50 + (station.x - minX) * scaleX;
            float plotY = static_cast<float>(window.getSize().y) - 150 - (station.y - minY) * scaleY;
            station.circle.setPosition(plotX - station.circle.getRadius(), plotY - station.circle.getRadius());
            window.draw(station.circle);
        }

        // Highlight Shortest Path
        if (!shortestPathResult.first.empty()) {
            for (size_t i = 0; i < shortestPathResult.first.size() - 1; ++i) {
                const std::string& s1Name = shortestPathResult.first[i];
                const std::string& s2Name = shortestPathResult.first[i + 1];
                if (stationsMap.count(s1Name) && stationsMap.count(s2Name)) {
                    const Station& s1 = stationsMap.at(s1Name);
                    const Station& s2 = stationsMap.at(s2Name);
                    float x1 = 50 + (s1.x - minX) * scaleX;
                    float y1 = static_cast<float>(window.getSize().y) - 150 - (s1.y - minY) * scaleY;
                    float x2 = 50 + (s2.x - minX) * scaleX;
                    float y2 = static_cast<float>(window.getSize().y) - 150 - (s2.y - minY) * scaleY;

                    sf::Vertex highlightLine[] = {
                        sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Red),
                        sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Red)
                    };
                    window.draw(highlightLine, 2, sf::Lines);
                }
            }
        }

        window.draw(backButton);
        window.draw(backButtonText);
        window.draw(shortestPathText);
        window.draw(minExchangesText);

        // Draw hover information
        if (hoveredStationIndex != -1) {
            hoverText.setString(stations[hoveredStationIndex].name);
            float dotPlotX = 50 + (stations[hoveredStationIndex].x - minX) * scaleX;
            float dotPlotY = static_cast<float>(window.getSize().y) - 150 - (stations[hoveredStationIndex].y - minY) * scaleY;
            float dotRadius = stations[hoveredStationIndex].circle.getRadius();
            float textWidth = hoverText.getLocalBounds().width;
            float textHeight = hoverText.getLocalBounds().height;
            float boxPadding = 5.0f;
            float boxWidth = textWidth + 2 * boxPadding;
            float boxHeight = textHeight + 2 * boxPadding;
            float boxX = dotPlotX + dotRadius + boxPadding;
            float boxY = dotPlotY - boxHeight / 2.0f;

            if (boxX + boxWidth > window.getSize().x) {
                boxX = dotPlotX - dotRadius - boxWidth - boxPadding;
            }
            if (boxY < 0) {
                boxY = dotPlotY + dotRadius + boxPadding;
            }
            else if (boxY + boxHeight > window.getSize().y) {
                boxY = dotPlotY - dotRadius - boxHeight - boxPadding;
            }

            hoverBox.setSize({ boxWidth, boxHeight });
            hoverBox.setPosition(boxX, boxY);
            hoverText.setPosition(boxX + boxPadding, boxY + boxPadding);
            window.draw(hoverBox);
            window.draw(hoverText);
        }
    }

    void handleWelcomePageEvents(sf::Event& event) {
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            if (sourceDropdownBounds.contains(mousePos)) {
                isSourceDropdownOpen = !isSourceDropdownOpen;
                isDestinationDropdownOpen = false; // Close other dropdown
            }
            else if (destinationDropdownBounds.contains(mousePos)) {
                isDestinationDropdownOpen = !isDestinationDropdownOpen;
                isSourceDropdownOpen = false; // Close other dropdown
            }
            else if (isSourceDropdownOpen) {
                for (size_t i = 0; i < sourceStationList.size(); ++i) {
                    if (sourceStationList[i].getGlobalBounds().contains(mousePos)) {
                        sourceStation = stations[i].name;
                        sourceDropdownText.setString(sourceStation);
                        isSourceDropdownOpen = false;
                        break;
                    }
                }
            }
             else if (isDestinationDropdownOpen) {
                for (size_t i = 0; i < destinationStationList.size(); ++i) {
                    if (destinationStationList[i].getGlobalBounds().contains(mousePos)) {
                        destinationStation = stations[i].name;
                        destinationDropdownText.setString(destinationStation);
                        isDestinationDropdownOpen = false;
                        break;
                    }
                }
            }
            else if (searchButtonBounds.contains(mousePos)) {
                 if (!sourceStation.empty() && !destinationStation.empty() && stationsMap.count(sourceStation) && stationsMap.count(destinationStation)) {
                    shortestPathResult = metroGraph.findShortestPath(sourceStation, destinationStation);
                    minExchangePathResult = metroGraph.findMinimumExchanges(sourceStation, destinationStation);
                    setupMapPage();
                    currentPage = Page::MAP;
                }
                else {
                    // Optionally display an error message
                }
            }
            else{
                isSourceDropdownOpen = false;
                isDestinationDropdownOpen = false;
            }
        }
        /*else if (event.type == sf::Event::TextEntered) { // Removed TextEntered event
            if (isSourceSelected) {
                if (event.text.unicode < 128) { //handle only ASCII characters
                    if (event.text.unicode == 8) { // Backspace
                        if (!sourceInputText.isEmpty())
                            sourceInputText.erase(sourceInputText.getSize() - 1, 1);
                    }
                    else if (event.text.unicode == 13) { // Enter key pressed
                        isSourceSelected = false;
                    }
                    else {
                        sourceInputText += static_cast<char>(event.text.unicode);
                    }
                    sourceInput.setString(sourceInputText);
                    sourceStation = sourceInputText;
                }
            }
            else if (isDestinationSelected) {
                if (event.text.unicode < 128) {
                    if (event.text.unicode == 8) {
                        if (!destinationInputText.isEmpty())
                            destinationInputText.erase(destinationInputText.getSize() - 1, 1);
                    }
                    else if (event.text.unicode == 13) { // Enter key pressed
                        isDestinationSelected = false;
                    }
                    else {
                        destinationInputText += static_cast<char>(event.text.unicode);
                    }
                    destinationInput.setString(destinationInputText);
                    destinationStation = destinationInputText;
                }
            }
        }*/
    }

    void handleMapPageEvents(sf::Event& event) {
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            if (backButtonBounds.contains(mousePos)) {
                setupWelcomePage();
                currentPage = Page::WELCOME;
            }
        }
        else if (event.type == sf::Event::MouseMoved) {
            mousePosition = window.mapPixelToCoords({ event.mouseMove.x, event.mouseMove.y });
            hoveredStationIndex = -1;
            for (size_t i = 0; i < stations.size(); ++i) {
                float plotX = 50 + (stations[i].x - minX) * scaleX;
                float plotY = static_cast<float>(window.getSize().y) - 150 - (stations[i].y - minY) * scaleY;
                stations[i].circle.setPosition(plotX, plotY);
                if (stations[i].circle.getGlobalBounds().contains(mousePosition)) {
                    hoveredStationIndex = i;
                    break;
                }
            }
        }
    }
    sf::Color getColorFromName(const std::string& colorName) {
        if (colorName == "Red") return sf::Color::Red;
        if (colorName == "Blue") return sf::Color::Blue;
        if (colorName == "Green") return sf::Color::Green;
        if (colorName == "Yellow") return sf::Color::Yellow;
        if (colorName == "Violet") return sf::Color::Magenta;
        if (colorName == "Orange") return sf::Color::Cyan;
        if (colorName == "Pink") return sf::Color::Magenta;
        // Add more colors as needed
        return sf::Color::White; // Default color
    }
};

int main(){
    MetroApp app(1200, 800);
    app.run();
    return 0;
}

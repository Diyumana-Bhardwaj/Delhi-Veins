// g++ metro_gui.cpp -o metro_gui.exe -I C:/SFML-2.6.1/include -L C:/SFML-2.6.1/lib -lsfml-graphics -lsfml-window -lsfml-system metro_coordinates.csv fonts/OpenSans-Regular.ttf
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>

struct Station {
    std::string name;
    float x;
    float y;
    sf::Color color;
    sf::CircleShape circle;
};

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

int main() {
    std::vector<Station> stations;
    std::ifstream file("metro_coordinates.csv");
    std::string line;

    std::getline(file, line); // Skip header

    static sf::Font font;
    if (!font.loadFromFile("fonts/OpenSans-Regular.ttf")) {
        std::cerr << "Error loading font!" << std::endl;
        return -1;
    }

    static sf::Font boldFont;
    if (!boldFont.loadFromFile("fonts/OpenSans-Bold.ttf")) {
        std::cerr << "Error loading font!" << std::endl;
        return -1;
    }

    int windowWidth = 1200;
    int windowHeight = 800;
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Delhi Metro Map");

    float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
    if (!stations.empty()) {
        minX = stations[0].x;
        maxX = stations[0].x;
        minY = stations[0].y;
        maxY = stations[0].y;
    }

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
        } else {
            if (station.x < minX) minX = station.x;
            if (station.x > maxX) maxX = station.x;
            if (station.y < minY) minY = station.y;
            if (station.y > maxY) maxY = station.y;
        }

        stations.push_back(station);
    }

    float scaleX = (windowWidth - 100) / (maxX - minX);
    float scaleY = (windowHeight - 100) / (maxY - minY);

    sf::Text hoverText;
    hoverText.setFont(boldFont); // Use bold font
    hoverText.setCharacterSize(14);
    hoverText.setFillColor(sf::Color::Black);

    sf::RectangleShape hoverBox;
    hoverBox.setFillColor(sf::Color(255, 255, 255, 200)); // Semi-transparent white
    hoverBox.setOutlineColor(sf::Color::Black);
    hoverBox.setOutlineThickness(1.0f);

    int hoveredStationIndex = -1;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
                hoveredStationIndex = -1; // Reset hover

                for (size_t i = 0; i < stations.size(); ++i) {
                    float plotX = 50 + (stations[i].x - minX) * scaleX;
                    float plotY = windowHeight - 50 - (stations[i].y - minY) * scaleY;
                    stations[i].circle.setPosition(plotX, plotY); // Ensure circle position is updated

                    if (stations[i].circle.getGlobalBounds().contains(mousePos)) {
                        hoveredStationIndex = i;
                        break;
                    }
                }
            }
        }

        window.clear(sf::Color::White);

        for (auto& s : stations) {
            float plotX = 50 + (s.x - minX) * scaleX;
            float plotY = windowHeight - 50 - (s.y - minY) * scaleY;
            s.circle.setPosition(plotX, plotY);
            window.draw(s.circle);
        }

        // Draw hover information in a box at the top right of the hovered dot
        if (hoveredStationIndex != -1) {
            hoverText.setString(stations[hoveredStationIndex].name);

            float dotPlotX = 50 + (stations[hoveredStationIndex].x - minX) * scaleX;
            float dotPlotY = windowHeight - 50 - (stations[hoveredStationIndex].y - minY) * scaleY;
            float dotRadius = stations[hoveredStationIndex].circle.getRadius();

            float textWidth = hoverText.getLocalBounds().width;
            float textHeight = hoverText.getLocalBounds().height;
            float boxPadding = 5.0f; // Reduced padding for dot-relative box

            float boxWidth = textWidth + 2 * boxPadding;
            float boxHeight = textHeight + 2 * boxPadding;
            float boxX = dotPlotX + dotRadius + boxPadding; // Right of the dot
            float boxY = dotPlotY - boxHeight / 2.0f;     // Vertically centered with the dot

            // Adjust position if the box goes out of the window bounds (right side)
            if (boxX + boxWidth > windowWidth) {
                boxX = dotPlotX - dotRadius - boxWidth - boxPadding; // Left of the dot
            }

            // Adjust position if the box goes out of the window bounds (top)
            if (boxY < 0) {
                boxY = dotPlotY + dotRadius + boxPadding; // Below the dot
            }
            // Adjust position if the box goes out of the window bounds (bottom)
            else if (boxY + boxHeight > windowHeight) {
                boxY = dotPlotY - dotRadius - boxHeight - boxPadding; // Above the dot
            }

            hoverBox.setSize({boxWidth, boxHeight});
            hoverBox.setPosition(boxX, boxY);

            hoverText.setPosition(boxX + boxPadding, boxY + boxPadding);

            window.draw(hoverBox);
            window.draw(hoverText);
        }

        window.display();
    }

    return 0;
}
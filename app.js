const sourceDropdown = document.getElementById('source');
const destinationDropdown = document.getElementById('destination');
const searchButton = document.getElementById('search-button');
const metroMapContainer = document.getElementById('metro-map-container');
const showMapButton = document.getElementById('Show-Map');
const header = document.querySelector('header');
const homeContainer = document.getElementById('home-container');
const mapArea = document.getElementById('map-area');
const Search_Result = document.getElementById('Search-Result');
const resultButtonsDiv = document.getElementById('result-buttons');
const pathHeading = document.getElementById('path-heading');
const totalTimeSpan = document.getElementById('total-time');
const totalFareSpan = document.getElementById('total-fare');
const totalInterchangesSpan = document.getElementById('total-interchanges');
const totalInterchangesResult = document.getElementById('total-interchanges-result');
const totalDistanceSpan = document.getElementById('total-distance');
const shortestPathBtn = document.getElementById('shortest-path-btn');
const minInterchangeBtn = document.getElementById('min-interchange-btn');
const sourceSpan= document.getElementById('source_station');
const destinationSpan = document.getElementById('destination_station');

    let allStations = [];
    let mapVisible = false;
    let currentPath = []; // To store the current path for map highlighting

    function getColorFromName(colorName) {
        switch (colorName.toLowerCase()) {
            case 'blue': return 'blue';
            case 'red': return 'red';
            case 'yellow': return 'yellow';
            case 'green': return 'green';
            case 'violet': return 'purple';
            case 'magenta': return 'magenta';
            case 'orange': return 'orange';
            case 'pink': return 'hotpink';
            default: return 'black';
        }
    }

    async function populateDropdowns() {
        try {
            const response = await fetch('metro_coordinates.csv');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const csvData = await response.text();
            const lines = csvData.trim().split('\n');
            const header = lines[0].split(',');
            const stationsData = lines.slice(1).map(line => {
                const values = line.split(',');
                return {
                    name: values[0].trim(),
                    x: parseFloat(values[1]),
                    y: parseFloat(values[2]),
                    color: values[3].trim()
                };
            });

            allStations = stationsData.map(station => station.name).sort();

            allStations.forEach(stationName => {
                const option1 = document.createElement('option');
                option1.value = stationName;
                option1.textContent = stationName;
                sourceDropdown.appendChild(option1);

                const option2 = document.createElement('option');
                option2.value = stationName;
                option2.textContent = stationName;
                destinationDropdown.appendChild(option2);
            });

        } catch (error) {
            console.error('Error loading station names:', error);
        }
    }

    async function loadAndDisplayMap(showConnections = true) { // Added showConnections parameter
        const mapContainerDiv = document.getElementById('metro-map-container');
        const mapDiv = document.getElementById('metro-map');
        const tooltipDiv = document.createElement('div');
        tooltipDiv.className = 'tooltip';
        document.body.appendChild(tooltipDiv);

        try {
            const coordinatesResponse = await fetch('metro_coordinates.csv');
            if (!coordinatesResponse.ok) {
                throw new Error(`HTTP error! status: ${coordinatesResponse.status}`);
            }
            const coordinatesCsvData = await coordinatesResponse.text();
            const coordinatesLines = coordinatesCsvData.trim().split('\n');
            const coordinatesHeader = coordinatesLines[0].split(',');
            const stationsData = coordinatesLines.slice(1).map(line => {
                const values = line.split(',');
                return {
                    name: values[0].trim(),
                    x: parseFloat(values[1]),
                    y: parseFloat(values[2]),
                    color: values[3].trim()
                };
            });

            // Calculate bounds for scaling
            let minX, maxX, minY, maxY;
            if (stationsData.length > 0) {
                minX = maxX = stationsData[0].x;
                minY = maxY = stationsData[0].y;
                stationsData.forEach(station => {
                    minX = Math.min(minX, station.x);
                    maxX = Math.max(maxX, station.x);
                    minY = Math.min(minY, station.y);
                    maxY = Math.max(maxY, station.y);
                });
            }

            const padding = 40;
            const mapWidth = 1000;
            const mapHeight = 650;
            mapContainerDiv.style.width = `${mapWidth}px`;
            mapContainerDiv.style.height = `${mapHeight}px`;


            const scaleX = (mapWidth - 2 * padding) / (maxX - minX);
            const scaleY = (mapHeight - 2 * padding) / (maxY - minY);

            // Clear any existing map elements
            mapDiv.innerHTML = '';

            stationsData.forEach(station => {
                const dot = document.createElement('div');
                dot.className = 'station';
                const radius = 6;
                dot.style.width = `${2 * radius}px`;
                dot.style.height = `${2 * radius}px`;
                dot.style.backgroundColor = getColorFromName(station.color);
                const xPos = padding + (station.x - minX) * scaleX - radius;
                const flippedYPos = mapHeight - (padding + (station.y - minY) * scaleY + radius * 2);

                dot.style.left = `${xPos}px`;
                dot.style.top = `${flippedYPos}px`;
                dot.setAttribute('data-station-name', station.name);
                station.dot = dot;
                mapDiv.appendChild(dot);
            });

            if (showConnections) { // Only draw connections if showConnections is true
                // Load and draw connections
                const linesResponse = await fetch('Delhi_Metro_Lines.csv');
                if (!linesResponse.ok) {
                    throw new Error(`HTTP error! status: ${linesResponse.status}`);
                }
                const linesCsvData = await linesResponse.text();
                const connections = parseMetroLines(linesCsvData);
                drawConnections(stationsData, connections, mapDiv);
                attachStationTooltips(stationsData, mapDiv, tooltipDiv);
            } else {
                attachStationTooltips(stationsData, mapDiv, tooltipDiv); // Still attach tooltips even without connections
            }


        } catch (error) {
            console.error('Error loading or processing CSV:', error);
            mapDiv.textContent = 'Error loading metro data.';
        }
    }

    function parseMetroLines(csvData) {
        const lines = csvData.trim().split('\n').slice(1);
        const connections = [];
        lines.forEach(line => {
            const values = line.split(',');
            if (values.length === 4) {
                connections.push({
                    station1: values[0].trim(),
                    station2: values[1].trim(),
                    color: values[2].trim(),
                    distance: parseFloat(values[3])
                });
            }
        });
        return connections;
    }

    function drawConnections(stations, connections, mapDiv) {
        const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("width", "100%");
        svg.setAttribute("height", "100%");
        svg.style.position = 'absolute';
        svg.style.pointerEvents = 'none';
        svg.style.zIndex = '2'; // Add this line

        connections.forEach(connection => {
            const station1 = stations.find(s => s.name === connection.station1);
            const station2 = stations.find(s => s.name === connection.station2);

            if (station1 && station2) {
                const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
                line.setAttribute("x1", parseFloat(station1.dot.style.left) + parseFloat(station1.dot.style.width) / 2);
                line.setAttribute("y1", parseFloat(station1.dot.style.top) + parseFloat(station1.dot.style.height) / 2);
                line.setAttribute("x2", parseFloat(station2.dot.style.left) + parseFloat(station2.dot.style.width) / 2);
                line.setAttribute("y2", parseFloat(station2.dot.style.top) + parseFloat(station2.dot.style.height) / 2);
                line.setAttribute("stroke", getColorFromName(connection.color.toLowerCase()));
                line.setAttribute("stroke-width", 2);
                svg.appendChild(line);
            }
        });

        mapDiv.appendChild(svg);
    }

    function attachStationTooltips(stationsData, mapDiv, tooltipDiv) {
        stationsData.forEach(station => {
            const dot = mapDiv.querySelector(`[data-station-name="${station.name}"]`);
            if (dot) {
                dot.addEventListener('mousemove', (event) => {
                    tooltipDiv.textContent = station.name;
                    tooltipDiv.style.left = `${event.clientX + 10}px`;
                    tooltipDiv.style.top = `${event.clientY + 10}px`;
                    tooltipDiv.classList.add('visible');
                });

                dot.addEventListener('mouseleave', () => {
                    tooltipDiv.classList.remove('visible');
                });
            }
        });
    }
    async function highlightPathOnMap(stations, path) {
        const mapDiv = document.getElementById('metro-map');
        const svg = mapDiv.querySelector('svg');
        if (!svg || path.length < 2) return;
    
        svg.querySelectorAll('line').forEach(line => line.remove());
        const connections = parseMetroLines(localStorage.getItem('metro_lines_data'));
    
        for (let i = 0; i < path.length - 1; i++) {
            const station1 = stations.find(s => s.name === path[i]);
            const station2 = stations.find(s => s.name === path[i + 1]);
            const connection = connections.find(conn =>
                (conn.station1 === path[i] && conn.station2 === path[i + 1]) ||
                (conn.station1 === path[i + 1] && conn.station2 === path[i])
            );
    
            if (station1 && station2 && connection) {
                const lineElement = document.createElementNS("http://www.w3.org/2000/svg", "line");
                lineElement.setAttribute("x1", parseFloat(station1.dot.style.left) + parseFloat(station1.dot.style.width) / 2);
                lineElement.setAttribute("y1", parseFloat(station1.dot.style.top) + parseFloat(station1.dot.style.height) / 2);
                lineElement.setAttribute("x2", parseFloat(station2.dot.style.left) + parseFloat(station2.dot.style.width) / 2);
                lineElement.setAttribute("y2", parseFloat(station2.dot.style.top) + parseFloat(station2.dot.style.height) / 2);
                lineElement.setAttribute("stroke", getColorFromName(connection.color));
                lineElement.setAttribute("stroke-width", 4);
                lineElement.classList.add('path-segment');
    
                svg.appendChild(lineElement);
    
                // Wait for a bit before drawing next segment
                await new Promise(resolve => setTimeout(resolve, 300)); // 300ms delay between segments
            }
        }
        path.forEach(name => {
            const station = stations.find(s => s.name === name);
            if (station && station.dot) {
                station.dot.classList.add('highlighted-station');
            }
        })
    }
    
    
    
    

    // Initial setup:
    populateDropdowns(); // Populate dropdowns on page load

    fetch('Delhi_Metro_Lines.csv')
    .then(response => response.text())
    .then(data => localStorage.setItem('metro_lines_data', data))
    .catch(error => console.error('Error loading metro lines data:', error));

    showMapButton.addEventListener('click', () => {
        header.style.display = 'flex';
        homeContainer.style.display = 'none';
        mapArea.style.display = 'flex';
        metroMapContainer.style.display='flex';
        loadAndDisplayMap(true); // Show map with connections
    });

    searchButton.addEventListener('click', () => {
        const source = sourceDropdown.value;
        const destination = destinationDropdown.value;

        if (source && destination && source !== destination) {
            header.style.display = 'flex';
            homeContainer.style.display = 'none';
            mapArea.style.display = 'flex';
            metroMapContainer.style.display='flex';
            loadAndDisplayMap(false); // Show only dots with tooltips
            resultButtonsDiv.style.display = 'block';
            Search_Result.style.display = 'none'; // Hide results initially after a new search
            pathHeading.textContent = '';
            currentPath = []; // Clear previous path
            highlightPathOnMap([], []); // Clear any map highlights
        } else {
            alert("Please select different source and destination stations.");
            resultButtonsDiv.style.display = 'none';
            resultDiv.style.display = 'none';
            pathHeading.textContent = '';
            highlightPathOnMap([], []);
        }
    });

    shortestPathBtn.addEventListener('click', async () => {
        const source = sourceDropdown.value;
        const destination = destinationDropdown.value;
    
        if (source && destination && source !== destination) {
            try {
                const response = await fetch(`http://localhost:8080/shortest_path?source=${encodeURIComponent(source)}&destination=${encodeURIComponent(destination)}`);
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const data = await response.json();
    
                if (data.error) {
                    alert(data.error);
                    Search_Result.style.display = 'none';
                    pathHeading.textContent = '';
                    highlightPathOnMap([], []);
                } else {
                    pathHeading.textContent = 'Shortest Path Route';
                    sourceSpan.textContent = source;
                    destinationSpan.textContent = destination;
                    totalTimeSpan.textContent = Math.round(data.total_distance * 60 / 40);
                    totalFareSpan.textContent = calculateFare(data.total_distance);
                    totalInterchangesResult.style.display = 'none';
                    totalDistanceSpan.textContent = data.total_distance.toFixed(2);
                    Search_Result.style.display = 'block';
                    currentPath = data.path;
                    await loadAndDisplayMap(true); // Load map with connections
                    const stationsData = await fetch('metro_coordinates.csv') // Fetch station coordinates again
                        .then(response => response.text())
                        .then(csvData => {
                            const lines = csvData.trim().split('\n').slice(1);
                            return lines.map(line => {
                                const values = line.split(',');
                                return { name: values[0].trim(), x: parseFloat(values[1]), y: parseFloat(values[2]), color: values[3].trim(), dot: document.querySelector(`[data-station-name="${values[0].trim()}"]`) };
                            });
                        });
                    highlightPathOnMap(stationsData, currentPath);
                }
            } catch (error) {
                console.error('Error fetching shortest path:', error);
                alert('Failed to fetch shortest path.');
                Search_Result.style.display = 'none';
                pathHeading.textContent = '';
                highlightPathOnMap([], []);
            }
        } else {
            alert("Please select source and destination stations first.");
        }
    });
    
    minInterchangeBtn.addEventListener('click', async () => {
        const source = sourceDropdown.value;
        const destination = destinationDropdown.value;
    
        if (source && destination && source !== destination) {
            try {
                const response = await fetch(`http://localhost:8080/min_exchanges?source=${encodeURIComponent(source)}&destination=${encodeURIComponent(destination)}`);
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const data = await response.json();
    
                if (data.error) {
                    alert(data.error);
                    Search_Result.style.display = 'none';
                    pathHeading.textContent = '';
                    highlightPathOnMap([], []);
                } else {
                    console.log("Minimum interchange response:", data);
                    pathHeading.textContent = 'Minimum Interchange Route';
                    sourceSpan.textContent = source;
                    destinationSpan.textContent = destination;
                    totalTimeSpan.textContent = Math.round(data.total_distance * 60 / 40);
                    totalFareSpan.textContent = calculateFare(data.total_distance);
                    totalInterchangesSpan.textContent = data.total_line_changes;
                    totalInterchangesResult.style.display = 'block';
                    totalDistanceSpan.textContent = data.total_distance.toFixed(2);
                    Search_Result.style.display = 'block';
                    currentPath = data.path;
                    await loadAndDisplayMap(true); // Load map with connections
                    const stationsData = await fetch('metro_coordinates.csv') // Fetch station coordinates again
                        .then(response => response.text())
                        .then(csvData => {
                            const lines = csvData.trim().split('\n').slice(1);
                            return lines.map(line => {
                                const values = line.split(',');
                                return { name: values[0].trim(), x: parseFloat(values[1]), y: parseFloat(values[2]), color: values[3].trim(), dot: document.querySelector(`[data-station-name="${values[0].trim()}"]`) };
                            });
                        });
                    highlightPathOnMap(stationsData, currentPath);
                }
            } catch (error) {
                console.error('Error fetching minimum exchange path:', error);
                alert('Failed to fetch minimum interchange path.');
                Search_Result.style.display = 'none';
                pathHeading.textContent = '';
                highlightPathOnMap([], []);
            }
        } else {
            alert("Please select source and destination stations first.");
        }
    });
    
    // Placeholder for fare calculation - replace with your actual logic
    function calculateFare(distance) {
        // Example rough calculation:
        if (distance <= 2) return 10;
        if (distance <= 5) return 15;
        if (distance <= 12) return 20;
        if (distance <= 21) return 30;
        if (distance <= 32) return 40;
        return 50;
    }
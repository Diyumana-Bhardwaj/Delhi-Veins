<h1 align="center">🛤️ Delhi Veins</h1>
<p align="center">
  <em>A complete metro route visualizer & pathfinder built on the real Delhi Metro network using Dijkstra’s algorithm and custom GUI.</em>
</p>

<p align="center">
  <img src="assets/delhi-banner.png" width="80%" alt="Delhi Veins Banner"/>
</p>

---

## 📌 Project Highlights

- 🔁 **Shortest Path Finder:** Implements Dijkstra’s algorithm to calculate the minimum number of stations.
- 🔄 **Minimum Line Exchange Path:** Uses BFS to compute paths with the least metro line changes.
- 🗺️ **Real Map Rendering:** GUI built with **SFML**, using true station coordinates.
- 🧮 **Fare & Distance Calculator:** Computes approximate travel cost based on station hops.
- 📂 **Custom Dataset:** Curated and cleaned Delhi Metro data from scratch using ChatGPT and external geolocation sources.
- 🌐 **Web-ready Data:** Terminal version upgraded to a graphical one with full **httplib** integration in the web version.

---

## 🧠 Project Overview

### 🧾 Problem Statement
Create a real-time metro route pathfinder that simulates Delhi Metro’s functioning with visual clarity and algorithmic accuracy.

### 🎯 Objectives
- Visualize metro stations and routes accurately
- Find optimal travel paths with respect to:
  - Shortest travel (least stations)
  - Least interchanges (minimum line changes)
- Calculate fare dynamically
- Represent station connectivity and lines with real coordinates and colors

---

## 💻 Tech Stack

| Domain               | Tools / Technologies                            |
|----------------------|--------------------------------------------------|
| Programming Language | C++, JavaScript (web version)                   |
| GUI Framework        | SFML (Simple and Fast Multimedia Library)       |
| Algorithms           | Dijkstra’s Algorithm, BFS                       |
| Backend (Web ver.)   | `httplib` (C++ HTTP Server Client Library)      |
| Data Format          | CSV (for stations & connections)                |

---

## 📂 Folder Structure


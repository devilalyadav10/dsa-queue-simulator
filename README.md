# 🚦 Traffic Junction Simulator

Welcome to the **Traffic Junction Simulator** for **COMP202: Data Structures and Algorithms** (Jan 2025). This project simulates a traffic intersection with four roads (A, B, C, D), each with three lanes, using queue-based vehicle management and SDL2 for visualization. 🚗


## 🌟 Project Overview

This simulator manages traffic at a junction with 12 lanes (A1-A3, B1-B3, C1-C3, D1-D3), handling:
- **Normal Condition**: Serves lanes based on vehicle count.
- **High-Priority Condition**: Prioritizes lane A2 when it has >10 vehicles, until <5.
- **Emergency Condition**: Serves any lane with >15 vehicles immediately.
- **Visualization**: Displays roads, vehicles, and traffic lights with animations.

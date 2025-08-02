# 🚦 Traffic Junction Simulator

Welcome to the **Traffic Junction Simulator** for **COMP202: Data Structures and Algorithms** (Jan 2025). This project simulates a traffic intersection with four roads (A, B, C, D), each with three lanes, using queue-based vehicle management and SDL2 for visualization. 🚗


## 🌟 Project Overview

This simulator manages traffic at a junction with 12 lanes (A1-A3, B1-B3, C1-C3, D1-D3), handling:
- **Normal Condition**: Serves lanes based on vehicle count.
- **High-Priority Condition**: Prioritizes lane A2 when it has >10 vehicles, until <5.
- **Emergency Condition**: Serves any lane with >15 vehicles immediately.
- **Visualization**: Displays roads, vehicles, and traffic lights with animations.


The system uses file-based communication (`vehicles.data`) between the simulator and a vehicle generator, with thread-safe queue processing.

## 📋 Features

- **Queue Management**:
  - 12 vehicle queues (one per lane).
  - Priority queue for lane scheduling, with A2 prioritized at >10 vehicles.
- **Traffic Logic**:
  - Normal: Highest vehicle count lane served.
  - High-Priority: A2 served first if >10 vehicles.
  - Emergency: Immediate service for lanes with >15 vehicles.
  - One lane green at a time to avoid deadlock.
- **GUI**: SDL2-based visualization with animated lights and vehicle movement.
- **Multithreading**: Separate threads for GUI rendering, queue processing, and file reading.
- **Logging**: Console output for vehicle additions, dequeues, and queue status.


## 🛠️ Installation & Setup

### Prerequisites
- **Compiler**: GCC
- **Libraries**: SDL2, SDL2_ttf, pthread
- **OS**: Linux (tested on Ubuntu; Windows untested, see [SDL2 Installation](https://wiki.libsdl.org/SDL2/Installation))

### Installation
1. **Install Dependencies and compile** (Ubuntu):

   ```bash
   sudo apt update
   sudo apt install gcc libsdl2-dev libsdl2-ttf-dev

   gcc simulator.c queue.c -o simulator -lSDL2 -lSDL2_ttf -pthread
   gcc traffic_generator.c -o traffic_generator
2. **Run traffic_gen in one terminal**:
   
   ```bash
   ./traffic_gen
3. **Run simulator in another terminal**:
   
   ```bash
   ./sim

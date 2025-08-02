# 🚦 Traffic Junction Simulator

Welcome to the **Traffic Junction Simulator** for **COMP202: Data Structures and Algorithms** (Jan 2025). This project simulates a traffic intersection with four roads (A, B, C, D), each with three lanes, using queue-based vehicle management and SDL2 for visualization. 🚗


## 🌟 Project Overview

This simulator manages traffic at a junction with 12 lanes (A1-A3, B1-B3, C1-C3, D1-D3), handling:
- **Normal Condition**: Serves lanes based on vehicle count.
- **High-Priority Condition**: Prioritizes lane A2 when it has >10 vehicles, until <5.
- **Emergency Condition**: Serves any lane with >15 vehicles immediately.
- **Visualization**: Displays roads, vehicles, and traffic lights with animations.


## 📂 Project Structure

- `simulator.c`: Main program with GUI, queue processing, and traffic logic.
- `traffic_generator.c`: Generates random vehicles, writes to vehicles.data.
- `queue.c`: Implements queue operations (enqueue, dequeue, etc.).
- `queue.h`: Defines queue structures and prototypes.


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
1. **Install Dependencies** (Ubuntu):

   ```bash
   sudo apt update
   sudo apt install gcc libsdl2-dev libsdl2-ttf-dev

2. **Compile**:
   ```bash
   gcc simulator.c queue.c -o simulator -lSDL2 -lSDL2_ttf -pthread
   gcc traffic_generator.c -o traffic_generator
3. **Run traffic_gen in one terminal**:
   
   ```bash
   ./traffic_gen
4. **Run simulator in another terminal**:
   
   ```bash
   ./sim


## 📊 How it Works?

- Vehicle Generation: traffic_generator.c creates vehicles (e.g., AB0CD123) every 1.5 seconds, writing to vehicles.data.
- File Reading: simulator.c reads vehicles.data in a thread, enqueuing vehicles to the correct lane.
- Queue Processing: A thread processes one vehicle every 4 seconds.
- Visualization: SDL2 renders the junction, vehicles (with license plates), and traffic lights with smooth transitions.


## 📚 References

- SDL2 Documentation
- Standard C libraries (stdio, string, etc.)
- Assignment-1-LinearDataStructure.pdf
- guide.txt

## Simulator Logs: 
![Simulator Terminal Logs](https://dxeul8wagn2zr.cloudfront.net/g7drcf%2Fpreview%2F69580633%2Fmain_large.gif?response-content-disposition=inline%3Bfilename%3D%22main_large.gif%22%3B&response-content-type=image%2Fgif&Expires=1754152307&Signature=KfhMwMAdzp1NUhh5wGAC1l-eL7czb88gWvjryOycYswuYH6AltDg6LZwoy6hYVoM32YVGvjpxw2gNbdGY-PzM3j2-VzfD-gpfiRsS9qjliklTyJ4QS1PYUx7j7xXkCKDRWxNrSweiX5TMQb9KTPmQsdW7AbFoVkmMPl-I6TFQ13hVZkneEnBXEgLD4fpWX1ufr3wNOQPnfvCEMXzde0mqXudOjrRsTEvXK5FykYnhggBBgjeJ~iITeL--t3FjqrtLb1jgTWAs~wKqGouwyth4FkGeIin7cD53I20A7yJ3UyFeZBgB7bRJSj~pKDXgxd9N6RLx0gPvIyBGZpPyl8Pjg__&Key-Pair-Id=APKAJT5WQLLEOADKLHBQ)


## Traffic Generator Logs: 
![Simulator Terminal Logs]([https://dxeul8wagn2zr.cloudfront.net/g7drcf%2Fpreview%2F69580633%2Fmain_large.gif?response-content-disposition=inline%3Bfilename%3D%22main_large.gif%22%3B&response-content-type=image%2Fgif&Expires=1754152307&Signature=KfhMwMAdzp1NUhh5wGAC1l-eL7czb88gWvjryOycYswuYH6AltDg6LZwoy6hYVoM32YVGvjpxw2gNbdGY-PzM3j2-VzfD-gpfiRsS9qjliklTyJ4QS1PYUx7j7xXkCKDRWxNrSweiX5TMQb9KTPmQsdW7AbFoVkmMPl-I6TFQ13hVZkneEnBXEgLD4fpWX1ufr3wNOQPnfvCEMXzde0mqXudOjrRsTEvXK5FykYnhggBBgjeJ~iITeL--t3FjqrtLb1jgTWAs~wKqGouwyth4FkGeIin7cD53I20A7yJ3UyFeZBgB7bRJSj~pKDXgxd9N6RLx0gPvIyBGZpPyl8Pjg__&Key-Pair-Id=APKAJT5WQLLEOADKLHBQ](https://d3rshtj5w2m4qx.cloudfront.net/7aqcf%2Fpreview%2F69580594%2Fmain_large.gif?response-content-disposition=inline%3Bfilename%3D%22main_large.gif%22%3B&response-content-type=image%2Fgif&Expires=1754152217&Signature=UkNwo57-GNccAlS03cMdENxmU7haP-3RZ4vpHazNBa3BMmTL2nvGLKvul0Vbw7DPI8H2f7i0m61XdixizDxsvFc47EF-cTVUxcdt7kHQZ7cc5tKxEUS3ZcnDsbjkWor8nwEre3tvvcS~irgdtnmVd2N1SEskwt-uBHe7x36b3PwaMGBXKc8mQFzmQG64uhkFGdXtn2EAuR4kozAEl1mBIGBhv86XDjFSzsH~JqoqIetZp3SdwD-t4TCV11tPsQERDJir6KuwvtMR8pRaHIVG7~66tXvO5gF-dZ0XklKQy~NTSWiJJmhpaJaXX9qkZ3c3-X5Xss2YyoM4WafEIq6o-Q__&Key-Pair-Id=APKAJT5WQLLEOADKLHBQ))


## Simulator Visual: 
![Simulator Terminal Logs](https://d2b6stxjw39da8.cloudfront.net/83qcf%2Fpreview%2F69580587%2Fmain_large.gif?response-content-disposition=inline%3Bfilename%3D%22main_large.gif%22%3B&response-content-type=image%2Fgif&Expires=1754152152&Signature=MKgSBmjLhkaxIvMnOfkg283VFm7Bs5UvHAdMTYIi2G4OJ7CcMpczL~Kb4~t65-5VuF5gfu2ZPO7JIQPkNDqbPxe~yjQrim0ko3qoe9dK7Xa2PY91WhfUyOTZdP4tSasq3LOR2jjemm1M485EQyvYw0xLmxSekOP1919U-Nc6DatCNTYBmciRYIVxuC70M5X~iGCd~VJqd1Xha72vmkWZ-2wpAtRZ11yyGgAyamZsGKvh02jjRBGbiM-rb9KGs7oyia~bxAl4n5lgl-XbbcSvBDIfIq-MGLS29H66ye0xaK-6RvtX8CiziHJu3akxxd7zE0NleRdWY~SBt4NV2I1ikw__&Key-Pair-Id=APKAJT5WQLLEOADKLHBQ)

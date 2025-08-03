#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "queue.h"

#define WINDOW_WIDTH 1400
#define WINDOW_HEIGHT 1000
#define ROAD_WIDTH 300
#define LANE_WIDTH 80
#define VEHICLE_WIDTH 40
#define VEHICLE_HEIGHT 20
#define VEHICLE_SPACING 10
#define LIGHT_RADIUS 12
#define MAX_VISIBLE_VEHICLES 8
#define TIME_PER_VEHICLE 4.0f // Increased from 1.0f
#define MAIN_FONT "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
#define PRIORITY_COOLDOWN 10
#define EMERGENCY_THRESHOLD 15
#define HIGH_PRIORITY_THRESHOLD 10
#define NORMAL_PRIORITY_THRESHOLD 5 // Changed from 3 to match assignment spec

const char *VEHICLE_FILE = "vehicles.data";

typedef struct
{
    Queue *queue;
    int priority;
    int road;
    int lane;
} PriorityQueueItem;

typedef struct
{
    int currentLight;
    int nextLight;
    SDL_mutex *mutex;
    float lightTransition;
    int high_priority_mode;
    int priority_cooldown;
    int emergency_override;
    float vehicle_process_timer;       // NEW: Timer for vehicle processing
    int vehicles_processed_this_cycle; // NEW: Track processed vehicles
} SharedData;

Queue laneA1, laneA2, laneA3;
Queue laneB1, laneB2, laneB3;
Queue laneC1, laneC2, laneC3;
Queue laneD1, laneD2, laneD3;
PriorityQueueItem priorityQueue[12];

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void displayText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, bool shadow);
void drawIntersection(SDL_Renderer *renderer, TTF_Font *font);
void drawTrafficLight(SDL_Renderer *renderer, bool isGreen, float transition, int x, int y, int road, int lane, TTF_Font *smallFont);
void drawVehicle(SDL_Renderer *renderer, int x, int y, char road, int lane, const char *plate, TTF_Font *smallFont, float offset);
void drawQueue(SDL_Renderer *renderer, Queue *queue, int startX, int startY, char road, int lane, TTF_Font *font, float offset, SharedData *sharedData);
void drawCurrentStatus(SDL_Renderer *renderer, TTF_Font *largeFont, int currentLight);
void render(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *largeFont, TTF_Font *smallFont, SharedData *sharedData);
void *processQueues(void *arg);
void *readAndParseFile(void *arg);
void initializePriorityQueue();
void updatePriorityQueue(SharedData *sharedData);
void printQueueStatus(SharedData *sharedData);
int getHighestPriorityLane();
int findMostCongestedLane(SharedData *sharedData);
void checkEmergencyOverflow(SharedData *sharedData);
SDL_Color getLaneColor(char road, int lane);

int main()
{
    pthread_t tQueue, tReadFile;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    printf("🚦 Traffic Junction Simulator Starting...\n");

    init_queue(&laneA1);
    init_queue(&laneA2);
    init_queue(&laneA3);
    init_queue(&laneB1);
    init_queue(&laneB2);
    init_queue(&laneB3);
    init_queue(&laneC1);
    init_queue(&laneC2);
    init_queue(&laneC3);
    init_queue(&laneD1);
    init_queue(&laneD2);
    init_queue(&laneD3);

    initializePriorityQueue();

    if (!initializeSDL(&window, &renderer))
    {
        fprintf(stderr, "SDL initialization failed\n");
        return -1;
    }

    SharedData sharedData = {0, 0, SDL_CreateMutex(), 0.0f, 0, 0, 0, 0.0f, 0};
    if (!sharedData.mutex)
    {
        fprintf(stderr, "Failed to create mutex: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    TTF_Font *font = TTF_OpenFont(MAIN_FONT, 18);
    TTF_Font *largeFont = TTF_OpenFont(MAIN_FONT, 32);
    TTF_Font *smallFont = TTF_OpenFont(MAIN_FONT, 10);
    if (!font || !largeFont || !smallFont)
    {
        fprintf(stderr, "Failed to load fonts: %s\n", TTF_GetError());
        SDL_DestroyMutex(sharedData.mutex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    pthread_create(&tQueue, NULL, processQueues, &sharedData);
    pthread_create(&tReadFile, NULL, readAndParseFile, &sharedData);

    bool running = true;
    float lastTime = SDL_GetTicks() / 1000.0f;
    printf("✅ Simulator initialized. Waiting for vehicles...\n");

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
        }

        float currentTime = SDL_GetTicks() / 1000.0f;
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        SDL_LockMutex(sharedData.mutex);

        // Update light transition animation
        if (sharedData.currentLight != 0)
            sharedData.lightTransition = fmin(sharedData.lightTransition + deltaTime * 2.0f, 1.0f);
        else
            sharedData.lightTransition = fmax(sharedData.lightTransition - deltaTime * 2.0f, 0.0f);

        render(renderer, font, largeFont, smallFont, &sharedData);
        SDL_UnlockMutex(sharedData.mutex);

        SDL_Delay(33); // ~30 FPS
    }

    pthread_cancel(tQueue);
    pthread_cancel(tReadFile);
    pthread_join(tQueue, NULL);
    pthread_join(tReadFile, NULL);

    SDL_DestroyMutex(sharedData.mutex);
    TTF_CloseFont(font);
    TTF_CloseFont(largeFont);
    TTF_CloseFont(smallFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0)
    {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    *window = SDL_CreateWindow("Traffic Junction Simulator",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window)
    {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer)
    {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    return true;
}

void initializePriorityQueue()
{
    priorityQueue[0] = (PriorityQueueItem){&laneA1, 0, 0, 1};
    priorityQueue[1] = (PriorityQueueItem){&laneA2, 1, 0, 2}; // AL2 starts with priority 1
    priorityQueue[2] = (PriorityQueueItem){&laneA3, 0, 0, 3};
    priorityQueue[3] = (PriorityQueueItem){&laneB1, 0, 1, 1};
    priorityQueue[4] = (PriorityQueueItem){&laneB2, 0, 1, 2};
    priorityQueue[5] = (PriorityQueueItem){&laneB3, 0, 1, 3};
    priorityQueue[6] = (PriorityQueueItem){&laneC1, 0, 2, 1};
    priorityQueue[7] = (PriorityQueueItem){&laneC2, 0, 2, 2};
    priorityQueue[8] = (PriorityQueueItem){&laneC3, 0, 2, 3};
    priorityQueue[9] = (PriorityQueueItem){&laneD1, 0, 3, 1};
    priorityQueue[10] = (PriorityQueueItem){&laneD2, 0, 3, 2};
    priorityQueue[11] = (PriorityQueueItem){&laneD3, 0, 3, 3};
}

void updatePriorityQueue(SharedData *sharedData)
{
    int al2_count = get_count(&laneA2);

    // High Priority Mode Logic (from assignment spec)
    if (al2_count > HIGH_PRIORITY_THRESHOLD) // >10 vehicles
    {
        if (!sharedData->high_priority_mode)
        {
            printf("🔴 HIGH PRIORITY MODE ACTIVATED - AL2 has %d vehicles\n", al2_count);
        }
        sharedData->high_priority_mode = 1;
        sharedData->priority_cooldown = PRIORITY_COOLDOWN;
    }
    else if (al2_count < NORMAL_PRIORITY_THRESHOLD) // <5 vehicles
    {
        if (sharedData->high_priority_mode)
        {
            printf("🟢 HIGH PRIORITY MODE DEACTIVATED - AL2 has %d vehicles\n", al2_count);
        }
        sharedData->high_priority_mode = 0;
        sharedData->priority_cooldown = 0;
    }

    // Update priorities
    for (int i = 0; i < 12; i++)
    {
        int count = get_count(priorityQueue[i].queue);
        if (priorityQueue[i].queue == &laneA2 && sharedData->high_priority_mode)
        {
            priorityQueue[i].priority = 1000; // Highest priority
        }
        else
        {
            priorityQueue[i].priority = count; // Normal priority based on queue length
        }
    }
}

int findMostCongestedLane(SharedData *sharedData)
{
    int maxCount = 0;
    for (int i = 0; i < 12; i++)
    {
        if (i != 1) // Skip AL2 in normal congestion check
        {
            int count = get_count(priorityQueue[i].queue);
            if (count > maxCount)
                maxCount = count;
        }
    }
    return maxCount;
}

void checkEmergencyOverflow(SharedData *sharedData)
{
    for (int i = 0; i < 12; i++)
    {
        int count = get_count(priorityQueue[i].queue);
        if (count > EMERGENCY_THRESHOLD)
        {
            printf("🚨 EMERGENCY OVERFLOW: Lane %d has %d vehicles\n", i + 1, count);
            sharedData->emergency_override = 1;
            sharedData->currentLight = i + 1;
            return;
        }
    }
    sharedData->emergency_override = 0;
}

int getHighestPriorityLane()
{
    int maxPriority = -1;
    int selectedLane = 0;

    for (int i = 0; i < 12; i++)
    {
        if (!is_empty(priorityQueue[i].queue))
        {
            if (priorityQueue[i].priority > maxPriority)
            {
                maxPriority = priorityQueue[i].priority;
                selectedLane = i + 1;
            }
        }
    }
    return selectedLane;
}

SDL_Color getLaneColor(char road, int lane)
{
    switch (road)
    {
    case 'A':
        return lane == 2 ? (SDL_Color){200, 60, 60, 255} : (SDL_Color){220, 100, 100, 255}; // AL2 is darker red
    case 'B':
        return (SDL_Color){60, 60, 200, 255};
    case 'C':
        return (SDL_Color){60, 200, 60, 255};
    case 'D':
        return (SDL_Color){200, 160, 60, 255};
    default:
        return (SDL_Color){150, 150, 150, 255};
    }
}

void displayText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, bool shadow)
{
    if (shadow)
    {
        SDL_Color shadowColor = {20, 20, 20, 180};
        SDL_Surface *shadowSurface = TTF_RenderText_Blended(font, text, shadowColor);
        if (shadowSurface)
        {
            SDL_Texture *shadowTexture = SDL_CreateTextureFromSurface(renderer, shadowSurface);
            SDL_Rect shadowRect = {x + 2, y + 2, shadowSurface->w, shadowSurface->h};
            SDL_RenderCopy(renderer, shadowTexture, NULL, &shadowRect);
            SDL_FreeSurface(shadowSurface);
            SDL_DestroyTexture(shadowTexture);
        }
    }

    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void drawIntersection(SDL_Renderer *renderer, TTF_Font *font)
{
    // Background gradient
    SDL_SetRenderDrawColor(renderer, 40, 45, 60, 255);
    SDL_RenderClear(renderer);

    // Draw roads
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect hRoad = {0, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_Rect vRoad = {WINDOW_WIDTH / 2 - ROAD_WIDTH / 2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &hRoad);
    SDL_RenderFillRect(renderer, &vRoad);

    // Draw intersection
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_Rect center = {WINDOW_WIDTH / 2 - ROAD_WIDTH / 2, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2, ROAD_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &center);

    // Draw lane dividers
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (int i = 1; i <= 2; i++) // Only 2 dividers for 3 lanes
    {
        // Horizontal road dividers
        int y = WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i;
        for (int x = 0; x < WINDOW_WIDTH; x += 40)
        {
            if (x < WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 || x > WINDOW_WIDTH / 2 + ROAD_WIDTH / 2)
                SDL_RenderDrawLine(renderer, x, y, x + 20, y);
        }

        // Vertical road dividers
        int x = WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i;
        for (int y2 = 0; y2 < WINDOW_HEIGHT; y2 += 40)
        {
            if (y2 < WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 || y2 > WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2)
                SDL_RenderDrawLine(renderer, x, y2, x, y2 + 20);
        }
    }

    // Draw directional labels
    SDL_Color white = {220, 220, 220, 255};
    displayText(renderer, font, "NORTH", WINDOW_WIDTH / 2 - 25, 10, white, true);
    displayText(renderer, font, "SOUTH", WINDOW_WIDTH / 2 - 25, WINDOW_HEIGHT - 30, white, true);
    displayText(renderer, font, "EAST", WINDOW_WIDTH - 50, WINDOW_HEIGHT / 2 - 15, white, true);
    displayText(renderer, font, "WEST", 10, WINDOW_HEIGHT / 2 - 15, white, true);
}

void drawTrafficLight(SDL_Renderer *renderer, bool isGreen, float transition, int x, int y, int road, int lane, TTF_Font *smallFont)
{
    // Shadow
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 100);
    SDL_Rect shadow = {x + 3, y + 3, LIGHT_RADIUS * 2 + 6, LIGHT_RADIUS * 2 + 6};
    SDL_RenderFillRect(renderer, &shadow);

    // Light background
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect bg = {x, y, LIGHT_RADIUS * 2, LIGHT_RADIUS * 2};
    SDL_RenderFillRect(renderer, &bg);

    // Light color with smooth transition
    int r = isGreen ? (int)(50 * (1.0f - transition)) : (int)(255 * transition);
    int g = isGreen ? (int)(255 * transition) : (int)(50 * (1.0f - transition));
    int b = 0;

    SDL_SetRenderDrawColor(renderer, r, g, b, 255);

    // Draw circular light
    for (int i = 0; i < LIGHT_RADIUS * 2; i++)
    {
        for (int j = 0; j < LIGHT_RADIUS * 2; j++)
        {
            int dx = i - LIGHT_RADIUS;
            int dy = j - LIGHT_RADIUS;
            if (dx * dx + dy * dy <= (LIGHT_RADIUS - 2) * (LIGHT_RADIUS - 2))
                SDL_RenderDrawPoint(renderer, x + i, y + j);
        }
    }

    // Lane label
    char roadNames[] = {'A', 'B', 'C', 'D'};
    char laneText[5];
    snprintf(laneText, sizeof(laneText), "%cL%d", roadNames[road], lane);
    SDL_Color white = {220, 220, 220, 255};
    displayText(renderer, smallFont, laneText, x - 5, y + LIGHT_RADIUS * 2 + 5, white, true);
}

void drawVehicle(SDL_Renderer *renderer, int x, int y, char road, int lane, const char *plate, TTF_Font *smallFont, float offset)
{
    SDL_Color color = getLaneColor(road, lane);

    // Shadow
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 100);
    SDL_Rect shadow = {x + 2, y + 2, VEHICLE_WIDTH, VEHICLE_HEIGHT};
    SDL_RenderFillRect(renderer, &shadow);

    // Vehicle body with movement offset
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect vehicle = {
        x + (int)(offset * (road == 'C' || road == 'D' ? VEHICLE_WIDTH + VEHICLE_SPACING : 0)),
        y + (int)(offset * (road == 'A' || road == 'B' ? VEHICLE_HEIGHT + VEHICLE_SPACING : 0)),
        VEHICLE_WIDTH, VEHICLE_HEIGHT};
    SDL_RenderFillRect(renderer, &vehicle);

    // Windshield
    SDL_SetRenderDrawColor(renderer, 180, 180, 220, 255);
    SDL_Rect windshield = {vehicle.x + 5, vehicle.y + 2, VEHICLE_WIDTH - 10, VEHICLE_HEIGHT / 3};
    SDL_RenderFillRect(renderer, &windshield);

    // License plate (first 3 chars)
    if (strlen(plate) >= 3)
    {
        char shortPlate[4] = {0};
        strncpy(shortPlate, plate, 3);
        SDL_Color black = {0, 0, 0, 255};
        displayText(renderer, smallFont, shortPlate, vehicle.x + 5, vehicle.y + VEHICLE_HEIGHT / 2 - 3, black, false);
    }
}

void drawQueue(SDL_Renderer *renderer, Queue *queue, int startX, int startY, char road, int lane, TTF_Font *font, float offset, SharedData *sharedData)
{
    int count = get_count(queue);
    int selectedLane = (sharedData->currentLight - 1);
    bool isGreen = (sharedData->currentLight != 0 && selectedLane >= 0 && selectedLane < 12 &&
                    priorityQueue[selectedLane].road == (road - 'A') && priorityQueue[selectedLane].lane == lane);

    for (int i = 0; i < count && i < MAX_VISIBLE_VEHICLES; i++)
    {
        int x = startX, y = startY;

        // Base position based on road direction
        if (road == 'A' || road == 'B') // Vertical roads
        {
            y += i * (VEHICLE_HEIGHT + VEHICLE_SPACING);
            if (isGreen)
            {
                // First vehicle moves to center, others follow proportionally
                float moveDistance = (i == 0) ? offset * (road == 'A' ? (WINDOW_HEIGHT / 2 - y - VEHICLE_HEIGHT) : -(y - (WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2)))
                                              : offset * (VEHICLE_HEIGHT + VEHICLE_SPACING) * 0.8f; // Slower follow for realism
                y += (int)moveDistance;
            }
        }
        else // Horizontal roads (C, D)
        {
            x += i * (VEHICLE_WIDTH + VEHICLE_SPACING);
            if (isGreen)
            {
                float moveDistance = (i == 0) ? offset * (road == 'D' ? (WINDOW_WIDTH / 2 - x - VEHICLE_WIDTH) : -(x - (WINDOW_WIDTH / 2 - ROAD_WIDTH / 2)))
                                              : offset * (VEHICLE_WIDTH + VEHICLE_SPACING) * 0.8f;
                x += (int)moveDistance;
            }
        }

        // Skip drawing first vehicle if it's close to the center (about to be dequeued)
        if (isGreen && i == 0 && offset > 0.95f) // Hide when near center
            continue;

        char plate[9] = "XXX";
        if (i < queue->count)
        {
            memcpy(plate, queue->items[(queue->front + i) % MAX_QUEUE_SIZE].vehicle_id, 8);
            plate[8] = '\0';
        }

        drawVehicle(renderer, x, y, road, lane, plate, font, 0.0f);
    }
}

void drawCurrentStatus(SDL_Renderer *renderer, TTF_Font *largeFont, int currentLight)
{
    char buffer[100];
    if (currentLight == 0)
    {
        strcpy(buffer, "🔴 ALL LANES: RED");
    }
    else
    {
        char roadName = "ABCD"[(currentLight - 1) / 3];
        int lane = (currentLight - 1) % 3 + 1;
        snprintf(buffer, sizeof(buffer), "SERVING: %cL%d", roadName, lane);
    }

    SDL_Color white = {220, 220, 220, 255};
    displayText(renderer, largeFont, buffer, WINDOW_WIDTH / 2 - 150, 30, white, true);
}

void render(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *largeFont, TTF_Font *smallFont, SharedData *sharedData)
{
    drawIntersection(renderer, font);

    // Draw traffic lights for all lanes
    int lightPositions[12][2] = {
        // Road A (North)
        {WINDOW_WIDTH / 2 - LANE_WIDTH - LIGHT_RADIUS, 120}, // AL1
        {WINDOW_WIDTH / 2 - LIGHT_RADIUS, 120},              // AL2
        {WINDOW_WIDTH / 2 + LANE_WIDTH - LIGHT_RADIUS, 120}, // AL3
        // Road B (South)
        {WINDOW_WIDTH / 2 - LANE_WIDTH - LIGHT_RADIUS, WINDOW_HEIGHT - 140}, // BL1
        {WINDOW_WIDTH / 2 - LIGHT_RADIUS, WINDOW_HEIGHT - 140},              // BL2
        {WINDOW_WIDTH / 2 + LANE_WIDTH - LIGHT_RADIUS, WINDOW_HEIGHT - 140}, // BL3
        // Road C (East)
        {WINDOW_WIDTH - 140, WINDOW_HEIGHT / 2 - LANE_WIDTH - LIGHT_RADIUS}, // CL1
        {WINDOW_WIDTH - 140, WINDOW_HEIGHT / 2 - LIGHT_RADIUS},              // CL2
        {WINDOW_WIDTH - 140, WINDOW_HEIGHT / 2 + LANE_WIDTH - LIGHT_RADIUS}, // CL3
        // Road D (West)
        {120, WINDOW_HEIGHT / 2 - LANE_WIDTH - LIGHT_RADIUS}, // DL1
        {120, WINDOW_HEIGHT / 2 - LIGHT_RADIUS},              // DL2
        {120, WINDOW_HEIGHT / 2 + LANE_WIDTH - LIGHT_RADIUS}  // DL3
    };

    for (int i = 0; i < 12; i++)
    {
        bool isGreen = (sharedData->currentLight == i + 1);
        drawTrafficLight(renderer, isGreen, sharedData->lightTransition,
                         lightPositions[i][0], lightPositions[i][1],
                         priorityQueue[i].road, priorityQueue[i].lane, smallFont);
    }

    // Vehicle movement offset for animation
    float offset = (sharedData->currentLight != 0) ? fmod(SDL_GetTicks() / 1000.0f, TIME_PER_VEHICLE) / TIME_PER_VEHICLE : 0.0f;

    // Draw vehicle queues
    drawQueue(renderer, &laneA1, WINDOW_WIDTH / 2 - LANE_WIDTH - VEHICLE_WIDTH / 2, 160, 'A', 1, smallFont, offset, sharedData);
    drawQueue(renderer, &laneA2, WINDOW_WIDTH / 2 - VEHICLE_WIDTH / 2, 160, 'A', 2, smallFont, offset, sharedData);
    drawQueue(renderer, &laneA3, WINDOW_WIDTH / 2 + LANE_WIDTH - VEHICLE_WIDTH / 2, 160, 'A', 3, smallFont, offset, sharedData);

    drawQueue(renderer, &laneB1, WINDOW_WIDTH / 2 - LANE_WIDTH - VEHICLE_WIDTH / 2, WINDOW_HEIGHT - 240, 'B', 1, smallFont, offset, sharedData);
    drawQueue(renderer, &laneB2, WINDOW_WIDTH / 2 - VEHICLE_WIDTH / 2, WINDOW_HEIGHT - 240, 'B', 2, smallFont, offset, sharedData);
    drawQueue(renderer, &laneB3, WINDOW_WIDTH / 2 + LANE_WIDTH - VEHICLE_WIDTH / 2, WINDOW_HEIGHT - 240, 'B', 3, smallFont, offset, sharedData);

    drawQueue(renderer, &laneC1, WINDOW_WIDTH - 240, WINDOW_HEIGHT / 2 - LANE_WIDTH - VEHICLE_HEIGHT / 2, 'C', 1, smallFont, offset, sharedData);
    drawQueue(renderer, &laneC2, WINDOW_WIDTH - 240, WINDOW_HEIGHT / 2 - VEHICLE_HEIGHT / 2, 'C', 2, smallFont, offset, sharedData);
    drawQueue(renderer, &laneC3, WINDOW_WIDTH - 240, WINDOW_HEIGHT / 2 + LANE_WIDTH - VEHICLE_HEIGHT / 2, 'C', 3, smallFont, offset, sharedData);

    drawQueue(renderer, &laneD1, 160, WINDOW_HEIGHT / 2 - LANE_WIDTH - VEHICLE_HEIGHT / 2, 'D', 1, smallFont, offset, sharedData);
    drawQueue(renderer, &laneD2, 160, WINDOW_HEIGHT / 2 - VEHICLE_HEIGHT / 2, 'D', 2, smallFont, offset, sharedData);
    drawQueue(renderer, &laneD3, 160, WINDOW_HEIGHT / 2 + LANE_WIDTH - VEHICLE_HEIGHT / 2, 'D', 3, smallFont, offset, sharedData);

    drawCurrentStatus(renderer, largeFont, sharedData->currentLight);
    SDL_RenderPresent(renderer);
}

void *processQueues(void *arg)
{
    SharedData *sharedData = (SharedData *)arg;
    int status_counter = 0;
    float lastProcessTime = SDL_GetTicks() / 1000.0f;

    printf("🔧 Queue processing thread started\n");

    while (1)
    {
        float currentTime = SDL_GetTicks() / 1000.0f;

        SDL_LockMutex(sharedData->mutex);

        // Print status every 5 seconds
        if (status_counter++ % 25 == 0)
        {
            printQueueStatus(sharedData);
        }

        // Update priority and check emergency conditions
        updatePriorityQueue(sharedData);
        checkEmergencyOverflow(sharedData);

        // Process vehicles based on timing
        if (currentTime - lastProcessTime >= TIME_PER_VEHICLE)
        {
            if (sharedData->high_priority_mode && !sharedData->emergency_override)
            {
                // HIGH PRIORITY MODE: Serve AL2 first
                if (!is_empty(&laneA2))
                {
                    sharedData->currentLight = 2; // AL2
                    Vehicle v = dequeue(&laneA2);
                    printf("🔴 [PRIORITY] Dequeued: %s from AL2 (count now: %d)\n",
                           v.vehicle_id, get_count(&laneA2));
                    lastProcessTime = currentTime;
                }
                else
                {
                    sharedData->currentLight = 0; // No vehicles in priority lane
                }
            }
            else
            {
                // NORMAL MODE: Serve highest priority lane
                int selectedLane = getHighestPriorityLane();
                if (selectedLane > 0)
                {
                    sharedData->currentLight = selectedLane;
                    Queue *laneToServe = priorityQueue[selectedLane - 1].queue;

                    if (!is_empty(laneToServe))
                    {
                        Vehicle v = dequeue(laneToServe);
                        printf("🟢 [NORMAL] Dequeued: %s from %cL%d (count now: %d)\n",
                               v.vehicle_id,
                               'A' + priorityQueue[selectedLane - 1].road,
                               priorityQueue[selectedLane - 1].lane,
                               get_count(laneToServe));
                        lastProcessTime = currentTime;
                    }
                }
                else
                {
                    sharedData->currentLight = 0; // No vehicles in any lane
                }
            }
        }

        SDL_UnlockMutex(sharedData->mutex);
        usleep(200000); // Check every 200ms
    }
    return NULL;
}

void *readAndParseFile(void *arg)
{
    SharedData *sharedData = (SharedData *)arg;
    printf("📁 File reading thread started\n");

    while (1)
    {
        FILE *file = fopen(VEHICLE_FILE, "r");
        if (!file)
        {
            // Don't spam error messages if file doesn't exist yet
            usleep(1000000); // Wait 1 second before trying again
            continue;
        }

        SDL_LockMutex(sharedData->mutex);

        char line[100];
        int vehicles_added = 0;

        while (fgets(line, sizeof(line), file))
        {
            // Remove newline
            line[strcspn(line, "\n")] = 0;

            // Skip empty lines
            if (strlen(line) == 0)
                continue;

            // Parse: VehicleID:Road:Lane
            char *vehicleNumber = strtok(line, ":");
            char *road = strtok(NULL, ":");
            char *laneStr = strtok(NULL, ":");

            if (vehicleNumber && road && laneStr)
            {
                Vehicle v;
                strncpy(v.vehicle_id, vehicleNumber, sizeof(v.vehicle_id) - 1);
                v.vehicle_id[sizeof(v.vehicle_id) - 1] = '\0';
                v.road = *road;
                v.lane = atoi(laneStr);

                // Find target queue
                Queue *target = NULL;
                switch (*road)
                {
                case 'A':
                    target = (v.lane == 1) ? &laneA1 : (v.lane == 2) ? &laneA2
                                                                     : &laneA3;
                    break;
                case 'B':
                    target = (v.lane == 1) ? &laneB1 : (v.lane == 2) ? &laneB2
                                                                     : &laneB3;
                    break;
                case 'C':
                    target = (v.lane == 1) ? &laneC1 : (v.lane == 2) ? &laneC2
                                                                     : &laneC3;
                    break;
                case 'D':
                    target = (v.lane == 1) ? &laneD1 : (v.lane == 2) ? &laneD2
                                                                     : &laneD3;
                    break;
                }

                if (target && !is_full(target))
                {
                    enqueue(target, v);
                    vehicles_added++;
                    printf("➕ Added vehicle %s to %cL%d\n", v.vehicle_id, v.road, v.lane);
                }
                else if (target && is_full(target))
                {
                    printf("⚠️  Lane %cL%d is full, cannot add %s\n", v.road, v.lane, v.vehicle_id);
                }
            }
        }

        SDL_UnlockMutex(sharedData->mutex);
        fclose(file);

        // Only clear the file if we actually processed vehicles
        // This prevents race conditions
        if (vehicles_added > 0)
        {
            FILE *temp = fopen(VEHICLE_FILE, "w");
            if (temp)
                fclose(temp);
            printf("📝 Processed %d vehicles, cleared file\n", vehicles_added);
        }

        usleep(1000000); // Check file every 1 second
    }
    return NULL;
}

void printQueueStatus(SharedData *sharedData)
{
    printf("\n═══════════════════════════════════════\n");
    printf("🚦 TRAFFIC JUNCTION STATUS\n");
    printf("═══════════════════════════════════════\n");
    printf("Road A: AL1=%2d | AL2=%2d | AL3=%2d\n",
           get_count(&laneA1), get_count(&laneA2), get_count(&laneA3));
    printf("Road B: BL1=%2d | BL2=%2d | BL3=%2d\n",
           get_count(&laneB1), get_count(&laneB2), get_count(&laneB3));
    printf("Road C: CL1=%2d | CL2=%2d | CL3=%2d\n",
           get_count(&laneC1), get_count(&laneC2), get_count(&laneC3));
    printf("Road D: DL1=%2d | DL2=%2d | DL3=%2d\n",
           get_count(&laneD1), get_count(&laneD2), get_count(&laneD3));
    printf("───────────────────────────────────────\n");
    printf("Priority Mode: %s | Current Light: %d\n",
           sharedData->high_priority_mode ? "🔴 HIGH" : "🟢 NORMAL",
           sharedData->currentLight);
    printf("═══════════════════════════════════════\n\n");
}
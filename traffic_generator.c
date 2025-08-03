#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define FILENAME "vehicles.data"

void generateVehicleNumber(char *buffer)
{
    buffer[0] = 'A' + rand() % 26;
    buffer[1] = 'A' + rand() % 26;
    buffer[2] = '0' + rand() % 10;
    buffer[3] = 'A' + rand() % 26;
    buffer[4] = 'A' + rand() % 26;
    buffer[5] = '0' + rand() % 10;
    buffer[6] = '0' + rand() % 10;
    buffer[7] = '0' + rand() % 10;
    buffer[8] = '\0';
}

int main()
{
    srand(time(NULL));

    while (1)
    {
        FILE *file = fopen(FILENAME, "a");
        if (!file)
        {
            fprintf(stderr, "Error opening vehicles.data: %s\n", strerror(errno));
            usleep(2000000);
            continue;
        }

        char vehicle[9];
        generateVehicleNumber(vehicle);
        char road = "ABCD"[rand() % 4];
        int lane = (rand() % 3) + 1;

        fprintf(file, "%s:%c:%d\n", vehicle, road, lane);
        fflush(file);
        fclose(file);
        printf("Generated: %s:%c:%d\n", vehicle, road, lane);
        usleep(1500000);
    }

    return 0;
}
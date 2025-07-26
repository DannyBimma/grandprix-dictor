/*Copyright (c) 2025 DannyBimma. All Rights Reserved.*/

#include <ctype.h>
#include <jansson.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DRIVERS 20
#define MAX_STRING_LENGTH 50
#define NUM_TEAMS 10
#define NUM_DRIVERS 20
#define SUCCESS 0
#define ERROR_INVALID_TEAM_INDEX -1
#define CONFIG_FILE "f1_config.json"

// Data structures
typedef struct {
  char name[MAX_STRING_LENGTH];
  char engine[MAX_STRING_LENGTH];
  bool isTopTeam;
} Team;

typedef struct {
  char name[MAX_STRING_LENGTH];
  int number;
  char country[MAX_STRING_LENGTH];
  char favoriteTrack[MAX_STRING_LENGTH];
  char homeTrack[MAX_STRING_LENGTH];
  Team *team;
  bool isTopDriver;
  bool isEliteDriver;
  int points;
  float percentage;
  int predictedPosition;
} Driver;

// Configuration data structure
typedef struct {
  char teamNames[NUM_TEAMS][MAX_STRING_LENGTH];
  char engines[NUM_TEAMS][MAX_STRING_LENGTH];
  bool isTopTeam[NUM_TEAMS];
  char driverNames[NUM_DRIVERS][MAX_STRING_LENGTH];
  int driverNumbers[NUM_DRIVERS];
  char driverCountries[NUM_DRIVERS][MAX_STRING_LENGTH];
  char driverFavTracks[NUM_DRIVERS][MAX_STRING_LENGTH];
  char driverHomeTracks[NUM_DRIVERS][MAX_STRING_LENGTH];
  int driverTeamIndices[NUM_DRIVERS];
  bool isTopDriver[NUM_DRIVERS];
  bool isEliteDriver[NUM_DRIVERS];
} F1Configuration;

// Prototypes
int initTeamsAndDrivers(Team teams[], Driver drivers[], int *driverCount,
                        const F1Configuration *config);
void calcPoints(Driver drivers[], int driverCount, const char *track,
                const char *condition);
void calcPercentages(Driver drivers[], int driverCount);
void predictPositions(Driver drivers[], int driverCount);
void printResults(Driver drivers[], int driverCount, const char *track,
                  const char *condition);
bool isStringInArray(const char *str, const char *array[], int size);
void toLowercase(char *str);
void usageInstructions(void);
F1Configuration *loadF1ConfigFromFile(const char *filename);
void freeF1Config(F1Configuration *config);

int main(int argc, char *argv[]) {
  Team teams[NUM_TEAMS];
  Driver drivers[MAX_DRIVERS];
  int driverCount = 0;
  char track[MAX_STRING_LENGTH] = "";
  char condition[MAX_STRING_LENGTH] = "";

  F1Configuration *config = loadF1ConfigFromFile(CONFIG_FILE);
  if (!config) {
    fprintf(stderr, "Config file? Not found! Program? Exiting!\n");

    return 1;
  }

  if (argc == 1) {
    printf("Error: Incorrect usage! No arguments provided!\n");
    usageInstructions();

    freeF1Config(config);

    return 1;
  }

  if (argc > 3) {
    printf("Error: Incorrect usage! Too many arguments provided!\n");
    usageInstructions();

    freeF1Config(config);

    return 1;
  } else if (argc >= 2) {
    strncpy(track, argv[1], MAX_STRING_LENGTH - 1);
    track[MAX_STRING_LENGTH - 1] = '\0';
  }

  if (argc == 3) {
    strncpy(condition, argv[2], MAX_STRING_LENGTH - 1);
    condition[MAX_STRING_LENGTH - 1] = '\0';
    toLowercase(condition);

    if (strcmp(condition, "wet") != 0 && strcmp(condition, "dry") != 0) {
      printf(
          "Error: Incorrect usage! Race condition must be 'wet' or 'dry'.\n");
      usageInstructions();

      freeF1Config(config);

      return 1;
    }
  }

  if (argc < 3) {
    printf("Note: For more accurate race predictions, run program with track "
           "name and race condition arguments.\n");
    usageInstructions();
  }

  if (initTeamsAndDrivers(teams, drivers, &driverCount, config) != SUCCESS) {
    fprintf(stderr, "Failed to initialise teams and drivers\n");

    freeF1Config(config);

    return 1;
  }

  calcPoints(drivers, driverCount, track, condition);
  calcPercentages(drivers, driverCount);
  predictPositions(drivers, driverCount);
  printResults(drivers, driverCount, track, condition);
  freeF1Config(config);

  return 0;
}

void usageInstructions(void) {
  printf("Usage: ./grand_prixdictor [track] [condition]\n");
  printf("Where [track] is the name of the race track or country\n");
  printf("And [condition] is either 'wet' or 'dry'\n");
  printf("Example: ./grand_prixdictor 'Monza' 'wet'\n");
}

void toLowercase(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    str[i] = tolower(str[i]);
  }
}

bool isStringInArray(const char *str, const char *array[], int size) {
  for (int i = 0; i < size; i++) {
    if (strcasecmp(str, array[i]) == 0) {
      return true;
    }
  }

  return false;
}

int initTeamsAndDrivers(Team teams[], Driver drivers[], int *driverCount,
                        const F1Configuration *config) {
  if (!teams || !drivers || !driverCount || !config) {
    return ERROR_INVALID_TEAM_INDEX;
  }

  // Initialisation loops
  for (int i = 0; i < NUM_TEAMS; i++) {
    strncpy(teams[i].name, config->teamNames[i], MAX_STRING_LENGTH - 1);
    teams[i].name[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(teams[i].engine, config->engines[i], MAX_STRING_LENGTH - 1);
    teams[i].engine[MAX_STRING_LENGTH - 1] = '\0';
    teams[i].isTopTeam = config->isTopTeam[i];
  }

  *driverCount = 0;
  for (int i = 0; i < NUM_DRIVERS; i++) {
    if (config->driverTeamIndices[i] < 0 ||
        config->driverTeamIndices[i] >= NUM_TEAMS) {
      return ERROR_INVALID_TEAM_INDEX;
    }

    strncpy(drivers[i].name, config->driverNames[i], MAX_STRING_LENGTH - 1);
    drivers[i].name[MAX_STRING_LENGTH - 1] = '\0';

    strncpy(drivers[i].country, config->driverCountries[i],
            MAX_STRING_LENGTH - 1);
    drivers[i].country[MAX_STRING_LENGTH - 1] = '\0';

    strncpy(drivers[i].favoriteTrack, config->driverFavTracks[i],
            MAX_STRING_LENGTH - 1);
    drivers[i].favoriteTrack[MAX_STRING_LENGTH - 1] = '\0';

    strncpy(drivers[i].homeTrack, config->driverHomeTracks[i],
            MAX_STRING_LENGTH - 1);
    drivers[i].homeTrack[MAX_STRING_LENGTH - 1] = '\0';

    drivers[i].number = config->driverNumbers[i];
    drivers[i].team = &teams[config->driverTeamIndices[i]];
    drivers[i].isTopDriver = config->isTopDriver[i];
    drivers[i].isEliteDriver = config->isEliteDriver[i];

    drivers[i].points = 0;
    drivers[i].percentage = 0.0f;
    drivers[i].predictedPosition = 0;

    (*driverCount)++;
  }

  return SUCCESS;
}

void calcPoints(Driver drivers[], int driverCount, const char *track,
                const char *condition) {
  // Skill points
  for (int i = 0; i < driverCount; i++) {
    if (drivers[i].team->isTopTeam) {
      drivers[i].points += 10;
    }

    if (drivers[i].isTopDriver) {
      drivers[i].points += 12;
    }

    if (drivers[i].isEliteDriver) {
      drivers[i].points += 15;
    }

    // Engine points
    if (strcmp(drivers[i].team->engine, "Mercedes") == 0 ||
        strcmp(drivers[i].team->engine, "Ferrari") == 0 ||
        strcmp(drivers[i].team->engine, "Honda RBPT") == 0) {
      drivers[i].points += 5;
    }

    // Track points
    if (track != NULL && strlen(track) > 0) {
      bool isFavoriteTrack = strcasecmp(track, drivers[i].favoriteTrack) == 0;
      bool isHomeTrack = strcasecmp(track, drivers[i].homeTrack) == 0 ||
                         strcasecmp(track, drivers[i].country) == 0;

      if (isFavoriteTrack && isHomeTrack) {
        drivers[i].points += 12;
      } else if (isFavoriteTrack || isHomeTrack) {
        drivers[i].points += 6;
      }
    }

    // Condition points
    if (condition != NULL && strcmp(condition, "wet") == 0 &&
        drivers[i].isTopDriver) {
      drivers[i].points += 6;
    }
  }
}

void calcPercentages(Driver drivers[], int driverCount) {
  int totalPoints = 0;

  for (int i = 0; i < driverCount; i++) {
    totalPoints += drivers[i].points;
  }

  for (int i = 0; i < driverCount; i++) {
    drivers[i].percentage = (float)drivers[i].points / totalPoints * 100.0;
  }
}

// Comparison function for qsort() to sort drivers by points
int compareDrivers(const void *a, const void *b) {
  Driver *driverA = (Driver *)a;
  Driver *driverB = (Driver *)b;

  if (driverA->points > driverB->points)
    return -1;
  if (driverA->points < driverB->points)
    return 1;
  return 0;
}

void predictPositions(Driver drivers[], int driverCount) {
  // Copy and sort drivers array
  Driver sortedDrivers[MAX_DRIVERS];
  memcpy(sortedDrivers, drivers, driverCount * sizeof(Driver));

  // By points (high to low)
  qsort(sortedDrivers, driverCount, sizeof(Driver), compareDrivers);

  // Assign positions into original driver array
  for (int i = 0; i < driverCount; i++) {
    for (int j = 0; j < driverCount; j++) {
      if (drivers[j].number == sortedDrivers[i].number) {
        drivers[j].predictedPosition = i + 1;

        break;
      }
    }
  }
}

void printResults(Driver drivers[], int driverCount, const char *track,
                  const char *condition) {
  printf("\n======= F1 Grand Prix Predictor =======\n\n");

  if (track != NULL && strlen(track) > 0) {
    printf("Track: %s\n", track);
  } else {
    printf("Track: Not specified\n");
  }

  if (condition != NULL && strlen(condition) > 0) {
    printf("Condition: %s\n", condition);
  } else {
    printf("Condition: Not specified\n");
  }

  printf("\n");

  // Sort drivers by predicted position
  Driver sortedDrivers[MAX_DRIVERS];
  memcpy(sortedDrivers, drivers, driverCount * sizeof(Driver));
  qsort(sortedDrivers, driverCount, sizeof(Driver), compareDrivers);

  Driver *grandprixWinner = &sortedDrivers[0];

  printf("The predicted winner is: %s (with a %.2f%% probability)\n\n",
         grandprixWinner->name, grandprixWinner->percentage);

  // Remaining grid positions
  printf("Predicted Grid:\n");
  printf("---------------------------------------------------------------------"
         "-----------\n");
  printf("| Pos | Driver        | Team           | Points | Probability | "
         "Number | Country       |\n");
  printf("---------------------------------------------------------------------"
         "-----------\n");

  for (int i = 0; i < driverCount; i++) {
    printf("| P%-2d | %-13s | %-14s | %-6d | %-10.2f%% | #%-5d | %-13s |\n",
           i + 1, sortedDrivers[i].name, sortedDrivers[i].team->name,
           sortedDrivers[i].points, sortedDrivers[i].percentage,
           sortedDrivers[i].number, sortedDrivers[i].country);
  }

  printf("---------------------------------------------------------------------"
         "-----------\n");
}

F1Configuration *loadF1ConfigFromFile(const char *filename) {
  F1Configuration *config = malloc(sizeof(F1Configuration));
  if (!config) {
    fprintf(stderr, "Failed to allocate memory for configuration\n");

    return NULL;
  }

  json_error_t error;
  json_t *root = json_load_file(filename, 0, &error);
  if (!root) {
    fprintf(stderr, "Error loading JSON file: %s\n", error.text);

    free(config);

    return NULL;
  }

  // Load team data
  json_t *teams = json_object_get(root, "teams");
  if (!json_is_array(teams)) {
    fprintf(stderr, "Teams must be an array\n");

    json_decref(root);

    free(config);

    return NULL;
  }

  size_t team_index;
  json_t *team;
  json_array_foreach(teams, team_index, team) {
    if (team_index >= NUM_TEAMS)
      break;

    json_t *name = json_object_get(team, "name");
    json_t *engine = json_object_get(team, "engine");
    json_t *isTop = json_object_get(team, "isTopTeam");

    if (json_is_string(name)) {
      strncpy(config->teamNames[team_index], json_string_value(name),
              MAX_STRING_LENGTH - 1);
      config->teamNames[team_index][MAX_STRING_LENGTH - 1] = '\0';
    }
    if (json_is_string(engine)) {
      strncpy(config->engines[team_index], json_string_value(engine),
              MAX_STRING_LENGTH - 1);
      config->engines[team_index][MAX_STRING_LENGTH - 1] = '\0';
    }
    if (json_is_boolean(isTop)) {
      config->isTopTeam[team_index] = json_boolean_value(isTop);
    }
  }

  // Load driver data
  json_t *drivers = json_object_get(root, "drivers");
  if (!json_is_array(drivers)) {
    fprintf(stderr, "Drivers must be an array\n");

    json_decref(root);

    free(config);

    return NULL;
  }

  size_t driver_index;
  json_t *driver;
  json_array_foreach(drivers, driver_index, driver) {
    if (driver_index >= NUM_DRIVERS)
      break;

    json_t *name = json_object_get(driver, "name");
    json_t *number = json_object_get(driver, "number");
    json_t *country = json_object_get(driver, "country");
    json_t *favTrack = json_object_get(driver, "favoriteTrack");
    json_t *homeTrack = json_object_get(driver, "homeTrack");
    json_t *teamIndex = json_object_get(driver, "teamIndex");
    json_t *isTop = json_object_get(driver, "isTopDriver");
    json_t *isElite = json_object_get(driver, "isEliteDriver");

    if (json_is_string(name)) {
      strncpy(config->driverNames[driver_index], json_string_value(name),
              MAX_STRING_LENGTH - 1);
      config->driverNames[driver_index][MAX_STRING_LENGTH - 1] = '\0';
    }
    if (json_is_integer(number))
      config->driverNumbers[driver_index] = json_integer_value(number);

    if (json_is_string(country)) {
      strncpy(config->driverCountries[driver_index], json_string_value(country),
              MAX_STRING_LENGTH - 1);
      config->driverCountries[driver_index][MAX_STRING_LENGTH - 1] = '\0';
    }
    if (json_is_string(favTrack)) {
      strncpy(config->driverFavTracks[driver_index],
              json_string_value(favTrack), MAX_STRING_LENGTH - 1);
      config->driverFavTracks[driver_index][MAX_STRING_LENGTH - 1] = '\0';
    }
    if (json_is_string(homeTrack)) {
      strncpy(config->driverHomeTracks[driver_index],
              json_string_value(homeTrack), MAX_STRING_LENGTH - 1);
      config->driverHomeTracks[driver_index][MAX_STRING_LENGTH - 1] = '\0';
    }
    if (json_is_integer(teamIndex))
      config->driverTeamIndices[driver_index] = json_integer_value(teamIndex);

    if (json_is_boolean(isTop))
      config->isTopDriver[driver_index] = json_boolean_value(isTop);

    if (json_is_boolean(isElite))
      config->isEliteDriver[driver_index] = json_boolean_value(isElite);
  }

  json_decref(root);

  return config;
}

void freeF1Config(F1Configuration *config) {
  if (config)
    free(config);
}

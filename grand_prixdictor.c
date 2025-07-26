/*Copyright (c) 2025 DannyBimma. All Rights Reserved.
 *
 * A program that predicts the complete race order of
 * a given F1 Grand Prix weekend based pertinent
 * calculations.
 *
 * */

#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_DRIVERS 20
#define MAX_STRING_LENGTH 50
#define MAX_URL_LENGTH 256
#define NUM_TEAMS 10
#define NUM_DRIVERS 20
#define SUCCESS 0
#define ERROR_INVALID_TEAM_INDEX -1
#define CONFIG_FILE "f1_config.json"
#define WEATHER_API_KEY ""
#define WEATHER_API_BASE_URL "http://api.openweathermap.org/data/2.5/weather"

typedef struct {
  char name[MAX_STRING_LENGTH];
  char engine[MAX_STRING_LENGTH];
  bool isTopTeam;
  int pitStopEfficiency;
  int tireStrategy;
  int aerodynamics;
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
  int overtakingAbility;
  int consistency;
  int experienceLevel;
  int wetWeatherSkill;
  int points;
  float percentage;
  int predictedPosition;
} Driver;

typedef struct {
  char teamNames[NUM_TEAMS][MAX_STRING_LENGTH];
  char engines[NUM_TEAMS][MAX_STRING_LENGTH];
  bool isTopTeam[NUM_TEAMS];
  int teamPitStopEfficiency[NUM_TEAMS];
  int teamTireStrategy[NUM_TEAMS];
  int teamAerodynamics[NUM_TEAMS];
  char driverNames[NUM_DRIVERS][MAX_STRING_LENGTH];
  int driverNumbers[NUM_DRIVERS];
  char driverCountries[NUM_DRIVERS][MAX_STRING_LENGTH];
  char driverFavTracks[NUM_DRIVERS][MAX_STRING_LENGTH];
  char driverHomeTracks[NUM_DRIVERS][MAX_STRING_LENGTH];
  int driverTeamIndices[NUM_DRIVERS];
  bool isTopDriver[NUM_DRIVERS];
  bool isEliteDriver[NUM_DRIVERS];
  int driverOvertaking[NUM_DRIVERS];
  int driverConsistency[NUM_DRIVERS];
  int driverExperience[NUM_DRIVERS];
  int driverWetSkill[NUM_DRIVERS];
} F1Configuration;

typedef struct {
  char description[MAX_STRING_LENGTH];
  float temperature;
  float humidity;
  float windSpeed;
  int rainProbability;
} WeatherData;

typedef struct {
  char *memory;
  size_t size;
} HTTPResponse;

int initTeamsAndDrivers(Team teams[], Driver drivers[], int *driverCount,
                        const F1Configuration *config);
void calcPoints(Driver drivers[], int driverCount, const char *track,
                const char *condition);
void calcEnhancedPoints(Driver drivers[], int driverCount, const char *track,
                       const char *condition, WeatherData *weather);
void calcPercentages(Driver drivers[], int driverCount);
void predictPositions(Driver drivers[], int driverCount);
void printResults(Driver drivers[], int driverCount, const char *track,
                  const char *condition);
bool isStringInArray(const char *str, const char *array[], int size);
void toLowercase(char *str);
void usageInstructions(void);
F1Configuration *loadF1ConfigFromFile(const char *filename);
void freeF1Config(F1Configuration *config);
WeatherData *getWeatherData(const char *location);
WeatherData *getSimulatedWeatherData(const char *location);
WeatherData *parseWeatherResponse(const char *jsonResponse);
void freeWeatherData(WeatherData *weather);
size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
int getDRSEffectiveness(const char *track);
int getTrackType(const char *track); // 1=street, 2=high-speed, 3=technical

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

  WeatherData *weather = NULL;
  if (strlen(track) > 0) {
    weather = getWeatherData(track);
  }

  if (weather) {
    calcEnhancedPoints(drivers, driverCount, track, condition, weather);
    freeWeatherData(weather);
  } else {
    calcPoints(drivers, driverCount, track, condition);
  }
  
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
    teams[i].pitStopEfficiency = config->teamPitStopEfficiency[i];
    teams[i].tireStrategy = config->teamTireStrategy[i];
    teams[i].aerodynamics = config->teamAerodynamics[i];
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
    drivers[i].overtakingAbility = config->driverOvertaking[i];
    drivers[i].consistency = config->driverConsistency[i];
    drivers[i].experienceLevel = config->driverExperience[i];
    drivers[i].wetWeatherSkill = config->driverWetSkill[i];

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

void calcEnhancedPoints(Driver drivers[], int driverCount, const char *track,
                       const char *condition, WeatherData *weather) {
  // Start with base points calculation
  calcPoints(drivers, driverCount, track, condition);
  
  int trackType = getTrackType(track);
  int drsEffectiveness = getDRSEffectiveness(track);
  
  for (int i = 0; i < driverCount; i++) {
    // Driver-specific metrics
    drivers[i].points += (drivers[i].overtakingAbility * drsEffectiveness) / 10;
    drivers[i].points += drivers[i].consistency;
    drivers[i].points += drivers[i].experienceLevel / 2;
    
    // Team granular factors
    drivers[i].points += drivers[i].team->pitStopEfficiency / 2;
    drivers[i].points += drivers[i].team->tireStrategy / 2;
    
    // Track-specific aerodynamics bonus
    if (trackType == 2) { // High-speed tracks
      drivers[i].points += drivers[i].team->aerodynamics / 2;
    } else if (trackType == 1) { // Street circuits
      drivers[i].points += drivers[i].overtakingAbility / 2;
    }
    
    // Enhanced weather calculations
    if (weather) {
      // Rain probability affects wet weather specialists
      if (weather->rainProbability > 30) {
        drivers[i].points += (drivers[i].wetWeatherSkill * weather->rainProbability) / 100;
      }
      
      // Temperature effects on tire performance
      if (weather->temperature > 30.0) { // Hot conditions
        drivers[i].points += drivers[i].team->tireStrategy / 3;
      } else if (weather->temperature < 15.0) { // Cold conditions
        drivers[i].points += drivers[i].experienceLevel / 3;
      }
      
      // Wind effects on aerodynamics
      if (weather->windSpeed > 20.0) {
        drivers[i].points += drivers[i].team->aerodynamics / 4;
      }
      
      // Humidity effects on consistency
      if (weather->humidity > 80.0) {
        drivers[i].points += drivers[i].consistency / 3;
      }
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

  // Load data
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
    json_t *pitStop = json_object_get(team, "pitStopEfficiency");
    json_t *tireStrat = json_object_get(team, "tireStrategy");
    json_t *aero = json_object_get(team, "aerodynamics");

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
    
    config->teamPitStopEfficiency[team_index] = json_is_integer(pitStop) ? json_integer_value(pitStop) : 5;
    config->teamTireStrategy[team_index] = json_is_integer(tireStrat) ? json_integer_value(tireStrat) : 5;
    config->teamAerodynamics[team_index] = json_is_integer(aero) ? json_integer_value(aero) : 5;
  }

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
    json_t *overtaking = json_object_get(driver, "overtakingAbility");
    json_t *consistency = json_object_get(driver, "consistency");
    json_t *experience = json_object_get(driver, "experienceLevel");
    json_t *wetSkill = json_object_get(driver, "wetWeatherSkill");

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
      
    // Load new driver metrics with defaults
    config->driverOvertaking[driver_index] = json_is_integer(overtaking) ? json_integer_value(overtaking) : 5;
    config->driverConsistency[driver_index] = json_is_integer(consistency) ? json_integer_value(consistency) : 5;
    config->driverExperience[driver_index] = json_is_integer(experience) ? json_integer_value(experience) : 5;
    config->driverWetSkill[driver_index] = json_is_integer(wetSkill) ? json_integer_value(wetSkill) : 5;
  }

  json_decref(root);

  return config;
}

void freeF1Config(F1Configuration *config) {
  if (config)
    free(config);
}

int getDRSEffectiveness(const char *track) {
  if (!track) return 5;
  
  // High DRS effectiveness tracks
  if (strcasecmp(track, "Monza") == 0 || strcasecmp(track, "Spa") == 0 ||
      strcasecmp(track, "Baku") == 0 || strcasecmp(track, "Jeddah") == 0) {
    return 8;
  }
  // Medium DRS effectiveness
  else if (strcasecmp(track, "Silverstone") == 0 || strcasecmp(track, "Austria") == 0 ||
           strcasecmp(track, "Bahrain") == 0) {
    return 6;
  }
  // Low DRS effectiveness (street circuits, technical tracks)
  else if (strcasecmp(track, "Monaco") == 0 || strcasecmp(track, "Hungary") == 0 ||
           strcasecmp(track, "Singapore") == 0) {
    return 3;
  }
  
  return 5; // Default
}

// Helper function to categorize track types
int getTrackType(const char *track) {
  if (!track) return 3;
  
  // Street circuits (1)
  if (strcasecmp(track, "Monaco") == 0 || strcasecmp(track, "Singapore") == 0 ||
      strcasecmp(track, "Baku") == 0 || strcasecmp(track, "Jeddah") == 0) {
    return 1;
  }
  // High-speed tracks (2)
  else if (strcasecmp(track, "Monza") == 0 || strcasecmp(track, "Spa") == 0 ||
           strcasecmp(track, "Silverstone") == 0) {
    return 2;
  }
  
  return 3; // Technical tracks (default)
}

size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  HTTPResponse *mem = (HTTPResponse *)userp;
  
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    return 0;
  }
  
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  
  return realsize;
}

WeatherData *getWeatherData(const char *location) {
  const char *api_key = getenv("OPENWEATHER_API_KEY");
  if (!api_key || strlen(api_key) == 0) {
    api_key = WEATHER_API_KEY;
  }
  
  if (strlen(api_key) == 0) {
    return getSimulatedWeatherData(location);
  }
  
  CURL *curl;
  CURLcode res;
  HTTPResponse response = {0};
  char url[512];
  
  snprintf(url, sizeof(url), "%s?q=%s&appid=%s&units=metric", 
           WEATHER_API_BASE_URL, location, api_key);
  
  curl = curl_easy_init();
  if (!curl) {
    return getSimulatedWeatherData(location);
  }
  
  response.memory = malloc(1);
  response.size = 0;
  
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  
  if (res != CURLE_OK || !response.memory) {
    if (response.memory) free(response.memory);
    return getSimulatedWeatherData(location);
  }
  
  WeatherData *weather = parseWeatherResponse(response.memory);
  free(response.memory);
  
  if (!weather) {
    return getSimulatedWeatherData(location);
  }
  
  return weather;
}

WeatherData *parseWeatherResponse(const char *jsonResponse) {
  json_error_t error;
  json_t *root = json_loads(jsonResponse, 0, &error);
  if (!root) {
    return NULL;
  }
  
  WeatherData *weather = malloc(sizeof(WeatherData));
  if (!weather) {
    json_decref(root);
    return NULL;
  }
  
  json_t *main = json_object_get(root, "main");
  json_t *wind = json_object_get(root, "wind");
  json_t *weatherArray = json_object_get(root, "weather");
  json_t *clouds = json_object_get(root, "clouds");
  
  if (main) {
    json_t *temp = json_object_get(main, "temp");
    json_t *humidity = json_object_get(main, "humidity");
    
    weather->temperature = json_is_number(temp) ? json_number_value(temp) : 20.0;
    weather->humidity = json_is_number(humidity) ? json_number_value(humidity) : 50.0;
  } else {
    weather->temperature = 20.0;
    weather->humidity = 50.0;
  }
  
  if (wind) {
    json_t *speed = json_object_get(wind, "speed");
    weather->windSpeed = json_is_number(speed) ? json_number_value(speed) * 3.6 : 10.0; // Convert m/s to km/h
  } else {
    weather->windSpeed = 10.0;
  }
  
  if (json_is_array(weatherArray) && json_array_size(weatherArray) > 0) {
    json_t *weatherObj = json_array_get(weatherArray, 0);
    json_t *description = json_object_get(weatherObj, "description");
    json_t *main_weather = json_object_get(weatherObj, "main");
    
    if (json_is_string(description)) {
      strncpy(weather->description, json_string_value(description), MAX_STRING_LENGTH - 1);
      weather->description[MAX_STRING_LENGTH - 1] = '\0';
    } else {
      strcpy(weather->description, "clear");
    }
    
    const char *main_str = json_is_string(main_weather) ? json_string_value(main_weather) : "";
    if (strstr(main_str, "Rain") || strstr(main_str, "Drizzle") || strstr(main_str, "Thunderstorm")) {
      weather->rainProbability = 80;
    } else if (strstr(main_str, "Clouds")) {
      weather->rainProbability = 30;
    } else {
      weather->rainProbability = 10;
    }
  } else {
    strcpy(weather->description, "clear");
    weather->rainProbability = 10;
  }
  
  if (clouds) {
    json_t *cloudiness = json_object_get(clouds, "all");
    if (json_is_number(cloudiness)) {
      int cloud_cover = json_number_value(cloudiness);
      weather->rainProbability = (weather->rainProbability + cloud_cover / 2) / 2;
    }
  }
  
  json_decref(root);
  return weather;
}

WeatherData *getSimulatedWeatherData(const char *location) {
  WeatherData *weather = malloc(sizeof(WeatherData));
  if (!weather) return NULL;
  
  srand(time(NULL));
  
  if (strcasecmp(location, "Monaco") == 0) {
    strcpy(weather->description, "partly cloudy");
    weather->temperature = 22.0 + (rand() % 8);
    weather->humidity = 65 + (rand() % 20);
    weather->windSpeed = 10 + (rand() % 15);
    weather->rainProbability = 20 + (rand() % 30);
  } else if (strcasecmp(location, "Silverstone") == 0 || strcasecmp(location, "Great Britain") == 0) {
    strcpy(weather->description, "overcast");
    weather->temperature = 15.0 + (rand() % 10);
    weather->humidity = 70 + (rand() % 25);
    weather->windSpeed = 15 + (rand() % 20);
    weather->rainProbability = 40 + (rand() % 40);
  } else if (strcasecmp(location, "Singapore") == 0) {
    strcpy(weather->description, "humid");
    weather->temperature = 28.0 + (rand() % 6);
    weather->humidity = 85 + (rand() % 10);
    weather->windSpeed = 5 + (rand() % 10);
    weather->rainProbability = 60 + (rand() % 30);
  } else {
    strcpy(weather->description, "clear");
    weather->temperature = 20.0 + (rand() % 15);
    weather->humidity = 50 + (rand() % 30);
    weather->windSpeed = 8 + (rand() % 12);
    weather->rainProbability = 10 + (rand() % 40);
  }
  
  return weather;
}

void freeWeatherData(WeatherData *weather) {
  if (weather) {
    free(weather);
  }
}

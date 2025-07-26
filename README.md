# Grand Prix Predictor

A Formula 1 Grand Prix prediction application written in C, that calculates the likely finishing positions of drivers based on a very shoddy algorithm.

## Features

- Predicts race outcomes based on team, driver, track, and real weather conditions
- Takes into account the following factors:
  - Team performance metrics (pit stop efficiency, tire strategy, aerodynamics)
  - Driver-specific abilities (overtaking, consistency, experience, wet weather skill)
  - Track characteristics (DRS effectiveness, track type)
  - Real-time weather data (temperature, humidity, wind, rain probability)
  - Command-line interface with optional arguments for track and weather

## Usage

### Build the application

```
make grand_prixdictor
```

### Run the application

Basic usage:

```
./grand_prixdictor
```

With track and condition parameters:

```
./grand_prixdictor [track] [condition]
```

Where:

- `[track]` is the name of the race track or country
- `[condition]` is either 'wet' or 'dry'

Examples:

```
./grand_prixdictor Monaco dry
./grand_prixdictor Silverstone wet
./grand_prixdictor Spain dry
```

## How It Works

The prediction algorithm, if you can even call it that, uses a points-based system:

1. **Team Probability Points**:

   - Top 5 teams (McLaren, Ferrari, Red Bull, Mercedes, Williams etc.): +10 points

2. **Driver Probability Points**:

   - Top 10 drivers: +12 points
   - Elite drivers: +15 additional points

3. **Engine Advantage Probability Points**:

   - Teams with top engines (Mercedes, Ferrari, Honda RBPT etc.): +5 points

4. **Track Advantage Probability**:

   - Driver at favorite track: +6 points
   - Driver at home track: +6 points
   - Driver at both favorite and home track: +12 points

5. **Weather Conditions**:
   - Top 10 drivers in wet conditions: +6 additional points

The application calculates the total probability points for each driver, converts them to percentages of all points available, and ranks them to predict finishing grid positions.

## Code Structure

- Data structures for teams and drivers
- Initialisation of 2025 F1 team and driver data
- Points calculation based on all factors
- Percentage and position calculation
- Fancy results display crafted by Claude AI

## Requirements

- GCC or compatible C compiler
- Make (for building)
- jansson library (JSON parsing)
- libcurl (HTTP requests)
- OpenWeatherMap API key (free registration)

## Weather API Setup (Optional)

For real weather data, get a free API key from [OpenWeatherMap](https://openweathermap.org/api):

1. Register at https://openweathermap.org/api
2. Get your free API key
3. Use one of these methods:

   **Method 1: Environment Variable (Recommended)**
   ```bash
   export OPENWEATHER_API_KEY="your_actual_api_key_here"
   ./grand_prixdictor Monaco dry
   ```

   **Method 2: Edit Source Code**
   Edit `grand_prixdictor.c` and replace the empty `WEATHER_API_KEY`:
   ```c
   #define WEATHER_API_KEY "your_actual_api_key_here"
   ```
   Then recompile: `make clean && make`

**Note**: Without an API key, the application uses simulated weather data as fallback.

## Cleanup

```
make clean
```

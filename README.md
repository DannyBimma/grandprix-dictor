# Grand Prix Predictor

A Formula 1 Grand Prix prediction application written in C, that calculates the likely finishing positions of drivers based on a very shoddy algorithm.

## Features

- Predicts race outcomes based on team, driver, track, and weather conditions
- Takes into account the following factors:
  - Top teams and drivers
  - Elite driver status (basically my faves)
  - Engine advantage
  - Favorite/home track advantage
  - Weather conditions (wet/dry)
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
- Standard C libraries

## Cleanup

```
make clean
```

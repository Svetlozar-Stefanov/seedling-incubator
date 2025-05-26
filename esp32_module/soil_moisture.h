
#define NUMBER_OF_SOIL_MOISTURE_SENSORS 4

void setupSoilMoisturePins();

int measureMoistureInPercentages();
const int * getMoistureMeasurements();
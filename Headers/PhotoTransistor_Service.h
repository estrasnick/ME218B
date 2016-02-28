/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef PhotoTransistor_H
#define PhotoTransistor_H

#include "ES_Types.h"

#define BEACON_INDEX_NW 0 
#define BEACON_INDEX_NE 1
#define BEACON_INDEX_SE 2
#define BEACON_INDEX_SW 3
 
// Public Function Prototypes
typedef struct {
	uint32_t period; 
	float lastEncoderAngle; 
	uint32_t lastUpdateTime;
	int priorBeacons[2];
	int xShift;
	int yShift;
	int thetaShift;
} Beacon;

bool InitPhotoTransistorService ( uint8_t Priority );
bool PostPhotoTransistorService( ES_Event ThisEvent );
ES_Event RunPhotoTransistorService( ES_Event ThisEvent );
void PhotoTransistor_InterruptResponse(void);

uint32_t GetLastUpdateTime(uint8_t beaconIndex);
void ResetUpdateTimes(void);
uint8_t mostRecentBeaconUpdate(void);
void ResetAligningToBucket(void);

float GetBeaconAngle_A(uint8_t beaconIndex);
float GetBeaconAngle_B(uint8_t beaconIndex);
float GetBeaconAngle_C(uint8_t beaconIndex);

int getXShift(uint8_t beaconIndex);
int getYShift(uint8_t beaconIndex);
int getThetaShift(uint8_t beaconIndex);

#endif 


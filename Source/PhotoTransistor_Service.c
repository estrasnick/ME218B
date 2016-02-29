/****************************************************************************
 Module
   PhotoTransistor_Service.c
 Description
		
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PhotoTransistor_Service.h"
#include "PositionLogic_Service.h"
#include "PeriscopeControl_Service.h"
#include "DriveTrainControl_Service.h"
#include "Helpers.h"
#include "DEFINITIONS.h"
#include "DriveTrainControl_Service.h"
#include "GameInfo.h"
#include "Master_SM.h"

/*----------------------------- Module Defines ----------------------------*/

#define PERIOD_MEASURING_ERROR_TOLERANCE 10 //in microseconds
#define NUMBER_PULSES_TO_BE_ALIGNED 3
#define NUMBER_PHOTOTRANSISTORS 1
#define NUMBER_BEACON_FREQUENCIES 4
#define NUMBER_PULSES_FOR_BUCKET 3

#define BEACON_F_NW 1450
#define BEACON_F_NE 1700  
#define BEACON_F_SE 1250
#define BEACON_F_SW 1950

#define BEACON_P_NW  690 //6206 //690
#define BEACON_P_NE  588  //5878 //588
#define BEACON_P_SE  800    //6399 //800
#define BEACON_P_SW  513    //6153 //513

#define BEACON_INDEX_NW 0
#define BEACON_INDEX_NE 1
#define BEACON_INDEX_SE 2
#define BEACON_INDEX_SW 3

#define DIRECTION 1 //(use 1 if CW and -1 if CCW)
	



/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

//static void UpdateLastPeriod(uint32_t period);

static uint8_t CheckForBeacon(uint32_t period);

//static bool IsBeaconMatch(uint8_t beaconIndex);

static bool ToleranceCheck(uint32_t period, uint32_t targetPeriod, uint8_t tolerance);

static bool TimeForUpdate(void);

static void ResetAverage(void);

static float CalculateAverage(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static Beacon beacons[] = 
{
	{.period = BEACON_P_NW, .priorBeacons = {BEACON_INDEX_NE, BEACON_INDEX_SE}, .xShift = 0, 							.yShift = 0, .thetaShift = 0}, //1450 Hz, NW
	{.period = BEACON_P_NE, .priorBeacons = {BEACON_INDEX_SE, BEACON_INDEX_SW}, .xShift = 0, 							.yShift = FIELD_LENGTH, .thetaShift = 270}, //1700 Hz, NE
	{.period = BEACON_P_SE, .priorBeacons = {BEACON_INDEX_SW, BEACON_INDEX_NW}, .xShift = FIELD_LENGTH, 	.yShift = FIELD_LENGTH, .thetaShift = 180}, //1250 Hz, SE
	{.period = BEACON_P_SW, .priorBeacons = {BEACON_INDEX_NW, BEACON_INDEX_NE}, .xShift = FIELD_LENGTH, 	.yShift = 0, .thetaShift = 90} //1950 Hz, SW
};


static uint32_t LastCapture;

static uint8_t LastBeacon = NULL_BEACON;

static bool AligningToBucket = false;

static uint32_t NumberSamples;
static float Sum;

static uint8_t LastBeacon;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPhotoTransistorService
 Parameters
     uint8_t : the priorty of this service
 Returns
     bool, false if error in initialization, true otherwise
****************************************************************************/
bool InitPhotoTransistorService ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;

  InitInputCapture(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	
	// post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
}

/****************************************************************************
 Function
     PostPhotoTransistorService
 Parameters
     EF_Event ThisEvent ,the event to post to the queue
 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostPhotoTransistorService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPhotoTransistorService
 Parameters
   ES_Event : the event to process
 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunPhotoTransistorService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
	if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == AVERAGE_BEACONS_TIMER))
	{
		switch (LastBeacon){
				case (BEACON_INDEX_NW):
					printf("\r\nNum ticks Beacon NW: %d\n\r\r\n", NumberSamples);
				break;
				case (BEACON_INDEX_NE):
					printf("\r\nNum ticks Beacon NE: %d\n\r\r\n", NumberSamples);
				break;
				case (BEACON_INDEX_SE):
					printf("\r\nNum ticks Beacon SE: %d\n\r\r\n", NumberSamples);
				break;
				case (BEACON_INDEX_SW):
					printf("\r\nNum ticks Beacon SW: %d\n\r", NumberSamples);
				break;
			}
		if (NumberSamples >= NUMBER_PULSES_TO_BE_ALIGNED)
		{
			
			beacons[LastBeacon].lastUpdateTime = captureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
			beacons[LastBeacon].lastEncoderAngle = CalculateAverage();
			switch (LastBeacon){
				case (BEACON_INDEX_NW):
					printf("\r\nAverage for Beacon NW: %f\n\r\r\n", beacons[LastBeacon].lastEncoderAngle);
				break;
				case (BEACON_INDEX_NE):
					printf("\r\nAverage for Beacon NE: %f\n\r\r\n", beacons[LastBeacon].lastEncoderAngle);
				break;
				case (BEACON_INDEX_SE):
					printf("\r\nAverage for Beacon SE: %f\n\r\r\n", beacons[LastBeacon].lastEncoderAngle);
				break;
				case (BEACON_INDEX_SW):
					printf("\r\nAverage for Beacon SW: %f\n\r", beacons[LastBeacon].lastEncoderAngle);
				break;
				
			}
			printf("Beacon Last Update Time: %d\n\r", beacons[LastBeacon].lastUpdateTime);
			
			// Determine if we should recalculate our position and angle based on whethe or not we have received 3 consecutive pulses
			if (TimeForUpdate())
			{
				ES_Event NewEvent;
				NewEvent.EventType = ES_CALCULATE_POSITION;
				PostPositionLogicService(NewEvent);
			}
		}
	}
	else if (ThisEvent.EventType == ES_ALIGN_TO_BUCKET)
	{
		AligningToBucket = true;
		enableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	}
	
  return ReturnEvent;
}

/****************************************************************************
 Function
    GetLastUpdateTime
 Parameters
   beaconIndex : which beacon to query
 Returns
   The last time that the corresponding beacon was updated
****************************************************************************/
uint32_t GetLastUpdateTime(uint8_t beaconIndex)
{
	return beacons[beaconIndex].lastUpdateTime;
}

/****************************************************************************
 Function
    GetBeaconAngle_
 Parameters
   beaconIndex : which beacon was the latest one captured
 Returns
   The angles A, B, C where A corresponds to the last beacon captured and C to the first
****************************************************************************/
float GetBeaconAngle_A(uint8_t beaconIndex)
{
	float beaconAngle_A = beacons[beaconIndex].lastEncoderAngle;
	return beaconAngle_A;
}

float GetBeaconAngle_B(uint8_t beaconIndex)
{
	uint8_t indexB = beacons[beaconIndex].priorBeacons[0];
	float beaconAngle_B = beacons[indexB].lastEncoderAngle;
	return beaconAngle_B;
}

float GetBeaconAngle_C(uint8_t beaconIndex)
{
	uint8_t indexC = beacons[beaconIndex].priorBeacons[1];
	float beaconAngle_C = beacons[indexC].lastEncoderAngle;
	return beaconAngle_C;
}

/****************************************************************************
 Function
    Get Axis Shifts
 Parameters
   beaconIndex : which beacon was the latest one captured
 Returns
   The shifts in the coordinate frame necessary to calculate position
****************************************************************************/
int getXShift(uint8_t beaconIndex){
	return beacons[beaconIndex].xShift;
}

int getYShift(uint8_t beaconIndex){
	return beacons[beaconIndex].yShift;
}

int getThetaShift(uint8_t beaconIndex){
	return beacons[beaconIndex].thetaShift;
}

/***************************************************************************
 private functions
 ***************************************************************************/
//The interrupt response for uor phototransistor
void PhotoTransistor_InterruptResponse(void)
{
	// Clear Interrupt
	clearCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	
	// Determine Capture time
	uint32_t ThisCapture = captureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	
	// Calculate period
	uint32_t Period = ((ThisCapture - LastCapture) * MICROSECONDS_DIVISOR ) / TICKS_PER_MS;
	
	//Store the Last Cpature
	LastCapture = ThisCapture;
	
	// Check if we have a beacon match
	uint8_t matchingBeacon = CheckForBeacon(Period);
	// If so, update the beacon structure
	if (matchingBeacon != NULL_BEACON)
	{
		ES_Timer_InitTimer(AVERAGE_BEACONS_TIMER, AVERAGE_BEACONS_T);
		
		if (matchingBeacon != LastBeacon)
		{
			printf("Switching from beacon %d to %d\r\n", LastBeacon, matchingBeacon);
			ResetAverage();
		}
		Sum += GetPeriscopeAngle();
		NumberSamples++;
		LastBeacon = matchingBeacon;
		
		if (AligningToBucket && NumberSamples >= NUMBER_PULSES_FOR_BUCKET)
		{
			if (((MyColor() == COLOR_BLUE) && (LastBeacon == BEACON_INDEX_NW)) || ((MyColor() == COLOR_RED) && (LastBeacon == BEACON_INDEX_SE)))
				{ 
					printf("Aligned to bucket!\r\n");
					clearDriveAligningToBucket();
					ES_Event AlignedEvent;
					AlignedEvent.EventType = ES_ALIGNED_TO_BUCKET;
					PostMasterSM(AlignedEvent);
				}	
		}
		
		/*
		if (((MyColor() == COLOR_BLUE) && (matchingBeacon == BEACON_INDEX_NW)) || ((MyColor() == COLOR_RED) && (matchingBeacon == BEACON_INDEX_SE)))
		{
			printf("Bucket beacon found!\r\n");
		}*/
		
		/*
		//Print which Beacon
		switch (beacons[matchingBeacon].period){
			case (BEACON_P_NW):
				printf("BEACON_P_NW \n\r");
			break;
			case (BEACON_P_NE):
				printf("BEACON_P_NE \n\r");
			break;
			case (BEACON_P_SE):
				printf("BEACON_P_SE \n\r");
			break;
			case (BEACON_P_SW):
				printf("BEACON_P_SW \n\r");
			break;
		}
		*/

	}
}

/*
//Update the Last Period by shiting every value down and ours into the 0th index
static void UpdateLastPeriod(uint32_t period)
{
	for (int i = NUMBER_CONSECUTIVE_PULSES_2STORE - 1; i > 0; i--)
	{
		PhotoTransistor_LastPeriods[i] = PhotoTransistor_LastPeriods[i-1];
	}
	PhotoTransistor_LastPeriods[0] = period;
}*/



//Check For a Beacon
static uint8_t CheckForBeacon(uint32_t period)
{
	for (int i = 0; i < NUMBER_BEACON_FREQUENCIES; i++)
	{
		if (ToleranceCheck(period, beacons[i].period, PERIOD_MEASURING_ERROR_TOLERANCE))
		{
			return i;
		}
	}
	
	return NULL_BEACON;
}

/*
static bool IsBeaconMatch(uint8_t beaconIndex)
{
	for (int j = 0; j < NUMBER_PULSES_TO_BE_ALIGNED; j++)
	{
		//printf("matching period: %d against target %d\r\n", PhotoTransistor_LastPeriods[j], beacons[beaconIndex].period);
		if (!ToleranceCheck(PhotoTransistor_LastPeriods[j], beacons[beaconIndex].period, PERIOD_MEASURING_ERROR_TOLERANCE))
		{
			return false;
		}
	}
	return true;
}*/

static bool TimeForUpdate()
{
	if (AligningToBucket)
	{
			return false;
	} 
	
	//Get our A, B, and C Update Times
	uint32_t beaconAUpdateTime = beacons[mostRecentBeaconUpdate()].lastUpdateTime;
	uint8_t indexB = beacons[mostRecentBeaconUpdate()].priorBeacons[0];
	uint32_t beaconBUpdateTime = beacons[indexB].lastUpdateTime;
	uint8_t indexC = beacons[mostRecentBeaconUpdate()].priorBeacons[1];
	uint32_t beaconCUpdateTime = beacons[indexC].lastUpdateTime;
	
	printf("Most recent beacon is: %d. Update times are A: %d, B: %d, C: %d\r\n", mostRecentBeaconUpdate(), beaconAUpdateTime, beaconBUpdateTime, beaconCUpdateTime);
	
	//Determine if Sequence has been met
	//if none of the update times were zero
	if ((beaconAUpdateTime != 0) && (beaconBUpdateTime != 0) && (beaconCUpdateTime != 0)){
		return true;
	} else {
		return false;
	}
	//Update only if got last three in sequence
	//return ((beaconAUpdateTime > beaconBUpdateTime && (beaconBUpdateTime > beaconCUpdateTime)) && beaconCUpdateTime > 0);
}


//Takes nothing as a paramater and returns the index of the beacon most recently updated
uint8_t mostRecentBeaconUpdate(void){
	/*
	//First Check which Was the last updated Beacon
	uint8_t mostRecentBeaconUpdated = NULL_BEACON;	//by default we assume the first beacon is the most recently updated 
	uint32_t mostRecentUpdateTime = 0;
	for (uint8_t i = 0; i < 4; i++){
		if (GetLastUpdateTime(i) > mostRecentUpdateTime){
			//Set the mostRecentBeaconUpdate to i
			mostRecentBeaconUpdated = i;
			mostRecentUpdateTime = GetLastUpdateTime(i);
		}
	}
	return mostRecentBeaconUpdated;*/
	return LastBeacon;
}



//Tolerance Check function to See if We are within our bounds
static bool ToleranceCheck(uint32_t period, uint32_t targetPeriod, uint8_t tolerance)
{
	return ((period <= targetPeriod + tolerance) && (period >= targetPeriod - tolerance));
}

void ResetUpdateTimes(void)
{
	for (int i = 0; i < NUMBER_BEACON_FREQUENCIES; i++)
	{
		beacons[i].lastUpdateTime = 0;
	}
}

void ResetAverage(void)
{
	Sum = 0;
	NumberSamples = 0;
}

void ResetAligningToBucket(void)
{
	AligningToBucket = false;
}

static float CalculateAverage(void)
{
	/*
	switch (LastBeacon){
			case (BEACON_INDEX_NW):
				printf("Adding to Beacon NW: %f\n\r", angle);
			break;
			case (BEACON_INDEX_NE):
				printf("Adding to Beacon NE: %f\n\r", angle);
			break;
			case (BEACON_INDEX_SE):
				printf("Adding to Beacon SE: %f\n\r", angle);
			break;
			case (BEACON_INDEX_SW):
				printf("Adding to Beacon SW: %f\n\r", angle);
			break;
		}*/
	return Sum / NumberSamples;
}

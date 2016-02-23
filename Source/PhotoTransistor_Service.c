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

#define PERIOD_MEASURING_ERROR_TOLERANCE 25 //in microseconds
#define NUMBER_CONSECUTIVE_PULSES_2STORE 4
#define NUMBER_PULSES_TO_BE_ALIGNED 4
#define NUMBER_PHOTOTRANSISTORS 1
#define NUMBER_BEACON_FREQUENCIES 4

#define BEACON_F_NW 1450
#define BEACON_F_NE 1700
#define BEACON_F_SE 1250
#define BEACON_F_SW 1950

#define BEACON_P_NW  690 //6206 //690
#define BEACON_P_NE  588  //5878 //588
#define BEACON_P_SE  800    //6399 //800
#define BEACON_P_SW  513    //6153 //513

#define DIRECTION 1 //(use 1 if CW and -1 if CCW)
	



/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void UpdateLastPeriod(uint32_t period);

static uint8_t CheckForBeacon(void);

static bool IsBeaconMatch(uint8_t beaconIndex);

static bool ToleranceCheck(uint32_t period, uint32_t targetPeriod, uint8_t tolerance);

static bool TimeForUpdate(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static Beacon beacons[] = 
{
	{.period = BEACON_P_NW}, //1450 Hz, NW
	{.period = BEACON_P_NE}, //1700 Hz, NE
	{.period = BEACON_P_SE}, //1250 Hz, SE
	{.period = BEACON_P_SW} //1950 Hz, SW
};

static uint32_t PhotoTransistor_LastPeriods[NUMBER_CONSECUTIVE_PULSES_2STORE];

static uint32_t LastCapture;

static uint8_t LastBeacon = NULL_BEACON;

static bool AligningToBucket = false;

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
  
	if (ThisEvent.EventType == ES_ALIGN_TO_BUCKET)
	{
		AligningToBucket = true;
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
    GetBeaconAngle

 Parameters
   beaconIndex : which beacon to query

 Returns
   The last recorded angle to the given beacon

****************************************************************************/
float GetBeaconAngle(uint8_t beaconIndex)
{
	return beacons[beaconIndex].lastEncoderAngle;
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
	
	/*
	static int i = 0;
	static uint32_t lastcap;
	i++;
	if (i >= 1000)
	{
		printf("The different after 1000 was: %d\r\n", lastcap - ThisCapture);
		lastcap = ThisCapture;
		i = 0;
	}*/
	
	//Store the Last Cpature
	LastCapture = ThisCapture;
	
	// Update our knowledge of the most recent periods
	UpdateLastPeriod(Period);
	// Check if we have a beacon match
	uint8_t matchingBeacon = CheckForBeacon();
	// If so, update the beacon structure
	if (matchingBeacon != NULL_BEACON)
	{
		if (AligningToBucket)
		{
			if (((MyColor() == COLOR_BLUE) && (matchingBeacon == BEACON_INDEX_NW)) || ((MyColor() == COLOR_RED) && (matchingBeacon == BEACON_INDEX_SE)))
			{
				setTargetDriveSpeed(0, 0);
				ES_Event AlignedEvent;
				AlignedEvent.EventType = ES_ALIGNED_TO_BUCKET;
				PostMasterSM(AlignedEvent);
			}
			AligningToBucket = false;
		}
		
		LastBeacon = matchingBeacon;
		beacons[matchingBeacon].lastUpdateTime = ThisCapture;
		beacons[matchingBeacon].lastEncoderAngle = GetPeriscopeAngle();
		
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
	
	// Determine if we should recalculate our position and angle
	if (matchingBeacon == BEACON_INDEX_NW && TimeForUpdate())
	{
		ResetUpdateTimes();
		ES_Event NewEvent;
		NewEvent.EventType = ES_CALCULATE_POSITION;
		PostPositionLogicService(NewEvent);
	}
 }
}

//Update the Last Period by shiting every value down and ours into the 0th index
static void UpdateLastPeriod(uint32_t period)
{
	for (int i = NUMBER_CONSECUTIVE_PULSES_2STORE - 1; i > 0; i--)
	{
		PhotoTransistor_LastPeriods[i] = PhotoTransistor_LastPeriods[i-1];
	}
	PhotoTransistor_LastPeriods[0] = period;
}



//Check For a Beacon
static uint8_t CheckForBeacon()
{
	for (int i = 0; i < NUMBER_BEACON_FREQUENCIES; i++)
	{
		if (IsBeaconMatch(i))
		{
			return i;
		}
	}
	
	return NULL_BEACON;
}

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
}

static bool TimeForUpdate()
{
	bool returnVal = ((beacons[BEACON_INDEX_NW].lastUpdateTime > beacons[BEACON_INDEX_NE].lastUpdateTime) && (beacons[BEACON_INDEX_NE].lastUpdateTime > beacons[BEACON_INDEX_SE].lastUpdateTime) && (beacons[BEACON_INDEX_SE].lastUpdateTime != 0));
	return returnVal;
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
/****************************************************************************
 Module
   PhotoTransistor_Service.c
 Description
	 Manages the phototransistor and its capture of beacon information
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
#define NUMBER_PULSES_TO_BE_ALIGNED 6
#define NUMBER_PHOTOTRANSISTORS 1
#define NUMBER_BEACON_FREQUENCIES 4
#define NUMBER_PULSES_FOR_BUCKET 4

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

static bool ToleranceCheck(uint32_t period, uint32_t targetPeriod, uint8_t tolerance);

static bool TimeForUpdate(void);

static void ResetAverage(void);

static float CalculateAverage(uint8_t which);

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

static uint8_t buckets[NUMBER_BEACON_FREQUENCIES];

static uint8_t numSamples[NUMBER_BEACON_FREQUENCIES];
static float sums[NUMBER_BEACON_FREQUENCIES];

static uint8_t LastUpdatedBeacon = NULL_BEACON;

static bool Bucketing = true;

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
	
	disableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	
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
  
	// If we have stopped seeing pulses, it's time to evaluate whether we saw a beacon
	if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == AVERAGE_BEACONS_TIMER))
	{
		// If the beacon we were interested in had enough pulses
		if (numSamples[LastBeacon] >= NUMBER_PULSES_TO_BE_ALIGNED)
		{
			// set the last update time for the beacon
			beacons[LastBeacon].lastUpdateTime = captureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
			
			// set the angle to the beacon based on the average of all pulses measured
			beacons[LastBeacon].lastEncoderAngle = CalculateAverage(LastBeacon);
			
			// Store this beacon as the last updated beacon
			LastUpdatedBeacon = LastBeacon;
			
			// Determine if we should recalculate our position and angle based on whether or not we have received 3 consecutive pulses
			if (TimeForUpdate())
			{
				ES_Event NewEvent;
				NewEvent.EventType = ES_CALCULATE_POSITION;
				PostPositionLogicService(NewEvent);
			}
			
		}
		
		// Reset stored average information
		ResetAverage();
		
		// Reallow new beacons to be recorded
		Bucketing = true;
		
		LastBeacon = NULL_BEACON;
		
		ES_Timer_StopTimer(AVERAGE_BEACONS_TIMER);
	}
	else if (ThisEvent.EventType == ES_ALIGN_TO_BUCKET)
	{
		// If we are trying to align to the bucket for a shot, 
		//  shift the way we handle interrupts
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
//The interrupt response for our phototransistor
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
	
	//Iterate Through the different frequency options
	for (int i = 0; i < NUMBER_BEACON_FREQUENCIES; i++)
	{
		// If the period matches a beacon period
		if (ToleranceCheck(Period, beacons[i].period, PERIOD_MEASURING_ERROR_TOLERANCE))
		{
			// If we're searching for a beacon
			if (Bucketing)
			{
				buckets[i]++;
			}
			
			// If we're supposed to align to the bucket, and the number of pulses we've seen for
			//  this beacon is greater than our threshold
			if ((AligningToBucket) && (buckets[i] >= NUMBER_PULSES_FOR_BUCKET))
			{
				// If this is the beacon corresponding to our target bucket
				if (((MyColor() == COLOR_BLUE) && (i == BEACON_INDEX_NW)) || ((MyColor() == COLOR_RED) && (i == BEACON_INDEX_SE)))
				{ 
					// stop our drive
					clearDriveAligningToBucket();
					
					// Post an aligned event
					ES_Event AlignedEvent;
					AlignedEvent.EventType = ES_ALIGNED_TO_BUCKET;
					PostMasterSM(AlignedEvent);
					
					// stop aligning
					AligningToBucket = false;
					
					// reset all buckets
					for (int j = 0; j < NUMBER_BEACON_FREQUENCIES; j++)
					{
						buckets[j] = 0;
					}
					
					// reset average information
					ResetAverage();
					
					// return to searching for beacons
					Bucketing = true;
					LastBeacon = NULL_BEACON;
					ES_Timer_StopTimer(AVERAGE_BEACONS_TIMER);
					return;
				}
			}
			// If we're not aligning to a bucket, and the number of pulses we've seen for this
			//  beacon is greater than our threshold
			else if (buckets[i] >= NUMBER_PULSES_TO_BE_ALIGNED && LastBeacon == NULL_BEACON && !AligningToBucket)
			{
				// store the beacon to be recorded
				LastBeacon = i;
				
				// reset all buckets
				for (int j = 0; j < NUMBER_BEACON_FREQUENCIES; j++)
				{
					buckets[j] = 0;
				}
				
				// restart the timer to evaluate the beacon 
				ES_Timer_InitTimer(AVERAGE_BEACONS_TIMER, AVERAGE_BEACONS_T);
				Bucketing = false;
			}
			
			// increment the sum of all encoder angles for this beacon
			sums[i] += GetPeriscopeAngle();
			
			// increment the number of pulses seen for this beacon
			numSamples[i]++;
			
			// If the evaluation timer is already running, restart it
			ES_Timer_SetTimer(AVERAGE_BEACONS_TIMER, AVERAGE_BEACONS_T);
		}
	}
		
}

// Determine if we have enough beacon information to calculate our absolute position
static bool TimeForUpdate()
{
	// If we're supposed to align to the bucket, don't calculate position
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

	//Determine if Sequence has been met
	//if none of the update times were zero
	return ((beaconAUpdateTime != 0) && (beaconBUpdateTime != 0) && (beaconCUpdateTime != 0));
}


//Takes nothing as a paramater and returns the index of the beacon most recently updated
uint8_t mostRecentBeaconUpdate(void){
	return LastUpdatedBeacon;
}



//Tolerance Check function to See if We are within our bounds
static bool ToleranceCheck(uint32_t period, uint32_t targetPeriod, uint8_t tolerance)
{
	return ((period <= targetPeriod + tolerance) && (period >= targetPeriod - tolerance));
}

// reset the update times for each beacon, so that we must see new beacons to 
// calculate position
void ResetUpdateTimes(void)
{
	for (int i = 0; i < NUMBER_BEACON_FREQUENCIES; i++)
	{
		beacons[i].lastUpdateTime = 0;
	}
}

// reset average information for all beacons
void ResetAverage(void)
{
	for (int i = 0; i < NUMBER_BEACON_FREQUENCIES; i++)
	{
		sums[i] = 0;
		numSamples[i] = 0;
	}
}

// Clear the aligning to bucket flag
void ResetAligningToBucket(void)
{
	AligningToBucket = false;
}

// Manually set beacon angles. Used for testing
void SetBeaconAngles(float A, float B, float C)
{
	beacons[0].lastUpdateTime = 3;
	beacons[1].lastUpdateTime = 2;
	beacons[2].lastUpdateTime = 1;
	
	beacons[0].lastEncoderAngle = A;
	beacons[1].lastEncoderAngle = B;
	beacons[2].lastEncoderAngle = C;
	
	LastUpdatedBeacon = 0;
}

// Return the average of all encoder angles seen for a given beacon
static float CalculateAverage(uint8_t which)
{
	return sums[which] / numSamples[which];
}

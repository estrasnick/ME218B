/****************************************************************************
 Module
   PeriscopeControl_Service.c

 Description
		Controls the periscope motor, as well as the mechanical servo
		  which latches the periscope for zeroing
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PeriscopeControl_Service.h"
#include "Helpers.h"
#include "Definitions.h"
#include "PWM_Service.h"
#include "PositionLogic_Service.h"
#include "DriveTrainControl_Service.h"
#include "PhotoTransistor_Service.h"
#include "Master_SM.h"
#include <Math.h>

/*----------------------------- Module Defines ----------------------------*/
#define PERISCOPE_FULL_ROTATION_ENCODER_TICKS 1000

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

//Encoder Input Capture Variables
static uint32_t numTicks;

static bool AligningToBucket = false;

static bool isZeroed;

static bool AttemptingToStop = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPeriscopeControlService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool InitPeriscopeControlService ( uint8_t Priority )
{
  ES_Event ThisEvent;
	
  MyPriority = Priority;
  printf("Periscope Service Attempt Initialization \n\r");
	
	//Initialize Our Input Captures for Encoder
	InitInputCapture(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_1);
	InitInputCapture(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_2);
	
	// Wait for all initializations to finish before starting the periscope
	ES_Timer_InitTimer(START_PERISCOPE_TIMER, START_PERISCOPE_T);
	
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
     PostPeriscopeControlService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostPeriscopeControlService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPeriscopeControlService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunPeriscopeControlService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

	// Start the periscope
	if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == START_PERISCOPE_TIMER))
	{
		SetPWM_Periscope(PERISCOPE_PWM_DUTY);
	}
	// If the periscope has stopped
	else if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == PERISCOPE_STOPPED_TIMER))
	{
		// If this was intentional (i.e. we are latching the periscope)
		if (AttemptingToStop)
		{
			// Stop the motor
			SetPWM_Periscope(0);
			
			// Reset periscope encoder ticks, zeroing the periscope
			ResetPeriscopeEncoderTicks();
			
			// If we are supposed to align to the bucket, 
			// post that we are ready to align
			if (AligningToBucket)
			{
				ES_Event RotateEvent;
				RotateEvent.EventType = ES_ALIGN_TO_BUCKET;
				setDriveToAlignToBucket();
				PostPhotoTransistorService(RotateEvent);
				AligningToBucket = false;
			}
			// Mark that we have zeroed the periscope
			if (!isZeroed)
			{
				isZeroed = true;
			}
		}
	}
	else if (ThisEvent.EventType == ES_ALIGN_TO_BUCKET)
	{
		AligningToBucket = true;
	}

	//If we are in testing mode
	if(TESTING_MODE){
	
		//If we receive any of these events (from keyboard presses)
		switch (ThisEvent.EventType){
			case (ES_START_PERISCOPE):
				SetPWM_Periscope(PERISCOPE_PWM_DUTY);
				break;
			case (ES_STOP_PERISCOPE):
				SetPWM_Periscope(0);
				break;
		}
	}
	
  return ReturnEvent;
}

/***************************************************************************
Interrupt Responses
We use the quadrature on the optical encoder - not for determining direction,
	but for finer resolution on our positioning
 ***************************************************************************/
void PeriscopeEncoder_InterruptResponse_1(void){
	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_1);
	
	// increment encoder ticks
	numTicks++;
	
	// restart the stall timer
	// We don't need to do this on both encoder interrupts
	ES_Timer_InitTimer(PERISCOPE_STOPPED_TIMER, PERISCOPE_STOPPED_T);
	
	// If we have performed a full rotation, reset to zero
	if (numTicks >= PERISCOPE_FULL_ROTATION_ENCODER_TICKS)
	{
		numTicks = 0;
	}
}

void PeriscopeEncoder_InterruptResponse_2(void){
	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_2);
	
	// increment encoder ticks
	numTicks++;
	
	// If we have performed a full rotation, reset to zero
	if (numTicks >= PERISCOPE_FULL_ROTATION_ENCODER_TICKS)
	{
		numTicks = 0;
	}
}

// Return the current angle of the periscope
float GetPeriscopeAngle(void)
{
	return (numTicks * 180.0) / (ENCODER_PULSES_PER_REV * PERISCOPE_GEAR_RATIO);
}

// reset the periscope angle
void ResetPeriscopeEncoderTicks(void)
{
	numTicks = 0;
}

// Raise the latch servo
void LatchPeriscope(void)
{
	SetPWM_PeriscopeLatch(PERISCOPE_LATCH_DUTY);
}

// Lower the latch servo and restart the periscope
void UnlatchPeriscope(void)
{
	SetPWM_PeriscopeLatch(PERISCOPE_UNLATCH_DUTY);
	SetPWM_Periscope(PERISCOPE_PWM_DUTY);
}

// mark that we require the periscope to be zeroed
void RequireZero(void)
{
	isZeroed = false;
	ES_Timer_InitTimer(PERISCOPE_STOPPED_TIMER, PERISCOPE_STOPPED_T);
}

// return iff the periscope has been zeroed
bool IsZeroed(void)
{
	return isZeroed;
}

// set a flag that we wish to stop the periscope
void SetAttemptingToStop(bool val)
{
	AttemptingToStop = val;
}

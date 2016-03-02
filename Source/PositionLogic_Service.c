/****************************************************************************
 Module
   PositionLogic_Service.c

 Description
		
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include <Math.h>

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PositionLogic_Service.h"
#include "PhotoTransistor_Service.h"
#include "Helpers.h"
#include "DEFINITIONS.h"
#include "DriveTrainControl_Service.h"
#include "PeriscopeControl_Service.h"
#include "GameInfo.h"
#include "Strategy_SM.h"
#include "DriveTrainControl_Service.h"
#include "AttackStrategy_SM.h"

/*----------------------------- Module Defines ----------------------------*/

#define PI 3.14159265f
#define DEGREE_CONVERSION_RATIO 57.29577951f
#define RADIAN_CONVERSION_RATIO 0.01745329251f

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void CalculateAbsolutePosition(void);
//static void CalculateRelativePosition(void);
static float ConvertEncoderTicksToInches(uint32_t ticks);
static uint32_t ConvertInchesToEncoderTicks(float inches);
static float ToRadians(float degrees);
static float ToDegrees(float radians);
//static float ToAppropriateRange_R(float angle);
static uint32_t EncoderTicksForGivenAngle(float angle);
static float DetermineDistanceToTarget(void);
static float DetermineAngleToTarget(float distanceToTarget);
//static float DetermineAngleToBucket(float distanceToBucket);
static void AlignToTarget(void);
static void DriveToTarget(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static float myX;
static float myY;
static float myTheta;

static float TargetX;
static float TargetY;

static bool AbsolutePosition = false;

static uint8_t backupIndex;

typedef struct {
	uint32_t leftTicks;
	uint32_t rightTicks;
	bool leftNeg;
	bool rightNeg;
} backupPlan;

static backupPlan backupArray[] = 
{
	{.leftTicks = BACK_UP_TICKS, .rightTicks = BACK_UP_TICKS << 1, .leftNeg = false, .rightNeg = false},
	{.leftTicks = BACK_UP_TICKS << 1, .rightTicks = BACK_UP_TICKS, .leftNeg = false, .rightNeg = false},
	{.leftTicks = BACK_UP_TICKS, .rightTicks = BACK_UP_TICKS << 1, .leftNeg = true, .rightNeg = true},
	{.leftTicks = BACK_UP_TICKS << 1, .rightTicks = BACK_UP_TICKS, .leftNeg = true, .rightNeg = true}
};

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPositionLogicService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool InitPositionLogicService ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  
	printf("Position Logic Initialized \n\r");

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
     PostPositionLogicService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostPositionLogicService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPositionLogicService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunPositionLogicService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
	switch (ThisEvent.EventType)
	{
		case ES_CALCULATE_POSITION:
		{
			CalculateAbsolutePosition();
			break;
		}
		/*
		case ES_TIMEOUT:
		{
			if (ThisEvent.EventParam == RELATIVE_POSITION_TIMER)
			{
				if (IsMoving())
				{
					CalculateRelativePosition();
					if (QueryStrategySM() == Travel_t)
					{
						ES_Timer_InitTimer(RELATIVE_POSITION_TIMER, RELATIVE_POSITION_T);
					}
				}
			}
			break;
		}*/
		case ES_FACE_TARGET:
		{
			printf("Facing target\r\n");
			AlignToTarget();
			break;
		}
		case ES_DRIVE_TO_TARGET:
		{
			DriveToTarget();
			break;
		}
		default:
			break;
	}
		
  return ReturnEvent;
}
/***************************************************************************
 Set a new destination
 ***************************************************************************/
void SetTargetLocation(float x, float y)
{
	TargetX = x;
	TargetY = y;
}

/***************************************************************************
 Set location manually (used for testing)
 ***************************************************************************/
void SetMyLocation(float x, float y, float theta)
{
	myX = x;
	myY = y;
	myTheta = theta;
}

/***************************************************************************
 Get my X coordinate
 ***************************************************************************/
float getX(void)
{
	return myX;
}

/***************************************************************************
 Get my Y coordinate
 ***************************************************************************/
float getY(void)
{
	return myY;
}

/***************************************************************************
 Reposition if we are having trouble getting a position
 ***************************************************************************/
void ExecuteBackup(void)
{
	if (QueryAttackStrategySM() == Attack_t)
	{
		return;
	}
	
	PausePositioning();
	SetBackingUp(true);
	setTargetEncoderTicks(backupArray[backupIndex].leftTicks, backupArray[backupIndex].rightTicks, backupArray[backupIndex].leftNeg, backupArray[backupIndex].rightNeg);
	backupIndex++;
	if (backupIndex >= NELEMS(backupArray))
	{
		backupIndex = 0;
	}
}

/***************************************************************************
 Check if our latest position was determined via triangulation
 ***************************************************************************/
bool IsAbsolutePosition(void)
{
	return AbsolutePosition;
}

/***************************************************************************
 Mark that we no longer know our absolute position
 ***************************************************************************/
void ResetAbsolutePosition(void)
{
	AbsolutePosition = 0;
}

/***************************************************************************
 Return the distance to the given point
 ***************************************************************************/
float DistanceToPoint(float TargetX, float TargetY)
{
	float yDist = TargetY - myY;
	float xDist = TargetX - myX;
	
	return sqrt((yDist * yDist) + (xDist * xDist));
}

/***************************************************************************
Absolute Position Calculations (ie. Photo Transistor Periscope Calculations)
 ***************************************************************************/
// Calculate absolute position by measuring the angles to 3 beacons. 
// See explanation of math for more details
static void CalculateAbsolutePosition()
{
	// Disable our phototransistor interrupts while we work, so that angles don't update
	// in the middle of the calculations
	disableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);

	printf("Calculating position...\r\n");

	//Get our A, B, and C Encoder Angles
	uint8_t lastBeaconIndex = mostRecentBeaconUpdate();
	float A =  GetBeaconAngle_A(lastBeaconIndex);
	float B =  GetBeaconAngle_B(lastBeaconIndex);
	float C =  GetBeaconAngle_C(lastBeaconIndex);
	
	printf("lastBeaconIndex: %d \n\r", lastBeaconIndex);
	
	//Get our Shifts for the rotation matrix
	int xShift = getXShift(lastBeaconIndex);
	int yShift = getYShift(lastBeaconIndex);
	int thetaShift = getThetaShift(lastBeaconIndex);
	
	//Declare Static Variabels for our Calculations
	float BMinusC;
	float AMinusB;
	float gamma;
	float delta;
	float tempAngle;
	float atan;
	float myTempX;
	float myTempY;
	
	//Perform Calculations with SW corner being used as the axis
	BMinusC = ToRadians(ToAppropriateRange(B - C));
	AMinusB = ToRadians(ToAppropriateRange(A - B));
	
	tempAngle = (1.5f * PI) - BMinusC - AMinusB;
	
	atan = atan2(-sin(tempAngle), (sin(AMinusB)/sin(BMinusC)) + cos(tempAngle));
	
	gamma = (atan < 0) ? -atan : PI - atan;
	delta = tempAngle - gamma;
	
	myX = 96.0 - (96.0 * sin(gamma) * (sin(PI - gamma - BMinusC)/sin(PI - BMinusC)));
	myY = 96.0 - (96.0 * sin(delta) * (sin(PI - delta - AMinusB)/sin(PI - AMinusB)));
	
	myTheta = ToAppropriateRange(ToDegrees(ToRadians(360 - B) + gamma + BMinusC - (.5f * PI)));
	
	printf("ABSOLUTE POSITION BEFORE ROTATION MATRIX: X: %f, Y: %f, theta: %f\n\r", myX, myY, myTheta);	
	
	//Peform Rotations and Appropriate Shifts
	myTempX = (myX*cos(ToRadians(thetaShift)) - myY*sin(ToRadians(thetaShift))) + xShift;
	myTempY = (myX*sin(ToRadians(thetaShift)) + myY*cos(ToRadians(thetaShift))) + yShift;	
	myTheta = ToAppropriateRange(myTheta + thetaShift); 
	myX = myTempX;
	myY = myTempY;
	
	//print our absolute position
	printf("ABSOLUTE POSITION: X: %f, Y: %f, theta: %f\n\r", myX, myY, myTheta);	
	printf("Last angle 'A' was: %d, A was: %f, B was: %f, C was: %f, gamma was: %f\r\n", lastBeaconIndex, A, B, C, ToDegrees(gamma));
	
	if ((myX < 0) || (myY < 0) || (myX > 96) || (myY > 96))
	{
		printf("Oops. We calculated an invalid position.");
		ExecuteBackup();
	}
	else
	{
		AbsolutePosition = true;
	}
	
	ResetUpdateTimes();
	
	// Reenable the phototransistor interrupts
	enableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
}


/***************************************************************************
Relative Position Calculations (ie. Encoder Distance Travelled Calculations)
 ***************************************************************************/
// Calculate position by comparing the number of encoder ticks on the
// left and right drive motors. (See explanation of math for more 
// details
/*static void CalculateRelativePosition()
{
	printf("RELATIVE POSITION!\r\n");
	uint32_t leftEncoderTicks;
	uint32_t rightEncoderTicks;
	float L1;
	float L2;
	float L2MinusL1;
	float displacement;
	float displacementAngle;
	float innerAngle;
	float myNewTheta;
	bool curveLeft;
	
	EnterCritical();
	leftEncoderTicks = GetLeftEncoderTicks();
	rightEncoderTicks = GetRightEncoderTicks();
	ExitCritical();
	
	if (leftEncoderTicks == rightEncoderTicks)
	{
		myX += ConvertEncoderTicksToInches(leftEncoderTicks) * cos(ToRadians(myTheta));
		myY += ConvertEncoderTicksToInches(leftEncoderTicks) * sin(ToRadians(myTheta));
		return;
	}
	else if (leftEncoderTicks > rightEncoderTicks)
	{
		L1 = ConvertEncoderTicksToInches(leftEncoderTicks);
		L2 = ConvertEncoderTicksToInches(rightEncoderTicks);
		curveLeft = false;
	}
	else
	{
		L1 = ConvertEncoderTicksToInches(rightEncoderTicks);
		L2 = ConvertEncoderTicksToInches(leftEncoderTicks);
		curveLeft = true;
	}
	
	L2MinusL1 = L2 - L1;
	innerAngle = L2MinusL1 / DISTANCE_BETWEEN_WHEELS;
	
	displacement = ((DISTANCE_BETWEEN_WHEELS * (L2 + L1)) / L2MinusL1) * sin(.5f * innerAngle);
	myNewTheta = myTheta + ToDegrees((curveLeft ? -innerAngle : innerAngle)); 
	displacementAngle = ToRadians((myTheta + myNewTheta) / 2);
	
	myX += displacement * cos(displacementAngle);
	myY += displacement * sin(displacementAngle);
	myTheta = myNewTheta;
}*/       

// Convert Encoder ticks to linear distance measurement
static float ConvertEncoderTicksToInches(uint32_t ticks)
{
	return (ticks * WHEEL_CIRCUMFERENCE) / (ENCODER_PULSES_PER_REV * DRIVE_GEAR_RATIO);
}

// Convert Encoder ticks to linear distance measurement
static uint32_t ConvertInchesToEncoderTicks(float inches)
{
	return (inches * ENCODER_PULSES_PER_REV * DRIVE_GEAR_RATIO) / (WHEEL_CIRCUMFERENCE);
}

// Convert radians to degrees
static float ToDegrees(float radians)
{
	return radians * DEGREE_CONVERSION_RATIO;
}

// Convert degrees to radians
static float ToRadians(float degrees)
{
	return degrees * RADIAN_CONVERSION_RATIO;
}

// Return an equivalent value for the angle within
// the range [0, 360.0]
float ToAppropriateRange(float angle)
{
	while (angle > 360.0f)
	{
		angle -= 360.0f;
	}
	while (angle < 0.0f)
	{
		angle += 360.0f;
	}
	return angle;
}

// Return an equivalent value for the angle within
// the range [0, 2*pi]
/*static float ToAppropriateRange_R(float angle)
{
	while (angle > 2 * PI)
	{
		angle -= 2 * PI;
	}
	while (angle < 0)
	{
		angle += 2 * PI;
	}
	return angle;
}*/

static uint32_t EncoderTicksForGivenAngle(float angle)
{
	return (ToRadians(angle) * DISTANCE_BETWEEN_WHEELS * DRIVE_GEAR_RATIO * ENCODER_PULSES_PER_REV) / (2 * WHEEL_CIRCUMFERENCE);
}

float DetermineDistanceToBucket(void)
{
	printf("Target x: %f, Target y: %f, My x: %f, My y: %f\r\n", TargetX, TargetY, myX, myY);
	float yDist = (MyColor() ? 102.3 : -6.3) - myY;
	float xDist = (MyColor() ? -6.3 : 102) - myX;
	
	return sqrt((yDist * yDist) + (xDist * xDist));
}

static float DetermineDistanceToTarget(void)
{
	printf("Target x: %f, Target y: %f, My x: %f, My y: %f\r\n", TargetX, TargetY, myX, myY);
	float yDist = TargetY - myY;
	float xDist = TargetX - myX;
	
	return sqrt((yDist * yDist) + (xDist * xDist));
}

static float DetermineAngleToTarget(float distanceToTarget)
{
	float theta = ToAppropriateRange(ToDegrees(asin((TargetY - myY)/distanceToTarget)));
	if ((TargetX - myX) < 0)
	{
		printf("less than 0\r\n");
		theta = ToAppropriateRange(180 - theta);
	}
	//printf("theta is: %f\r\n", ToAppropriateRange(theta));
	return ToAppropriateRange(myTheta - theta);
}
/*
static float DetermineAngleToBucket(float distanceToBucket)
{
	float theta = ToAppropriateRange(ToDegrees(asin(((MyColor() ? 96 : 0) - myY)/distanceToBucket)));
	if (((MyColor() ? 0 : 96) - myX) < 0)
	{
		printf("less than 0\r\n");
		theta = ToAppropriateRange(180 - theta);
	}
	//printf("theta is: %f\r\n", ToAppropriateRange(theta));
	return ToAppropriateRange(myTheta - theta);
}*/

static void AlignToTarget(void)
{
	//printf("Distance to Target: %f\r\n", DetermineDistanceToTarget());
	float angle = DetermineAngleToTarget(DetermineDistanceToTarget());
	printf("Angle to Target: %f\r\n", angle);
	uint32_t ticks = EncoderTicksForGivenAngle(angle);
	setTargetEncoderTicks(ticks, ticks, false, true);
}
 
static void DriveToTarget(void)
{
	uint32_t ticks = ConvertInchesToEncoderTicks(DetermineDistanceToTarget());
	
	printf("Changing ticks from %d", ticks);
	if (ticks >= HALL_SENSOR_OFFSET_IN_TICKS)
	{
		ticks -= HALL_SENSOR_OFFSET_IN_TICKS;
		setTargetEncoderTicks(ticks, ticks, false, false);
	}
	else
	{
		ticks = HALL_SENSOR_OFFSET_IN_TICKS - ticks;
		setTargetEncoderTicks(ticks, ticks, true, true);
	}
	printf(" to %d\r\n", ticks);
}



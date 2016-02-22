// Game Info mdoule

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "BITDEFS.H"
#include "string.h"

#include "Helpers.h"
#include "DEFINITIONS.h"


/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

//Polling Station Structure
typedef struct {
	uint8_t location_code;
	float location_x;
	float location_y;
	Claimed_b claimed_status;
	uint8_t byte;
	uint8_t bit1;
	uint8_t bit2;	
} PS_Struct;

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static void updateClaimedStatus(uint8_t index);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static bool MyBlockStatus;
static bool EnemyBlockStatus;

static PS_Struct PS_Array[] = {
	{.location_code = SACREMENTO_CODE, .location_x = 6.0f, .location_y = 55.8f, 	.claimed_status = Unclaimed_b, .byte = 1, .bit1 = BIT7HI, .bit2 = BIT6HI},
	{.location_code = SEATTLE_CODE, .location_x = 9.0f, .location_y = 88.5f, 			.claimed_status = Unclaimed_b, .byte = 1, .bit1 = BIT5HI, .bit2 = BIT4HI},
	{.location_code = BILLINGS_CODE, .location_x = 29.1f, .location_y = 75.8f, 		.claimed_status = Unclaimed_b, .byte = 1, .bit1 = BIT3HI, .bit2 = BIT2HI},
	{.location_code = DENVER_CODE, .location_x = 33.3f, .location_y = 53.8f, 			.claimed_status = Unclaimed_b, .byte = 1, .bit1 = BIT1HI, .bit2 = BIT0HI},
	{.location_code = DALLAS_CODE, .location_x = 45.3f, .location_y = 29.4f, 			.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT7HI, .bit2 = BIT6HI},
	{.location_code = CHICAGO_CODE, .location_x = 61.8f, .location_y = 60.8f, 		.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT5HI, .bit2 = BIT4HI},
	{.location_code = MIAMI_CODE, .location_x = 80.8f, .location_y = 9.2f, 				.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT3HI, .bit2 = BIT2HI},
	{.location_code = WASHINGTON_CODE, .location_x = 81.5f, .location_y = 55.2f, 	.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT1HI, .bit2 = BIT0HI},
	{.location_code = CONCORD_CODE, .location_x = 87.6f, .location_y = 72.6f, 		.claimed_status = Unclaimed_b, .byte = 3, .bit1 = BIT7HI, .bit2 = BIT6HI}
};

/*------------------------------ Module Code ------------------------------*/

//Check my color
uint8_t MyColor()
{
	//Set default to Red because everyone is using blue in their testing
	return RED; //HWREG(GAME_BASE + (GPIO_O_DATA + ALL_BITS)) & COLOR_PIN;
}

//Check if Game Started
bool isGameStarted()
{
	uint8_t *byte;
	byte = getResponseArray();
	
	return (*(byte + 4) & BIT0HI) == BIT0HI ;
}

void UpdateADStatus(void)
{
	uint8_t *byte;
	byte = getResponseArray();
	uint8_t RS = *(byte + 4);
	
	// Color pin is high for Blue, low for Red
	if (MyColor() == COLOR_RED)
	{
		MyBlockStatus = RS & BIT2HI;
		EnemyBlockStatus = RS & BIT1HI;
	}
	else
	{
		MyBlockStatus = RS & BIT1HI;
		EnemyBlockStatus = RS & BIT2HI;		
	}
}

bool AmIBlocked(void)
{
	return MyBlockStatus;
}

bool IsEnemyBlocked(void)
{
	return EnemyBlockStatus;
}

void SetStationOwner(uint8_t which, Claimed_b owner)
{
	PS_Array[which].claimed_status = owner;
}

uint8_t GetStationOwner(Claimed_b which)
{
	return PS_Array[which].claimed_status;
}

float GetStationX(uint8_t which)
{
	return PS_Array[which].location_x;
}

float GetStationY(uint8_t which)
{
	return PS_Array[which].location_y;
}



//Update Location Statuses
void updatePSStatuses(){
	for (int i = 0; i < NUM_STATIONS; i++){
		updateClaimedStatus(i);
	}
}

//Pass in byte and bits and returns whether or not claimed
static void updateClaimedStatus(uint8_t index){
	uint8_t *RA;
	RA = getResponseArray();	
	
	uint8_t byte = *(RA + PS_Array[index].byte);
	uint8_t bit1 = PS_Array[index].bit1;
	uint8_t bit2 = PS_Array[index].bit2;
	
	PS_Array[index].claimed_status = (byte & bit1) ? ((byte & bit2) ?  Undefined_b : RED_b ) : ((byte & bit2) ? BLUE_b : Unclaimed_b );
}

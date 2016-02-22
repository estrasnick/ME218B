/****************************************************************************
HallEffect Header File
 ****************************************************************************/

#ifndef HallEffect_H
#define HallEffect_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Measure_t, Requesting_t } HallEffectState_t ;


// Public Function Prototypes

ES_Event RunHallEffectSM( ES_Event CurrentEvent );
void StartHallEffectSM ( ES_Event CurrentEvent );
HallEffectState_t QueryHallEffectSM ( void );

//Interrupt Responses
void HE_OuterLeft_InterruptResponse(void);
void HE_InnerLeft_InterruptResponse(void);
void HE_InnerRight_InterruptResponse(void);
void HE_OuterRight_InterruptResponse(void);


#endif 


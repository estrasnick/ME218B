/****************************************************************************
PACLogic Header File
 ****************************************************************************/

#ifndef CapturePS_H
#define CapturePS_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Measuring1_t, Request1_t, Measuring2_t, Request2_t, Measuring3_t } CapturePSState_t ;


// Public Function Prototypes

ES_Event RunCapturePSSM( ES_Event CurrentEvent );
void StartCapturePSSM ( ES_Event CurrentEvent );
CapturePSState_t QueryCapturePSSM ( void );

#endif /*SHMTemplate_H */


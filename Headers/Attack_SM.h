/****************************************************************************
Attack Header File
 ****************************************************************************/

#ifndef Attack_H
#define Attack_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Align_and_StartCannon_t, Aligned_t, CannonReady_t, Fire_t } AttackState_t ;


// Public Function Prototypes

ES_Event RunAttackSM( ES_Event CurrentEvent );
void StartAttackSM ( ES_Event CurrentEvent );
AttackState_t QueryAttackSM ( void );

#endif 


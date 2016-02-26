/****************************************************************************
AttackStrategy Header File
 ****************************************************************************/

#ifndef AttackStrategy_H
#define AttackStrategy_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Wait4AttackPhase_t, Attack_t, Wait4NextAttack_t } AttackStrategyState_t ;


// Public Function Prototypes

ES_Event RunAttackStrategySM( ES_Event CurrentEvent );
void StartAttackStrategySM ( ES_Event CurrentEvent );
AttackStrategyState_t QueryAttackStrategySM ( void );
void disableAttacking(void);

#endif 


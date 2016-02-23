/****************************************************************************
GameInfo Header File
 ****************************************************************************/

#include "ES_Types.h"

#ifndef GameInfo_H
#define GameInfo_H

// Public Function Prototypes
uint8_t MyColor(void);
bool isGameStarted(void);
void UpdateADStatus(void);
bool AmIBlocked(void);
bool IsEnemyBlocked(void);
bool IsObstructed(uint8_t which);
void MarkObstructed(uint8_t which);

void SetStationOwner(uint8_t which, Claimed_b owner);
uint8_t GetStationOwner(uint8_t which);

float GetStationX(uint8_t which);
float GetStationY(uint8_t which);

void updatePSStatuses(void);
void updateCapturedFrequency(uint8_t station, uint8_t f_index);
bool checkOwnFrequency(uint8_t f_index);
bool NotByOurStation(void);

#endif /*GameInfo_H */



/* varius registers imported from FoenixMCP kernel - a reference
 * implementation for foenix-series machines
 * 
 * See: https://github.com/pweingar/FoenixMCP/tree/a2560-unified
 */

#ifndef __MCP_SYS_GENERAL_H
#define __MCP_SYS_GENERAL_H

#define MODEL_FOENIX_FMX            0
#define MODEL_FOENIX_C256U          1
#define MODEL_FOENIX_GENX           4
#define MODEL_FOENIX_C256U_PLUS     5
#define MODEL_FOENIX_A2560U_PLUS    6
#define MODEL_FOENIX_A2560X         8
#define MODEL_FOENIX_A2560U         9
#define MODEL_FOENIX_A2560K         11
#define MODEL_FOENIX_A2560M         12

#if MODEL == MODEL_FOENIX_A2560K || MODEL == MODEL_FOENIX_GENX || MODEL == MODEL_FOENIX_A2560X
#include "A2560K/timers_a2560k.h"
#elif MODEL == MODEL_FOENIX_A2560U || MODEL == MODEL_FOENIX_A2560U_PLUS
#include "A2560U/timers_a2560u.h"
#endif

#endif

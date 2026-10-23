/*----------------------
 | movie_decode_kos.h
 | Description: The KallistiOS worker thread libpress_dc.c hands MDEC decodes to, standing in for the PS1's MDEC running beside the CPU.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_MOVIE_DECODE_KOS_H
#define LOM_MOVIE_DECODE_KOS_H

/*----------------------
 | movie_decode_kos_init
 | Description: Starts the decode worker one priority above the game thread and installs it as libpress_dc.c's runner.
 | Author: suinevere
 | Dependencies: thd_create_ex, sem_init, libpress_dc_set_runner
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void movie_decode_kos_init(void);

#endif

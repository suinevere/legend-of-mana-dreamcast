/*----------------------
 | libc_rand.c
 | Description: The Psy-Q libc rand and srand the game was built against, replacing newlib's, whose 31-bit results overflow the game's (rand() * range) >> 15 scaling.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_rand_seed
 | Params: N/A
 | Returns: N/A
 ----------------------*/

/*----------------------
 | s_rand_seed
 | Description: The generator state, zero at boot like the PS1's BSS word at 0x80047440.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned int s_rand_seed;

/*----------------------
 | rand
 | Description: Advances the PS1's linear congruential generator and returns bits 16-30 of the new state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_rand_seed
 | Params: N/A
 | Returns: 0..32767
 ----------------------*/
int rand(void)
{
    s_rand_seed = s_rand_seed * 0x41C64E6Du + 0x3039u;
    return (int)((s_rand_seed >> 16) & 0x7FFFu);
}

/*----------------------
 | srand
 | Description: Sets the generator state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_rand_seed
 | Params: seed -- new state
 | Returns: N/A
 ----------------------*/
void srand(unsigned int seed)
{
    s_rand_seed = seed;
}

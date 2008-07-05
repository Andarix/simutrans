/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>

#define UNICODE 1
#include <windows.h>
#include <mmsystem.h>

#include "../tpl/debug_helper.h"
#include "sound.h"


/*
 * Hajo: flag if sound module should be used
 * with Win32 the number+1 of the device used
 */
static int use_sound = 0;

/* this list contains all the samples
 */
static void *samples[64];
static int sample_number = 0;


/**
 * Sound initialisation routine
 */
void dr_init_sound()
{
	use_sound = 1;
}



/**
 * loads a single sample
 * @return a handle for that sample or -1 on failure
 * @author Hj. Malthaner
 */
int dr_load_sample(char const* filename)
{
	if(use_sound  &&  sample_number>=0  &&  sample_number<64) {
		FILE *fIn=fopen(filename,"rb");
		if(fIn) {
			long len;
			fseek( fIn, 0, SEEK_END );
			len = ftell( fIn );

			if(len>0) {
				samples[sample_number] = GlobalLock( GlobalAlloc(  GMEM_MOVEABLE, (len+4)&0x7FFFFFFCu ) );

				rewind( fIn );
				fread( samples[sample_number], len, 1, fIn );
				fclose( fIn );

				return sample_number++;
			}
		}
	}
	return -1;
}



/**
 * plays a sample
 * @param key the key for the sample to be played
 * @author Hj. Malthaner
 */
void dr_play_sample(int sample_number, int volume)
{
	if(use_sound!=0  &&  sample_number>=0  &&  sample_number<64  &&  volume>1) {
//MESSAGE("dr_play_sample()", "%i sample %i, volume %i ",use_sound,sample_number,volume);
		// prissis short version
		static int oldvol = -1;
		volume = (volume<<8)-1;
		if(oldvol!=volume) {
			long vol = (volume<<16)|volume;
			waveOutSetVolume( 0, vol );
			oldvol = volume;
		}
		// terminate the current sound
		sndPlaySound( NULL, SND_ASYNC );
		// now play
		sndPlaySound(static_cast<WCHAR const*>(samples[sample_number]), SND_MEMORY | SND_ASYNC | SND_NODEFAULT); // XXX this cast seems wrong, samples[] contains char strings, not wide char strings.
	}
}

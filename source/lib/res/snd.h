// OpenAL sound engine
//
// Copyright (c) 2004 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "handle.h"


// introduction:
// this module initializes the sound hardware and provides an interface
// (backed by the resource manager) that allows any number of sounds
// to be emitted simultaneously; they are also closed automatically.
// the most "important" sounds are actually played, while the others
// are discarded (necessary due to limited number of hardware voices).


//
// device enumeration
//

// prepare to enumerate all device names (this resets the list returned by
// snd_dev_next). return 0 on success, otherwise -1 (only if the requisite
// OpenAL extension isn't available). on failure, a "cannot enum device"
// message should be presented to the user, and snd_dev_set need not be
// called; OpenAL will use its default device.
// may be called each time the device list is needed.
extern int snd_dev_prepare_enum();

// return the next device name, or 0 if all have been returned.
// do not call unless snd_dev_prepare_enum succeeded!
// not thread-safe! (static data from snd_dev_prepare_enum is used)
extern const char* snd_dev_next();


//
// sound system setup
//

// tell OpenAL to use the specified device in future.
// name = 0 reverts to OpenAL's default choice, which will also
// be used if this routine is never called.
//
// the device name is typically taken from a config file at init-time;
// the snd_dev* enumeration routines below are used to present a list
// of choices to the user in the options screen.
//
// if OpenAL hasn't yet been initialized (i.e. no sounds have been opened),
//   this just stores the device name for use when init does occur.
//   note: we can't check now if it's invalid (if so, init will fail).
// otherwise, we shut OpenAL down (thereby stopping all sounds) and
// re-initialize with the new device. that's fairly time-consuming,
// so preferably call this routine before sounds are loaded.
//
// return 0 on success, or the status returned by OpenAL re-init.
extern int snd_dev_set(const char* alc_new_dev_name);

// set maximum number of voices to play simultaneously,
// to reduce mixing cost on low-end systems.
// return 0 on success, or 1 if limit was ignored
// (e.g. if higher than an implementation-defined limit anyway).
extern int snd_set_max_voices(uint cap);

// set amplitude modifier, which is effectively applied to all sounds.
// must be non-negative; 1 -> unattenuated, 0.5 -> -6 dB, 0 -> silence.
extern int snd_set_master_gain(float gain);


//
// sound instance
//

// open and return a handle to a sound.
//
// if <snd_fn> is a text file (extension ".txt"), it is assumed
// to be a definition file containing the sound file name and
// its gain (0.0 .. 1.0).
// otherwise, <snd_fn> is taken to be the sound file name and
// gain is set to the default of 1.0 (no attenuation).
//
// is_stream (default false) forces the sound to be opened as a stream:
// opening is faster, it won't be kept in memory, but only one instance
// can be open at a time.
extern Handle snd_open(const char* snd_fn, bool stream = false);

// close the sound <hs> and set hs to 0. if it was playing,
// it will be stopped. sounds are closed automatically when done
// playing; this is provided for completeness only.
extern int snd_free(Handle& hs);

// request the sound <hs> be played. once done playing, the sound is
// automatically closed (allows fire-and-forget play code).
// if no hardware voice is available, this sound may not be played at all,
// or in the case of looped sounds, later.
// priority (min 0 .. max 1, default 0) indicates which sounds are
// considered more important; this is attenuated by distance to the
// listener (see snd_update).
extern int snd_play(Handle hs, float priority = 0.0f);

// change 3d position of the sound source.
// if relative (default false), (x,y,z) is treated as relative to the
// listener; otherwise, it is the position in world coordinates.
// may be called at any time; fails with invalid handle return if
// the sound has already been closed (e.g. it never played).
extern int snd_set_pos(Handle hs, float x, float y, float z, bool relative = false);

// change gain (amplitude modifier) of the sound source.
// must be non-negative; 1 -> unattenuated, 0.5 -> -6 dB, 0 -> silence.
// may be called at any time; fails with invalid handle return if
// the sound has already been closed (e.g. it never played).
extern int snd_set_gain(Handle hs, float gain);

// enable/disable looping on the sound source.
// used to implement variable-length sounds (e.g. while building).
// may be called at any time; fails with invalid handle return if
// the sound has already been closed (e.g. it never played).
//
// notes:
// - looping sounds are not discarded if they cannot be played for lack of
//   a hardware voice at the moment play was requested.
// - once looping is again disabled and the sound has reached its end,
//   the sound instance is freed automatically (as if never looped).
extern int snd_set_loop(Handle hs, bool loop);


//
// sound engine
//

// perform housekeeping (e.g. streaming); call once a frame.
//
// additionally, if any parameter is non-NULL, we set the listener
// position, look direction, and up vector (in world coordinates).
extern int snd_update(const float* pos, const float* dir, const float* up);

// free all resources and shut down the sound system.
// call before h_mgr_shutdown.
extern void snd_shutdown();

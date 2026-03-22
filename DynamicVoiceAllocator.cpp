/**
 * @file dynamic_voice_allocator.cpp
 * @brief MIDI voice allocation management
 * @author osoumen
 * @date 2014/11/30
 */

#include "DynamicVoiceAllocator.h"
#include <assert.h>

//-----------------------------------------------------------------------------
DynamicVoiceAllocator::VoiceStatusList::VoiceStatusList(int numSlots)
{
	mSlots = new VoiceSlotStatus[numSlots];
	mNumSlots = mVoiceLimit = numSlots;
	reset();
}

//-----------------------------------------------------------------------------
DynamicVoiceAllocator::VoiceStatusList::~VoiceStatusList()
{
	delete [] mSlots;
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::VoiceStatusList::reset()
{
	for (int i=0; i<mNumSlots; ++i) {
		mSlots[i].isAlloced = false;
		mSlots[i].isKeyOn = false;
		mSlots[i].midiCh = 0;
		mSlots[i].priority = 0;
		mSlots[i].uniqueId = 0;
		mSlots[i].timestamp = 0;
	}
	mTimeStamp = 0;
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::VoiceStatusList::allocVoice(int slot, int midiCh, int prio, int uniqueId)
{
	assert(slot < mNumSlots);
	mSlots[slot].isAlloced = true;
	mSlots[slot].midiCh = midiCh;
	mSlots[slot].priority = prio;
	mSlots[slot].isKeyOn = false;
	mSlots[slot].uniqueId = uniqueId;
	mSlots[slot].timestamp = mTimeStamp;
	mTimeStamp++;
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::VoiceStatusList::removeVoice(int slot)
{
	assert(slot < mNumSlots);
	mSlots[slot].isAlloced = false;
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::VoiceStatusList::changeVoiceLimit(int voiceLimit)
{
	mVoiceLimit = voiceLimit;
	if (mVoiceLimit > mNumSlots) {
		mVoiceLimit = mNumSlots;
	}
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::VoiceStatusList::setKeyOn(int slot)
{
	assert(slot < mNumSlots);
	if (mSlots[slot].isAlloced) {
		mSlots[slot].isKeyOn = true;
	}
}

//-----------------------------------------------------------------------------
bool DynamicVoiceAllocator::VoiceStatusList::isKeyOn(int slot)
{
	assert(slot < mNumSlots);
	return mSlots[slot].isKeyOn;
}

//-----------------------------------------------------------------------------
bool DynamicVoiceAllocator::VoiceStatusList::isAlloced(int slot)
{
	assert(slot < mNumSlots);
	return mSlots[slot].isAlloced;
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::VoiceStatusList::changeState(int slot, int prio, int uniqueId, bool isKeyOn)
{
	mSlots[slot].priority = prio;
	mSlots[slot].uniqueId = uniqueId;
	mSlots[slot].isKeyOn = isKeyOn;
}

//-----------------------------------------------------------------------------
int DynamicVoiceAllocator::VoiceStatusList::getVoiceMidiCh(int slot)
{
	assert(slot < mNumSlots);
	return mSlots[slot].midiCh;
}

//-----------------------------------------------------------------------------
int DynamicVoiceAllocator::VoiceStatusList::getVoiceUniqueId(int slot)
{
	assert(slot < mNumSlots);
	return mSlots[slot].uniqueId;
}

//-----------------------------------------------------------------------------
int	DynamicVoiceAllocator::VoiceStatusList::findFreeVoice(int priorCh)
{
	int time_stamp_max = -1;
	int v = VOICE_UNSET;
	
	if (priorCh == CHANNEL_UNSET) {
		for (int i=0; i<mVoiceLimit; ++i) {
			int time_stamp = mTimeStamp - mSlots[i].timestamp;
			if ((time_stamp > time_stamp_max) &&
				(mSlots[i].isAlloced == false)) {
				time_stamp_max = time_stamp;
				v = i;
			}
		}
	}
	else {
		for (int i=0; i<mVoiceLimit; ++i) {
			if (mSlots[i].midiCh == priorCh) {
				int time_stamp = mTimeStamp - mSlots[i].timestamp;
				if ((time_stamp > time_stamp_max) &&
					(mSlots[i].isAlloced == false)) {
					time_stamp_max = time_stamp;
					v = i;
				}
			}
		}
	}
	return v;
}

//-----------------------------------------------------------------------------
int	DynamicVoiceAllocator::VoiceStatusList::findWeakestVoiceInMidiCh(int midiCh)
{
	// Find the allocated voice sounding on midiCh with the lowest priority and oldest timestamp
	// Returns VOICE_UNSET if no voice on the same midiCh is found
	
	int time_stamp_max = -1;
	int v = VOICE_UNSET;
	int prio_min = 0x7fff;
	
	for (int i=0; i<mVoiceLimit; ++i) {
		if ((mSlots[i].midiCh == midiCh) &&
			(mSlots[i].isAlloced == true) &&
			(mSlots[i].priority <= prio_min)) {
			int time_stamp = mTimeStamp - mSlots[i].timestamp;
			if (time_stamp > time_stamp_max) {
				v = i;
				time_stamp_max = time_stamp;
				prio_min = mSlots[i].priority;
			}
		}
	}
	return v;
}

//-----------------------------------------------------------------------------
int	DynamicVoiceAllocator::VoiceStatusList::findWeakestVoice()
{
	int v = VOICE_UNSET;
	
	// Find the allocated voice with the lowest priority and oldest timestamp among all voices
	int time_stamp_max = -1;
	int prio_min = 0x7fff;
	
	for (int i=0; i<mVoiceLimit; ++i) {
		if ((mSlots[i].isAlloced == true) &&
			(mSlots[i].priority <= prio_min)) {
			int time_stamp = mTimeStamp - mSlots[i].timestamp;
			if (time_stamp > time_stamp_max) {
				v = i;
				time_stamp_max = time_stamp;
				prio_min = mSlots[i].priority;
			}
		}
	}
	return v;
}

//-----------------------------------------------------------------------------
DynamicVoiceAllocator::DynamicVoiceAllocator() :
mVoiceList(MAX_VOICE),
mVoiceLimit(8),
mAllocMode(ALLOC_MODE_OLDEST)
{
	
}

//-----------------------------------------------------------------------------
DynamicVoiceAllocator::~DynamicVoiceAllocator()
{
	
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::Initialize(int voiceLimit)
{
	mVoiceLimit = voiceLimit;
	mVoiceList.changeVoiceLimit(voiceLimit);
	for (int i=0; i<16; i++) {
		mChNoteOns[i] = 0;
		mChLimit[i] = 127;
	}
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::Reset()
{
	for (int i=0; i<16; i++) {
		mChNoteOns[i] = 0;
	}
	mVoiceList.reset();
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::ChangeVoiceLimit(int voiceLimit)
{
	if ( voiceLimit < mVoiceLimit ) {
		//Remove from the free voice list
		for ( int i=voiceLimit; i<mVoiceLimit; i++ ) {
			mVoiceList.removeVoice(i);
		}
	}
	mVoiceLimit = voiceLimit;
	mVoiceList.changeVoiceLimit(voiceLimit);
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::SetVoiceAllocMode(VoiceAllocMode mode)
{
	mAllocMode = mode;
}

//-----------------------------------------------------------------------------
int DynamicVoiceAllocator::AllocVoice(int prio, int ch, int uniqueID, int forceVo,
									  int *releasedCh, bool *isLegato)
{
	int v = VOICE_UNSET;
	*releasedCh = CHANNEL_UNSET;
	*isLegato = false;
	
	if (forceVo != VOICE_UNSET) {
		v = forceVo;     // Allocate a fixed channel
		if (mVoiceList.isAlloced(v)) {
			if (mVoiceList.getVoiceMidiCh(v) == ch) {
				// Sound played as legato
				// If triggered twice before key-on, only the last note-on should be effective
				if (mVoiceList.isKeyOn(v)) {
					*isLegato = true;
				}
			}
			else {
				// If a voice from a different channel already exists
				mVoiceList.removeVoice(v);
				*releasedCh = mVoiceList.getVoiceMidiCh(v);
				mChNoteOns[mVoiceList.getVoiceMidiCh(v)]--;
			}
		}
	}
	else {
		if (mChNoteOns[ch] >= mChLimit[ch]) {
			// If channel note count is exceeded, stop one note on that channel and play the next
			v = mVoiceList.findWeakestVoiceInMidiCh(ch);
			if (v != VOICE_UNSET) {
				mVoiceList.removeVoice(v);
				*releasedCh = mVoiceList.getVoiceMidiCh(v);
				mChNoteOns[mVoiceList.getVoiceMidiCh(v)]--;
			}
		}
		else {
			// If channel limit is not set or not exceeded, steal the lowest priority voice from all voices (last-in takes priority)
			if (mAllocMode == ALLOC_MODE_SAMECH) {
				v = mVoiceList.findFreeVoice(ch);
				if (v == VOICE_UNSET) {
					v = mVoiceList.findFreeVoice();
				}
			}
			else {
				v = mVoiceList.findFreeVoice();
			}
			
			if (v == VOICE_UNSET) {
				if (mAllocMode == ALLOC_MODE_SAMECH) {
					v = mVoiceList.findWeakestVoiceInMidiCh(ch);
				}
				v = mVoiceList.findWeakestVoice();
				mVoiceList.removeVoice(v);
				*releasedCh = mVoiceList.getVoiceMidiCh(v);
				// Decrement count for the stopped slot
				mChNoteOns[mVoiceList.getVoiceMidiCh(v)]--;
			}
		}
	}
	
	if (v != VOICE_UNSET) {
		if (*isLegato == false && !mVoiceList.isAlloced(v)) { // Handle simultaneous note-on in mono mode
			mChNoteOns[ch]++;
		}
		mVoiceList.allocVoice(v, ch, prio, uniqueID);
	}
	return v;
}

//-----------------------------------------------------------------------------
int DynamicVoiceAllocator::ReleaseVoice(int relPrio, int ch, int uniqueID, int *relVo)
{
	int stops = 0;
	
	// Release one of the sounding voices that matches the uniqueID
	// TODO: Find the oldest one
	for (int vo=0; vo<MAX_VOICE; ++vo) {
		if (mVoiceList.getVoiceUniqueId(vo) == uniqueID) {
			if (mVoiceList.isKeyOn(vo)) {
				mVoiceList.changeState(vo, relPrio, 0, false);
			}
			mVoiceList.removeVoice(vo);
			stops++;
			mChNoteOns[ch]--;
			*relVo = vo;
			break;  // Leave double-triggering to the host
		}
	}
	return stops;
}

//-----------------------------------------------------------------------------
bool DynamicVoiceAllocator::ReleaseAllVoices(int ch)
{
	bool stoped = false;
	
	for (int vo=0; vo<MAX_VOICE; ++vo) {
		if (mVoiceList.getVoiceMidiCh(vo) == ch) {
			if (mVoiceList.isKeyOn(vo)) {
				mVoiceList.changeState(vo, 0, 0, false);
			}
			mVoiceList.removeVoice(vo);
			stoped = true;
			mChNoteOns[ch]--;
		}
	}
	return stoped;
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::SetChLimit(int ch, int value)
{
	mChLimit[ch & 0xf] = value;
}

//-----------------------------------------------------------------------------
int DynamicVoiceAllocator::GetChLimit(int ch)
{
	return mChLimit[ch & 0xf];
}

//-----------------------------------------------------------------------------
int DynamicVoiceAllocator::GetNoteOns(int ch)
{
	return mChNoteOns[ch & 0xf];
}

//-----------------------------------------------------------------------------
void DynamicVoiceAllocator::SetKeyOn(int vo)
{
	mVoiceList.setKeyOn(vo);
}

//-----------------------------------------------------------------------------
bool DynamicVoiceAllocator::IsKeyOn(int vo)
{
	return mVoiceList.isKeyOn(vo);
}

//-----------------------------------------------------------------------------
bool DynamicVoiceAllocator::IsPlayingVoice(int v)
{
	return mVoiceList.isAlloced(v);
}

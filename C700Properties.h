//
//  C700Properties.h
//  C700
//
//  Created by osoumen on 2017/02/16.
//
//

#ifndef __C700__C700Properties__
#define __C700__C700Properties__

#include "C700defines.h"
#include <map>


// properties
enum
{
    kAudioUnitCustomProperty_Begin = 64000,
    
    kAudioUnitCustomProperty_ProgramName = kAudioUnitCustomProperty_Begin,
    kAudioUnitCustomProperty_BRRData,
    kAudioUnitCustomProperty_Rate,
    kAudioUnitCustomProperty_BaseKey,
    kAudioUnitCustomProperty_LowKey,
    kAudioUnitCustomProperty_HighKey,
    kAudioUnitCustomProperty_LoopPoint,
    kAudioUnitCustomProperty_Loop,
    kAudioUnitCustomProperty_AR,
    kAudioUnitCustomProperty_DR,
    kAudioUnitCustomProperty_SL,
    kAudioUnitCustomProperty_SR1,
    kAudioUnitCustomProperty_VolL,
    kAudioUnitCustomProperty_VolR,
    kAudioUnitCustomProperty_Echo,
    kAudioUnitCustomProperty_Bank,
    kAudioUnitCustomProperty_EditingProgram,
    kAudioUnitCustomProperty_EditingChannel,
    
    // Echo section (below)
    kAudioUnitCustomProperty_Band1,
    kAudioUnitCustomProperty_Band2,
    kAudioUnitCustomProperty_Band3,
    kAudioUnitCustomProperty_Band4,
    kAudioUnitCustomProperty_Band5,
    
    kAudioUnitCustomProperty_TotalRAM,          // read only
    
    kAudioUnitCustomProperty_PGDictionary,
    kAudioUnitCustomProperty_XIData,            // read only
    
    kAudioUnitCustomProperty_NoteOnTrack_1,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_2,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_3,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_4,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_5,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_6,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_7,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_8,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_9,     // read only
    kAudioUnitCustomProperty_NoteOnTrack_10,    // read only
    kAudioUnitCustomProperty_NoteOnTrack_11,    // read only
    kAudioUnitCustomProperty_NoteOnTrack_12,    // read only
    kAudioUnitCustomProperty_NoteOnTrack_13,    // read only
    kAudioUnitCustomProperty_NoteOnTrack_14,    // read only
    kAudioUnitCustomProperty_NoteOnTrack_15,    // read only
    kAudioUnitCustomProperty_NoteOnTrack_16,    // read only
    
    kAudioUnitCustomProperty_MaxNoteTrack_1,
    kAudioUnitCustomProperty_MaxNoteTrack_2,
    kAudioUnitCustomProperty_MaxNoteTrack_3,
    kAudioUnitCustomProperty_MaxNoteTrack_4,
    kAudioUnitCustomProperty_MaxNoteTrack_5,
    kAudioUnitCustomProperty_MaxNoteTrack_6,
    kAudioUnitCustomProperty_MaxNoteTrack_7,
    kAudioUnitCustomProperty_MaxNoteTrack_8,
    kAudioUnitCustomProperty_MaxNoteTrack_9,
    kAudioUnitCustomProperty_MaxNoteTrack_10,
    kAudioUnitCustomProperty_MaxNoteTrack_11,
    kAudioUnitCustomProperty_MaxNoteTrack_12,
    kAudioUnitCustomProperty_MaxNoteTrack_13,
    kAudioUnitCustomProperty_MaxNoteTrack_14,
    kAudioUnitCustomProperty_MaxNoteTrack_15,
    kAudioUnitCustomProperty_MaxNoteTrack_16,
    
    kAudioUnitCustomProperty_SourceFileRef,
    kAudioUnitCustomProperty_IsEmaphasized,
    
    kAudioUnitCustomProperty_SustainMode,
    kAudioUnitCustomProperty_MonoMode,
    kAudioUnitCustomProperty_PortamentoOn,
    kAudioUnitCustomProperty_PortamentoRate,
    kAudioUnitCustomProperty_NoteOnPriority,
    kAudioUnitCustomProperty_ReleasePriority,
    
    kAudioUnitCustomProperty_IsHwConnected,     // read only
    
    // Global settings file is shared between AU and VST, so saveToGlobal properties use CString
    kAudioUnitCustomProperty_SongRecordPath,    // char[PATH_LEN_MAX] ->dsp
    kAudioUnitCustomProperty_RecSaveAsSpc,      // bool ->dsp
    kAudioUnitCustomProperty_RecSaveAsSmc,      // bool ->dsp
    kAudioUnitCustomProperty_RecordStartBeatPos,// double ->kernel
    kAudioUnitCustomProperty_RecordLoopStartBeatPos,// double ->kernel
    kAudioUnitCustomProperty_RecordEndBeatPos,  // double ->kernel
    kAudioUnitCustomProperty_TimeBaseForSmc,    // int ->dsp
    kAudioUnitCustomProperty_GameTitle,         // char[32] (truncated to 21 bytes for SMC output) ->dsp
    kAudioUnitCustomProperty_SongTitle,         // char[32] ->dsp
    kAudioUnitCustomProperty_NameOfDumper,      // char[16] ->dsp
    kAudioUnitCustomProperty_ArtistOfSong,      // char[32] ->dsp
    kAudioUnitCustomProperty_SongComments,      // char[32] ->dsp
    kAudioUnitCustomProperty_RepeatNumForSpc,   // float ->dsp
    kAudioUnitCustomProperty_FadeMsTimeForSpc,  // int ->dsp
    
    kAudioUnitCustomProperty_SongPlayerCode,    // CFData ->dsp write only
    kAudioUnitCustomProperty_SongPlayerCodeVer, // int32 read only
    
    kAudioUnitCustomProperty_HostBeatPos,       // double read only
    
    kAudioUnitCustomProperty_MaxNoteOnTotal,    // int
    
    kAudioUnitCustomProperty_PitchModulationOn,
    kAudioUnitCustomProperty_NoiseOn,
    kAudioUnitCustomProperty_SR2,
    
    // New properties must always be added after this point
    
    kAudioUnitCustomProperty_End,
    kNumberOfProperties = kAudioUnitCustomProperty_End-kAudioUnitCustomProperty_Begin
};

enum PropertyDataType {
    propertyDataTypeFloat32,        // 32-bit float
    propertyDataTypeDouble,         // 64-bit float
    propertyDataTypeInt32,          // 32-bit integer
    propertyDataTypeBool,           // bool (stored as int in VST)
    propertyDataTypeStruct,         // Struct with size specified by outDataSize
    propertyDataTypeString,         // Wrapped as CFString in AU; CString with max length outDataSize in VST
    propertyDataTypeFilePath,       // Wrapped as CFURL in AU; CString with max length outDataSize in VST
    propertyDataTypeVariableData,   // Variable-length data wrapped as CFData in AU; size set during Set/Get in VST
    propertyDataTypePointer,        // Plain pointer type; used only for UI communication, not saved
};

typedef struct {
    unsigned int     propId;        // Property ID
    unsigned int     outDataSize;   // Data size; bool indicates 1 but uses 4 bytes when saving as VST chunk
    bool             outWritable;   // Whether the data retrieved via AU Get is writable
    PropertyDataType dataType;      // Data type
    bool             readOnly;      // Set to true if set is not implemented
    bool             saveToProg;    // Include in program (patch) data
    bool             saveToSong;    // Include when saving a song
    bool             saveToGlobal;  // Include in global settings
    char             savekey[32];   // Key used when saving in AU; required if any save flag is set
    double           defaultValue;  // Default value used when not set; negative value disables it
} PropertyDescription;

void createPropertyParamMap(std::map<int, PropertyDescription> &m);

#endif /* defined(__C700__C700Properties__) */

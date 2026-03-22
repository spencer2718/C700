/*
 *  AudioFile.cpp
 *  C700
 *
 *  Created by osoumen on 12/10/10.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "AudioFile.h"
#include "vstgui.h"

#if MAC
#include <AudioToolbox/AudioFile.h>
#include <AudioToolbox/AudioConverter.h>
#else
#pragma comment ( lib, "winmm.lib" )
#include <mmsystem.h>
#include <math.h>
#endif

//-----------------------------------------------------------------------------
AudioFile::AudioFile( const char *path, bool isWriteable )
: FileAccess(path, isWriteable)
, m_pAudioData( NULL )
, mLoadedSamples( 0 )
{
	mPi = acosf(-1.0f);
}

//-----------------------------------------------------------------------------
AudioFile::~AudioFile()
{
	if ( m_pAudioData ) {
		delete [] m_pAudioData;
	}
}

//-----------------------------------------------------------------------------
bool AudioFile::IsVarid()
{
#if MAC
	AudioFileID mAudioFileID;
	CFURLRef	url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)GetFilePath(), strlen(GetFilePath()), false);
	OSStatus err = AudioFileOpenURL(url, kAudioFileReadPermission, 0, &mAudioFileID);
	CFRelease(url);
	if (err == noErr) {
		AudioFileClose(mAudioFileID);
		return true;
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
bool AudioFile::Load()
{
#if MAC
	AudioFileID mAudioFileID;
    AudioStreamBasicDescription fileDescription, outputFormat;
    SInt64 dataSize64;
    UInt32 dataSize;
	
	OSStatus err;
	UInt32 size;
	
    // Open the file
	CFURLRef	url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)GetFilePath(), strlen(GetFilePath()), false);
	err = AudioFileOpenURL(url, kAudioFileReadPermission, 0, &mAudioFileID);
	CFRelease(url);
    if (err) {
        //NSLog(@"AudioFileOpen failed");
        return false;
    }
	
    // Get the basic info of the opened file into fileDescription
    size = sizeof(AudioStreamBasicDescription);
	err = AudioFileGetProperty(mAudioFileID, kAudioFilePropertyDataFormat, 
							   &size, &fileDescription);
    if (err) {
        //NSLog(@"AudioFileGetProperty failed");
        AudioFileClose(mAudioFileID);
        return false;
    }
	
    // Get the byte count of the opened file's data section into dataSize
    size = sizeof(SInt64);
	err = AudioFileGetProperty(mAudioFileID, kAudioFilePropertyAudioDataByteCount, 
							   &size, &dataSize64);
    if (err) {
        //NSLog(@"AudioFileGetProperty failed");
        AudioFileClose(mAudioFileID);
        return false;
    }
	dataSize = static_cast<UInt32>(dataSize64);
	
	AudioFileTypeID	fileTypeID;
	size = sizeof( AudioFileTypeID );
	err = AudioFileGetProperty(mAudioFileID, kAudioFilePropertyFileFormat, &size, &fileTypeID);
	if (err) {
        //NSLog(@"AudioFileGetProperty failed");
        AudioFileClose(mAudioFileID);
        return false;
    }
	
	// Initialize instrument info
	mInstData.basekey	= 60;
	mInstData.lowkey	= 0;
	mInstData.highkey	= 127;
	mInstData.loop		= 0;

	// Get loop points
	Float64		st_point=0.0,end_point=0.0;
	if ( fileTypeID == kAudioFileAIFFType || fileTypeID == kAudioFileAIFCType ) {
		// Get the INST chunk
		AudioFileGetUserDataSize(mAudioFileID, 'INST', 0, &size);
		if ( size > 4 ) {
			UInt8	*instChunk = new UInt8[size];
			AudioFileGetUserData(mAudioFileID, 'INST', 0, &size, instChunk);
			
			// Get MIDI info
			mInstData.basekey = instChunk[0];
			mInstData.lowkey = instChunk[2];
			mInstData.highkey = instChunk[3];
			
			if ( instChunk[9] > 0 ) {	// Check the loop flag
				// Get markers
				UInt32	writable;
				err = AudioFileGetPropertyInfo(mAudioFileID, kAudioFilePropertyMarkerList,
											   &size, &writable);
				if (err) {
					//NSLog(@"AudioFileGetPropertyInfo failed");
					AudioFileClose(mAudioFileID);
					return NULL;
				}
				UInt8	*markersBuffer = new UInt8[size];
				AudioFileMarkerList	*markers = reinterpret_cast<AudioFileMarkerList*>(markersBuffer);
				
				err = AudioFileGetProperty(mAudioFileID, kAudioFilePropertyMarkerList, 
										   &size, markers);
				if (err) {
					//NSLog(@"AudioFileGetProperty failed");
					AudioFileClose(mAudioFileID);
					return NULL;
				}
				
				// Set loop points
				for (unsigned int i=0; i<markers->mNumberMarkers; i++) {
					if (markers->mMarkers[i].mMarkerID == instChunk[11] ) {
						st_point = markers->mMarkers[i].mFramePosition;
					}
					else if (markers->mMarkers[i].mMarkerID == instChunk[13] ) {
						end_point = markers->mMarkers[i].mFramePosition;
					}
					CFRelease(markers->mMarkers[i].mName);
				}
				if ( st_point < end_point ) {
					mInstData.loop = 1;
				}
				delete [] markersBuffer;
			}
			delete [] instChunk;
		}
		
	}
	else if ( fileTypeID == kAudioFileWAVEType ) {
		// Get the smpl chunk
		AudioFileGetUserDataSize( mAudioFileID, 'smpl', 0, &size );
		if ( size >= sizeof(WAV_smpl) ) {
			UInt8	*smplChunk = new UInt8[size];
			AudioFileGetUserData( mAudioFileID, 'smpl', 0, &size, smplChunk );
			WAV_smpl	*smpl = (WAV_smpl *)smplChunk;
			
			smpl->loops = EndianU32_LtoN( smpl->loops );
			
			if ( smpl->loops > 0 ) {
				mInstData.loop = true;
				mInstData.basekey = EndianU32_LtoN( smpl->note );
				st_point = EndianU32_LtoN( smpl->start );
				end_point = EndianU32_LtoN( smpl->end ) + 1;	// SoundForge etc. interpret the end point as inclusive
				//end_point = EndianU32_LtoN( smpl->end );	// For some reason, Peak uses the same interpretation as AIFF
			}
			else {
				mInstData.basekey = EndianU32_LtoN( smpl->note );
			}
			delete [] smplChunk;
		}
	}
	
	// Limit the size
	SInt64	dataSamples = dataSize / fileDescription.mBytesPerFrame;
	if ( dataSamples > MAXIMUM_SAMPLES ) {
		dataSize = MAXIMUM_SAMPLES * fileDescription.mBytesPerFrame;
	}
	if ( st_point > MAXIMUM_SAMPLES ) {
		st_point = MAXIMUM_SAMPLES;
	}
	if ( end_point > MAXIMUM_SAMPLES ) {
		end_point = MAXIMUM_SAMPLES;
	}
	
    // Allocate memory for temporary waveform loading
    char *fileBuffer;
	unsigned int	fileBufferSize;
	if (mInstData.loop) {
		fileBufferSize = dataSize+EXPAND_BUFFER*fileDescription.mBytesPerFrame;
	}
	else {
		fileBufferSize = dataSize;
	}
	fileBuffer = new char[fileBufferSize];
	memset(fileBuffer, 0, fileBufferSize);
	
	// Read waveform data from the file
	err = AudioFileReadBytes(mAudioFileID, false, 0, &dataSize, fileBuffer);
    if (err) {
        //NSLog(@"AudioFileReadBytes failed");
        AudioFileClose(mAudioFileID);
        delete [] fileBuffer;
        return false;
    }
    AudioFileClose(mAudioFileID);
	
    // Expand the loop
    Float64	adjustment = 1.0;
    outputFormat=fileDescription;
	if (mInstData.loop) {
		UInt32	plusalpha=0, framestocopy;
		while (plusalpha < EXPAND_BUFFER) {
			framestocopy = 
			(end_point-st_point)>(EXPAND_BUFFER-plusalpha)?(EXPAND_BUFFER-plusalpha):end_point-st_point;
			memcpy(fileBuffer+((int)end_point+plusalpha)*fileDescription.mBytesPerFrame,
				   fileBuffer+(int)st_point*fileDescription.mBytesPerFrame,
				   framestocopy*fileDescription.mBytesPerFrame);
			plusalpha += framestocopy;
		}
		dataSize += plusalpha*fileDescription.mBytesPerFrame;
		
		// Fix to 16-sample boundary
		adjustment = ( (long long)((end_point-st_point)/16) ) / ((end_point-st_point)/16.0);
		st_point *= adjustment;
		end_point *= adjustment;
	}
	outputFormat.mFormatID = kAudioFormatLinearPCM;
    outputFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian;
	outputFormat.mChannelsPerFrame = 1;
	outputFormat.mBytesPerFrame = sizeof(float);
	outputFormat.mBitsPerChannel = 32;
	outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame;
	
    // Set up a converter for byte order conversion
    AudioConverterRef converter;
	err = AudioConverterNew(&fileDescription, &outputFormat, &converter);
    if (err) {
        //NSLog(@"AudioConverterNew failed");
        delete [] fileBuffer;
        return false;
    }
	
	// Set sample rate conversion quality to maximum
//	if (fileDescription.mSampleRate != outputFormat.mSampleRate) {
//		size = sizeof(UInt32);
//		UInt32	setProp = kAudioConverterQuality_Max;
//		AudioConverterSetProperty(converter, kAudioConverterSampleRateConverterQuality,
//								  size, &setProp);
//        
//        size = sizeof(UInt32);
//		setProp = kAudioConverterSampleRateConverterComplexity_Mastering;
//		AudioConverterSetProperty(converter, kAudioConverterSampleRateConverterComplexity,
//								  size, &setProp);
//        
//	}
	
    // Get a sufficient buffer size for the output
	UInt32	outputSize = dataSize;
	size = sizeof(UInt32);
	err = AudioConverterGetProperty(converter, kAudioConverterPropertyCalculateOutputBufferSize, 
									&size, &outputSize);
	if (err) {
		//NSLog(@"AudioConverterGetProperty failed");
		delete [] fileBuffer;
		AudioConverterDispose(converter);
        return false;
	}
    UInt32 monoSamples = outputSize/sizeof(float);
    
    // Byte order conversion
    float *monoData = new float[monoSamples];
	AudioConverterConvertBuffer(converter, dataSize, fileBuffer,
								&outputSize, monoData);
    if(outputSize == 0) {
        //NSLog(@"AudioConverterConvertBuffer failed");
        delete [] fileBuffer;
        AudioConverterDispose(converter);
        return false;
    }
    
    // If loop length is not a multiple of 16, perform sample rate conversion
    Float64 inputSampleRate = fileDescription.mSampleRate;
    Float64 outputSampleRate = fileDescription.mSampleRate * adjustment;
    int	outSamples = monoSamples;
    if ( outputSampleRate == inputSampleRate ) {
        m_pAudioData = new short[monoSamples];
        for (int i=0; i<monoSamples; i++) {
            m_pAudioData[i] = static_cast<short>(monoData[i] * 32768);
        }
    }
    else {
        outSamples = static_cast<int>(monoSamples / (inputSampleRate / outputSampleRate));
        m_pAudioData = new short[outSamples];
        resampling(monoData, monoSamples, inputSampleRate,
                   m_pAudioData, &outSamples, outputSampleRate);
    }
    
    // Cleanup
    delete [] monoData;
    delete [] fileBuffer;
    AudioConverterDispose(converter);
	
	// Set instrument data
	if ( st_point > MAXIMUM_SAMPLES ) {
		mInstData.lp = MAXIMUM_SAMPLES;
	}
	else {
		mInstData.lp			= st_point;
	}
	if ( end_point > MAXIMUM_SAMPLES ) {
		mInstData.lp_end = MAXIMUM_SAMPLES;
	}
	else {
		mInstData.lp_end		= end_point;
	}
	mInstData.srcSamplerate	= outputSampleRate;
    mLoadedSamples			= outSamples;
	
	mIsLoaded = true;
	
	return true;
#else
	// Windows audio file loading process

	// Open the file
	HMMIO	hmio = NULL;
	MMRESULT	err;
	DWORD		size;

	hmio = mmioOpen( mPath, NULL, MMIO_READ );
	if ( !hmio ) {
		return false;
	}
	
	// Find the RIFF chunk
	MMCKINFO	riffChunkInfo;
	riffChunkInfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	err = mmioDescend( hmio, &riffChunkInfo, NULL, MMIO_FINDRIFF );
	if ( err != MMSYSERR_NOERROR ) {
		mmioClose( hmio, 0 );
		return false;
	}
	if ( (riffChunkInfo.ckid != FOURCC_RIFF) || (riffChunkInfo.fccType != mmioFOURCC('W', 'A', 'V', 'E') ) ) {
		mmioClose( hmio, 0 );
		return false;
	}

	// Find the format chunk
	MMCKINFO	formatChunkInfo;
	formatChunkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	err = mmioDescend( hmio, &formatChunkInfo, &riffChunkInfo, MMIO_FINDCHUNK );
	if ( err != MMSYSERR_NOERROR ) {
		mmioClose( hmio, 0 );
		return false;
	}
	if ( formatChunkInfo.cksize < sizeof(PCMWAVEFORMAT) ) {
		mmioClose( hmio, 0 );
		return false;
	}

	// Get format info
	WAVEFORMATEX	pcmWaveFormat;
	DWORD			fmsize = (formatChunkInfo.cksize > sizeof(WAVEFORMATEX)) ? sizeof(WAVEFORMATEX):formatChunkInfo.cksize;
	size = mmioRead( hmio, (HPSTR)&pcmWaveFormat, fmsize );
	if ( size != fmsize ) {
		mmioClose( hmio, 0 );
		return false;
	}
	if ( pcmWaveFormat.wFormatTag != WAVE_FORMAT_PCM ) {
		mmioClose( hmio, 0 );
		return false;
	}
	mmioAscend(hmio, &formatChunkInfo, 0);

	// Initialize instrument info
	mInstData.basekey	= 60;
	mInstData.lowkey	= 0;
	mInstData.highkey	= 127;
	mInstData.loop		= 0;

	// Find the smpl chunk
	MMCKINFO	smplChunkInfo;
	smplChunkInfo.ckid = mmioFOURCC('s', 'm', 'p', 'l');
	err = mmioDescend( hmio, &smplChunkInfo, &riffChunkInfo, MMIO_FINDCHUNK );
	if ( err != MMSYSERR_NOERROR ) {
		smplChunkInfo.cksize = 0;
	}
	double	st_point=0.0;
	double	end_point=0.0;
	if ( smplChunkInfo.cksize >= sizeof(WAV_smpl) ) {
		// Get loop points
		unsigned char	*smplChunk = new unsigned char[smplChunkInfo.cksize];
		size = mmioRead(hmio,(HPSTR)smplChunk, smplChunkInfo.cksize);
		WAV_smpl	*smpl = (WAV_smpl *)smplChunk;

		if ( smpl->loops > 0 ) {
			mInstData.loop = 1;
			mInstData.basekey = smpl->note;
			st_point = smpl->start;
			end_point = smpl->end + 1;	// SoundForge etc. interpret the end point as inclusive
		}
		else {
			mInstData.basekey = smpl->note;
		}
		delete [] smplChunk;
	}
	mmioAscend(hmio, &formatChunkInfo, 0);

	// Find the data chunk
	MMCKINFO dataChunkInfo;
	dataChunkInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
	err = mmioDescend( hmio, &dataChunkInfo, &riffChunkInfo, MMIO_FINDCHUNK );
	if( err != MMSYSERR_NOERROR ) {
		mmioClose( hmio, 0 );
		return false;
	}

	// Allocate memory for temporary waveform loading
	unsigned int	dataSize = dataChunkInfo.cksize;
	int				bytesPerSample = pcmWaveFormat.nBlockAlign;
	char			*fileBuffer;
	unsigned int	fileBufferSize;

	// Size limit
	int	dataSamples = dataSize / pcmWaveFormat.nBlockAlign;
	if ( dataSamples > MAXIMUM_SAMPLES ) {
		dataSize = MAXIMUM_SAMPLES * pcmWaveFormat.nBlockAlign;
	}
	if ( st_point > MAXIMUM_SAMPLES ) {
		st_point = MAXIMUM_SAMPLES;
	}
	if ( end_point > MAXIMUM_SAMPLES ) {
		end_point = MAXIMUM_SAMPLES;
	}
	
	
	if (mInstData.loop) {
		fileBufferSize = dataSize+EXPAND_BUFFER*bytesPerSample;
	}
	else {
		fileBufferSize = dataSize;
	}
	fileBuffer = new char[fileBufferSize];
	memset(fileBuffer, 0, fileBufferSize);
	
	// Read waveform data from the file
	size = mmioRead(hmio, (HPSTR)fileBuffer, dataSize);
	if ( size != dataSize ) {
		mmioClose( hmio, 0 );
		return false;
	}
	mmioClose(hmio,0);

	// Expand the loop
	double	inputSampleRate = pcmWaveFormat.nSamplesPerSec;
	double	outputSampleRate = inputSampleRate;
	if (mInstData.loop) {
		unsigned int	plusalpha=0;
		double			framestocopy;
		while (plusalpha < EXPAND_BUFFER) {
			framestocopy = 
			(end_point-st_point)>(EXPAND_BUFFER-plusalpha)?(EXPAND_BUFFER-plusalpha):end_point-st_point;
			memcpy(fileBuffer+((int)end_point+plusalpha)*bytesPerSample,
				   fileBuffer+(int)st_point*bytesPerSample,
				   static_cast<size_t>(framestocopy*bytesPerSample));
			plusalpha += static_cast<unsigned int>(framestocopy);
		}
		dataSize += plusalpha*bytesPerSample;
		
		// Fix to 16-sample boundary
		double	adjustment = ( (long long)((end_point-st_point)/16) ) / ((end_point-st_point)/16.0);
		outputSampleRate *= adjustment;
		st_point *= adjustment;
		end_point *= adjustment;
	}

	// Convert to float mono data first
	int	bytesPerChannel = bytesPerSample / pcmWaveFormat.nChannels;
	unsigned int	inputPtr = 0;
	unsigned int	outputPtr = 0;
	int				monoSamples = dataSize / bytesPerSample;
	float	range = static_cast<float>((1<<(bytesPerChannel*8-1)) * pcmWaveFormat.nChannels);
	float	*monoData = new float[monoSamples];
	while (inputPtr < dataSize) {
		int	frameSum = 0;
		for (int ch=0; ch<pcmWaveFormat.nChannels; ch++) {
			for (int i=0; i<bytesPerChannel; i++) {
				if (i<bytesPerChannel-1) {
					frameSum += (unsigned char)fileBuffer[inputPtr] << (8*i);
				}
				else {
					frameSum += fileBuffer[inputPtr] << (8*i);
				}
				inputPtr++;
			}
		}
		monoData[outputPtr] = frameSum / range;
		outputPtr++;
	}

	// If loop length is not a multiple of 16, perform sample rate conversion
	int	outSamples = monoSamples;
	if ( outputSampleRate == inputSampleRate ) {
		m_pAudioData = new short[monoSamples];
		for (int i=0; i<monoSamples; i++) {
			m_pAudioData[i] = static_cast<short>(monoData[i] * 32768);
		}
	}
	else {
		outSamples = static_cast<int>(monoSamples / (inputSampleRate / outputSampleRate));
		m_pAudioData = new short[outSamples];
		resampling(monoData, monoSamples, inputSampleRate,
				   m_pAudioData, &outSamples, outputSampleRate);
	}

	// Cleanup
	delete [] fileBuffer;
	delete [] monoData;

	// Set instrument data
	mInstData.lp			= static_cast<int>(st_point);
	mInstData.lp_end		= static_cast<int>(end_point);
	mInstData.srcSamplerate	= outputSampleRate;
    mLoadedSamples			= outSamples;

	mIsLoaded = true;

	return true;
#endif
}

//-----------------------------------------------------------------------------
int AudioFile::resampling(const float *src, int srcSamples, double srcRate,
						  short *dst, int *dstSamples, double dstRate)
{
	static const int	window_len = 256;
	static const float	stopband = 0.9999f;
	int					half_window_len = window_len / 2;
	float				srcStride = srcRate / dstRate;
	float				cutoffRate = (1.0/srcStride) < stopband?(1.0/srcStride):stopband;
	int					dstSize = *dstSamples;
	int					actualDstSize = static_cast<int>(srcSamples / srcStride);
	
	if ( actualDstSize < dstSize ) {
		dstSize = actualDstSize;
	}
	for (int i=0; i<dstSize; i++) {
		float	src_pos = i*srcStride;
		float	dstSum = .0f;
		for (int j=-half_window_len; j<half_window_len; j++) {
			int	src_index = static_cast<int>(floorf(src_pos+0.5f)) + j;
			if (src_index >= 0 && src_index < srcSamples) {
				float	x = src_index - src_pos;
				float	window_x = x/half_window_len + 1.0f;
				if (window_x < 0.0f)	window_x = 0.0f;
				if (window_x > 2.0f)	window_x = 2.0f;
				float	window = 0.5f - 0.5f*cosf(mPi*(window_x));
				float	value = src[src_index] * sinc(x*cutoffRate) * window;
				dstSum += value * cutoffRate;
			}
		}
		if (dstSum > 1.0f) {
			dstSum = 1.0f;
		}
		if (dstSum < -1.0f) {
			dstSum = -1.0f;
		}
		dst[i] = static_cast<short>(dstSum * 32767.0f);
	}
	*dstSamples = dstSize;
	return dstSize;
}

//-----------------------------------------------------------------------------
float AudioFile::sinc(float p_x1)
{
	if ( p_x1==.0f ) return 1.0f;
	else {
		float x1 = p_x1*mPi;
		return (sinf(x1)/x1);
	}
}

//-----------------------------------------------------------------------------
short *AudioFile::GetAudioData()
{
	return m_pAudioData;
}

//-----------------------------------------------------------------------------
int AudioFile::GetLoadedSamples()
{
	return mLoadedSamples;
}

//-----------------------------------------------------------------------------
bool AudioFile::GetInstData( InstData *instData )
{
	if ( IsLoaded() ) {
		*instData = mInstData;
		return true;
	}
	return false;
}

﻿//*****************************************************************************
//
//	Copyright 2015 Microsoft Corporation
//
//	Licensed under the Apache License, Version 2.0 (the "License");
//	you may not use this file except in compliance with the License.
//	You may obtain a copy of the License at
//
//	http ://www.apache.org/licenses/LICENSE-2.0
//
//	Unless required by applicable law or agreed to in writing, software
//	distributed under the License is distributed on an "AS IS" BASIS,
//	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//	See the License for the specific language governing permissions and
//	limitations under the License.
//
//*****************************************************************************

#pragma once
#include <queue>
#include <mutex>
#include <pplawait.h>
#include "FFmpegReader.h"
#include "MediaSampleProvider.h"
#include "MediaThumbnailData.h"
#include "VideoFrame.h"
#include "AvEffectDefinition.h"
#include "StreamInfo.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Core;

namespace WFM = Windows::Foundation::Metadata;

extern "C"
{
#include <libavformat/avformat.h>
}

namespace FFmpegInterop
{
	public ref class FFmpegInteropMSS sealed
	{
	public:
		static IAsyncOperation<FFmpegInteropMSS^>^ CreateFromStreamAsync(IRandomAccessStream^ stream, FFmpegInteropConfig^ config);
		static IAsyncOperation<FFmpegInteropMSS^>^ CreateFromStreamAsync(IRandomAccessStream^ stream) { return CreateFromStreamAsync(stream, ref new FFmpegInteropConfig()); }

		static IAsyncOperation<FFmpegInteropMSS^>^ CreateFromUriAsync(String^ uri, FFmpegInteropConfig^ config);
		static IAsyncOperation<FFmpegInteropMSS^>^ CreateFromUriAsync(String^ uri) { return CreateFromUriAsync(uri, ref new FFmpegInteropConfig()); }

		[WFM::Deprecated("Use the CreateFromStreamAsync method.", WFM::DeprecationType::Deprecate, 0x0)]
		static FFmpegInteropMSS^ CreateFFmpegInteropMSSFromStream(IRandomAccessStream^ stream, bool forceAudioDecode, bool forceVideoDecode, PropertySet^ ffmpegOptions, MediaStreamSource^ mss);
		[WFM::Deprecated("Use the CreateFromStreamAsync method.", WFM::DeprecationType::Deprecate, 0x0)]
		static FFmpegInteropMSS^ CreateFFmpegInteropMSSFromStream(IRandomAccessStream^ stream, bool forceAudioDecode, bool forceVideoDecode, PropertySet^ ffmpegOptions);
		[WFM::Deprecated("Use the CreateFromStreamAsync method.", WFM::DeprecationType::Deprecate, 0x0)]
		static FFmpegInteropMSS^ CreateFFmpegInteropMSSFromStream(IRandomAccessStream^ stream, bool forceAudioDecode, bool forceVideoDecode);

		[WFM::Deprecated("Use the CreateFromUriAsync method.", WFM::DeprecationType::Deprecate, 0x0)]
		static FFmpegInteropMSS^ CreateFFmpegInteropMSSFromUri(String^ uri, bool forceAudioDecode, bool forceVideoDecode, PropertySet^ ffmpegOptions);
		[WFM::Deprecated("Use the CreateFromUriAsync method.", WFM::DeprecationType::Deprecate, 0x0)]
		static FFmpegInteropMSS^ CreateFFmpegInteropMSSFromUri(String^ uri, bool forceAudioDecode, bool forceVideoDecode);

		static IAsyncOperation<VideoFrame^>^ ExtractVideoFrameAsync(IRandomAccessStream^ stream, TimeSpan position, bool exactSeek, int maxFrameSkip);
		static IAsyncOperation<VideoFrame^>^ ExtractVideoFrameAsync(IRandomAccessStream^ stream, TimeSpan position, bool exactSeek) { return ExtractVideoFrameAsync(stream, position, exactSeek, 0); };
		static IAsyncOperation<VideoFrame^>^ ExtractVideoFrameAsync(IRandomAccessStream^ stream, TimeSpan position) { return ExtractVideoFrameAsync(stream, position, false, 0); };
		static IAsyncOperation<VideoFrame^>^ ExtractVideoFrameAsync(IRandomAccessStream^ stream) { return ExtractVideoFrameAsync(stream, { 0 }, false, 0); };

		void SetAudioEffects(IVectorView<AvEffectDefinition^>^ audioEffects);
		void SetVideoEffects(IVectorView<AvEffectDefinition^>^ videoEffects);
		void DisableAudioEffects();
		void DisableVideoEffects();
		MediaThumbnailData^ ExtractThumbnail();

		// Contructor
		MediaStreamSource^ GetMediaStreamSource();
		virtual ~FFmpegInteropMSS();

		// Properties

		property TimeSpan Duration
		{
			TimeSpan get()
			{
				return mediaDuration;
			};
		};

		property VideoStreamInfo^ VideoStream
		{
			VideoStreamInfo^ get() { return videoStreamInfo; }
		}

		property IVectorView<AudioStreamInfo^>^ AudioStreams
		{
			IVectorView<AudioStreamInfo^>^ get() { return audioStreamInfos; }
		}

		property IVectorView<SubtitleStreamInfo^>^ SubtitleStreams
		{
			IVectorView<SubtitleStreamInfo^>^ get() { return subtitleStreamInfos; }
		}

		property bool HasThumbnail
		{
			bool get() { return thumbnailStreamIndex; }
		}

		[WFM::Deprecated("Use the AudioStreams property.", WFM::DeprecationType::Deprecate, 0x0)]
		property AudioStreamDescriptor^ AudioDescriptor
		{
			AudioStreamDescriptor^ get()
			{
				return currentAudioStream ? dynamic_cast<AudioStreamDescriptor^>(currentAudioStream->StreamDescriptor) : nullptr;
			};
		};

		[WFM::Deprecated("Use the VideoStream property.", WFM::DeprecationType::Deprecate, 0x0)]
		property VideoStreamDescriptor^ VideoDescriptor
		{
			VideoStreamDescriptor^ get()
			{
				return videoStream ? dynamic_cast<VideoStreamDescriptor^>(videoStream->StreamDescriptor) : nullptr;
			};
		};

		[WFM::Deprecated("Use the VideoStream property.", WFM::DeprecationType::Deprecate, 0x0)]
		property String^ VideoCodecName
		{
			String^ get()
			{
				return videoStream ? videoStream->CodecName : nullptr;
			};
		};

		[WFM::Deprecated("Use the AudioStreams property.", WFM::DeprecationType::Deprecate, 0x0)]
		property String^ AudioCodecName
		{
			String^ get()
			{
				return audioStreamInfos->Size > 0 ? audioStreamInfos->GetAt(0)->CodecName : nullptr;
			};
		};


	private:
		FFmpegInteropMSS(FFmpegInteropConfig^ config);

		static FFmpegInteropMSS^ CreateFromStream(IRandomAccessStream^ stream, FFmpegInteropConfig^ config, MediaStreamSource^ mss);
		static FFmpegInteropMSS^ CreateFromUri(String^ uri, FFmpegInteropConfig^ config);
		HRESULT CreateMediaStreamSource(IRandomAccessStream^ stream, MediaStreamSource^ mss);
		HRESULT CreateMediaStreamSource(String^ uri);
		HRESULT InitFFmpegContext();
		MediaSampleProvider^ CreateAudioStream(AVStream * avStream, int index);
		MediaSampleProvider^ CreateVideoStream(AVStream * avStream, int index);
		MediaSampleProvider^ CreateAudioSampleProvider(AVStream * avStream, AVCodecContext* avCodecCtx, int index);
		MediaSampleProvider^ CreateVideoSampleProvider(AVStream * avStream, AVCodecContext* avCodecCtx, int index);
		HRESULT ParseOptions(PropertySet^ ffmpegOptions);
		void OnStarting(MediaStreamSource ^sender, MediaStreamSourceStartingEventArgs ^args);
		void OnSampleRequested(MediaStreamSource ^sender, MediaStreamSourceSampleRequestedEventArgs ^args);
		void OnSwitchStreamsRequested(MediaStreamSource^ sender, MediaStreamSourceSwitchStreamsRequestedEventArgs^ args);
		HRESULT Seek(TimeSpan position);

		MediaStreamSource^ mss;
		EventRegistrationToken startingRequestedToken;
		EventRegistrationToken sampleRequestedToken;
		EventRegistrationToken switchStreamRequestedToken;

	internal:
		AVDictionary * avDict;
		AVIOContext* avIOCtx;
		AVFormatContext* avFormatCtx;

	private:
		FFmpegInteropConfig ^ config;
		std::vector<MediaSampleProvider^> sampleProviders;
		std::vector<MediaSampleProvider^> audioStreams;
		MediaSampleProvider^ videoStream;
		MediaSampleProvider^ currentAudioStream;
		IVectorView<AvEffectDefinition^>^ currentAudioEffects;
		int thumbnailStreamIndex;

		VideoStreamInfo^ videoStreamInfo;
		IVectorView<AudioStreamInfo^>^ audioStreamInfos;
		IVectorView<SubtitleStreamInfo^>^ subtitleStreamInfos;

		std::recursive_mutex mutexGuard;

		String^ videoCodecName;
		String^ audioCodecName;
		TimeSpan mediaDuration;
		IStream* fileStreamData;
		unsigned char* fileStreamBuffer;
		FFmpegReader^ m_pReader;
		bool isFirstSeek;
	};
}

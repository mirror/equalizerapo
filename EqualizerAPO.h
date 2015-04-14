/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2012  Jonas Thedering

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <Unknwn.h>
#include <audioenginebaseapo.h>
#include <BaseAudioProcessingObject.h>
#include "FilterEngine.h"

class INonDelegatingUnknown
{
	virtual HRESULT __stdcall NonDelegatingQueryInterface(const IID& iid, void** ppv) = 0;
	virtual ULONG __stdcall NonDelegatingAddRef() = 0;
	virtual ULONG __stdcall NonDelegatingRelease() = 0;
};

// {EACD2258-FCAC-4FF4-B36D-419E924A6D79}
const GUID EQUALIZERAPO_PRE_MIX_GUID = { 0xeacd2258, 0xfcac, 0x4ff4, { 0xb3, 0x6d, 0x41, 0x9e, 0x92, 0x4a, 0x6d, 0x79 } };
// {EC1CC9CE-FAED-4822-828A-82A81A6F018F}
const GUID EQUALIZERAPO_POST_MIX_GUID = { 0xec1cc9ce, 0xfaed, 0x4822, { 0x82, 0x8a, 0x82, 0xa8, 0x1a, 0x6f, 0x01, 0x8f } };

class EqualizerAPO
	: public CBaseAudioProcessingObject, public IAudioSystemEffects, public INonDelegatingUnknown
{
public:
	EqualizerAPO(IUnknown* pUnkOuter);
	virtual ~EqualizerAPO();

	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	virtual ULONG __stdcall AddRef();
	virtual ULONG __stdcall Release();

	// IAudioProcessingObject
	virtual HRESULT __stdcall GetLatency(HNSTIME* pTime);
	virtual HRESULT __stdcall Initialize(UINT32 cbDataSize, BYTE* pbyData);
	virtual HRESULT __stdcall IsInputFormatSupported(IAudioMediaType* pOutputFormat,
		IAudioMediaType* pRequestedInputFormat, IAudioMediaType** ppSupportedInputFormat);

	// IAudioProcessingObjectConfiguration
	virtual HRESULT __stdcall LockForProcess(UINT32 u32NumInputConnections,
		APO_CONNECTION_DESCRIPTOR** ppInputConnections, UINT32 u32NumOutputConnections,
		APO_CONNECTION_DESCRIPTOR** ppOutputConnections);
    virtual HRESULT __stdcall UnlockForProcess(void);

	// IAudioProcessingObjectRT
	virtual void __stdcall APOProcess(UINT32 u32NumInputConnections,
		APO_CONNECTION_PROPERTY** ppInputConnections, UINT32 u32NumOutputConnections,
		APO_CONNECTION_PROPERTY** ppOutputConnections);

	// INonDelegatingUnknown
	virtual HRESULT __stdcall NonDelegatingQueryInterface(const IID& iid, void** ppv);
	virtual ULONG __stdcall NonDelegatingAddRef();
	virtual ULONG __stdcall NonDelegatingRelease();

	static long instCount;
	static const CRegAPOProperties<1> regPostMixProperties;
	static const CRegAPOProperties<1> regPreMixProperties;

private:
	long refCount;
	IUnknown* pUnkOuter;
	FilterEngine engine;

	void resetChild();

	IAudioProcessingObject* childAPO;
	IAudioProcessingObjectRT* childRT;
	IAudioProcessingObjectConfiguration* childCfg;
};

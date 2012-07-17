/**

  @file omx_aacdec_component.c

  This component implement AAC decoder.

  Copyright (C) 2007-2008  STMicroelectronics
  Copyright (C) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).
  Copyright (C) 2009-2010 Telechips Inc.

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#include <omxcore.h>
#include <omx_base_audio_port.h>

#include <omx_aacdec_component.h>
#include <tccaudio.h>
#include <OMX_TCC_Index.h>
#include "TCCFile.h"
#include "TCCMemory.h"

#ifdef HAVE_ANDROID_OS
#define USE_EXTERNAL_BUFFER 0
#define LOG_TAG	"OMX_TCC_AACDEC"
#include <utils/Log.h>

//#define DEBUG_ON
#ifdef DEBUG_ON
#define DBUG_MSG(msg...)	LOGD(msg)
#else
#define DBUG_MSG(msg...)
#endif
#endif /* HAVE_ANDROID_OS */

#ifdef HAVE_ANDROID_OS
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE openmaxStandComp, OMX_STRING cCompontName)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;

	err = omx_aacdec_component_Constructor(openmaxStandComp,cCompontName);

	return err;
}
#endif

OMX_ERRORTYPE omx_aacdec_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) 
{

	OMX_ERRORTYPE err = OMX_ErrorNone;  
	omx_aacdec_component_PrivateType* omx_aacdec_component_Private;
	omx_base_audio_PortType *inPort,*outPort;
	OMX_U32 i;

#ifdef HAVE_ANDROID_OS
	if (1)
#else
	if (!openmaxStandComp->pComponentPrivate) 
#endif
	{
		openmaxStandComp->pComponentPrivate = TCC_calloc(1, sizeof(omx_aacdec_component_PrivateType));

		if(openmaxStandComp->pComponentPrivate==NULL)  
		{
			return OMX_ErrorInsufficientResources;
		}
	} 
	else 
	{
	    DBUG_MSG("In %s, Error Component %x Already Allocated\n", 
	              __func__, (int)openmaxStandComp->pComponentPrivate);
	}

	omx_aacdec_component_Private = openmaxStandComp->pComponentPrivate;
	omx_aacdec_component_Private->ports = NULL;

	/** we could create our own port structures here
	* fixme maybe the base class could use a "port factory" function pointer?  
	*/
	err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

	DBUG_MSG("constructor of AAC decoder component is called\n");

	/* Domain specific section for the ports. */  
	/* first we set the parameter common to both formats */
	/* parameters related to input port which does not depend upon input audio format    */
	/* Allocate Ports and call port constructor. */  

	omx_aacdec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
  	omx_aacdec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = 2;

	if (omx_aacdec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_aacdec_component_Private->ports) 
	{
	    omx_aacdec_component_Private->ports = TCC_calloc(omx_aacdec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts, sizeof(omx_base_PortType *));
	    if (!omx_aacdec_component_Private->ports) 
	    {
	  	    return OMX_ErrorInsufficientResources;
	    }
	    for (i=0; i < omx_aacdec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) 
	    {
		      omx_aacdec_component_Private->ports[i] = TCC_calloc(1, sizeof(omx_base_audio_PortType));
		      if (!omx_aacdec_component_Private->ports[i]) 
		      {
		        	return OMX_ErrorInsufficientResources;
		      }
	    }
	}

	base_audio_port_Constructor(openmaxStandComp, &omx_aacdec_component_Private->ports[0], 0, OMX_TRUE); // input
	base_audio_port_Constructor(openmaxStandComp, &omx_aacdec_component_Private->ports[1], 1, OMX_FALSE); // output

	/** parameters related to input port */
	inPort = (omx_base_audio_PortType *) omx_aacdec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	  
	inPort->sPortParam.nBufferSize = DEFAULT_IN_BUFFER_SIZE*2;
	strcpy(inPort->sPortParam.format.audio.cMIMEType, "audio/aac");
	inPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
	inPort->sAudioParam.eEncoding = OMX_AUDIO_CodingAAC;
	
	/** parameters related to output port */
	outPort = (omx_base_audio_PortType *) omx_aacdec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	outPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	outPort->sPortParam.nBufferSize = AUDIO_DEC_OUT_BUFFER_SIZE;
	outPort->sAudioParam.eEncoding = OMX_AUDIO_CodingPCM;

    //Default values for AAC audio param port
	setHeader(&omx_aacdec_component_Private->pAudioAac, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    omx_aacdec_component_Private->pAudioAac.nPortIndex = 0;
    omx_aacdec_component_Private->pAudioAac.nChannels = 2;
    omx_aacdec_component_Private->pAudioAac.nBitRate = 0;
    omx_aacdec_component_Private->pAudioAac.nSampleRate = 44100;
    omx_aacdec_component_Private->pAudioAac.nAudioBandWidth = 0;
    omx_aacdec_component_Private->pAudioAac.nFrameLength = 2048; // use HE_PS frame size as default
    omx_aacdec_component_Private->pAudioAac.eChannelMode = OMX_AUDIO_ChannelModeStereo;
    omx_aacdec_component_Private->pAudioAac.eAACProfile = OMX_AUDIO_AACObjectHE_PS;    //OMX_AUDIO_AACObjectLC;
    omx_aacdec_component_Private->pAudioAac.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;

	/** settings of output port audio format - pcm */
	setHeader(&omx_aacdec_component_Private->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	omx_aacdec_component_Private->pAudioPcmMode.nPortIndex = 1;
	omx_aacdec_component_Private->pAudioPcmMode.nChannels = 2;
	omx_aacdec_component_Private->pAudioPcmMode.eNumData = OMX_NumericalDataSigned;
	omx_aacdec_component_Private->pAudioPcmMode.eEndian = OMX_EndianLittle;
	omx_aacdec_component_Private->pAudioPcmMode.bInterleaved = OMX_TRUE;
	omx_aacdec_component_Private->pAudioPcmMode.nBitPerSample = 16;
	omx_aacdec_component_Private->pAudioPcmMode.nSamplingRate = 44100;
	omx_aacdec_component_Private->pAudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	omx_aacdec_component_Private->pAudioPcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
	omx_aacdec_component_Private->pAudioPcmMode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

	/** now it's time to know the audio coding type of the component */
	if(!strcmp(cComponentName, AUDIO_DEC_AAC_NAME))  
	{   
		 omx_aacdec_component_Private->audio_coding_type = OMX_AUDIO_CodingAAC;
	} 
	else if (!strcmp(cComponentName, AUDIO_DEC_BASE_NAME)) 
	{
		omx_aacdec_component_Private->audio_coding_type = OMX_AUDIO_CodingUnused;
	}
	else  
	{
	    // IL client specified an invalid component name
	    
		LOGE("OMX_ErrorInvalidComponentName %s", cComponentName);
	    return OMX_ErrorInvalidComponentName;
	}


	/** general configuration irrespective of any audio formats */
	/**  setting values of other fields of omx_maddec_component_Private structure */
	
	omx_aacdec_component_Private->BufferMgmtCallback = omx_audiodec_component_BufferMgmtCallback;
	omx_aacdec_component_Private->messageHandler = omx_audiodec_component_MessageHandler;
	omx_aacdec_component_Private->destructor = omx_audiodec_component_Destructor;
	openmaxStandComp->SetParameter = omx_audiodec_component_SetParameter;
	openmaxStandComp->GetParameter = omx_audiodec_component_GetParameter;
	openmaxStandComp->GetExtensionIndex = omx_audiodec_component_GetExtensionIndex;

	omx_aacdec_component_Private->decode_ready = OMX_FALSE;	
	
	memset(&omx_aacdec_component_Private->cdk_core, 0x00, sizeof(cdk_core_t));
	memset(&omx_aacdec_component_Private->cdmx_info, 0x00, sizeof(cdmx_info_t));
	memset(&omx_aacdec_component_Private->cdmx_out, 0x00, sizeof(cdmx_output_t));
	
	omx_aacdec_component_Private->cdk_core.m_iAudioProcessMode = 2; /* decoded pcm mode */

	omx_aacdec_component_Private->cdk_core.m_psCallback = &(omx_aacdec_component_Private->callback_func);
	omx_aacdec_component_Private->cdk_core.m_psCallback->m_pfMalloc   = (void* (*) ( unsigned int ))malloc;
	omx_aacdec_component_Private->cdk_core.m_psCallback->m_pfRealloc  = (void* (*) ( void*, unsigned int ))realloc;
	omx_aacdec_component_Private->cdk_core.m_psCallback->m_pfFree	  = (void  (*) ( void* ))free;
	omx_aacdec_component_Private->cdk_core.m_psCallback->m_pfMemcpy   = (void* (*) ( void*, const void*, unsigned int ))memcpy;
	omx_aacdec_component_Private->cdk_core.m_psCallback->m_pfMemmove  = (void* (*) ( void*, const void*, unsigned int ))memmove;
	omx_aacdec_component_Private->cdk_core.m_psCallback->m_pfMemset   = (void  (*) ( void*, int, unsigned int ))memset;
  
	omx_aacdec_component_Private->iAdecType = AUDIO_ID_AAC;
	omx_aacdec_component_Private->iCtype = CONTAINER_TYPE_RMFF;
	omx_aacdec_component_Private->cb_function = TCC_AAC_DEC;

	if (omx_aacdec_component_Private->pRest == NULL)
	{
		omx_aacdec_component_Private->pRest = (OMX_U8*)malloc(DEFAULT_OUT_BUFFER_SIZE);
	}

	DBUG_MSG("constructor of AAC decoder component is completed ret = %d \n", err);

	return err;

}


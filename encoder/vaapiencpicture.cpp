/*
 *  vaapiencpicture.cpp - va encoder picture
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "vaapiencpicture.h"
#include "vaapicodedbuffer.h"

#include "log.h"
#ifdef __BUILD_GET_MV__
#include <va/va_intel_fei.h>
#endif

namespace YamiMediaCodec{
VaapiEncPicture::VaapiEncPicture(const ContextPtr& context,
                                 const SurfacePtr & surface,
                                 int64_t timeStamp)
:VaapiPicture(context, surface, timeStamp)
{
}

bool VaapiEncPicture::encode()
{
    return render();
}

bool VaapiEncPicture::doRender()
{
    RENDER_OBJECT(m_sequence);
    RENDER_OBJECT(m_packedHeaders);
    RENDER_OBJECT(m_miscParams);
    RENDER_OBJECT(m_picture);
    RENDER_OBJECT(m_qMatrix);
    RENDER_OBJECT(m_huffTable);
    RENDER_OBJECT(m_slices);
#ifdef __BUILD_GET_MV__
    RENDER_OBJECT(m_FEIBuffer);
#endif
    return true;
}

bool VaapiEncPicture::
addPackedHeader(VAEncPackedHeaderType packedHeaderType, const void *header,
                uint32_t headerBitSize)
{
    VAEncPackedHeaderParameterBuffer *packedHeader;
    BufObjectPtr param =
        createBufferObject(VAEncPackedHeaderParameterBufferType,
                           packedHeader);
    BufObjectPtr data =
        createBufferObject(VAEncPackedHeaderDataBufferType,
                           (headerBitSize + 7) / 8, header, NULL);
    bool ret = addObject(m_packedHeaders, param, data);
    if (ret) {
        packedHeader->type = packedHeaderType;
        packedHeader->bit_length = headerBitSize;
        packedHeader->has_emulation_bytes = 0;
    }
    return ret;
}

Encode_Status VaapiEncPicture::getOutput(VideoEncOutputBuffer * outBuffer)
{
    ASSERT(outBuffer);
    uint32_t size = m_codedBuffer->size();
    if (size > outBuffer->bufferSize) {
        outBuffer->dataSize = 0;
        return ENCODE_BUFFER_TOO_SMALL;
    }
    if (size > 0) {
        m_codedBuffer->copyInto(outBuffer->data);
        outBuffer->flag |= m_codedBuffer->getFlags();
    }
    outBuffer->dataSize = size;
    outBuffer->timeStamp = this->m_timeStamp;
    return ENCODE_SUCCESS;
}

#ifdef __BUILD_GET_MV__
bool VaapiEncPicture::editMVBuffer(void*& buffer, uint32_t *size)
{
    VABufferID bufID;
    VAEncMiscParameterFEIFrameControlH264Intel   fei_pic_param;
    VAEncMiscParameterFEIFrameControlH264Intel *misc_fei_pic_control_param;

    if (!m_MVBuffer) {
        m_MVBuffer = createBufferObject(VAEncFEIMVBufferTypeIntel, *size, NULL, (void**)&buffer);
        bufID = m_MVBuffer->getID();
        fei_pic_param.mv_data = bufID;
        fei_pic_param.function = VA_ENC_FUNCTION_ENC_PAK_INTEL;
        fei_pic_param.num_mv_predictors   = 1;

        if (!newMisc(VAEncMiscParameterTypeFEIFrameControlIntel, misc_fei_pic_control_param))
            return false;
        *misc_fei_pic_control_param = fei_pic_param;
    } else {
        buffer = m_MVBuffer->map();
        *size = m_MVBuffer->getSize();
    }
    return true;
}

#endif

}

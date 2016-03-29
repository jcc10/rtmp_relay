//
//  rtmp_relay
//

#include <iostream>
#include <cmath>
#include "RTMP.h"
#include "Utils.h"

namespace rtmp
{
    uint32_t decodeHeader(const std::vector<uint8_t>& data, uint32_t offset, Header& header)
    {
        uint32_t originalOffset = offset;
        
        if (data.size() - offset < 1)
        {
            return 0;
        }
        
        uint8_t headerData = *(data.data() + offset);
        offset += 1;
        
        if ((headerData & 0x3F) != 0x03)
        {
            std::cerr << "Wrong header version" << std::endl;
            return 0;
        }
        
        header.type = static_cast<HeaderType>(headerData >> 6);
        
        if (header.type != HeaderType::ONE_BYTE)
        {
            uint32_t ret = decodeInt(data, offset, 3, header.timestamp);
            
            if (!ret)
            {
                return 0;
            }
            
            offset += ret;
            
            if (header.type != HeaderType::FOUR_BYTE)
            {
                ret = decodeInt(data, offset, 3, header.length);
                
                if (!ret)
                {
                    return 0;
                }
                
                offset += ret;
                
                if (data.size() - offset < 1)
                {
                    return 0;
                }
                
                header.messageType = static_cast<MessageType>(*(data.data() + offset));
                offset += 1;
                
                if (header.type != HeaderType::EIGHT_BYTE)
                {
                    // little endian
                    header.messageStreamId = *reinterpret_cast<const uint32_t*>(data.data() + offset);
                    offset += sizeof(header.messageStreamId);
                }
            }
        }
        
        return offset - originalOffset;
    }
    
    uint32_t decodePacket(const std::vector<uint8_t>& data, uint32_t offset, uint32_t chunkSize, Packet& packet)
    {
        uint32_t originalOffset = offset;
        
        uint32_t remainingBytes = 0;
        
        do
        {
            Header header;
            uint32_t ret = decodeHeader(data, offset, header);
            
            if (!ret)
            {
                return 0;
            }
            
            offset += ret;
            
            if (packet.data.empty())
            {
                packet.header = header;
                remainingBytes = packet.header.length - static_cast<uint32_t>(packet.data.size());
            }
            
            if (offset - data.size() < remainingBytes)
            {
                return 0;
            }
            else
            {
                uint32_t packetSize = (remainingBytes > chunkSize ? chunkSize : remainingBytes);
                
                packet.data.insert(packet.data.end(), data.begin() + offset, data.begin() + offset + packetSize);
                
                remainingBytes -= packetSize;
                offset += packetSize;
            }
            
        }
        while (remainingBytes);
        
        return offset - originalOffset;
    }
    
    uint32_t encodeHeader(std::vector<uint8_t>& data, const Header& header)
    {
        uint32_t originalSize = static_cast<uint32_t>(data.size());
        
        uint8_t headerData = 0x03;
        headerData &= (static_cast<uint32_t>(header.type) << 6);
        
        if (header.type != HeaderType::ONE_BYTE)
        {
            uint32_t ret = encodeInt(data, 3, header.timestamp);
            
            if (!ret)
            {
                return 0;
            }
            
            if (header.type != HeaderType::FOUR_BYTE)
            {
                ret = encodeInt(data, 3, header.length);
                
                if (!ret)
                {
                    return 0;
                }
                
                data.insert(data.end(), static_cast<uint8_t>(header.messageType));
                
                if (header.type != HeaderType::EIGHT_BYTE)
                {
                    // little endian
                    const uint8_t* messageStreamId = reinterpret_cast<const uint8_t*>(&header.messageStreamId);
                    data.insert(data.end(), messageStreamId, messageStreamId + sizeof(uint32_t));
                }
            }
        }
        
        return static_cast<uint32_t>(data.size()) - originalSize;
    }
    
    uint32_t encodePacket(std::vector<uint8_t>& data, uint32_t chunkSize, const Packet& packet)
    {
        uint32_t originalSize = static_cast<uint32_t>(data.size());
        
        const uint32_t packetCount = ((static_cast<uint32_t>(packet.data.size()) + chunkSize - 1) / chunkSize);
        
        data.reserve(12 + packet.data.size() + packetCount);

        for (uint32_t i = 0; i < packetCount; ++i)
        {
            if (i == 0)
            {
                encodeHeader(data, packet.header);
            }
            else
            {
                Header oneByteHeader;
                oneByteHeader.type = HeaderType::ONE_BYTE;
                encodeHeader(data, oneByteHeader);
            }

            uint32_t start = i * chunkSize;
            uint32_t end = start + chunkSize;
            
            if (end > packet.data.size()) end = static_cast<uint32_t>(packet.data.size());
            
            data.insert(data.end(), packet.data.begin() + start, packet.data.begin() + end);
        }
        
        return static_cast<uint32_t>(data.size()) - originalSize;
    }
}

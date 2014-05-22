/*
    netLink: c++ 11 networking library
    Copyright 2014 Alexander Meißner (lichtso@gamefortec.net)

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the use of this software.
    Permission is granted to anyone to use this software for any purpose, 
    including commercial applications, and to alter it and redistribute it freely, 
    subject to the following restrictions:
    
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "Header.h"

namespace MsgPack {

    //! MsgPack::Header with a additional buffer used as body
    class Data : public Header {
        friend Serializer;
        friend Deserializer;
        protected:
        std::unique_ptr<uint8_t[]> data;
        std::streamsize serialize(int64_t& pos, std::basic_streambuf<uint8_t>* streamBuffer, std::streamsize bytes);
        std::streamsize deserialize(int64_t& pos, std::basic_streambuf<uint8_t>* streamBuffer, std::streamsize bytes);
    };

    //! MsgPack::Data to represent binary/raw data elements
    class Binary : public Data {
        friend Serializer;
        friend Deserializer;
        protected:
        Binary() { }
        int64_t getEndPos() const;
        int64_t getHeaderLength() const;
        public:
        Binary(uint32_t len, const uint8_t* data);
        void stringify(std::ostream& stream) const;
        //! Returns a pointer to the binary data
        uint8_t* getData() const;
    };
    
    //! MsgPack::Data to represent extended elements
    class Extended : public Data {
        friend Serializer;
        friend Deserializer;
        protected:
        Extended() { }
        int64_t getEndPos() const;
        int64_t getHeaderLength() const;
        public:
        Extended(uint8_t type, uint32_t len, const uint8_t* data);
        void stringify(std::ostream& stream) const;
        //! Returns the user defined data type
        uint8_t getDataType() const;
        //! Returns a pointer to the binary data
        uint8_t* getData() const;
        uint32_t getLength() const;
    };

    //! MsgPack::Data to represent strings
    class String : public Data {
        friend Serializer;
        friend Deserializer;
        protected:
        String() { }
        int64_t getEndPos() const;
        int64_t getHeaderLength() const;
        public:
        String(const std::string& str);
        void stringify(std::ostream& stream) const;
        //! Returns a std::string represenation of the content
        std::string getStr() const;
    };

};
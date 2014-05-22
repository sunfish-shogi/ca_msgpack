//
//  ca_msgpack.cpp
//  msgpack_test
//
//  Created by Kubo Ryosuke on 2014/05/13.
//
//

#include "ca_msgpack.h"

namespace ca_msgpack {

	void MapDeserializer::parseObject(Element& header, const char* dir) {
		uint32_t len = static_cast<MsgPack::Header*>(header.get())->getLength();
		for (uint32_t i = 0; i < len; i++) {
			Element key;
			_deserializer.deserialize(key, false);
			if (!key) { break; }
			if (!isString(key->getType())) {
				std::ostringstream oss;
				oss << "key is not string: " << dir;
				throw ca_msgpack::MsgPackError(oss.str());
			}
			std::ostringstream keyStr;
			keyStr << dir << (*key);
			value(keyStr.str().c_str());
		}
	}

	void MapDeserializer::parseArray(Element& header, const char* dir, Member& member) {
		uint32_t len = static_cast<MsgPack::Header*>(header.get())->getLength();
		(this->*member.initFunc)(member.obj);
		for (uint32_t i = 0; i < len; i++) {
			std::ostringstream key;
			key << dir << i;
			(this->*member.addFunc)(key.str().c_str(), NULL, member.obj);
			value(key.str().c_str());
		}
	}

	void MapDeserializer::parseMap(Element& header, const char* dir, Member& member) {
		uint32_t len = static_cast<MsgPack::Header*>(header.get())->getLength();
		(this->*member.initFunc)(member.obj);
		for (uint32_t i = 0; i < len; i++) {
			Element key;
			_deserializer.deserialize(key, false);
			if (!key) { break; }
			if (!isString(key->getType())) {
				std::ostringstream oss;
				oss << "key is not string: " << dir;
				throw ca_msgpack::MsgPackError(oss.str());
			}
			std::ostringstream keyStr;
			keyStr << dir << (*key);
			(this->*member.addFunc)(keyStr.str().c_str(), static_cast<MsgPack::String*>(key.get())->getStr().c_str(), member.obj);
			value(keyStr.str().c_str());
		}
	}

	template <class T>
	void MapDeserializer::parseNumber(Element& element, Member& member, const char* key) {
		T& obj = *(T*)member.obj;
		MsgPack::Type type = element->getType();
		if (type >= 0xe0) {
			type = MsgPack::Type::INT_8;
		}
		if (type < 0x80) {
			type = MsgPack::Type::UINT_8;
		}
		switch(type) {
#define __CAMP_PARSE_NUMBER__(type) (T)static_cast<MsgPack::Number*>(element.get())->getValue<type>()
			case MsgPack::Type::INT_8: obj = __CAMP_PARSE_NUMBER__(int8_t); break;
			case MsgPack::Type::INT_16: obj = __CAMP_PARSE_NUMBER__(int16_t); break;
			case MsgPack::Type::INT_32: obj = __CAMP_PARSE_NUMBER__(int32_t); break;
			case MsgPack::Type::INT_64: obj = __CAMP_PARSE_NUMBER__(int64_t); break;
			case MsgPack::Type::UINT_8: obj = __CAMP_PARSE_NUMBER__(uint8_t); break;
			case MsgPack::Type::UINT_16: obj = __CAMP_PARSE_NUMBER__(uint16_t); break;
			case MsgPack::Type::UINT_32: obj = __CAMP_PARSE_NUMBER__(uint32_t); break;
			case MsgPack::Type::UINT_64: obj = __CAMP_PARSE_NUMBER__(uint64_t); break;
			case MsgPack::Type::FLOAT_32: obj = __CAMP_PARSE_NUMBER__(float); break;
			case MsgPack::Type::FLOAT_64: obj = __CAMP_PARSE_NUMBER__(double); break;
			default:
				std::ostringstream oss;
				oss << "unsupported type: 0x" << std::hex << element->getType() << ": " << key;
				throw ca_msgpack::MsgPackError(oss.str());
#undef __CAMP_PARSE_NUMBER__
		}
	}

	void MapDeserializer::parseValue(Element& element, Member& member, const char* key) {
		if (member.type == Type::Integer32) {
			parseNumber<int32_t>(element, member, key);
		} else if (member.type == Type::Integer32U) {
			parseNumber<uint32_t>(element, member, key);
		} else if (member.type == Type::Integer64) {
			parseNumber<int64_t>(element, member, key);
		} else if (member.type == Type::Integer64U) {
			parseNumber<uint64_t>(element, member, key);
		} else if (member.type == Type::Float) {
			parseNumber<float>(element, member, key);
		} else if (member.type == Type::Double) {
			parseNumber<double>(element, member, key);
		} else if (member.type == Type::Boolean) {
			bool& obj = *(bool*)member.obj;
			obj = static_cast<MsgPack::Primitive*>(element.get())->getValue();
		} else if (member.type == Type::String) {
			std::string& obj = *(std::string*)member.obj;
			obj = static_cast<MsgPack::String*>(element.get())->getStr();
		}
	}

	void MapDeserializer::value(const char* key, void* obj) {
		Element element;
		_deserializer.deserialize(element, false);
		if (!element) {
			std::ostringstream oss;
			oss << "data has no value: " << key;
			throw ca_msgpack::MsgPackError(oss.str());
		}
		auto ite = _members.find(key);
		if (ite != _members.end()) {
			Member& member = ite->second;
			if (member.type == Type::Object) {
				std::ostringstream dir;
				dir << key << '/';
				parseObject(element, dir.str().c_str());
			} else if (member.type == Type::Array || member.type == Type::Map) {
				std::ostringstream dir;
				dir << key << '/';
				if (member.type == Type::Array) {
					parseArray(element, dir.str().c_str(), member);
				} else {
					parseMap(element, dir.str().c_str(), member);
				}
			} else {
				parseValue(element, member, key);
			}
		} else {
			std::ostringstream oss;
			oss << "unkown property: " << key;
			throw ca_msgpack::MsgPackError(oss.str());
		}
	}

}
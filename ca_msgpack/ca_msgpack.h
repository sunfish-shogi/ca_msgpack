//
//  ca_msgpack.h
//  msgpack_test
//
//  Created by Kubo Ryosuke on 2014/05/09.
//  Copyright (c) 2014 Kubo Ryosuke. All rights reserved.
//

#ifndef msgpack_test_ca_msgpack_h
#define msgpack_test_ca_msgpack_h

#include "MsgPack.h"
#include <iostream>
#include <vector>
#include <map>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/variadic/size.hpp>

namespace ca_msgpack {

	typedef std::unique_ptr<MsgPack::Element> Element;

	class MsgPackError {
	};

	class MapSerializer {
	private:
		MsgPack::Serializer& _serializer;
		void serializeValue(const std::string& value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::String(value));
			_serializer << _value;
		}
		void serializeValue(int32_t value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Number((int64_t)value));
			_serializer << _value;
		}
		void serializeValue(uint32_t value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Number((uint64_t)value));
			_serializer << _value;
		}
		void serializeValue(int64_t value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Number(value));
			_serializer << _value;
		}
		void serializeValue(uint64_t value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Number(value));
			_serializer << _value;
		}
		void serializeValue(float value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Number(value));
			_serializer << _value;
		}
		void serializeValue(double value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Number(value));
			_serializer << _value;
		}
		void serializeValue(bool value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::Primitive(value));
			_serializer << _value;
		}
		template <class T>
		void serializeValue(const std::vector<T>& value) {
			std::unique_ptr<MsgPack::Element> _header(new MsgPack::ArrayHeader((uint32_t)value.size()));
			_serializer << _header;
			for (auto ite = value.begin(); ite != value.end(); ite++) {
				serializeValue((T)*ite);
			}
		}
		template <class T>
		void serializeValue(const std::map<std::string, T>& value) {
			std::unique_ptr<MsgPack::Element> _header(new MsgPack::MapHeader((uint32_t)value.size()));
			_serializer << _header;
			for (auto ite = value.begin(); ite != value.end(); ite++) {
				serializeValue((*ite).first);
				serializeValue((*ite).second);
			}
		}
		template <class T>
		void serializeValue(const T& value) {
			value.pack(_serializer);
		}
	public:
		MapSerializer(MsgPack::Serializer& serializer, int count) : _serializer(serializer) {
			std::unique_ptr<MsgPack::Element> _header(new MsgPack::MapHeader(count));
			_serializer << _header;
		}
		template<class T>
		void serialize(const char* key, T value) {
			std::unique_ptr<MsgPack::Element> _key(new MsgPack::String(key));
			_serializer << _key;
			serializeValue(value);
		}
	};

	class MapDeserializer {
	private:
		typedef void (MapDeserializer::*ADD_MEMBER_FUNC)(const char*, const char*, void*);
		enum class Type {
			Integer32, Integer32U, Integer64, Integer64U,
			Float, Double, Boolean, String, Object, Map, Array
		};
		struct Member {
			Type type;
			void* obj;
			ADD_MEMBER_FUNC func;
			Member() {}
			Member(Type type, void* obj, ADD_MEMBER_FUNC func = NULL) : type(type), obj(obj), func(func) {}
		};
		MsgPack::Deserializer& _deserializer;
		std::map<std::string, Member> _members;
		bool isString(uint32_t type) {
			return (type & 0xf0) == MsgPack::Type::FIXSTR || type == MsgPack::Type::STR_8
				|| type == MsgPack::Type::STR_16 || type == MsgPack::Type::STR_32;
		}
		bool isMap(uint32_t type) {
			return (type & 0xf0) == MsgPack::Type::FIXMAP
				|| type == MsgPack::Type::MAP_16 || type == MsgPack::Type::MAP_32;
		}
		bool isArray(uint32_t type) {
			return (type & 0xf0) == MsgPack::Type::FIXARRAY
				|| type == MsgPack::Type::ARRAY_16 || type == MsgPack::Type::ARRAY_32;
		}
		void parseObject(Element& header, const char* dir = "") {
			uint32_t len = static_cast<MsgPack::Header*>(header.get())->getLength();
			for (uint32_t i = 0; i < len; i++) {
				Element key;
				_deserializer.deserialize(key, false);
				if (!key) { break; }
				if (!isString(key->getType())) {
					throw ca_msgpack::MsgPackError();
				}
				std::ostringstream keyStr;
				keyStr << dir << (*key);
				value(keyStr.str().c_str());
			}
		}
		void parseArray(Element& header, const char* dir, Member& member) {
			uint32_t len = static_cast<MsgPack::Header*>(header.get())->getLength();
			for (uint32_t i = 0; i < len; i++) {
				std::ostringstream key;
				key << dir << i;
				(this->*member.func)(key.str().c_str(), NULL, member.obj);
				value(key.str().c_str());
			}
		}
		void parseMap(Element& header, const char* dir, Member& member) {
			uint32_t len = static_cast<MsgPack::Header*>(header.get())->getLength();
			for (uint32_t i = 0; i < len; i++) {
				Element key;
				_deserializer.deserialize(key, false);
				if (!key) { break; }
				if (!isString(key->getType())) {
					throw ca_msgpack::MsgPackError();
				}
				std::ostringstream keyStr;
				keyStr << dir << (*key);
				(this->*member.func)(keyStr.str().c_str(), static_cast<MsgPack::String*>(key.get())->getStr().c_str(), member.obj);
				value(keyStr.str().c_str());
			}
		}
		void parseValue(Element& element, Member& member) {
			if (member.type == Type::Integer32) {
				int32_t& obj = *(int32_t*)member.obj;
				obj = static_cast<MsgPack::Number*>(element.get())->getValue<int32_t>();
			} else if (member.type == Type::Integer32U) {
				uint32_t& obj = *(uint32_t*)member.obj;
				obj = static_cast<MsgPack::Number*>(element.get())->getValue<uint32_t>();
			} else if (member.type == Type::Integer64) {
				int64_t& obj = *(int64_t*)member.obj;
				obj = static_cast<MsgPack::Number*>(element.get())->getValue<int64_t>();
			} else if (member.type == Type::Integer64U) {
				uint64_t& obj = *(uint64_t*)member.obj;
				obj = static_cast<MsgPack::Number*>(element.get())->getValue<uint64_t>();
			} else if (member.type == Type::Float) {
				float& obj = *(float*)member.obj;
				obj = static_cast<MsgPack::Number*>(element.get())->getValue<float>();
			} else if (member.type == Type::Double) {
				double& obj = *(double*)member.obj;
				obj = static_cast<MsgPack::Number*>(element.get())->getValue<double>();
			} else if (member.type == Type::Boolean) {
				bool& obj = *(bool*)member.obj;
				obj = static_cast<MsgPack::Primitive*>(element.get())->getValue();
			} else if (member.type == Type::String) {
				std::string& obj = *(std::string*)member.obj;
				obj = static_cast<MsgPack::String*>(element.get())->getStr();
			}
		}
		void value(const char* key, void* obj = NULL) {
			Element element;
			_deserializer.deserialize(element, false);
			if (!element) { throw ca_msgpack::MsgPackError(); }
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
					parseValue(element, member);
				}
			} else {
				throw ca_msgpack::MsgPackError();
			}
		}
		template <class T>
		void addArray(const char* key, const char* shortKey, void* p) {
			std::vector<T>& array = *(std::vector<T>*)p;
			size_t index = array.size();
			array.push_back(std::move(T()));
			addMember(key, &array[index]);
		}
		template <class T>
		void addMap(const char* key, const char* shortKey, void* p) {
			std::map<std::string, T>& map = *(std::map<std::string, T>*)p;
			map[shortKey] = std::move(T());
			addMember(key, &map[shortKey]);
		}
	public:
		MapDeserializer(MsgPack::Deserializer& deserializer) : _deserializer(deserializer) {
		}
		void addMember(const char* key, int32_t* obj) {
			_members[key] = Member(Type::Integer32, obj);
		}
		void addMember(const char* key, uint32_t* obj) {
			_members[key] = Member(Type::Integer32U, obj);
		}
		void addMember(const char* key, int64_t* obj) {
			_members[key] = Member(Type::Integer64, obj);
		}
		void addMember(const char* key, uint64_t* obj) {
			_members[key] = Member(Type::Integer64U, obj);
		}
		void addMember(const char* key, float* obj) {
			_members[key] = Member(Type::Float, obj);
		}
		void addMember(const char* key, bool* obj) {
			_members[key] = Member(Type::Boolean, obj);
		}
		void addMember(const char* key, std::string* obj) {
			_members[key] = Member(Type::String, obj);
		}
		template <class T>
		void addMember(const char* key, std::vector<T>* obj) {
			_members[key] = Member(Type::Array, obj, &MapDeserializer::addArray<T>);
		}
		template <class T>
		void addMember(const char* key, std::map<std::string, T>* obj) {
			_members[key] = Member(Type::Map, obj, &MapDeserializer::addMap<T>);
		}
		template <class T>
		void addMember(const char* key, T* obj) {
			std::ostringstream dir;
			dir << key << '/';
			obj->addMembers(*this, dir.str().c_str());
			_members[key] = Member(Type::Object, obj);
		}
		void execute() {
			Element header;
			_deserializer.deserialize(header, false);
			if (!header) { throw ca_msgpack::MsgPackError(); }
			parseObject(header);
		}
		void execute(Element& header) {
			parseObject(header);
		}
	};
}

#define CA_MSGPACK(...)									\
void pack(std::streambuf* sb) const { \
	MsgPack::Serializer s(sb); \
	pack(s); \
} \
void pack(MsgPack::Serializer& serializer) const { \
	ca_msgpack::MapSerializer mapSerializer(serializer, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)); \
	BOOST_PP_CAT(BOOST_PP_CAT(MSGPACK_SER_, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)) (__VA_ARGS__),) \
}\
void unpack(std::streambuf* sb) { \
	MsgPack::Deserializer d(sb); \
	unpack(d); \
} \
void unpack(MsgPack::Deserializer& deserializer) { \
	ca_msgpack::MapDeserializer mapDeserializer(deserializer); \
	addMembers(mapDeserializer); \
	mapDeserializer.execute(); \
} \
void addMembers(ca_msgpack::MapDeserializer& mapDeserializer, const char* key = "") { \
	BOOST_PP_CAT(BOOST_PP_CAT(MSGPACK_DES_, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)) (__VA_ARGS__),) \
} \
static void addMemberArray(ca_msgpack::Element&, const char*, void*) { \
} \
static void addMemberMap(ca_msgpack::Element&, const char*, void*) { \
} \

#define MSGPACK_SER(x)									mapSerializer.serialize(#x, x);
#define MSGPACK_SER_1(x)								MSGPACK_SER(x)
#define MSGPACK_SER_2(x, ...)						MSGPACK_SER(x) MSGPACK_SER_1 (__VA_ARGS__)
#define MSGPACK_SER_3(x, ...)						MSGPACK_SER(x) MSGPACK_SER_2 (__VA_ARGS__)
#define MSGPACK_SER_4(x, ...)						MSGPACK_SER(x) MSGPACK_SER_3 (__VA_ARGS__)
#define MSGPACK_SER_5(x, ...)						MSGPACK_SER(x) MSGPACK_SER_4 (__VA_ARGS__)
#define MSGPACK_SER_6(x, ...)						MSGPACK_SER(x) MSGPACK_SER_5 (__VA_ARGS__)
#define MSGPACK_SER_7(x, ...)						MSGPACK_SER(x) MSGPACK_SER_6 (__VA_ARGS__)
#define MSGPACK_SER_8(x, ...)						MSGPACK_SER(x) MSGPACK_SER_7 (__VA_ARGS__)
#define MSGPACK_SER_9(x, ...)						MSGPACK_SER(x) MSGPACK_SER_8 (__VA_ARGS__)
#define MSGPACK_SER_10(x, ...)					MSGPACK_SER(x) MSGPACK_SER_9 (__VA_ARGS__)
#define MSGPACK_SER_11(x, ...)					MSGPACK_SER(x) MSGPACK_SER_10(__VA_ARGS__)
#define MSGPACK_SER_12(x, ...)					MSGPACK_SER(x) MSGPACK_SER_11(__VA_ARGS__)
#define MSGPACK_SER_13(x, ...)					MSGPACK_SER(x) MSGPACK_SER_12(__VA_ARGS__)
#define MSGPACK_SER_14(x, ...)					MSGPACK_SER(x) MSGPACK_SER_13(__VA_ARGS__)
#define MSGPACK_SER_15(x, ...)					MSGPACK_SER(x) MSGPACK_SER_14(__VA_ARGS__)
#define MSGPACK_SER_16(x, ...)					MSGPACK_SER(x) MSGPACK_SER_15(__VA_ARGS__)
#define MSGPACK_SER_17(x, ...)					MSGPACK_SER(x) MSGPACK_SER_16(__VA_ARGS__)
#define MSGPACK_SER_18(x, ...)					MSGPACK_SER(x) MSGPACK_SER_17(__VA_ARGS__)
#define MSGPACK_SER_19(x, ...)					MSGPACK_SER(x) MSGPACK_SER_18(__VA_ARGS__)
#define MSGPACK_SER_20(x, ...)					MSGPACK_SER(x) MSGPACK_SER_19(__VA_ARGS__)
#define MSGPACK_SER_21(x, ...)					MSGPACK_SER(x) MSGPACK_SER_20(__VA_ARGS__)
#define MSGPACK_SER_22(x, ...)					MSGPACK_SER(x) MSGPACK_SER_21(__VA_ARGS__)
#define MSGPACK_SER_23(x, ...)					MSGPACK_SER(x) MSGPACK_SER_22(__VA_ARGS__)
#define MSGPACK_SER_24(x, ...)					MSGPACK_SER(x) MSGPACK_SER_23(__VA_ARGS__)
#define MSGPACK_SER_25(x, ...)					MSGPACK_SER(x) MSGPACK_SER_24(__VA_ARGS__)
#define MSGPACK_SER_26(x, ...)					MSGPACK_SER(x) MSGPACK_SER_25(__VA_ARGS__)
#define MSGPACK_SER_27(x, ...)					MSGPACK_SER(x) MSGPACK_SER_26(__VA_ARGS__)
#define MSGPACK_SER_28(x, ...)					MSGPACK_SER(x) MSGPACK_SER_27(__VA_ARGS__)
#define MSGPACK_SER_29(x, ...)					MSGPACK_SER(x) MSGPACK_SER_28(__VA_ARGS__)
#define MSGPACK_SER_30(x, ...)					MSGPACK_SER(x) MSGPACK_SER_29(__VA_ARGS__)
#define MSGPACK_SER_31(x, ...)					MSGPACK_SER(x) MSGPACK_SER_30(__VA_ARGS__)
#define MSGPACK_SER_32(x, ...)					MSGPACK_SER(x) MSGPACK_SER_31(__VA_ARGS__)
#define MSGPACK_SER_33(x, ...)					MSGPACK_SER(x) MSGPACK_SER_32(__VA_ARGS__)
#define MSGPACK_SER_34(x, ...)					MSGPACK_SER(x) MSGPACK_SER_33(__VA_ARGS__)
#define MSGPACK_SER_35(x, ...)					MSGPACK_SER(x) MSGPACK_SER_34(__VA_ARGS__)
#define MSGPACK_SER_36(x, ...)					MSGPACK_SER(x) MSGPACK_SER_35(__VA_ARGS__)
#define MSGPACK_SER_37(x, ...)					MSGPACK_SER(x) MSGPACK_SER_36(__VA_ARGS__)
#define MSGPACK_SER_38(x, ...)					MSGPACK_SER(x) MSGPACK_SER_37(__VA_ARGS__)
#define MSGPACK_SER_39(x, ...)					MSGPACK_SER(x) MSGPACK_SER_38(__VA_ARGS__)
#define MSGPACK_SER_40(x, ...)					MSGPACK_SER(x) MSGPACK_SER_39(__VA_ARGS__)
#define MSGPACK_SER_41(x, ...)					MSGPACK_SER(x) MSGPACK_SER_40(__VA_ARGS__)
#define MSGPACK_SER_42(x, ...)					MSGPACK_SER(x) MSGPACK_SER_41(__VA_ARGS__)
#define MSGPACK_SER_43(x, ...)					MSGPACK_SER(x) MSGPACK_SER_42(__VA_ARGS__)
#define MSGPACK_SER_44(x, ...)					MSGPACK_SER(x) MSGPACK_SER_43(__VA_ARGS__)
#define MSGPACK_SER_45(x, ...)					MSGPACK_SER(x) MSGPACK_SER_44(__VA_ARGS__)
#define MSGPACK_SER_46(x, ...)					MSGPACK_SER(x) MSGPACK_SER_45(__VA_ARGS__)
#define MSGPACK_SER_47(x, ...)					MSGPACK_SER(x) MSGPACK_SER_46(__VA_ARGS__)
#define MSGPACK_SER_48(x, ...)					MSGPACK_SER(x) MSGPACK_SER_47(__VA_ARGS__)
#define MSGPACK_SER_49(x, ...)					MSGPACK_SER(x) MSGPACK_SER_48(__VA_ARGS__)
#define MSGPACK_SER_50(x, ...)					MSGPACK_SER(x) MSGPACK_SER_49(__VA_ARGS__)

#define MSGPACK_DES(x)									mapDeserializer.addMember((std::string(key) + '"' + #x + '"').c_str(), &x);
#define MSGPACK_DES_1(x)								MSGPACK_DES(x)
#define MSGPACK_DES_2(x, ...)						MSGPACK_DES(x) MSGPACK_DES_1 (__VA_ARGS__)
#define MSGPACK_DES_3(x, ...)						MSGPACK_DES(x) MSGPACK_DES_2 (__VA_ARGS__)
#define MSGPACK_DES_4(x, ...)						MSGPACK_DES(x) MSGPACK_DES_3 (__VA_ARGS__)
#define MSGPACK_DES_5(x, ...)						MSGPACK_DES(x) MSGPACK_DES_4 (__VA_ARGS__)
#define MSGPACK_DES_6(x, ...)						MSGPACK_DES(x) MSGPACK_DES_5 (__VA_ARGS__)
#define MSGPACK_DES_7(x, ...)						MSGPACK_DES(x) MSGPACK_DES_6 (__VA_ARGS__)
#define MSGPACK_DES_8(x, ...)						MSGPACK_DES(x) MSGPACK_DES_7 (__VA_ARGS__)
#define MSGPACK_DES_9(x, ...)						MSGPACK_DES(x) MSGPACK_DES_8 (__VA_ARGS__)
#define MSGPACK_DES_10(x, ...)					MSGPACK_DES(x) MSGPACK_DES_9 (__VA_ARGS__)
#define MSGPACK_DES_11(x, ...)					MSGPACK_DES(x) MSGPACK_DES_10(__VA_ARGS__)
#define MSGPACK_DES_12(x, ...)					MSGPACK_DES(x) MSGPACK_DES_11(__VA_ARGS__)
#define MSGPACK_DES_13(x, ...)					MSGPACK_DES(x) MSGPACK_DES_12(__VA_ARGS__)
#define MSGPACK_DES_14(x, ...)					MSGPACK_DES(x) MSGPACK_DES_13(__VA_ARGS__)
#define MSGPACK_DES_15(x, ...)					MSGPACK_DES(x) MSGPACK_DES_14(__VA_ARGS__)
#define MSGPACK_DES_16(x, ...)					MSGPACK_DES(x) MSGPACK_DES_15(__VA_ARGS__)
#define MSGPACK_DES_17(x, ...)					MSGPACK_DES(x) MSGPACK_DES_16(__VA_ARGS__)
#define MSGPACK_DES_18(x, ...)					MSGPACK_DES(x) MSGPACK_DES_17(__VA_ARGS__)
#define MSGPACK_DES_19(x, ...)					MSGPACK_DES(x) MSGPACK_DES_18(__VA_ARGS__)
#define MSGPACK_DES_20(x, ...)					MSGPACK_DES(x) MSGPACK_DES_19(__VA_ARGS__)
#define MSGPACK_DES_21(x, ...)					MSGPACK_DES(x) MSGPACK_DES_20(__VA_ARGS__)
#define MSGPACK_DES_22(x, ...)					MSGPACK_DES(x) MSGPACK_DES_21(__VA_ARGS__)
#define MSGPACK_DES_23(x, ...)					MSGPACK_DES(x) MSGPACK_DES_22(__VA_ARGS__)
#define MSGPACK_DES_24(x, ...)					MSGPACK_DES(x) MSGPACK_DES_23(__VA_ARGS__)
#define MSGPACK_DES_25(x, ...)					MSGPACK_DES(x) MSGPACK_DES_24(__VA_ARGS__)
#define MSGPACK_DES_26(x, ...)					MSGPACK_DES(x) MSGPACK_DES_25(__VA_ARGS__)
#define MSGPACK_DES_27(x, ...)					MSGPACK_DES(x) MSGPACK_DES_26(__VA_ARGS__)
#define MSGPACK_DES_28(x, ...)					MSGPACK_DES(x) MSGPACK_DES_27(__VA_ARGS__)
#define MSGPACK_DES_29(x, ...)					MSGPACK_DES(x) MSGPACK_DES_28(__VA_ARGS__)
#define MSGPACK_DES_30(x, ...)					MSGPACK_DES(x) MSGPACK_DES_29(__VA_ARGS__)
#define MSGPACK_DES_31(x, ...)					MSGPACK_DES(x) MSGPACK_DES_30(__VA_ARGS__)
#define MSGPACK_DES_32(x, ...)					MSGPACK_DES(x) MSGPACK_DES_31(__VA_ARGS__)
#define MSGPACK_DES_33(x, ...)					MSGPACK_DES(x) MSGPACK_DES_32(__VA_ARGS__)
#define MSGPACK_DES_34(x, ...)					MSGPACK_DES(x) MSGPACK_DES_33(__VA_ARGS__)
#define MSGPACK_DES_35(x, ...)					MSGPACK_DES(x) MSGPACK_DES_34(__VA_ARGS__)
#define MSGPACK_DES_36(x, ...)					MSGPACK_DES(x) MSGPACK_DES_35(__VA_ARGS__)
#define MSGPACK_DES_37(x, ...)					MSGPACK_DES(x) MSGPACK_DES_36(__VA_ARGS__)
#define MSGPACK_DES_38(x, ...)					MSGPACK_DES(x) MSGPACK_DES_37(__VA_ARGS__)
#define MSGPACK_DES_39(x, ...)					MSGPACK_DES(x) MSGPACK_DES_38(__VA_ARGS__)
#define MSGPACK_DES_40(x, ...)					MSGPACK_DES(x) MSGPACK_DES_39(__VA_ARGS__)
#define MSGPACK_DES_41(x, ...)					MSGPACK_DES(x) MSGPACK_DES_40(__VA_ARGS__)
#define MSGPACK_DES_42(x, ...)					MSGPACK_DES(x) MSGPACK_DES_41(__VA_ARGS__)
#define MSGPACK_DES_43(x, ...)					MSGPACK_DES(x) MSGPACK_DES_42(__VA_ARGS__)
#define MSGPACK_DES_44(x, ...)					MSGPACK_DES(x) MSGPACK_DES_43(__VA_ARGS__)
#define MSGPACK_DES_45(x, ...)					MSGPACK_DES(x) MSGPACK_DES_44(__VA_ARGS__)
#define MSGPACK_DES_46(x, ...)					MSGPACK_DES(x) MSGPACK_DES_45(__VA_ARGS__)
#define MSGPACK_DES_47(x, ...)					MSGPACK_DES(x) MSGPACK_DES_46(__VA_ARGS__)
#define MSGPACK_DES_48(x, ...)					MSGPACK_DES(x) MSGPACK_DES_47(__VA_ARGS__)
#define MSGPACK_DES_49(x, ...)					MSGPACK_DES(x) MSGPACK_DES_48(__VA_ARGS__)
#define MSGPACK_DES_50(x, ...)					MSGPACK_DES(x) MSGPACK_DES_49(__VA_ARGS__)

#endif

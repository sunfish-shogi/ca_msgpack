//
//  ca_msgpack.h
//  msgpack_test
//
//  Created by Kubo Ryosuke on 2014/05/09.
//  Copyright (c) 2014 Kubo Ryosuke. All rights reserved.
//

#ifndef msgpack_test_ca_msgpack_h
#define msgpack_test_ca_msgpack_h

#define BOOST_PP_VARIADICS 1

#include "MsgPack.h"
#include <iostream>
#include <vector>
#include <map>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/variadic/size.hpp>

namespace ca_msgpack {

	typedef std::unique_ptr<MsgPack::Element> Element;

	/**
	 * pack/unpack 中に継続不能なエラーが発生した時に投げられる例外クラス
	 */
	class MsgPackError : public std::exception {
	private:
		std::shared_ptr<std::string> _message;
	public:
		MsgPackError(const MsgPackError& src) : _message(src._message) {
		}
		MsgPackError(const char* message) : _message(std::make_shared<std::string>(message)) {
		}
		MsgPackError(const std::shared_ptr<std::string>& message) : _message(message) {
		}
		MsgPackError(const std::string& message) : _message(std::make_shared<std::string>(message)) {
		}
		const std::string& getMessage() const _NOEXCEPT {
			return *_message.get();
		}
		virtual const char* what() const _NOEXCEPT override {
			return _message.get()->c_str();
		}
	};

	/**
	 * シリアライズを行うクラス
	 */
	class Serializer {
	private:
		MsgPack::Serializer& _serializer;

		// 文字列
		void serializeValue(const std::string& value) {
			std::unique_ptr<MsgPack::Element> _value(new MsgPack::String(value));
			_serializer << _value;
		}
		// プリミティブ型
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
		// 配列
		template <class T>
		void serializeValue(const std::vector<T>& value) {
			std::unique_ptr<MsgPack::Element> _header(new MsgPack::ArrayHeader((uint32_t)value.size()));
			_serializer << _header;
			for (auto ite = value.begin(); ite != value.end(); ite++) {
				serializeValue((T)*ite);
			}
		}
		// 連想配列
		template <class T>
		void serializeValue(const std::map<std::string, T>& value) {
			std::unique_ptr<MsgPack::Element> _header(new MsgPack::MapHeader((uint32_t)value.size()));
			_serializer << _header;
			for (auto ite = value.begin(); ite != value.end(); ite++) {
				serializeValue((*ite).first);
				serializeValue((*ite).second);
			}
		}
		// 任意のオブジェクト
		template <class T>
		void serializeValue(const T& value) {
			value.pack(_serializer);
		}

	public:
		Serializer(MsgPack::Serializer& serializer, int count) : _serializer(serializer) {
			std::unique_ptr<MsgPack::Element> _header(new MsgPack::MapHeader(count));
			_serializer << _header;
		}

		/** シリアライゼーションの実行 */
		template<class T>
		void serialize(const char* key, const T& value) {
			std::unique_ptr<MsgPack::Element> _key(new MsgPack::String(key));
			_serializer << _key;
			serializeValue(value);
		}
	};

	/**
	 * デシリアライズを行うクラス
	 */
	class Deserializer {
	private:
		enum class Type {
			Integer32, Integer32U, Integer64, Integer64U,
			Float, Double, Boolean, String, Object, Map, Array
		};

		typedef void (Deserializer::*ADD_MEMBER_FUNC)(const char*, const char*, void*);

		struct Member {
			Type type; // メンバ変数の型
			void* obj; // メンバ変数へのポインタ
			ADD_MEMBER_FUNC addFunc; // 連想配列、もしくは配列への要素追加を行う関数へのポインタ
			Member() {}
			Member(Type type, void* obj, ADD_MEMBER_FUNC addFunc = NULL)
				: type(type), obj(obj), addFunc(addFunc) {}
		};

		MsgPack::Deserializer& _deserializer;
		std::map<std::string, Member> _members;
		std::map<std::string, bool*> _updatedFlags;

		bool isNilOrUndef(uint32_t type) {
			return type == MsgPack::Type::NIL || type == MsgPack::Type::UNDEFINED;
		}
		bool isString(uint32_t type) {
			return (type >= MsgPack::Type::FIXSTR && type < MsgPack::Type::NIL)
				|| type == MsgPack::Type::STR_8
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

		void parseObject(Element& header, const char* dir = "");
		void parseArray(Element& header, const char* dir, Member& member);
		void parseMap(Element& header, const char* dir, Member& member);
		template <class T>
		void parseNumber(Element& element, Member& member, const char* key);
		void parseValue(Element& element, Member& member, const char* key);
		void ignoreValue(Element& element, const char* key);
		void value(const char* key, void* obj = NULL);

		/** 配列に要素を追加 */
		template <class T>
		void addArray(const char* key, const char* shortKey, void* p) {
			std::vector<T>& array = *(std::vector<T>*)p;
			size_t index = array.size();
			array.push_back(std::move(T()));
			addMember(key, &array[index]);
		}

		/** 連想配列に要素を追加 */
		template <class T>
		void addMap(const char* key, const char* shortKey, void* p) {
			std::map<std::string, T>& map = *(std::map<std::string, T>*)p;
			map[shortKey] = std::move(T());
			addMember(key, &map[shortKey]);
		}

	public:
		/** コンストラクタ */
		Deserializer(MsgPack::Deserializer& deserializer) : _deserializer(deserializer) {
		}

		// 更新フラグを登録します。
		void addUpdatedFlag(const char* key, bool* flag) {
			_updatedFlags[key] = flag;
		}

		// execute を呼ぶ前に addMember を使って 展開先の構造体のメンバ変数の情報を登録します。
		void addMember(const char* key, int32_t* obj) {
			_members[key] = Member(Type::Integer32, obj);
			*obj = 0;
		}
		void addMember(const char* key, uint32_t* obj) {
			_members[key] = Member(Type::Integer32U, obj);
			*obj = 0l;
		}
		void addMember(const char* key, int64_t* obj) {
			_members[key] = Member(Type::Integer64, obj);
			*obj = 0ll;
		}
		void addMember(const char* key, uint64_t* obj) {
			_members[key] = Member(Type::Integer64U, obj);
			*obj = 0ull;
		}
		void addMember(const char* key, float* obj) {
			_members[key] = Member(Type::Float, obj);
			*obj = 0.0f;
		}
		void addMember(const char* key, double* obj) {
			_members[key] = Member(Type::Double, obj);
			*obj = 0.0;
		}
		void addMember(const char* key, bool* obj) {
			_members[key] = Member(Type::Boolean, obj);
			*obj = false;
		}
		void addMember(const char* key, std::string* obj) {
			_members[key] = Member(Type::String, obj);
			*obj = "";
		}
		template <class T>
		void addMember(const char* key, std::vector<T>* obj) {
			_members[key] = Member(Type::Array, obj, &Deserializer::addArray<T>);
			obj->clear();
		}
		template <class T>
		void addMember(const char* key, std::map<std::string, T>* obj) {
			_members[key] = Member(Type::Map, obj, &Deserializer::addMap<T>);
			obj->clear();
		}
		template <class T>
		void addMember(const char* key, T* obj) {
			std::ostringstream dir;
			dir << key << '/';
			obj->addMembers(*this, dir.str().c_str());
			_members[key] = Member(Type::Object, obj);
		}

		/** デシリアライゼーションの実行 */
		void execute() {
			Element header;
			_deserializer.deserialize(header, false);
			if (!header) {
				std::ostringstream oss;
				oss << "data has no elements.";
				throw ca_msgpack::MsgPackError(oss.str());
			}
			parseObject(header);
		}

	};

}

#define CA_MSGPACK_0()									__CA_MSGPACK__(0)
#define CA_MSGPACK(...)									__CA_MSGPACK__(__VA_ARGS__, 0)

#define __CA_MSGPACK__(...)									\
private: \
	bool ___ca_msgpack_updated___; \
public: \
	bool isUpdated() const { \
		return ___ca_msgpack_updated___; \
	} \
	void pack(std::streambuf* sb) const { \
		MsgPack::Serializer s(sb); \
		pack(s); \
	} \
	void pack(MsgPack::Serializer& s) const { \
		ca_msgpack::Serializer serializer(s, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__) - 1); \
		BOOST_PP_CAT(BOOST_PP_CAT(MSGPACK_SER_, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)) (__VA_ARGS__),) \
	}\
	void unpack(std::streambuf* sb) { \
		MsgPack::Deserializer d(sb); \
		unpack(d); \
	} \
	void unpack(MsgPack::Deserializer& d) { \
		ca_msgpack::Deserializer deserializer(d); \
		addMembers(deserializer); \
		deserializer.execute(); \
	} \
	void addMembers(ca_msgpack::Deserializer& deserializer, const char* key = "") { \
		___ca_msgpack_updated___ = false; \
		deserializer.addUpdatedFlag(key, &___ca_msgpack_updated___); \
		BOOST_PP_CAT(BOOST_PP_CAT(MSGPACK_DES_, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)) (__VA_ARGS__),) \
	} \
	static void addMemberArray(ca_msgpack::Element&, const char*, void*) { \
	} \
	static void addMemberMap(ca_msgpack::Element&, const char*, void*) { \
	} \

#define MSGPACK_SER(x)									serializer.serialize(#x, x);
#define MSGPACK_SER_1(x)
#define MSGPACK_SER_2(x, ...)						MSGPACK_SER(x)
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

#define MSGPACK_DES(x)									deserializer.addMember((std::string(key) + '"' + #x + '"').c_str(), &x);
#define MSGPACK_DES_1(x)
#define MSGPACK_DES_2(x, ...)						MSGPACK_DES(x)
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

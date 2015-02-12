/*
 * main.cpp
 * msgpack_test
 *
 * Created by Kubo Ryosuke on 2014/05/09.
 * Copyright (c) 2014 Kubo Ryosuke. All rights reserved.
 */

#include "ca_msgpack.h"

// CA_MSGPACKマクロでメンバを指定する。
struct ObjectB {
	// constructors
	ObjectB() {}
	ObjectB(int integer, const std::string& string)
	: integer(integer)
	, string(string) {}
	// members
	int integer;
	std::string string;
	// msgpack decralation
	CA_MSGPACK(integer, string);
};

struct ObjectA {
	ObjectA() {}
	ObjectA(int integer,
			bool boolean,
			const std::string& string,
			const ObjectB& objectB,
			const std::vector<std::string>& stringArray,
			const std::map<std::string, std::string>& stringMap,
			const std::vector<ObjectB>& objectBArray,
			const std::map<std::string, ObjectB>& objectBMap)
	: integer(integer)
	, boolean(boolean)
	, string(string)
	, objectB(objectB)
	, stringArray(stringArray)
	, stringMap(stringMap)
	, objectBArray(objectBArray)
	, objectBMap(objectBMap)
	{
	}
	// members
	int integer;
	bool boolean;
	std::string string;
	ObjectB objectB;
	std::vector<std::string> stringArray;
	std::map<std::string, std::string> stringMap;
	std::vector<ObjectB> objectBArray;
	std::map<std::string, ObjectB> objectBMap;
	// msgpack decralation
	CA_MSGPACK(integer, boolean, string, objectB, stringArray, stringMap, objectBArray, objectBMap);
};

int main(int argc, const char * argv[]) {
	ObjectA in = {
		12,
		true,
		"foo",
		{ 52, "bar" },
		std::vector<std::string>{ "baz1", "baz2", "baz3" },
		std::map<std::string, std::string>{ { "key1", "value1" }, { "key2", "value2" } },
		std::vector<ObjectB>{ { 111, "xxx" }, { 222, "yyy" } },
		std::map<std::string, ObjectB>{ { "111", { 111, "xxx" } }, { "222", { 222, "yyy" } } }
	};
	ObjectA out;
	std::stringbuf buf;

	{
		// 構造体から pack
		std::cout << "pack" << std::endl;
		in.pack(&buf);
	}

	{
		// pack したデータを json フォーマットで出力してみる。
		std::stringbuf tmpBuf(buf.str());
		MsgPack::Deserializer deserializer(&tmpBuf);
		ca_msgpack::Element element;
		deserializer.deserialize(element, true);
		std::cout << *element << std::endl;
	}

	{
		// 構造体へ unpack
		std::cout << "unpack" << std::endl;
		out.unpack(&buf);
	}

	{
		// もう一度 pack して json フォーマットで出力してみる。
		std::stringbuf checkBuf;
		MsgPack::Serializer serializer(&checkBuf);
		MsgPack::Deserializer deserializer(&checkBuf);
		out.pack(serializer);
		ca_msgpack::Element element;
		deserializer.deserialize(element, true);
		std::cout << *element << std::endl;
	}

	return 0;
}


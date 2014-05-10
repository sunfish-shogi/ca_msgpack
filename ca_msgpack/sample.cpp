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
	int integer;
	std::string string;
	CA_MSGPACK(integer, string);
};

struct ObjectA {
	int integer;
	bool boolean;
	std::string string;
	ObjectB objectB;
	std::vector<std::string> stringArray;
	std::map<std::string, std::string> stringMap;
	std::vector<ObjectB> objectBArray;
	std::map<std::string, ObjectB> objectBMap;
	CA_MSGPACK(integer, boolean, string, objectB, stringArray, stringMap, objectBArray, objectBMap);
};

int main(int argc, const char * argv[]) {
	std::stringbuf inBuf;

	{
		// 構造体から pack
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

		std::cout << "pack" << std::endl;
		in.pack(&inBuf);
	}

	const std::string& packed = inBuf.str();

	{
		// テスト
		std::stringbuf outBuf(packed);
		MsgPack::Deserializer deserializer(&outBuf);
		ca_msgpack::Element element;
		deserializer.deserialize(element, true);
		std::cout << *element << std::endl;
	}

	{
		// 構造体へ unpack
		std::cout << "unpack" << std::endl;
		ObjectA out;
		std::stringbuf outBuf(packed);
		out.unpack(&outBuf);

		// テスト
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


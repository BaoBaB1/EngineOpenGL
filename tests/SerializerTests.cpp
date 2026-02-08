#include "gtest/gtest.h"
#include "core/Macros.hpp"
#include <vector>
#include <filesystem>

using namespace fury;

namespace
{
	struct SimpleClass
	{
		FURY_REGISTER_CLASS(SimpleClass)
			FURY_DECLARE_SERIALIZABLE_FIELDS(
				FURY_SERIALIZABLE_FIELD(1, &SimpleClass::a),
				FURY_SERIALIZABLE_FIELD(2, &SimpleClass::b),
				FURY_SERIALIZABLE_FIELD(3, &SimpleClass::name),
				FURY_SERIALIZABLE_FIELD(4, &SimpleClass::vec)
			)
		int a;
		float b;
		std::string name;
		std::vector<int> vec;
		bool operator==(const SimpleClass& other) {
			return a == other.a && b == other.b && name == other.name && vec == other.vec;
		}
	};

	struct Base
	{
		FURY_REGISTER_CLASS(Base)
		FURY_DECLARE_SERIALIZABLE_FIELDS(
			FURY_SERIALIZABLE_FIELD(1, &Base::base_field)
		)
		float base_field;
	};

	struct Derived : Base
	{
		FURY_REGISTER_DERIVED_CLASS(Derived, Base)
		FURY_DECLARE_SERIALIZABLE_FIELDS(
			FURY_SERIALIZABLE_FIELD(1, &Derived::derived_field)
		)
		int derived_field;
	};

	static constexpr std::string_view test_file("test.bin");

	void create_test_file_and_open_stream(std::ofstream& ofs)
	{
		ofs.open(test_file, std::ios_base::binary | std::ios_base::out);
		if (!ofs.is_open()) {
			throw std::exception("Failed to open test file");
		}
	}
}

TEST(SerializationTest, DefaultSerialization)
{
	SimpleClass obj;
	obj.a = 10;
	obj.b = 5.5;
	obj.name = "Simple class";
	obj.vec = { 1, 2, 3, 4 };
	std::ofstream ofs;
	::create_test_file_and_open_stream(ofs);
	uint64_t bytes_written = Serializer<SimpleClass>::write(ofs, &obj);
	ofs.close();

	SimpleClass obj2;
	std::ifstream ifs;
	ifs.open(test_file, std::ios_base::binary);
	uint64_t bytes_read = Serializer<SimpleClass>::read(ifs, &obj2);
	ifs.close();
	std::filesystem::remove(test_file);
	
	EXPECT_TRUE(bytes_read == bytes_written);
	EXPECT_TRUE(obj == obj2);
}

TEST(SerializationTest, SerializationWithFieldSkip)
{
	SimpleClass obj;
	obj.a = 10;
	obj.b = 5.5;
	obj.name = "Simple class";
	obj.vec = { 1, 2, 3, 4 };
	std::ofstream ofs;
	::create_test_file_and_open_stream(ofs);
	uint64_t bytes_written = Serializer<SimpleClass>::write(ofs, &obj);
	ofs.close();

	// DIRTY HACKS COMING
	// replace unique tags with already existing to trick the serializer
	// to think that we only have 2 unique tags left from original 4,
	// thus mimicing behavior when we changed class member fields or don't want
	// to serialize some fields that we serialized before
	const auto& tup = SimpleClass::get_serializable_fields();
	// keep only tag 1 and 4
	const_cast<uint16_t&>(std::get<0>(std::get<1>(tup))) = 1;
	const_cast<uint16_t&>(std::get<0>(std::get<2>(tup))) = 1;

	SimpleClass obj2;
	std::ifstream ifs;
	ifs.open(test_file, std::ios_base::binary);
	uint64_t bytes_read = Serializer<SimpleClass>::read(ifs, &obj2);
	ifs.close();

	EXPECT_TRUE(bytes_read == bytes_written);
	EXPECT_TRUE(obj2.a == 10);
	EXPECT_TRUE(obj2.vec == std::vector<int>({1, 2, 3, 4}));
	std::filesystem::remove(test_file);
}

TEST(SerializationTest, DerivedClassSerialization)
{
	Derived d;
	d.derived_field = 100;
	d.base_field = 12.12;
	std::ofstream ofs;
	::create_test_file_and_open_stream(ofs);
	uint64_t bytes_written = Serializer<Derived>::write(ofs, &d);
	ofs.close();

	Derived d2;
	std::ifstream ifs;
	ifs.open(test_file, std::ios_base::binary);
	uint64_t bytes_read = Serializer<Derived>::read(ifs, &d2);
	ifs.close();
	
	EXPECT_TRUE(bytes_read == bytes_written);
	// we read both derived and base part
	EXPECT_TRUE(d2.derived_field == 100);
	EXPECT_FLOAT_EQ(d2.base_field, 12.12);
	std::filesystem::remove(test_file);
}

TEST(SerializationTest, SerializationViaBasePointer)
{
	Base* d = new Derived;
	d->base_field = 12.12;
	static_cast<Derived*>(d)->derived_field = 100;
	std::ofstream ofs;
	::create_test_file_and_open_stream(ofs);
	// dynamic dispatch. resolved to Serializer<Derived>::write
	uint64_t bytes_written = d->write(ofs);
	ofs.close();

	
	Base* d2 = new Derived;
	std::ifstream ifs;
	ifs.open(test_file, std::ios_base::binary);
	// dynamic dispatch. resolved to Serializer<Derived>::read
	uint64_t bytes_read = d2->read(ifs);
	ifs.close();

	EXPECT_TRUE(bytes_read == bytes_written);
	// we read both derived and base part
	EXPECT_TRUE(d2->base_field == d->base_field);
	EXPECT_TRUE(static_cast<Derived*>(d)->derived_field == static_cast<Derived*>(d)->derived_field);
	
	std::filesystem::remove(test_file);
	delete d;
	delete d2;
}

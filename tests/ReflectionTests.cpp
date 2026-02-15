#include "gtest/gtest.h"
#include "core/Macros.hpp"

namespace
{
	struct Base
	{
		FURY_REGISTER_CLASS(Base)	
	};

	struct Derived : Base
	{
		FURY_REGISTER_DERIVED_CLASS(Derived, Base)
	};
}

TEST(RttiTest, TestBaseDerivedClassRttiStatic)
{
	Base b;
	Derived d;
	EXPECT_TRUE(b.get_dynamic_type_id() == Base::get_static_type_id());
	EXPECT_TRUE(d.get_dynamic_type_id() == Derived::get_static_type_id());
	EXPECT_TRUE(d.get_dynamic_type_id() != b.get_dynamic_type_id());
	EXPECT_TRUE(typeid(Derived::Base) == typeid(Base));

	EXPECT_TRUE(b.get_static_type_id() == Base::get_static_type_id());
	EXPECT_TRUE(d.get_static_type_id() == Derived::get_static_type_id());
}

TEST(RttiTest, TestBaseDerivedClassRttiDynamic)
{
	Base* b = new Derived;
	EXPECT_TRUE(b->get_dynamic_type_id() == Derived::get_static_type_id());
	delete b;
}

TEST(RttiTest, TestRttiNames)
{
	Base b;
	Derived d;
	EXPECT_TRUE(Base::cls_name == "Base");
	EXPECT_TRUE(Derived::cls_name == "Derived");
}

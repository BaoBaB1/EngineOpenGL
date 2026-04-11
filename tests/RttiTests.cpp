#include "gtest/gtest.h"
#include "core/Macros.hpp"
#include <memory>

namespace
{
	struct Base
	{
		FURY_REGISTER_BASE_CLASS(Base)	
	};

	struct Derived : Base
	{
		FURY_REGISTER_DERIVED_CLASS(Derived, Base)
	};

	struct Derived2 : Derived
	{
		FURY_REGISTER_DERIVED_CLASS(Derived2, Derived)
	};

	struct BaseFromBase : Base
	{
		FURY_REGISTER_DERIVED_CLASS(BaseFromBase, Base)
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

TEST(RttiTest, TestIfClassIsInSameInheritanceChain)
{
	std::unique_ptr<Base> b = std::make_unique<Derived2>();
	std::unique_ptr<Base> bfb = std::make_unique<BaseFromBase>();

	// within same chain from base pointer
	EXPECT_TRUE((b->is_a(Derived2::get_static_type_id()) && b->is_a(Derived::get_static_type_id())));
	EXPECT_TRUE(bfb->is_a(BaseFromBase::get_static_type_id()));
	
	EXPECT_TRUE(b->is_a(Base::get_static_type_id()));
	EXPECT_TRUE(bfb->is_a(Base::get_static_type_id()));

	// different chains
	EXPECT_FALSE((b->is_a(BaseFromBase::get_static_type_id()) || bfb->is_a(Derived::get_static_type_id())));
}

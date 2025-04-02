#include "gtest/gtest.h"
#include "ge/Ray.hpp"
#include "ge/BoundingBox.hpp"

using namespace fury;

TEST(RayTest, TestGetters)
{
	Ray r(glm::vec3(), glm::vec3(0, 0, -1));
	EXPECT_TRUE(r.get_origin() == glm::vec3() && r.get_direction() == glm::vec3(0, 0, -1));
}

TEST(RayTest, RayPlaneIntersection)
{
	// ray intersects plane
	{
		Ray r(glm::vec3(), glm::vec3(0, 0, 1));
		const glm::vec3 plane_origin = glm::vec3(0, 0, 5);
		const glm::vec3 plane_normal = glm::vec3(0, 0, -1);
		auto hit = r.intersect_plane(plane_origin, plane_normal);
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == plane_origin);
		EXPECT_TRUE(hit->distance == 5.f);
		EXPECT_TRUE(hit->normal == plane_normal);
	}
	// ray doesn't intersect plane
	{
		Ray r(glm::vec3(), glm::vec3(0, 1, 0));
		const glm::vec3 plane_origin = glm::vec3(0, 0, 5);
		const glm::vec3 plane_normal = glm::vec3(0, 0, -1);
		auto hit = r.intersect_plane(plane_origin, plane_normal);
		EXPECT_FALSE(hit);
	}
}

TEST(RayTest, RayBoundingBoxIntersection)
{
	// ray lies inside bbox
	{
		BoundingBox bbox(glm::vec3(-1.f), glm::vec3(1.f));
		Ray r(glm::vec3(), glm::vec3(0, 0, 1));
		auto hit = r.intersect_aabb(bbox);
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == glm::vec3(0, 0, 1));
		EXPECT_TRUE(hit->distance == 1.f);
		EXPECT_TRUE(hit->normal == glm::vec3());
	}
	// ray doesn't intersect bbox
	{
		BoundingBox bbox(glm::vec3(-1.f), glm::vec3(1.f));
		Ray r(glm::vec3(5, 0, 0), glm::vec3(0, 0, 1));
		auto hit = r.intersect_aabb(bbox);
		EXPECT_FALSE(hit);
	}
	// ray intersects bbox
	{
		BoundingBox bbox(glm::vec3(-1.f), glm::vec3(1.f));
		Ray r(glm::vec3(5, 0, 0), glm::vec3(-1, 0, 0));
		auto hit = r.intersect_aabb(bbox);
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == glm::vec3(1, 0, 0));
		EXPECT_TRUE(hit->distance == 4.f);
		EXPECT_TRUE(hit->normal == glm::vec3());
	}
	// ray starts behind bbox
	{
		BoundingBox bbox(glm::vec3(-1.f), glm::vec3(1.f));
		Ray r(glm::vec3(0, 0, 5), glm::vec3(0, 0, 1));
		auto hit = r.intersect_aabb(bbox);
		EXPECT_FALSE(hit);
	}
}

TEST(RayTest, RaySphereIntersection)
{
	// ray starts inside sphere
	{
		Ray r(glm::vec3(), glm::vec3(0, 0, 1));
		auto hit = r.intersect_sphere(glm::vec3(0), 1.f);
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == glm::vec3(0, 0, 1));
		EXPECT_TRUE(hit->distance == 1.f);
		EXPECT_TRUE(hit->normal == glm::vec3(0, 0, 1));
	}
	// ray touches sphere's edge
	{
		Ray r(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
		auto hit = r.intersect_sphere(glm::vec3(0), 1.f);
		// if ray touches sphere, it returns false... maybe make it return true ?
		EXPECT_FALSE(hit);
	}
	// ray intersects sphere twice
	{
		Ray r(glm::vec3(0, 0, -2), glm::vec3(0, 0, 1));
		auto hit = r.intersect_sphere(glm::vec3(0), 1.f);
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == glm::vec3(0, 0, -1));
		EXPECT_TRUE(hit->distance == 1.f);
		EXPECT_TRUE(hit->normal == glm::vec3(0, 0, -1));
	}
}

TEST(RayTest, RayTriangleIntersection)
{
	// ray starts inside triangle
	{
		Ray r(glm::vec3(0.f, 0.5f, 0.f), glm::vec3(0, 0, 1));
		// CCW
		auto hit = r.intersect_triangle({ 0, 1, 0 }, { -1, 0, 0 }, { 1, 0, 0 });
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == glm::vec3(0, 0.5, 0));
		EXPECT_TRUE(hit->distance == 0.f);
		EXPECT_TRUE(hit->normal == glm::vec3(0, 0, 1));
	}
	// ray doesn't intersect triangle
	{
		Ray r(glm::vec3(1.1f, 0.f, 0.f), glm::vec3(0, 0, 1));
		auto hit = r.intersect_triangle({ 0, 1, 0 }, { -1, 0, 0 }, { 1, 0, 0 });
		EXPECT_FALSE(hit);
	}
	// ray intersects triangle
	{
		Ray r(glm::vec3(1.f, 0.f, -1.f), glm::vec3(0, 0, 1));
		auto hit = r.intersect_triangle({ 0, 1, 0 }, { -1, 0, 0 }, { 1, 0, 0 });
		EXPECT_TRUE(hit);
		EXPECT_TRUE(hit->position == glm::vec3(1.0, 0.0, 0.0));
		EXPECT_TRUE(hit->distance == 1.f);
		EXPECT_TRUE(hit->normal == glm::vec3(0, 0, 1));
	}
}

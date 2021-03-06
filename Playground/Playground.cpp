// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------
#include "stdafx.h"

#include <iostream>
#include <memory>
#include <thread>

#include <array>

#include <Core/Core.h>
#include <IO/IO.h>
#include <Visualization/Visualization.h>

#pragma optimize("", off)
const std::vector<std::string> filenames = { "D:/cube1.ply",
										"D:/cube2.ply"
};



const std::vector<std::string> icp_sample_filenames = { 
	"D:/Projects/Open3D/examples/TestData/ICP/cloud_bin_0.pcd",
	"D:/Projects/Open3D/examples/TestData/ICP/cloud_bin_1.pcd",
	"D:/Projects/Open3D/examples/TestData/ICP/cloud_bin_2.pcd"
};

typedef std::vector<std::shared_ptr<open3d::PointCloud>> PCptrvector;


bool LoadPointClouds(const std::vector<std::string>& filenames, PCptrvector& pointClouds)
{
	for (auto name : filenames)
	{
		auto cloud_ptr = std::make_shared<open3d::PointCloud>();
		if (open3d::ReadPointCloud(name, *cloud_ptr)) {
			//	PrintWarning("Successfully read %s\n", filename);
			pointClouds.push_back(cloud_ptr);
		}
		else {
			open3d::PrintError("Failed to read %s\n\n", name);
		}
	}

	return pointClouds.size() == filenames.size();
}


std::tuple<std::shared_ptr<open3d::PointCloud>, std::shared_ptr<open3d::Feature>>
LoadAndPreprocessPointCloud(const char* file_name)
{
	//this is for ransac
	auto pcd = open3d::CreatePointCloudFromFile(file_name);
	auto pcd_down = VoxelDownSample(*pcd, 0.05);
	EstimateNormals(*pcd_down, open3d::KDTreeSearchParamHybrid(0.1, 30));
	auto pcd_fpfh = ComputeFPFHFeature(
		*pcd_down, open3d::KDTreeSearchParamHybrid(0.25, 100));
	return std::make_tuple(pcd_down, pcd_fpfh);
}

void DrawRegistrationResults(std::shared_ptr<open3d::PointCloud> source, std::shared_ptr<open3d::PointCloud> target, const Eigen::Matrix4d &transformation)
{
	auto tmpSource = std::make_shared<open3d::PointCloud>(*source);
	auto tmpTarget = std::make_shared<open3d::PointCloud>(*target);
	tmpSource->PaintUniformColor({ 1, 0.706, 0 });
	tmpTarget->PaintUniformColor({ 0, 0.651, 0.929 });

	tmpSource->Transform(transformation);
	open3d::DrawGeometries({ tmpSource, tmpTarget }, "PointCloud", 1600, 1200);
}

void PrintEval(open3d::RegistrationResult& ret)
{
	open3d::PrintInfo("rmse : %ld\n", ret.inlier_rmse_);
	open3d::PrintInfo("fitness : %ld\n", ret.fitness_);
}

int main(int argc, char *argv[])
{
	using namespace open3d;

	SetVerbosityLevel(VerbosityLevel::VerboseAlways);

	PCptrvector pointClouds;
	LoadPointClouds(icp_sample_filenames, pointClouds);

	Eigen::Matrix4d initialT;
	initialT = Eigen::Matrix4d::Identity();
	initialT << 0.862, 0.011, -0.507, 0.5,
		-0.139, 0.967, -0.215, 0.7,
		0.487, 0.255, 0.835, -1.4,
		0.0, 0.0, 0.0, 1.0;


	PrintWarning("Initial alignment");
	const double threshold = 0.02;

	Eigen::Matrix4d dummy;
	dummy << 0.0, 0.0, 1.0, 0.0,
				1.0, 0.0, 0.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 0.0, 1.0;
	pointClouds[0]->Transform(dummy);

	RegistrationResult evaluation = open3d::EvaluateRegistration(*pointClouds[0], *pointClouds[1], threshold, Eigen::Matrix4d::Identity());
	PrintEval(evaluation);
	DrawRegistrationResults(pointClouds[0], pointClouds[1], initialT);

	PrintWarning("Apply point-to-point ICP");
	ICPConvergenceCriteria criteria;
	criteria.max_iteration_ = 2000;
	RegistrationResult newEvaluation = open3d::RegistrationICP(*pointClouds[0], *pointClouds[1], threshold, initialT, TransformationEstimationPointToPoint(), criteria);
	PrintEval(newEvaluation);

	DrawRegistrationResults(pointClouds[0], pointClouds[1], newEvaluation.transformation_);

	PrintInfo("End of the test.\n");

	return 1;
}

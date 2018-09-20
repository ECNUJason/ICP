
/******************************************************
ICP�㷨��cpuʵ�֣�ʵ����ԭʼ��icp�㷨
���в����Լ�����OpenMPʵ��
********************************************************/
#include "icp.h"

int main()
{
	// The point clouds we will be using
	PointCloudT::Ptr cloud_in(new PointCloudT);  // Original point cloud
	PointCloudT::Ptr cloud_tr(new PointCloudT);  // Transformed point cloud
	PointCloudT::Ptr cloud_icp(new PointCloudT);  // ICP output point cloud
	cloud_in = ReadFile("../data/bunny.ply");

	// �����˲�����
	pcl::VoxelGrid<pcl::PointXYZ> filter;
	filter.setInputCloud(cloud_in);
	// ��������դ��Ĵ�СΪ 1x1x1c
	filter.setLeafSize(0.01f, 0.01f, 0.01f);
	filter.filter(*cloud_in);
	cout << "���ƴ�С:" << cloud_in->size() << endl;
	pcl::io::savePLYFileASCII("bunny1.ply", *cloud_in);

	// Defining a rotation matrix and translation vector
	Eigen::Matrix4d transformation_matrix = Eigen::Matrix4d::Identity();
	Eigen::Matrix4d matrix_icp = Eigen::Matrix4d::Identity();

	// A rotation matrix (see https://en.wikipedia.org/wiki/Rotation_matrix)
	double theta = M_PI / 8;  // The angle of rotation in radians
	transformation_matrix(0, 0) = cos(theta);
	transformation_matrix(0, 1) = -sin(theta);
	transformation_matrix(1, 0) = sin(theta);
	transformation_matrix(1, 1) = cos(theta);

	// A translation on Z axis (0.4 meters)
	transformation_matrix(2, 3) = 0.4;
	transformation_matrix(1, 3) = 0.2;
	// Display in terminal the transformation matrix
	std::cout << "Applying this rigid transformation to: cloud_in -> cloud_icp" << std::endl;
	print4x4Matrix(transformation_matrix);

	// Executing the transformation
	pcl::transformPointCloud(*cloud_in, *cloud_icp, transformation_matrix);
	*cloud_tr = *cloud_icp;  // We backup cloud_icp into cloud_tr for later use
	//pcl::io::savePLYFileASCII("bunnyt.ply",*cloud_tr);

	//icp algorithm
	pcl::console::TicToc time;
	time.tic();
	Iter_para iter{ cloud_in->size(),20,0.001,0.8 };//��������
	icp( cloud_in, cloud_icp, iter, matrix_icp);
	std::cout << "Applied " << iter.Maxiterate << " ICP iteration(s) in " << time.toc() << " ms" << std::endl;

	//show regatation
	getchar();
    return 0;
}
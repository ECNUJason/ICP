#pragma once

#include <string>
#include <iostream>

#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/icp.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/console/time.h>   // TicToc
#include <omp.h>

//#include "kdTree.cpp"

typedef pcl::PointXYZ PointT;
typedef pcl::PointCloud<PointT> PointCloudT;

struct Iter_para //Interation paraments
{
	int ControlN;//���Ƶ����
	int Maxiterate;//����������
	double threshold;//��ֵ
	double acceptrate;//������

};

double distance(Eigen::Vector3d x, Eigen::Vector3d y)
{
	return (x - y).norm();
}

/******************************************************
����������������������֮�������ľ�����û��ʹ�ø߼������ݽṹ
���������cloud_targetĿ����ƾ���cloud_sourceԭʼ���ƾ���
���������error ������������������ֵ,ConQ��P��Ӧ�Ŀ��Ƶ����
********************************************************/
double FindClosest(const Eigen::MatrixXd  cloud_target,
	const Eigen::MatrixXd cloud_source, Eigen::MatrixXd &ConQ)
{
	double error = 0.0;
	int *Index = new int[cloud_target.cols()];
	//ʱ�临�Ӷȱ�ը,��Ҫʹ��kd-tree���ֽṹ
#pragma omp parallel for
	for (int i = 0; i < cloud_target.cols(); i++)
	{
		double maxdist = 100.0;
		for (int j = 0; j < cloud_source.cols(); j++)
		{
			double dist = distance(cloud_target.col(i), cloud_source.col(j));
			if (dist<maxdist)
			{
				maxdist = dist;
				Index[i] = j;
			}
		}
		Eigen::Vector3d v = cloud_source.col(Index[i]);
		ConQ.col(i) = v;
		error += maxdist;
	}
	return error;
}

/******************************************************
��������������������֮��ı任����
���������ConPĿ����ƿ��Ƶ�3*N��ConQԭʼ���ƿ��Ƶ�3*N
���������transformation_matrix����֮��任����4*4
********************************************************/
Eigen::Matrix4d GetTransform(const Eigen::MatrixXd ConP, const Eigen::MatrixXd ConQ)
{
	int nsize = ConP.cols();
	//��������Ĳ��Ƶ����ĵ�
	Eigen::VectorXd MeanP = ConP.rowwise().mean();
	Eigen::VectorXd MeanQ = ConQ.rowwise().mean();
	//cout << MeanP<< MeanQ<<endl;
	Eigen::MatrixXd ReP = ConP.colwise() - MeanP;
	Eigen::MatrixXd ReQ  = ConQ.colwise() - MeanQ;
	//�����ת����
	//Eigen::MatrixXd H = ReQ*(ReP.transpose());
	Eigen::MatrixXd H = ReP*(ReQ.transpose());
	Eigen::JacobiSVD<Eigen::MatrixXd> svd(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
	Eigen::Matrix3d U = svd.matrixU();
	Eigen::Matrix3d V = svd.matrixV();
	float det = (U * V.transpose()).determinant();
	Eigen::Vector3d diagVec(1.0, 1.0, det);
	Eigen::MatrixXd R = V * diagVec.asDiagonal() * U.transpose();
	//Eigen::MatrixXd R = H*((ReP*(ReP.transpose())).inverse());
	//���ƽ������
	Eigen::VectorXd T = MeanQ - R*MeanP;

	Eigen::MatrixXd Transmatrix = Eigen::Matrix4d::Identity();
	Transmatrix.block(0, 0, 3, 3) = R; 
	Transmatrix.block(0, 3, 3, 1) = T;
	cout << Transmatrix << endl;
	return Transmatrix;
}

/******************************************************
�������������Ʊ任
���������ConP���ƿ��Ƶ�3*N��transformation_matrix����֮��任����4*4
���������NewP�µĵ��ƿ��Ƶ�3*N
********************************************************/
Eigen::MatrixXd Transform(const Eigen::MatrixXd ConP, const Eigen::MatrixXd Transmatrix)
{
	Eigen::initParallel();
	Eigen::MatrixXd R = Transmatrix.block(0, 0, 3, 3);
	Eigen::VectorXd T = Transmatrix.block(0, 3, 3, 1);
	
	Eigen::MatrixXd NewP = (R*ConP).colwise() + T;
	return NewP;
}

/*�������ݴ�����������*/
Eigen::MatrixXd cloud2data(const pcl::PointCloud<pcl::PointXYZ>::Ptr cloud)
{
	int nsize = cloud->size();
	Eigen::MatrixXd Q(3, nsize);
	Eigen::initParallel();
	omp_set_num_threads(4);
#pragma omp parallel for
	for (int i = 0; i < nsize; i++) {
			Q(0, i) = cloud->points[i].x;
			Q(1, i) = cloud->points[i].y;
			Q(2, i) = cloud->points[i].z;
		}
	return Q;
}


/******************************************************
�������ܣ�ICP�㷨��������������֮���ת����ϵ
���������cloud_targetĿ����ƣ�cloud_sourceԭʼ����
        Iter��������
���������transformation_matrix ת������
********************************************************/
void icp(const PointCloudT::Ptr cloud_target,
	const PointCloudT::Ptr cloud_source,
	const Iter_para Iter, Eigen::Matrix4d &transformation_matrix)
{
	//����洢
	int nP = cloud_target->size();
	int nQ = cloud_source->size();
	Eigen::MatrixXd P = cloud2data(cloud_target);
	Eigen::MatrixXd Q = cloud2data(cloud_source);

	//Ѱ�ҵ�������
	//�������
	int i = 1;
	Eigen::MatrixXd ConP = P;
	Eigen::MatrixXd ConQ = Q;
	while(i<Iter.Maxiterate)
	{
		//1.Ѱ��P�е���Q�о�������ĵ�
		double error=FindClosest(ConP, Q, ConQ);
		//2.����Ӧ�ĸ���任
		transformation_matrix = GetTransform(ConP, ConQ);
		//3.��P���任�õ��µĵ���
		ConP = Transform(ConP, transformation_matrix);
		//4.������������ֱ������
		if (abs(error)<Iter.ControlN*Iter.acceptrate*Iter.threshold)//80%�����С��0.01
		{
			break;
		}
		i++;
	}
	//transformation_matrix = Transform(P, Q);
	transformation_matrix = GetTransform(P,ConP);
}

void print4x4Matrix(const Eigen::Matrix4d & matrix)
{
	printf("Rotation matrix :\n");
	printf("    | %6.3f %6.3f %6.3f | \n", matrix(0, 0), matrix(0, 1), matrix(0, 2));
	printf("R = | %6.3f %6.3f %6.3f | \n", matrix(1, 0), matrix(1, 1), matrix(1, 2));
	printf("    | %6.3f %6.3f %6.3f | \n", matrix(2, 0), matrix(2, 1), matrix(2, 2));
	printf("Translation vector :\n");
	printf("t = < %6.3f, %6.3f, %6.3f >\n\n", matrix(0, 3), matrix(1, 3), matrix(2, 3));
}

PointCloudT::Ptr ReadFile(std::string FileName)
{

	PointCloudT::Ptr cloud(new PointCloudT);
	if (pcl::io::loadPLYFile(FileName, *cloud) < 0)
	{
		PCL_ERROR("Error loading cloud %s.\n", FileName);
	}
	return cloud;
}
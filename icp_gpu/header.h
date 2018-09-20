#pragma once
#include<time.h>//ʱ�����ͷ�ļ����������к�������ͼ�����ٶ�    
#include <iostream>  
#include <vector>
#include <Eigen/Dense>
#define N 35947
//#define N 761
#define M_PI 3.1415926

struct Iter_para //Interation paraments
{
	int ControlN;//���Ƶ����
	int Maxiterate;//����������
	double threshold;//��ֵ
	double acceptrate;//������

};

using namespace std;
using Eigen::Map;

void icp(const Eigen::MatrixXd cloud_target,
	const Eigen::MatrixXd cloud_source,
	const Iter_para Iter, Eigen::Matrix4d &transformation_matrix);
void Getinfo();
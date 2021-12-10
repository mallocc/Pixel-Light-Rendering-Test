// Pixel Lighting Engine 2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <CL\cl.hpp>
#include <CL\opencl.h>
#include <GL\glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdlib.h>  
#include <time.h>
#include <Windows.h>

const int pxlsize = 3;
const int winW = 1600;
const int winH = 900;
const int pxlsrow = winW / pxlsize;
const int pxlscol = winH / pxlsize;
static const int nthread = 256;
static const int dataSize = pxlsrow * pxlscol;
static const int maxlights = 5;
static const int maxwalls = 40;
cl_float mbc = 10.0;
cl_float4 pxltrans = { pxlsize / 2.0f, pxlsize / 2.0f, 0.0f, 0.0f };
cl_float4 ambcolour = { 0.1f, 0.1f, 0.1f, 0.0f };
bool showentities = false;

inline float rndN() { return ((double)rand() / (RAND_MAX)); }
inline void calc();
inline void CheckError(cl_int error);

const char * ker = "test2";
cl_command_queue queue;
cl_kernel kernel;
std::vector<cl_device_id> deviceIds;
cl_int error;
cl_context context;

cl_float4 p[dataSize], c[dataSize], li[maxlights], lic[maxlights], wp1[maxwalls], wp2[maxwalls], lim[maxlights*dataSize];
cl_bool liu[maxlights*dataSize];
cl_mem pB, cB, liB, licB, wp1B, wp2B, limB, liuB;


float fps, afps, asfps;
int counter, counter2, counter3, counter4;
cl_int4 mousepos = { 0.0f, 0.0f, 0.0f, 0.0f };
float zoom = 1;
float zoominc = zoom / 10;
int framc = 0;
cl_int4 target = { 0, 0, 0, 0 };

void drawScene()
{
	float beginT = GetTickCount();
	counter4 += 1;
	if (counter4 > 0)
	{
		counter4 = 0;
		calc();
		framc++;
		if (framc > 1) framc = 0;
	}

	counter += 1;
	if (counter > 10)
	{
		counter = 0;
		POINT mp;
		GetCursorPos(&mp);
		mousepos.s[0] = (mp.x - glutGet(GLUT_WINDOW_X)) / zoom;
		mousepos.s[1] = (mp.y - glutGet(GLUT_WINDOW_Y)) / zoom;

		// Get the results back to the host
		CheckError(clEnqueueReadBuffer(queue, cB, CL_TRUE, 0,
			sizeof(cl_float4) * dataSize,
			c,
			0, nullptr, nullptr));

		if (GetAsyncKeyState(VK_UP))
		{
		}
		if (GetAsyncKeyState(VK_DOWN))
		{

		}


		if (GetAsyncKeyState('W')){}
		//target.s[1] -= 1;
		if (GetAsyncKeyState('S')){}
		//	target.s[1] += 1;
		if (GetAsyncKeyState('A')){}
		//target.s[0] -= 1;
		if (GetAsyncKeyState('D')){}
		//	target.s[0] += 1;


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glShadeModel(GL_SMOOTH);
		//draw shit here		

		glPushMatrix();
		//glScalef(1 / zoom, 1 / zoom, 0.0f);
		//glTranslatef(zoom *winW / 2, zoom * winH / 2, 0.0f);

		glBegin(GL_QUADS);
		int nc = 0;
		for (int i = 0; i < pxlsrow; i++)
		{
			for (int j = 0; j < pxlscol; j++, nc++)
			{
				//glBegin(GL_POLYGON);
				glColor3f(c[nc].s[0], c[nc].s[1], c[nc].s[2]);
				//glColor3f(0.5f, 0.5f,  0.5f);
				float x = p[nc].s[0];
				float y = p[nc].s[1];
				glVertex2f(0 + x, 0 + y);
				glVertex2f(pxlsize + x, 0 + y);
				glVertex2f(pxlsize + x, pxlsize + y);
				glVertex2f(0 + x, pxlsize + y);
				//glEnd();
			}
		}
		glEnd();

		if (showentities)
		{
			glBegin(GL_POINTS);
			for (int i = 0; i < maxlights; i++)
			{
				glColor3f(1, 1, 1);
				glVertex2f(li[i].s[0], li[i].s[1]);
			}
			glEnd();

			glBegin(GL_LINES);
			for (int i = 0; i < maxwalls; i++)
			{
				glColor3f(1, 1, 0);
				glVertex2f(wp1[i].s[0], wp1[i].s[1]);
				glVertex2f(wp2[i].s[0], wp2[i].s[1]);
			}
			glEnd();
		}

		glPopMatrix();

		float dT = GetTickCount() - beginT;
		if (dT > 0) fps = 1000.0f / (dT);

		int fpsf = 10;
		afps += fps / fpsf;
		counter3 += 1;
		if (counter3 > fpsf - 1)
		{
			counter3 = 0;
			asfps = afps;
			afps = 0;
		}
	}

	counter2 += 1;
	if (counter2 > 200)
	{
		counter2 = 0;
		std::system("CLS");
		//std::cout << "FPS: " << fps << std::endl;
		std::cout << "FPS: " << asfps << std::endl;

		std::cout << "Pixels: " << dataSize << std::endl;
		std::cout << "Lights: " << maxlights << std::endl;
		std::cout << "Walls: " << maxwalls << std::endl;
		std::cout << "Max Combinations: " << dataSize*maxlights*maxwalls << std::endl;
		std::cout << "Work Group Size: " << nthread << std::endl;
		std::cout << "Kernel Running: " << ker << std::endl;

		std::cout << "Zoom scale: " << 1 / zoom << std::endl;
	}
	li[0].s[0] = mousepos.s[0];
	li[0].s[1] = mousepos.s[1];

	for (int j = 0; j < dataSize; j++)
	{
		liu[0 * dataSize + j] = true;
	}

	if (ker == "test") CheckError(clEnqueueWriteBuffer(queue, liuB, CL_TRUE, 0,
		sizeof(cl_bool) * (maxlights*dataSize),
		liu,
		0, nullptr, nullptr));

	CheckError(clEnqueueWriteBuffer(queue, liB, CL_TRUE, 0,
		sizeof(cl_float4) * (maxlights),
		li,
		0, nullptr, nullptr));

	CheckError(clEnqueueUnmapMemObject(queue,
		cB, c, 0, NULL, NULL));

	glutSwapBuffers();
	glutPostRedisplay();
}

void changeSize(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);

}


void startGLscene(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(winW, winH);
	glutCreateWindow("A Window");
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
	glPointSize(1.0);
	//glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	//glColor3f(0.8f, 0.8f, 0.8f);
	glutDisplayFunc(drawScene);
	glutReshapeFunc(changeSize);
	glutMainLoop();
}

std::string GetPlatformName(cl_platform_id id)
{
	size_t size = 0;
	clGetPlatformInfo(id, CL_PLATFORM_NAME, 0, nullptr, &size);

	std::string result;
	result.resize(size);
	clGetPlatformInfo(id, CL_PLATFORM_NAME, size,
		const_cast<char*> (result.data()), nullptr);

	return result;
}

std::string GetDeviceName(cl_device_id id)
{
	size_t size = 0;
	clGetDeviceInfo(id, CL_DEVICE_NAME, 0, nullptr, &size);

	std::string result;
	result.resize(size);
	clGetDeviceInfo(id, CL_DEVICE_NAME, size,
		const_cast<char*> (result.data()), nullptr);

	return result;
}

inline void CheckError(cl_int error)
{
	if (error != CL_SUCCESS) {
		std::cerr << "OpenCL call failed with error " << error << std::endl;
		std::system("PAUSE");
		std::exit(1);
	}
}

std::string LoadKernel(const char* name)
{
	std::ifstream in(name);
	std::string result(
		(std::istreambuf_iterator<char>(in)),
		std::istreambuf_iterator<char>());
	return result;
}

cl_program CreateProgram(const std::string& source,
	cl_context context)
{
	size_t lengths[1] = { source.size() };
	const char* sources[1] = { source.data() };

	cl_int error = 0;
	cl_program program = clCreateProgramWithSource(context, 1, sources, lengths, &error);
	CheckError(error);

	return program;
}

inline void calc()
{
	if (ker == "test")
	{
		clSetKernelArg(kernel, 0, sizeof(int), &maxlights);
		clSetKernelArg(kernel, 1, sizeof(int), &maxwalls);
		clSetKernelArg(kernel, 2, sizeof(cl_mem), &cB);
		clSetKernelArg(kernel, 3, sizeof(cl_mem), &pB);
		clSetKernelArg(kernel, 4, sizeof(cl_mem), &liB);
		clSetKernelArg(kernel, 5, sizeof(cl_mem), &licB);
		clSetKernelArg(kernel, 6, sizeof(cl_mem), &wp1B);
		clSetKernelArg(kernel, 7, sizeof(cl_mem), &wp2B);
		clSetKernelArg(kernel, 8, sizeof(cl_float4), &pxltrans);
		clSetKernelArg(kernel, 9, sizeof(cl_float4), &ambcolour);
		clSetKernelArg(kernel, 10, sizeof(cl_float), &mbc);
		clSetKernelArg(kernel, 11, sizeof(cl_mem), &limB);
		clSetKernelArg(kernel, 12, sizeof(int), &dataSize);
		clSetKernelArg(kernel, 13, sizeof(cl_mem), &liuB);
		clSetKernelArg(kernel, 14, sizeof(int), &framc);
		clSetKernelArg(kernel, 15, sizeof(int), &pxlsrow);
		clSetKernelArg(kernel, 16, sizeof(int), &pxlscol);
	}
	else if (ker == "test2")
	{
		clSetKernelArg(kernel, 0, sizeof(int), &maxlights);
		clSetKernelArg(kernel, 1, sizeof(int), &maxwalls);
		clSetKernelArg(kernel, 2, sizeof(cl_mem), &cB);
		clSetKernelArg(kernel, 3, sizeof(cl_mem), &pB);
		clSetKernelArg(kernel, 4, sizeof(cl_mem), &liB);
		clSetKernelArg(kernel, 5, sizeof(cl_mem), &licB);
		clSetKernelArg(kernel, 6, sizeof(cl_mem), &wp1B);
		clSetKernelArg(kernel, 7, sizeof(cl_mem), &wp2B);
		clSetKernelArg(kernel, 8, sizeof(cl_float4), &pxltrans);
		clSetKernelArg(kernel, 9, sizeof(cl_float4), &ambcolour);
		clSetKernelArg(kernel, 10, sizeof(cl_float), &mbc);
		clSetKernelArg(kernel, 11, sizeof(int), &dataSize);
		clSetKernelArg(kernel, 12, sizeof(int), &pxlsrow);
		clSetKernelArg(kernel, 13, sizeof(int), &pxlscol);
	}

	cl_event eve;
	const size_t globalWorkSize[] = { dataSize, 0, 0 };
	const size_t localWorkSize[] = { nthread, 0, 0 };
	CheckError(clEnqueueNDRangeKernel(queue, kernel, 1,
		nullptr,
		globalWorkSize,
		localWorkSize,
		0, nullptr, &eve));

	CheckError(clFlush(queue));
}

inline void createBuffers()
{
	cB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (dataSize),
		c, &error);
	CheckError(error);
	pB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (dataSize),
		p, &error);
	CheckError(error);
	liB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (maxlights),
		li, &error);
	CheckError(error);
	licB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (maxlights),
		lic, &error);
	CheckError(error);
	wp1B = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (maxwalls),
		wp1, &error);
	CheckError(error);
	wp2B = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (maxwalls),
		wp2, &error);
	CheckError(error);
	limB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_float4) * (maxlights*dataSize),
		lim, &error);
	CheckError(error);
	liuB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_bool) * (maxlights*dataSize),
		liu, &error);
	CheckError(error);
}

inline void setupData()
{
	int i = 0;
	int k = 0;
	for (; i < pxlsrow; i++)
	{
		for (int j = 0; j < pxlscol; j++, k++)
		{
			c[k].s[0] = 0.0f;
			c[k].s[1] = 0.0f;
			c[k].s[2] = 0.0f;
			c[k].s[3] = 0.0f;
			//c[k].s[0] = rndN()/10;
			p[k].s[0] = i * pxlsize;
			p[k].s[1] = j * pxlsize;
			p[k].s[2] = 0.0f;
			p[k].s[3] = 0.0f;
		}
	}

	i = 0;
	for (; i < maxlights; i++)
	{
		li[i].s[0] = rndN() * winW;
		li[i].s[1] = rndN() * winH;

		lic[i].s[0] = rndN() * 0.5+0.5;
		lic[i].s[1] = rndN() * 0.5+0.5;
		lic[i].s[2] = rndN() * 0.5+0.5;
		//lic[i].s[0] = 1.0f;
		//lic[i].s[1] = 0.5f;
		//lic[i].s[2] = 0.5f;

		lic[i].s[3] = 500.0f;

		for (int j = 0; j < dataSize; j++)
		{
			lim[i * dataSize + j].s[0] = 0.0f;
			lim[i * dataSize + j].s[1] = 0.0f;
			lim[i * dataSize + j].s[2] = 0.0f;
			lim[i * dataSize + j].s[3] = 0.0f;
			liu[i * dataSize + j] = true;
		}
	}
	lic[0].s[3] = 5000.0f;

	//li[1].s[0] = 0.0f;
	//li[1].s[1] = -1000;
	//lic[1].s[0] = 0.8f;
	//lic[1].s[1] = 0.8f;
	//lic[1].s[2] = 1.0f;
	//lic[1].s[3] = 200000.0f;

	i = 0;
	//for (; i < maxwalls; i++)
	//{
	//	wp1[i].s[0] = rndN() * winW;
	//	wp1[i].s[1] = rndN() * winH;
	//	wp2[i].s[0] = rndN() * winW;
	//	wp2[i].s[1] = rndN() * winH;
	//}

	//float dr = 4.0f*3.141f / maxwalls;
	//for (; i < maxwalls; i++)
	//{
	//	wp1[i].s[0] = cos(dr * i) * 200 + 300 + dr * i*50;
	//	wp1[i].s[1] = sin(dr * i) * 200 + 300;
	//	wp2[i].s[0] = cos(dr * (i + 1)) * 200 + 300 + dr * (i+1)*50;
	//	wp2[i].s[1] = sin(dr * (i + 1)) * 200 + 300;
	//}

	int nums = maxwalls / 4;
	for (int j = 0; j < nums; j++)
	{
		int count = j * 4;
		int wid = 50;
		float x = rndN() * winW;
		float y = rndN() * winH;
		wp1[count].s[0] = x;
		wp1[count].s[1] = y;
		wp2[count].s[0] = x + wid;
		wp2[count].s[1] = y;

		wp1[count + 1].s[0] = x + wid;
		wp1[count + 1].s[1] = y;
		wp2[count + 1].s[0] = x + wid;
		wp2[count + 1].s[1] = y + wid;

		wp1[count + 2].s[0] = x + wid;
		wp1[count + 2].s[1] = y + wid;
		wp2[count + 2].s[0] = x;
		wp2[count + 2].s[1] = y + wid;

		wp1[count + 3].s[0] = x;
		wp1[count + 3].s[1] = y + wid;
		wp2[count + 3].s[0] = x;
		wp2[count + 3].s[1] = y;
	}
}

int _tmain(int argc, char* argv[]){
	srand(time(NULL));

	setupData();

	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, nullptr, &platformIdCount);

	if (platformIdCount == 0) {
		std::cerr << "No OpenCL platform found" << std::endl;
		return 1;
	}
	else {
		std::cout << "Found " << platformIdCount << " platform(s)" << std::endl;
	}

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);
	for (cl_uint i = 0; i < platformIdCount; ++i) {
		std::cout << "\t (" << (i + 1) << ") : " << GetPlatformName(platformIds[i]) << std::endl;
	}


	//find devices
	cl_uint deviceIdCount = 0;
	clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_ALL, 0, nullptr,
		&deviceIdCount);

	if (deviceIdCount == 0) {
		std::cerr << "No OpenCL devices found" << std::endl;
		return 1;
	}
	else {
		std::cout << "Found " << deviceIdCount << " device(s)" << std::endl;
	}

	std::vector<cl_device_id> deviceIds(deviceIdCount);
	clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_ALL, deviceIdCount,
		deviceIds.data(), nullptr);
	for (cl_uint i = 0; i < deviceIdCount; ++i) {
		std::cout << "\t (" << (i + 1) << ") : " << GetDeviceName(deviceIds[i]) << std::endl;
	}

	//create the context
	const cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties> (platformIds[0]),
		0, 0
	};

	cl_int error = CL_SUCCESS;
	context = clCreateContext(contextProperties, deviceIdCount,
		deviceIds.data(), nullptr, nullptr, &error);
	CheckError(error);

	std::cout << "Context created" << std::endl;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//create a program from the CL file
	cl_program program = CreateProgram(LoadKernel("kernels.cl"),
		context);
	CheckError(clBuildProgram(program, deviceIdCount, deviceIds.data(), nullptr, nullptr, nullptr));

	kernel = clCreateKernel(program, ker, &error);
	CheckError(error);
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	createBuffers();

	queue = clCreateCommandQueue(context, deviceIds[0],
		0, &error);
	CheckError(error);

	startGLscene(argc, argv);

	clReleaseCommandQueue(queue);
	clReleaseMemObject(cB);
	clReleaseMemObject(pB);
	clReleaseMemObject(liB);
	clReleaseMemObject(licB);
	clReleaseMemObject(wp1B);
	clReleaseMemObject(wp2B);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseContext(context);

	return 0;
}


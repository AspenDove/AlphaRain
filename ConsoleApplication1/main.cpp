#pragma comment(lib, "glut32.lib")
#define GLUT_STATIC_LIB
#include <GL/glut.h>
#include <math.h>
#include <iostream>
#include <random>
#include <time.h>
#include <vector>
#include "random.hpp"
#include "fasttrigo.h"
#include <tuple>
#include <functional>
#include <vector>

using Random = effolkronium::random_static;
struct param
{
	GLfloat fxScale = 1.f, fyScale = 1.f;
	GLfloat fDir = 10;
	GLfloat fMaxDir = 15;
	const GLint nChar = 300;
	GLint cxClient = 1000, cyClient = 480;
}params;

template <typename T, int MaxLen>
class FixedVector : public std::vector<T>
{
public:
	FixedVector()
	{
		this->resize(MaxLen);
	}
	void push_back(const T& value)
	{
		for (int i = 0; i < this->size() - 1; ++i)
		{
			this->at(i) = this->at(i + 1);
		}
		this->at(this->size() - 1) = value;
	}
};


class AlphaRain
{
protected:
	struct GLPoint
	{
		GLfloat x, y;
	};
	const GLfloat PI = 3.1415926;
	const GLfloat length_speed_ratio = 4e2;
	const GLfloat speed_ratio = 8e-4;
	GLint alpha = 255, Nth = 0, n = 0;
	GLint alpha_max = 255, alpha_min = 55;
	GLfloat fThickness = 2;
	GLfloat size_max = 4000, size_min = 2000;
	GLPoint ptHead{}, ptTail{}, speed{};
	GLfloat theta{}, fLength{}, size{}, fDirOld{};

	char character = 'A';
public:
	void render()
	{
		if (n <= Nth)return;

		glLineWidth(fThickness);
		glColor4ub(0, 0, 0, alpha);

		glPushMatrix();
		{
			glTranslatef(ptHead.x, ptHead.y, 0);
			glScalef(size, size, size);
			glutStrokeCharacter(GLUT_STROKE_ROMAN, character);
		}
		glColor4ub(100, 150, 255, 128);
		glPopMatrix();

		glLineWidth(1.0);
		glBegin(GL_LINES);
		{
			glVertex2f(ptHead.x + 90.f * size / 2, ptHead.y + 100.f * size);
			glVertex2f(ptTail.x + 90.f * size / 2, ptTail.y + 100.f * size);
		}
		glEnd();
	}

};

class Alpha final : public AlphaRain
{
public:
	const GLint density = 5000;
	GLfloat fDirOld{};
	GLPoint speed_delta = { 0,2e-9 };

	Alpha()
	{
		reset();
		Nth = Random::get<std::uniform_int_distribution<GLint>>(0, density);
	}
	void reset()
	{
		ptHead = { Random::get<std::uniform_real_distribution<GLfloat>>(-params.fxScale, params.fxScale),params.fyScale };
		theta = Random::get<std::normal_distribution<GLfloat>>(params.fDir / 180.f*PI, 0.01f);
		size = Random::get<std::uniform_real_distribution<GLfloat>>(1 / size_max, 1 / size_min);
		GLfloat k = (alpha_max - alpha_min) / (size_min - size_max);
		alpha = k / size + (alpha_max - k * size_min);
		character = Random::get('A', 'Z');

		speed = { 0,Random::get<std::normal_distribution<GLfloat>>(speed_ratio, 1e-4f) };
		speed_delta.x = (tan(theta) - tan(params.fDir / 180.f*PI))*speed.y;
		fLength = sqrt(pow(speed.x, 2) + pow(speed.y, 2))*length_speed_ratio;
		ptTail = { ptHead.x + fLength * FT::sin(theta), ptHead.y + fLength * FT::cos(theta) };
		fDirOld = params.fDir;
	}
	void update()
	{
		if (n++ <= Nth)return;
		speed.y += speed_delta.y;
		speed.x = speed_delta.x + (params.fDir / 180.f*PI)*speed_ratio;
		fDirOld = params.fDir;
		theta = FT::atan2(speed.x, speed.y);

		ptHead.x -= std::abs(speed.x) * FT::sin(theta);
		ptHead.y -= speed.y * FT::cos(theta);
		fLength = sqrt(pow(speed.x, 2) + pow(speed.y, 2))*length_speed_ratio;
		ptTail = { ptHead.x + fLength * FT::sin(theta), ptHead.y + fLength * FT::cos(theta) };

		if (ptTail.y < -params.fyScale || std::abs(ptTail.x)>params.fxScale)
		{
			//std::cout << "speedx  " << speed.x << ",theta  " << theta << std::endl;
			reset();
		}
	}
};

class AlphaStr final :public AlphaRain
{
public:
	GLPoint ptDest{}, speed0{};
	GLint Niter{}, Nhold{}, Nfall{};
	GLfloat fKSlow = 0.8f, fSpdyMax{};
	FixedVector<GLPoint, 50> tail;

	AlphaStr(const std::tuple<int, int, int, int>& N, char character, GLPoint ptDest) :
		ptDest(ptDest),
		Niter(std::get<1>(N)),//falling to position time
		Nhold(std::get<2>(N)),//holding time
		Nfall(std::get<3>(N))//falling speed up time
	{
		Nth = std::get<0>(N);
		this->character = character;
		this->fThickness = 3.f;

		alpha_max = 255, alpha_min = 150;
		size_max = 2000, size_min = 1500;

		size = Random::get<std::uniform_real_distribution<GLfloat>>(1 / size_max, 1 / size_min);
		GLfloat k = (alpha_max - alpha_min) / (size_min - size_max);
		alpha = k / size + (alpha_max - k * size_min);
		//for (auto&&pt : tail)
		//{
		//	pt = ptHead;
		//}
	}

	GLfloat delta_spdx(GLint n)noexcept
	{
		static GLfloat fFinalspd;
		if (n <= Niter)
			return -speed0.x / (Niter - Nth);
		else if (n == Niter + Nhold)
			return fFinalspd = Random::get<std::normal_distribution<GLfloat>>(speed_ratio*tan(params.fDir / 180.f*PI), 1e-4f*abs(tan(params.fDir / 180.f*PI))), 0.f;
		else if (n > Niter + Nhold && n < Niter + Nhold + Nfall)
			return fFinalspd / Nfall;
		else
			return 0.f;
	}
	GLfloat delta_spdy(GLint n)noexcept
	{
		static GLfloat fFinalspd;
		if (n <= fKSlow * (Niter - Nth) + Nth)
			return (fSpdyMax - speed0.y) / (fKSlow*(Niter - Nth));
		else if (n <= Niter)
			return -fSpdyMax / ((1 - fKSlow)*(Niter - Nth));
		else if (n == Niter + Nhold)
			return fFinalspd = Random::get<std::normal_distribution<GLfloat>>(speed_ratio, 1e-4f), 0.f;
		else if (n > Niter + Nhold && n < Niter + Nhold + Nfall)
			return fFinalspd / Nfall;
		else
			return 0.f;
	}

	bool update()
	{
		++n;
		if (n < Nth)return true;
		else if (n == Nth)
		{
			float x = ptDest.x + Random::get<std::normal_distribution<GLfloat>>(abs(ptHead.y - ptDest.y)*tan(params.fDir / 180.f*PI), 0.01f);
			ptHead = { std::max(-params.fxScale,std::min(x,params.fxScale)),params.fyScale };

			speed0 = speed = { -2 * (ptDest.x - ptHead.x) / (Niter - Nth), Random::get<std::normal_distribution<GLfloat>>(speed_ratio, 1e-4f) };
			fSpdyMax = (2 * (ptHead.y - ptDest.y) - speed.y * fKSlow*(Niter - Nth)) / (Niter - Nth);

			fLength = sqrt(pow(speed.x, 2) + pow(speed.y, 2))*length_speed_ratio;
			theta = FT::atan2(speed.x, speed.y);
			ptTail = { ptHead.x + fLength * FT::sin(theta), ptHead.y + fLength * FT::cos(theta) };
		}
		//if(n%10 == 0)
		//	tail.push_back(ptHead);

		speed.y += delta_spdy(n);
		speed.x += delta_spdx(n);
		ptHead.x -= speed.x;
		ptHead.y -= speed.y;
		theta = FT::atan2(speed.x, speed.y);
		fLength = sqrt(speed.x*speed.x + speed.y*speed.y)*length_speed_ratio;
		ptTail = { ptHead.x + fLength * FT::sin(theta), ptHead.y + fLength * FT::cos(theta) };
		if (ptTail.y < -params.fyScale || std::abs(ptTail.x) > params.fxScale)
		{
			return false;
		}
		return true;
	}
	//for (auto &&pt : tail)
	//{
	//	glVertex2f(pt.x + 90.f * size / 2, pt.y + 100.f * size);
	//}

};

class AlphaStrManager
{
	std::vector<AlphaStr> alphastr;
public:
	const GLfloat fAlphaSpace = 152 / 2000.f;
	AlphaStrManager(const std::string& str = "HELLOWORLD", const GLint& Ndelay = 1000)
	{
		GLfloat x, y;;
		x = Random::get(-params.fxScale, params.fxScale - fAlphaSpace * str.size());
		y = Random::get(-1.f, 1.f);
		for (auto&& c : str)
		{
			alphastr.push_back(
				AlphaStr(
					{
						Random::get(Ndelay,Ndelay + 2000),
						Random::get(Ndelay + 3000,Ndelay + 4000),
						1500,
						Random::get(1000,3000)
					}, toupper(c), { x,y }));
			x += fAlphaSpace;
		}
	}
	bool update()
	{
		bool bNotFinish = false;
		for (auto&& a : alphastr)
		{
			bNotFinish |= a.update();
		}
		return bNotFinish;
	}
	void render()
	{
		for (auto&& a : alphastr)
		{
			a.render();
		}
	}

};

std::vector<Alpha> alphas;
std::vector<AlphaStrManager> alphastr;

void display(void)
{
	glClearColor(225 / 255.f, 225 / 255.f, 225 / 255.f, 0); // Set background color to black and opaque
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glDepthMask(false);

	for (auto&& alpha : alphas)
	{
		alpha.update();
		alpha.render();
	}
	static auto it = alphastr.begin();
	if (it != alphastr.end())
	{
		if (it->update())
		{
			it->render();
		}
		else ++it;
	}

	glutSwapBuffers();
	glFlush();
}
void idle()
{
	glutPostRedisplay();   // Post a re-paint request to activate display()
}
/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshape(GLsizei width, GLsizei height)
{  // GLsizei for non-negative integer
   // Compute aspect ratio of the new window
	if (height == 0) height = 1;                // To prevent divide by 0
	GLfloat aspect = (GLfloat)width / (GLfloat)height;
	params.cxClient = width;
	params.cyClient = height;
	// Set the viewport to cover the new window
	glViewport(0, 0, width, height);

	// Set the aspect ratio of the clipping area to match the viewport
	glMatrixMode(GL_PROJECTION);  // To operate on the Projection matrix
	glLoadIdentity();             // Reset the projection matrix
	if (width >= height)
	{
		// aspect >= 1, set the height from -1 to 1, with larger width
		gluOrtho2D(-1.0 * aspect, 1.0 * aspect, -1.0, 1.0);
		params.fxScale = aspect;
		params.fyScale = 1;
	}
	else
	{
		// aspect < 1, set the width to -1 to 1, with larger height
		gluOrtho2D(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect);
		params.fxScale = 1;
		params.fyScale = 1 / aspect;
	}
}

void onMouseMove(GLint x, GLint y)
{
	params.fDir = 2.f * params.fMaxDir*x / params.cxClient - params.fMaxDir;
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(params.cxClient, params.cyClient);
	glutCreateWindow("Text");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glDepthMask(false);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	reshape(params.cxClient, params.cyClient);
	glutIdleFunc(idle);             // Register callback handler if no other event
	glutPassiveMotionFunc(onMouseMove);

	for (int i = 0; i != params.nChar; ++i)
	{
		alphas.push_back(Alpha());
	}

	alphastr.push_back(AlphaStrManager("Make me thy lyre, even as the forest is :"));
	alphastr.push_back(AlphaStrManager("What if my leavers are falling like its own!"));
	alphastr.push_back(AlphaStrManager("The tmult of thy mighty harmonies"));
	alphastr.push_back(AlphaStrManager("Will take from both a deep, autumnal tone,"));
	alphastr.push_back(AlphaStrManager("Sweet though in sadness.Be thou, Spirit fierce,"));
	alphastr.push_back(AlphaStrManager("My spirit!Be thou me, impetuous one!"));
	alphastr.push_back(AlphaStrManager("Drive my dead thoughts over the universe"));
	alphastr.push_back(AlphaStrManager("Like witheered leaves to quicken a new birth!"));
	alphastr.push_back(AlphaStrManager("And, by the incantation of this verse,"));
	alphastr.push_back(AlphaStrManager("Scatter, is from an unextinguished hearth"));
	alphastr.push_back(AlphaStrManager("Ashes and sparks, my words among mankind!"));
	alphastr.push_back(AlphaStrManager("Be through my lips to unawakened earth"));
	alphastr.push_back(AlphaStrManager("The trumpet of a prophecy!O Wind,"));
	alphastr.push_back(AlphaStrManager("If Winter comes, can Spring be far behind?"));

	glutMainLoop();
	return 0;
}
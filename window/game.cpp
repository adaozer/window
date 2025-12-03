#include "window.h"
#include "core.h"
#include "maths.h"
#include <fstream>
#include <sstream>
#include "PSOManager.h"
#include "mesh.h"
#include "Plane.h"
#include "Cube.h"
#include "Sphere.h"
#include "ConstantBuffer.h"
#include "GamesEngineeringBase.h"
#include "GEMLoader.h"
#include "GEMObject.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	Window win;
	Core core;
	win.init(1024, 1024, 0, 0, "My Window");
	core.init(win.hwnd, win.width, win.height);
	GamesEngineeringBase::Timer tim;
	ShaderManager shaderManager;

	GEMObject gem(&shaderManager);
	gem.init(&core, "acacia_003.gem");

	float theta = 90.0f;    
	float fov = 1.0f;
	float nearZ = 0.01f;
	float farZ = 100000.f;

	Matrix proj = Matrix().projMatrix((float)win.width, (float)win.height, theta, fov, farZ, nearZ);

	float time = 0.f;

	while (true) {
		float dt = tim.dt();
		time += dt;

		win.processMessages();
		if (win.keys[VK_ESCAPE] == 1) break;

		Vec3 from = Vec3(11 * cos(time), 5, 11 * sinf(time));
		Vec3 to = Vec3(0, 0, 0);
		Matrix v = Matrix().lookAt(to, from, Vec3(0, 1, 0));

		Matrix VP = proj.mul(v);
		Matrix W;
		Vec3 scaler(0.01f, 0.01f, 0.01f);
		W = W.scale(scaler);
		
		core.beginFrame();
		core.beginRenderPass();
		gem.draw(&core, W, VP);

		Vec3 trans(0.05, 0, 0);
		W = W.translate(trans);
		//gem.draw(&core, W, VP);
		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}
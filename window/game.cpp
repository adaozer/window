#include "window.h"
#include "core.h"
#include "maths.h"
#include <fstream>
#include <sstream>
#include "pso.h"
#include "meshes.h"
#include "triangle.h"
#include "buffering.h"
#include "GamesEngineeringBase.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	Window win;
	Core core;
	win.init(1024, 1024, 0, 0, "My Window");
	core.init(win.hwnd, win.height, win.width);
	Triangle t;
	t.init(&core);
	GamesEngineeringBase::Timer tim;

	ConstantBuffer2 constBufferCPU2;
	constBufferCPU2.time = 0;

	while (true) {
		float dt = tim.dt();
		constBufferCPU2.time += dt;
		for (int i = 0; i < 4; i++) {
			float angle = constBufferCPU2.time + (i * M_PI / 2.0f);
			constBufferCPU2.lights[i] = Vec4(win.width / 2.0f + (cosf(angle) * (win.width * 0.3f)), win.height / 2.0f + (sinf(angle) * (win.height * 0.3f)), 0, 0);
		}
		core.resetCommandList();
		core.beginFrame();
		win.processMessages();
		if (win.keys[VK_ESCAPE] == 1) break;
		t.draw(&core, &constBufferCPU2);
		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}
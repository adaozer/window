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
#include "AnimatedModel.h"
#include "Camera.h"
#include "Texture.h"
#include "AnimationManager.h"
#include "Gun.h"
#include "Enemy.h"
#include "InstancedObject.h"
#include "collision.h"

void listAnimationNames(AnimatedModel am)
{
	for (auto& [name, seq] : am.animation.animations) {
		std::cout << name << " : "
			<< seq.frames.size() << " frames, "
			<< seq.ticksPerSecond << " tps, "
			<< seq.duration() << " sec\n";
	}
	
}
static float rand01() { return (float)rand() / (float)RAND_MAX; }
static float randRange(float a, float b) { return a + (b - a) * rand01(); }

static void sampleDisk(float R, float& outX, float& outZ) {
	float a = randRange(0.0f, 2.0f * (float)M_PI);
	float r = sqrtf(rand01()) * R;
	outX = cosf(a) * r;
	outZ = sinf(a) * r;
}
// instancing, animation manager, enemy, gun
// collision thingy
//skybox sphere bound thingy


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	Window win;
	Core core;
	win.init(1024, 1024, 0, 0, "My Window");
	core.init(win.hwnd, win.width, win.height);
	GamesEngineeringBase::Timer tim;
	ShaderManager shaderManager;
	TextureManager tex;
	/*
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	*/
	GEMObject bananaTree(&shaderManager, &tex, "textures/banana2_LOD5_ALB.png", "textures/banana2_LOD5_NH.png");
	bananaTree.init(&core, "models/banana2_LOD5.gem");

	AnimatedModel gunModel(&shaderManager, &tex, "textures/AC5_Albedo_alb.png", "textures/AC5_Albedo_nh.png");
	gunModel.load(&core, "models/AutomaticCarbine.gem");

	AnimatedModel enemyModel(&shaderManager, &tex, "textures/T-rex_Base_Color_alb.png", "textures/T-rex_Base_Color_nh.png");
	enemyModel.load(&core, "models/TRex.gem");

	const int TREE_COUNT = 1000;
	const int GRASS_COUNT = 10000;
	const float SPAWN_R = 500;  

	std::vector<INSTANCE> treeInstances;
	treeInstances.reserve(TREE_COUNT);

	for (int i = 0; i < TREE_COUNT; i++) {
		float x, z;
		sampleDisk(SPAWN_R, x, z);

		float yaw = randRange(0.0f, 2.0f * (float)M_PI);

		INSTANCE inst;
		inst.W = Matrix::scale(Vec3(0.015, 0.015, 0.015))
			* Matrix::rotateY(yaw)
			* Matrix::translate(Vec3(x, 0.0f, z));

		treeInstances.push_back(inst);
	}

	InstancedObject bamboo(&shaderManager, &tex, "textures/bamboo branch_ALB.png", "textures/bamboo branch_NH.png", "pixelshader_normalmapped.hlsl");
	bamboo.init(&core, "models/bamboo.gem", treeInstances);

	std::vector<INSTANCE> grassInstances;
	grassInstances.reserve(GRASS_COUNT);

	for (int i = 0; i < GRASS_COUNT; i++) {
		float x, z;
		sampleDisk(SPAWN_R, x, z);

		float yaw = randRange(0.0f, 2.0f * (float)M_PI);

		INSTANCE inst;
		inst.W = Matrix::scale(Vec3(10.f, 10.f, 10.f))
			* Matrix::rotateY(yaw)
			* Matrix::translate(Vec3(x, 0.0f, z));

		grassInstances.push_back(inst);
	}

	InstancedObject grass(&shaderManager, &tex, "textures/TX_GrassClumps_01_ALB.png", "textures/TX_GrassClumps_01_NH.png", "pixelshader_alphatesting.hlsl");
	grass.init(&core, "models/Grass_Clump_01c.gem", grassInstances);

	AnimationManager animationManager;
	EnemyManager enemyManager;

	animationManager.set("AutomaticCarbine", AnimationName::Idle, "04 idle");
	animationManager.set("AutomaticCarbine", AnimationName::Attack, "08 fire");
	animationManager.set("AutomaticCarbine", AnimationName::Reload, "17 reload");
	animationManager.set("AutomaticCarbine", AnimationName::Inspect, "05 inspect");

	animationManager.set("TRex", AnimationName::Idle, "idle");
	animationManager.set("TRex", AnimationName::Attack, "attack");
	animationManager.set("TRex", AnimationName::Death,  "death");

	Gun gun;
	gun.init(&gunModel, animationManager);
	
	enemyManager.spawn(&enemyModel, animationManager);
	BoundingSphere enemyBounds;
	enemyBounds.radius = 4.f;

	Plane floor(&shaderManager, &tex, "textures/wood_chip_path_diff_4k.png", "textures/wood_chip_path_nor_dx_4k.png");
	floor.init(&core);

	Sphere skybox(&shaderManager, &tex, "textures/kloofendal_48d_partly_cloudy_puresky.jpg");
	skybox.init(&core, 1000.f);

	BoundingSphere playerBounds;
	playerBounds.radius = 0.5;

	float fov = 90.0f;
	float n = 0.01f;
	float f = 100000.f;
	float aspect = win.width / win.height;
	Matrix p = Matrix::projMatrix(aspect, fov, f, n);
	Camera cam;
	float time = 0.f;

	bool lastMouseDown = false;
	bool centered = false;
	bool lastZPress = false;

	while (true) {
		core.beginFrame();

		float dt = tim.dt();
		win.processMessages();
		if (centered) {
			win.centerCursor();
		}

		if (win.keys[VK_ESCAPE] == 1) break;
		bool zPress = win.keys['Z'];
		if (zPress && !lastZPress) {
			if (centered) {
				centered = false;
				ShowCursor(TRUE);
			}
			else {
				centered = true;
				ShowCursor(FALSE);
			}
		}
		lastZPress = zPress;

		time += dt;
		bool mouseDown = win.mouseButtons[0];

		if (mouseDown && !lastMouseDown) gun.onTriggerPressed();
		if (!mouseDown && lastMouseDown) gun.onTriggerReleased();

		lastMouseDown = mouseDown;

		cam.yaw += win.dx * cam.sensitivity;
		cam.pitch += win.dy * cam.sensitivity;

		float boundary = (89.f * M_PI) / 180.f;
		if (cam.pitch > boundary) cam.pitch = boundary;
		if (cam.pitch < -boundary) cam.pitch = -boundary;

		win.dx = 0;
		win.dy = 0;

		float yaw = cam.yaw;

		Vec3 forward = Vec3(std::cos(yaw), 0.0f, std::sin(yaw));
		Vec3 right = Vec3(-std::sin(yaw), 0.0f, std::cos(yaw));
		Vec3 enemyLoc(10, 0, 0);
		enemyBounds.centre = enemyLoc;
		playerBounds.centre = cam.pos;
		Vec3 move(0.f, 0.f, 0.f);
		if (win.keys['W']) move += forward;
		if (win.keys['A']) move += right;
		if (win.keys['S']) move -= forward;
		if (win.keys['D']) move -= right;
		if (win.keys['R']) gun.reload(animationManager);
		if (win.keys['F']) gun.inspect(animationManager);
		Vec3 desiredStep(0, 0, 0);
		if (move.lengthSquare() > 0.f) {
			move = move.normalize();
			desiredStep = move * cam.moveSpeed * dt;
		}

		BoundingSphere candidate = playerBounds;
		candidate.centre = cam.pos + desiredStep;
		if (!intersectsXZ(candidate, enemyBounds))
		{
			cam.pos = candidate.centre;
		}
		else
		{
			Vec3 toPlayer = candidate.centre - enemyBounds.centre;
			toPlayer.y = 0.0f;

			float dist2 = toPlayer.lengthSquare();
			if (dist2 > 1e-8f)
			{
				float dist = sqrtf(dist2);
				Vec3 nrm = toPlayer * (1.0f / dist);

				float minDist = playerBounds.radius + enemyBounds.radius;

				float penetration = minDist - dist;
				candidate.centre += nrm * penetration;

				Vec3 stepXZ = desiredStep;
				stepXZ.y = 0.0f;

				float into = stepXZ.Dot(stepXZ, nrm);
				Vec3 slideStep = stepXZ - nrm * into;

				BoundingSphere slid = playerBounds;
				slid.centre = candidate.centre + slideStep;

				if (!intersectsXZ(slid, enemyBounds))
				{
					cam.pos = slid.centre;
				}
				else
				{
					cam.pos = candidate.centre;
				}
			}
		}

		cam.pos.y = 3.5f;
		playerBounds.centre = cam.pos;

		float radius = 950.f;

		float d2 = cam.pos.x * cam.pos.x + cam.pos.z * cam.pos.z;
		if (d2 > radius * radius) {
			float d = sqrtf(d2);
			cam.pos.x = cam.pos.x / d * radius;
			cam.pos.z = cam.pos.z / d * radius;
		}

		Matrix v = cam.viewMatrix();
		Matrix camWorld = v.invert();
		Vec3 offsetCam = Vec3(0, 0, 0);
		Matrix vp = v * p;
		Matrix local = Matrix::scale(Vec3(0.02f, 0.02f, 0.02f)) * Matrix::rotateY(M_PI) * Matrix::translate(offsetCam);

		Matrix gunW = local * camWorld;
		
		gun.update(dt, animationManager);
		enemyManager.update(dt, animationManager);

		core.beginRenderPass();

		gun.draw(&core, gunW, vp);
		Matrix enemyW = Matrix::scale(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translate(enemyLoc);
		enemyManager.draw(&core, enemyW, vp);

		bamboo.draw(&core, vp);
		Matrix W;

		grass.draw(&core, vp);

		floor.draw(&core, W, vp);
		skybox.draw(&core, W, vp);

		W = Matrix::scale(Vec3(0.05, 0.05, 0.05));
		bananaTree.draw(&core, W, vp);

		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}
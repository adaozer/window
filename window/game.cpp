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

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout); // Console for debugging

	Window win;
	Core core;
	win.init(1024, 768, 0, 0, "My Window");
	core.init(win.hwnd, win.width, win.height); // Initialise window and core
	GamesEngineeringBase::Timer tim; // Timer for dt
	ShaderManager shaderManager;
	TextureManager tex; // Init shader manager and texture manager to pass into the object inits

	GEMObject bananaTree(&shaderManager, &tex, "textures/banana2_LOD5_ALB.png", "textures/banana2_LOD5_NH.png");
	bananaTree.init(&core, "models/banana2_LOD5.gem"); // Static object, one tree to mark the centre of the map

	AnimatedModel gunModel(&shaderManager, &tex, "textures/AC5_Albedo_alb.png");
	gunModel.load(&core, "models/AutomaticCarbine.gem"); // the gun that we'll be shooting as this is an fps

	AnimatedModel enemyModel(&shaderManager, &tex, "textures/T-rex_Base_Color_alb.png");
	enemyModel.load(&core, "models/TRex.gem"); // Load the TRex model to use as the enemy

	const int treeAmount = 1000;
	const int grassAmount = 10000;
	const float spawnRadius = 500;  // For instancing

	std::vector<INSTANCE> treeInstances;

	for (int i = 0; i < treeAmount; i++) {
		float direction = float(rand()) / RAND_MAX * 2.0f * M_PI; // Which direction from the centre the tree will be placed
		float radius = sqrtf(float(rand()) / RAND_MAX) * 500.0f; // How far from the centre the tree will be placed (random point on the radius)

		float x = cosf(direction) * radius;
		float z = sinf(direction) * radius; // Randomly pick an x and z value in a 500 radius circle

		INSTANCE inst;
		inst.W = Matrix::scale(Vec3(0.015, 0.015, 0.015)) * Matrix::translate(Vec3(x, 0.0f, z)); // Scale/translate them to place

		treeInstances.push_back(inst); 
	}

	std::vector<AABB> treeAABBs; // Initialise collision AABBs for the trees.

	AABB localTree;
	localTree.min = Vec3(-50.f, 0.f, -50.f);
	localTree.max = Vec3(50.f, 200.f, 50.f); // min-max of the tree to create a bounding box for collision math
	for (int i = 0; i < treeAmount; i++) {
		AABB aabbTree; // AABB for each tree
		Vec3 corners[8] = { // Define the 8 corners of the tree like a cube
		Vec3(localTree.min.x, localTree.min.y, localTree.min.z), 
		Vec3(localTree.max.x, localTree.min.y, localTree.min.z),
		Vec3(localTree.min.x, localTree.max.y, localTree.min.z),
		Vec3(localTree.max.x, localTree.max.y, localTree.min.z),
		Vec3(localTree.min.x, localTree.min.y, localTree.max.z),
		Vec3(localTree.max.x, localTree.min.y, localTree.max.z),
		Vec3(localTree.min.x, localTree.max.y, localTree.max.z),
		Vec3(localTree.max.x, localTree.max.y, localTree.max.z)
		};
		for (int j = 0; j < 8; j++) {
			Vec4 worldCorner = treeInstances[i].W.mulPoint(Vec4(corners[j])); // translate to the real world matrix
			aabbTree.extend(Vec3(worldCorner.x, worldCorner.y, worldCorner.z)); // extend the AABB according to the corners
		}
		treeAABBs.push_back(aabbTree); // for collision checking later
	}

	AABB localBanana;
	localBanana.min = Vec3(-50.f, 0.f, -50.f);
	localBanana.max = Vec3(50.f, 200.f, 50.f);
	AABB bananaAABB;
	Vec3 c[8] = { // Build an AABB for the banana tree as well
		Vec3(localBanana.min.x, localBanana.min.y, localBanana.min.z),
		Vec3(localBanana.max.x, localBanana.min.y, localBanana.min.z),
		Vec3(localBanana.min.x, localBanana.max.y, localBanana.min.z),
		Vec3(localBanana.max.x, localBanana.max.y, localBanana.min.z),
		Vec3(localBanana.min.x, localBanana.min.y, localBanana.max.z),
		Vec3(localBanana.max.x, localBanana.min.y, localBanana.max.z),
		Vec3(localBanana.min.x, localBanana.max.y, localBanana.max.z),
		Vec3(localBanana.max.x, localBanana.max.y, localBanana.max.z)
	};
	for (int i = 0; i < 8; i++) {
		Vec4 wc = Matrix::scale(Vec3(0.05, 0.05, 0.05)).mulPoint(Vec4(c[i]));
		bananaAABB.extend(Vec3(wc.x, wc.y, wc.z));
	}
	treeAABBs.push_back(bananaAABB);

	InstancedObject bamboo(&shaderManager, &tex, "textures/bamboo branch_ALB.png", "textures/bamboo branch_NH.png", "pixelshader_normalmapped.hlsl");
	bamboo.init(&core, "models/bamboo.gem", treeInstances); // Tree models

	std::vector<INSTANCE> grassInstances; // Same thing for the grass as we did for tree instances

	for (int i = 0; i < grassAmount; i++) {
		float direction = float(rand()) / RAND_MAX * 2.0f * M_PI;
		float radius = sqrtf(float(rand()) / RAND_MAX) * 500.0f;

		float x = cosf(direction) * radius;
		float z = sinf(direction) * radius;

		INSTANCE inst;
		inst.W = Matrix::scale(Vec3(10.f, 10.f, 10.f)) * Matrix::translate(Vec3(x, 0.f, z));

		grassInstances.push_back(inst);
	}

	InstancedObject grass(&shaderManager, &tex, "textures/TX_GrassClumps_01_ALB.png", "textures/TX_GrassClumps_01_NH.png", "pixelshader_alphatesting.hlsl");
	grass.init(&core, "models/Grass_Clump_01c.gem", grassInstances); // No collision necessary as there will be a lot more grass

	AnimationManager animationManager;
	EnemyManager enemyManager;

	animationManager.set("AutomaticCarbine", AnimationName::Idle, "04 idle");
	animationManager.set("AutomaticCarbine", AnimationName::Attack, "08 fire");
	animationManager.set("AutomaticCarbine", AnimationName::Reload, "17 reload");
	animationManager.set("AutomaticCarbine", AnimationName::Inspect, "05 inspect"); 

	animationManager.set("TRex", AnimationName::Idle, "idle");
	animationManager.set("TRex", AnimationName::Run, "run");
	animationManager.set("TRex", AnimationName::Attack, "attack");
	animationManager.set("TRex", AnimationName::Death,  "death"); // Set the animation names as global names for the animation manager to make them easier to access.

	Gun gun;
	gun.init(&gunModel, animationManager); // Initialise the gun class. (basically player)
	
	BoundingSphere enemyBounds;
	enemyBounds.radius = 4.f; // EnemyBounds for collision checking

	Plane floor(&shaderManager, &tex, "textures/wood_chip_path_diff_4k.png", "textures/wood_chip_path_nor_dx_4k.png");
	floor.init(&core); // This is a plane that I will be using as the floor.

	Sphere skybox(&shaderManager, &tex, "textures/kloofendal_48d_partly_cloudy_puresky.jpg");
	skybox.init(&core, 1000.f); // The skybox sphere. 1000 radius to look more realistic.

	enemyManager.spawn(&enemyModel, animationManager, Vec3(150, 0.0f, 150)); // spawn the enemy using the enemymanager.

	float fov = 90.0f;
	float n = 0.01f;
	float f = 100000.f;
	float aspect = win.width / win.height;
	Matrix p = Matrix::projMatrix(aspect, fov, f, n); // Simple projection matrix command

	Camera cam; // Initialise player camera
	float time = 0.f;

	BoundingSphere playerBounds;
	playerBounds.radius = 0.5;
	playerBounds.centre = cam.pos; // For player collisions

	bool lastMouseDown = false;
	bool centered = false;
	bool lastZPress = false; // Helpers for key presses etc.
	float fpsPrint = 0.f; // fps counter
	while (true) {
		core.beginFrame();

		float dt = tim.dt(); // dt
		float fps = 1.0f / dt; // fps
		fpsPrint += dt; // increment fps counter to print fps once a second

		win.processMessages();
		if (centered) {
			win.centerCursor(); // for an fps, its better to have the cursor invisible and centered at all times.
		}

		if (win.keys[VK_ESCAPE] == 1) break;

		bool zPress = win.keys['Z']; // Pressing Z hides the cursor and centers it for a smoother experience.
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
		bool mouseDown = win.mouseButtons[0]; // Shooting logic

		if (mouseDown && !lastMouseDown) gun.onTriggerPressed();
		if (!mouseDown && lastMouseDown) gun.onTriggerReleased(); // Check if shot fired at this exact frame or not

		lastMouseDown = mouseDown; // Checking exact mousedown frames to not create a gattling gun on accident

		cam.yaw += win.dx * cam.sensitivity;
		cam.pitch += win.dy * cam.sensitivity; // pitch and yaw to move our camera smoothly

		float boundary = (89.f * M_PI) / 180.f;
		if (cam.pitch > boundary) cam.pitch = boundary;
		if (cam.pitch < -boundary) cam.pitch = -boundary; // so looking up/down doesnt break the camera and warp the view.

		win.dx = 0;
		win.dy = 0;

		float yaw = cam.yaw;

		Vec3 forward = Vec3(std::cos(yaw), 0.0f, std::sin(yaw));
		Vec3 right = Vec3(-std::sin(yaw), 0.0f, std::cos(yaw));

		if (!enemyManager.enemies.empty()) {
			enemyBounds.centre = enemyManager.enemies[0].pos;
		}
		else {
			enemyBounds.centre = Vec3(1000, 1000, 1000);
		}

		Vec3 move(0.f, 0.f, 0.f); // move vector that we will apply translations to
		if (win.keys['W']) move += forward; 
		if (win.keys['A']) move += right;
		if (win.keys['S']) move -= forward;
		if (win.keys['D']) move -= right; // simple movement, WASD
		if (win.keys['R']) gun.reload(animationManager); // R to reload
		if (win.keys['F']) gun.inspect(animationManager); // F to inspect
		Vec3 desiredStep(0, 0, 0);
		if (move.lengthSquare() > 0.f) {
			move = move.normalize();
			desiredStep = move * cam.moveSpeed * dt; // Normalise the move vector and get ready to apply it to the player position.
		}

		BoundingSphere candidate = playerBounds;
		candidate.centre = cam.pos + desiredStep;
		candidate.centre.y = 3.5; // Create a temp object. The goal is to check if we would collide with something if we were to apply the step to our current position.
		bool hitTree = false;

		for (const AABB& aabb : treeAABBs) {
			if (sphereAABB_intersect(candidate, aabb)) {
				hitTree = true; // Iterate through the treeAABBs and check if we've hit one.
				break;
			}
		}
		bool hitEnemy = intersectsXZ(candidate, enemyBounds); // same thing for enemies.
		if (!hitTree && !hitEnemy) {
			cam.pos = candidate.centre; // If we haven't hit anything, we can move.
		}
		cam.pos.y = 3.5f; // Manual y = 3.5 as otherwise the camera looks weird
		playerBounds.centre = cam.pos; // Update player bounding sphere as well.

		float radius = 950.f; // This section is so we don't walk outside the skybox.

		float d2 = cam.pos.x * cam.pos.x + cam.pos.z * cam.pos.z; // Calculate distance square
		if (d2 > radius * radius) { 
			float d = sqrtf(d2); // If distance is greater than the radius of the skybox, stop the player from moving forward anymore.
			cam.pos.x = cam.pos.x / d * radius;
			cam.pos.z = cam.pos.z / d * radius;
		}

		Matrix v = cam.viewMatrix(); // view matrix for the camera and the objects
		Matrix camWorld = v.invert(); // This is how we're going to make the gun "attach" to our camera
		Matrix vp = v * p; // VP matrix
		Matrix local = Matrix::scale(Vec3(0.015f, 0.015f, 0.015f)) * Matrix::rotateY(M_PI); // scale and rotate as otherwise it faces the wrong way

		Matrix gunW = local * camWorld; // create gunW to follow the camera at all times
		
		gun.update(dt, animationManager);
		enemyManager.update(dt, animationManager, gun, cam); //update functions for movement/damage/animations etc.

		bool hitEnemyBullet = false; // This is to check if we have hit an enemy with a bullet we've fired.
		if (gun.firedThisFrame && !enemyManager.enemies.empty()) {
			Ray ray;
			ray.init(cam.pos, cam.front()); //Initialise a ray starting from the camera pos where the gun is.

			BoundingSphere s;
			s.centre = enemyManager.enemies[0].pos;
			s.centre.y = 12.f;
			s.radius = 4.f; // Make a bounding box for the enemy to act as their hitbox

			if (raySphereIntersect(ray, s)) enemyManager.enemies[0].takeDamage(gun.damage, animationManager);// If hit, deal damage to the enemy.
		}

		core.beginRenderPass(); // Draw calls for everything.

		gun.draw(&core, gunW, vp); // GunW always follows the camera around
		enemyManager.draw(&core, vp); // enemyPos is calculated elsewhere.

		bamboo.draw(&core, vp); // Bamboos are instanced.

		grass.draw(&core, vp); // Same with grass
		Matrix W;
		floor.draw(&core, W, vp);
		skybox.draw(&core, W, vp); // These start at 0,0,0 and have 1000 radius/xyz

		W = Matrix::scale(Vec3(0.05, 0.05, 0.05));
		bananaTree.draw(&core, W, vp);  // banana Tree to mark the centre.

		if (!gun.isAlive) return 0; // if dead, game over.

		if (fpsPrint > 1.0f) { // Print fps every second
			std::cout << "FPS: " << fps << std::endl; 
			fpsPrint = 0.f;
		}

		core.finishFrame();
	}
	core.flushGraphicsQueue();
	return 0;
}
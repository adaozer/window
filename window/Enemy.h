#pragma once
#include "AnimationManager.h"
#include "Gun.h"
#include "Camera.h"
#include <iostream>

class Enemy {
public:
	AnimatedModel* model = nullptr; // Model of the trex
	AnimationInstance anim; // Animation Instance

	std::string name = "TRex"; // Name of model to call later
	std::string currentAction; // For the animation manager

	bool isAlive = true;
	int health = 200; // health and isAlive to check status
	bool dying = false; // to let the death animation resolve
	Vec3 pos; // Pos to draw

	int damage = 25;
	float moveSpeed = 25.f; // Slower than the player to make the game playable
	float chaseRange = 200.0f; // When the dinosaur will start chasing the player

	bool attacking = false;
	float attackRange = 12.0f; // Range when the dinosaur will attack
	float attackCooldown = 0.0f;
	float attackRate = 2.3f; // Basically invul param for the player
	float yaw = 0.f; // To make it face the player while chasing it

	void init(AnimatedModel* m, AnimationManager& animationManager, const Vec3& startPos) {
		model = m;
		pos = startPos;
		anim.init(&model->animation, 0); // Init model with correct starting position
		play(animationManager, AnimationName::Idle, true, true); // Play idle animation at the start
	}

	void takeDamage(int damage, AnimationManager& animationManager) {
		if (!isAlive) return;
		if (dying) return; // Don't take damage while actually dead/dying
		health -= damage; // Take damage
		if (health <= 0) {
			die(animationManager); // Die if health < 0
		}
	}

	void die(AnimationManager& animationManager) {
		if (dying) return;
		dying = true; // Dying = true to make sure the animation resolves and nothing else happens
		play(animationManager, AnimationName::Death, true, false); // Play death animation
	}

	void draw(Core* core, Matrix& W, Matrix& vp) {
		if (!isAlive) return;
		model->draw(core, &anim, W, vp); // Draw if alive.
	}

	void update(float dt, AnimationManager& animationManager, Gun& gun, Camera& camera) {
		if (!isAlive) return;
		if (!currentAction.empty()) {
			anim.update(currentAction, dt);
		} // update the animation instance
		if (dying) {
			if (anim.animationFinished()) {
				isAlive = false; // if dying animation finished, destroy
			}
			return;
		}
		if (attackCooldown > 0.0f) attackCooldown -= dt; // update attack cooldown

		if (attacking) {
			if (anim.animationFinished()) {
				attacking = false; // If attacking finished, go back to normal and keep chasing
				play(animationManager, AnimationName::Run, false, true);
			}
			return;
		}

		Vec3 toPlayer = camera.pos - pos; // Distance between the player and the enemy
		toPlayer.y = 0.f; // camera y = 3.5 so normalize y
		float dist2 = toPlayer.lengthSquare(); // Calculate distance
		float attack2 = attackRange * attackRange; // Radius for attacking
		float chase2 = chaseRange * chaseRange; // Radius for chasing
		if (dist2 <= attack2) { // If in attacking distance
			if (attackCooldown <= 0.0f) {
				attacking = true; // Play attack animation, deal damage, set attack cooldown
				play(animationManager, AnimationName::Attack, true, false);
				attackCooldown = attackRate;	
				gun.takeDamage(damage);
			}
			else {
				if (anim.animationFinished()) play(animationManager, AnimationName::Idle, false, true);
			}
			return;
		}

		if (dist2 <= chase2 && !attacking) { // If in chasing distance
			Vec3 dir = toPlayer.normalize(); 
			yaw = atan2f(dir.x, dir.z); // To make the enemy face the player as it chases
			pos += dir * moveSpeed * dt; // For walking
			play(animationManager, AnimationName::Run, false, true); // Play run animation
		}
		else { // If player is too far, idle
			play(animationManager, AnimationName::Idle, false, true);
			
		}
	}

	void play(AnimationManager& animationManager, AnimationName action, bool restart, bool loop) {
		if (!animationManager.has(name, action)) return; // Check if the animation actually exists, otherwise crash

		const std::string& realAction = animationManager.get(name, action); // get the actual string (08 fire instead of Attack etc.)
		if (!restart && realAction == currentAction) return;

		currentAction = realAction;
		anim.currentAnimation = currentAction; // Run the action
		anim.looping = loop; // For looping things like running animation while chasing, idle animation etc.

		if (restart) anim.resetAnimationTime(); // Play the new action over the existing action (run should play over idle. 
		// Idle shouldn't play over attack if attack is still ongoing
	}
};

class EnemyManager {
public: // Simple enemy manager
	std::vector<Enemy> enemies; // Enemy array. Right now there is only 1 enemy but this allows extendability if necessary

	void spawn(AnimatedModel* model, AnimationManager& animationManager, const Vec3& spawnPos) {
		Enemy e;
		e.init(model, animationManager, spawnPos);
		enemies.push_back(e); // Init enemy model and add to array
	}

	void update(float dt, AnimationManager& animationManager, Gun& gun, Camera& camera) {
		for (auto& e : enemies) {
			e.update(dt, animationManager, gun, camera); // Run update on each enemy, erase them from the array if they die
		}

		for (size_t i = 0; i < enemies.size();) {
			if (!enemies[i].isAlive) {
				enemies.erase(enemies.begin() + i);
			}
			else {
				i++;
			}
		} 
	}

	void draw(Core* core, Matrix& vp) {
		for (auto& e : enemies) {
			Matrix W = Matrix::scale(Vec3(0.03, 0.03, 0.03)) * Matrix::rotateY(e.yaw) * Matrix::translate(e.pos);
			e.draw(core, W, vp); // Draw enemy. Translate by pos as it updates. RotateY by yaw to make the enemy facing the player
		}
	}
};
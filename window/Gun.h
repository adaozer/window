#pragma once
#include "AnimationManager.h"
#include <iostream>

class Gun {
public:
	AnimatedModel* model = nullptr;
	AnimationInstance anim; // model and animation instance initialise

	std::string name = "AutomaticCarbine";
	std::string actionName; // For animations

	int bullets = 10;
	int bulletCount = 10; // bullet counter

	bool reloading = false;
	float reloadTime = 0.f;
	float reloadLength = 1.875f; // to play the reload animation and functionality

	bool triggerPressed = false;
	bool triggerPressedThisFrame = false;
	float fireRate = 3.3f;
	float shotCooldown = 0.f; // For shooting, to not make it a machine gun and apply damage properly

	bool firedThisFrame = false;

	int health = 100;
	int damage = 5;
	bool isAlive = true; // Player properties since there's no player class

	void onTriggerPressed() { triggerPressed = true;  triggerPressedThisFrame = true; }
	void onTriggerReleased() { triggerPressed = false; triggerPressedThisFrame = false; }

	void init(AnimatedModel* m, AnimationManager& animationManager) {
		model = m;
		anim.init(&model->animation, 0);
		play(animationManager, AnimationName::Idle, true, true); // Init the model and the animation instance. Start by playing an idle animation
	}

	void reload(AnimationManager& animationManager) {
		if (reloading) return;
		if (bullets == bulletCount) return; // If already reloading or mag is full, do nothing
		reloading = true;
		reloadTime = reloadLength; // Reload and play reloading animation
		play(animationManager, AnimationName::Reload, true, false);
	}

	void inspect(AnimationManager& animationManager) {
		if (reloading) return;
		if (triggerPressed) return;
		play(animationManager, AnimationName::Inspect, true, false); // If not reloading or shooting, inspect.
	}

	void draw(Core* core, Matrix& W, Matrix& vp) {
		if (!model) return;
		model->draw(core, &anim, W, vp); // Draw the model
	}

	void update(float dt, AnimationManager& animationManager) {
		firedThisFrame = false;
		if (!actionName.empty())
			anim.update(actionName, dt); // Update the animation instance

		if (shotCooldown > 0.f)
			shotCooldown -= dt; // Shot cooldown timer

		if (reloading)
		{
			reloadTime -= dt;
			if (reloadTime <= 0.f)
			{
				reloading = false;
				bullets = bulletCount;
				reloadTime = 0.f; // For reloading, cant fire while reloading and the animation plays

				play(animationManager, AnimationName::Idle, false, true);
			}

			triggerPressedThisFrame = false;
			firedThisFrame = false; // Set these to false after reloading
			return;
		}

		if (triggerPressedThisFrame || (triggerPressed && shotCooldown <= 0.f))
		{
			shoot(animationManager); // If shooting this exact moment OR trigger held down and shot cooldown is active (no machine gun)
			triggerPressedThisFrame = false; // shoot
			return;
		}

		if (anim.animationFinished())
		{
			play(animationManager, AnimationName::Idle, false, true); // after shootings done, play idle
		}

		triggerPressedThisFrame = false;
	}

	void shoot(AnimationManager& animationManager) {
		if (bullets <= 0) {
			reload(animationManager); // Auto reload if no bullets
			return;
		}
		bullets--; // Subtract a bullet per shot
		shotCooldown = 1.f / fireRate; // Adjust with firerate so not too OP.
		firedThisFrame = true;
		play(animationManager, AnimationName::Attack, true, false); // Shooting animation
	}

	void takeDamage(int damage) {
		health -= damage; // Health - damage, isAlive = false if life runs out
		if (health <= 0) {
			isAlive = false;
		}
	}

	void play(AnimationManager& animationManager, AnimationName action, bool restart, bool loop) {
		if (!animationManager.has(name, action)) return; // Check if action exists

		const std::string& realAction = animationManager.get(name, action); // Get the real filename of the action
		if (!restart && realAction == actionName) return;

		actionName = realAction;
		anim.currentAnimation = actionName;
		anim.looping = loop; // Run the selected action. Loop if necessary

		if (restart) anim.resetAnimationTime(); // If going to play over the existing animation
	}
};
#pragma once
#include "mesh.h"
#include "maths.h"

struct Bone
{
	std::string name;
	Matrix offset;
	int parentIndex;
};
struct Skeleton
{
	std::vector<Bone> bones;
	Matrix globalInverse;

};

struct AnimationFrame
{
	std::vector<Vec3> positions;
	std::vector<Quaternion> rotations;
	std::vector<Vec3> scales;
};

class AnimationSequence
{
public:
	std::vector<AnimationFrame> frames;
	float ticksPerSecond;

	Vec3 interpolate(Vec3 p1, Vec3 p2, float t) {
		return ((p1 * (1.0f - t)) + (p2 * t));
	}
	Quaternion interpolate(Quaternion q1, Quaternion q2, float t)
	{
		return Quaternion::slerp(q1, q2, t);
	}
	float duration() {
		return ((float)frames.size() / ticksPerSecond);
	}

	void calcFrame(float t, int& frame, float& interpolationFact)
	{
		/*interpolationFact = t * ticksPerSecond;
		frame = (int)floorf(interpolationFact);
		interpolationFact = interpolationFact - (float)frame;
		frame = std::min(frame, (int)frames.size() - 1); */
		if (frames.empty() || ticksPerSecond <= 0.0f) {
			frame = 0;
			interpolationFact = 0.0f;
			return;
		}

		float totalTicks = t * ticksPerSecond;
		float frameFloat = fmodf(totalTicks, (float)frames.size());
		if (frameFloat < 0.0f) frameFloat += (float)frames.size();

		frame = (int)frameFloat;
		interpolationFact = frameFloat - frame;
		frame = wrapFrameIndex(frame);

	}
	int getFrameCount() const {
		return static_cast<int>(frames.size());
	}

	int wrapFrameIndex(int idx) const {
		int n = getFrameCount();
		if (n == 0) return 0;              // avoid div by zero
		idx %= n;
		if (idx < 0) idx += n;
		return idx;
	}

	int nextFrame(int baseFrame) const {
		int n = getFrameCount();
		if (n == 0) return 0;
		baseFrame = wrapFrameIndex(baseFrame);
		return wrapFrameIndex(baseFrame + 1);
	}

	Matrix interpolateBoneToGlobal(Matrix* matrices, int baseFrame, float interpolationFact, Skeleton* skeleton, int boneIndex)
	{
		/*Matrix scale = Matrix::scale(interpolate(frames[baseFrame].scales[boneIndex], frames[nextFrame(baseFrame)].scales[boneIndex], interpolationFact));
		Matrix rotation = interpolate(frames[baseFrame].rotations[boneIndex], frames[nextFrame(baseFrame)].rotations[boneIndex], interpolationFact).toMatrix();
		Matrix translation = Matrix::translate(interpolate(frames[baseFrame].positions[boneIndex], frames[nextFrame(baseFrame)].positions[boneIndex], interpolationFact));
		 */
		if (frames.empty()) return Matrix(); // NEW

		baseFrame = wrapFrameIndex(baseFrame);         // NEW
		int nf = nextFrame(baseFrame);                 // NEW

		const AnimationFrame& f0 = frames[baseFrame];  // NEW
		const AnimationFrame& f1 = frames[nf];         // NEW

		// NEW safety block
		if (boneIndex >= f0.positions.size() ||
			boneIndex >= f1.positions.size())
		{
			return Matrix();
		}

		Matrix scale = Matrix::scale(interpolate(f0.scales[boneIndex],
			f1.scales[boneIndex],
			interpolationFact));

		Matrix rotation = interpolate(f0.rotations[boneIndex],
			f1.rotations[boneIndex],
			interpolationFact).toMatrix();

		Matrix translation = Matrix::translate(interpolate(f0.positions[boneIndex],
			f1.positions[boneIndex],
			interpolationFact));
		Matrix local = scale * rotation * translation;
		if (skeleton->bones[boneIndex].parentIndex > -1)
		{
			Matrix global = local * matrices[skeleton->bones[boneIndex].parentIndex];
			return global;
		}
		return local;
	}
};

class Animation
{
public:
	std::map<std::string, AnimationSequence> animations;
	Skeleton skeleton;

	int bonesSize() {
		return skeleton.bones.size();
	}

	void calcFrame(std::string name, float t, int& frame, float& interpolationFact) {
		//animations[name].calcFrame(t, frame, interpolationFact);
		auto it = animations.find(name);
		if (it == animations.end()) {
			frame = 0;
			interpolationFact = 0.0f;
			return;
		}
		it->second.calcFrame(t, frame, interpolationFact);

	}
	/*Matrix interpolateBoneToGlobal(std::string name, Matrix* matrices, int baseFrame, float
		interpolationFact, int boneIndex) {
		return animations[name].interpolateBoneToGlobal(matrices, baseFrame,
			interpolationFact, &skeleton, boneIndex);
	} */
	Matrix interpolateBoneToGlobal(std::string name, Matrix* matrices, int baseFrame,
		float interpolationFact, int boneIndex)
	{
		auto it = animations.find(name);
		if (it == animations.end()) {
			return Matrix();
		}
		return it->second.interpolateBoneToGlobal(matrices, baseFrame,
			interpolationFact, &skeleton, boneIndex);
	}

	void calcFinalTransforms(Matrix* matrices, Matrix coordTransform)
	{
		for (int i = 0; i < bonesSize(); i++)
		{
			matrices[i] = skeleton.bones[i].offset * matrices[i] * skeleton.globalInverse * coordTransform;
		}
	}
};

class AnimationInstance
{
public:
	Animation* animation;
	std::string currentAnimation;
	float t;
	Matrix matrices[256];
	Matrix matricesPose[256];
	Matrix coordTransform;

	void init(Animation* _animation, int fromYZX)
	{
		animation = _animation;
		if (fromYZX == 1)
		{
			memset(coordTransform.a, 0, 16 * sizeof(float));
			coordTransform.a[0][0] = 1.0f;
			coordTransform.a[2][1] = 1.0f;
			coordTransform.a[1][2] = -1.0f;
			coordTransform.a[3][3] = 1.0f;
		}
	}

	void resetAnimationTime()
	{
		t = 0;
	}

	bool animationFinished()
	{
		//if (t > animation->animations[currentAnimation].duration()) return true;
		auto it = animation->animations.find(currentAnimation);
		if (it == animation->animations.end()) return true;
		if (t > it->second.duration()) return true;
		
		return false;
	}

	void update(std::string name, float dt) {
		if (name == currentAnimation) {
			t += dt;
		}
		else {
			currentAnimation = name; t = 0;
		}
		if (animationFinished() == true) { resetAnimationTime(); }
		int frame = 0;
		float interpolationFact = 0;
		animation->calcFrame(currentAnimation, t, frame, interpolationFact);
		//animation->calcFrame(name, t, frame, interpolationFact);
		for (int i = 0; i < animation->bonesSize(); i++)
		{
			matrices[i] = animation->interpolateBoneToGlobal(currentAnimation, matrices, frame, interpolationFact, i);
		} // name
		animation->calcFinalTransforms(matrices, coordTransform);
	}
};
#pragma once

//================================================================================================
// Originally Based on debug_draw DX11 sample code by Guilherme R. Lampert (Public Domain)
// The framework has been extended and modified by D.Moore
//================================================================================================

#include "CommonHeader.h"

//================================================================================
// Time releated functions
//================================================================================

std::int64_t getTimeMicroseconds();

double getTimeSeconds();

// ========================================================
// Key/Mouse input + A simple 3D camera:
// ========================================================

struct Keys
{
	// For the first-person camera controls.
	bool wDown;
	bool sDown;
	bool aDown;
	bool dDown;
	// Flags:
	bool showLabels; // True if object labels are drawn. Toggle with the space bar.
	bool showGrid;   // True if the ground grid is drawn. Toggle with the return key.
};

struct Mouse
{
	enum { MaxDelta = 100 };
	int  deltaX;
	int  deltaY;
	int  lastPosX;
	int  lastPosY;
	bool leftButtonDown;
	bool rightButtonDown;
};

v2 getMousePosition();

struct Time
{
	float seconds;
	std::int64_t milliseconds;
};

struct Camera
{
	//
	// Camera Axes:
	//
	//    (up)
	//    +Y   +Z (forward)
	//    |   /
	//    |  /
	//    | /
	//    + ------ +X (right)
	//  (eye)
	//
	v3 right;
	v3 up;
	v3 forward;
	v3 eye;
	m4x4 viewMatrix;
	m4x4 projMatrix;
	m4x4 vpMatrix;
	f32 fovY;
	f32 aspect;
	f32 nearClip;
	f32 farClip;

	// Frustum planes for clipping:
	enum { A, B, C, D };
	v4 planes[6];

	// Tunable values:
	float movementSpeed = 10.0f;
	float lookSpeed = 10.0f;

	enum MoveDir
	{
		Forward, // Move forward relative to the camera's space
		Back,    // Move backward relative to the camera's space
		Left,    // Move left relative to the camera's space
		Right    // Move right relative to the camera's space
	};

	Camera();

	void pitch(const float angle);

	void rotate(const float angle);

	void move(const MoveDir dir, const float amount);

	void checkKeyboardMovement();

	void checkMouseRotation();

	void resizeViewport(u32 width, u32 height);

	void updateMatrices();

	void look_at(const v3& vTarget);

	v3 getTarget() const;

	bool pointInFrustum(const v3& v) const;

	static v3 rotateAroundAxis(const v3 & vec, const v3 & axis, const float angle);
};



// ========================================================
// The SystemsInterface provide access to
// systems and device contexts.
// ========================================================
struct SystemsInterface
{
	ID3D11Device* pD3DDevice;
	ID3D11DeviceContext* pD3DContext;
	ID3D11RenderTargetView* pSwapRenderTarget; // 
	dd::ContextHandle pDebugDrawContext;
	Camera* pCamera;
	u32 width;
	u32 height;
};

// ========================================================
// Framework Application class inherit from this.
// ========================================================
class FrameworkApp
{
public:
	virtual void on_init(SystemsInterface& rSystems) = 0;
	virtual void on_update(SystemsInterface& rSystems) = 0;
	virtual void on_render(SystemsInterface& rSystems) = 0;
	virtual void on_resize(SystemsInterface& rSystems) = 0;
protected:
private:
};

// ========================================================
// Macro to define entry point.
// ========================================================

int framework_main(FrameworkApp& rApp, const char* pTitleString, HINSTANCE hInstance, int nCmdShow);

#define FRAMEWORK_IMPLEMENT_MAIN(app, appTitle) \
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)\
{\
	return framework_main(app, appTitle, hInstance, nCmdShow); \
}

//================================================================================
// Some Demo Functions for features.
//================================================================================
namespace DemoFeatures {

void editorHud(dd::ContextHandle ctx);

void drawGrid(dd::ContextHandle ctx);

void drawLabel(dd::ContextHandle ctx, ddVec3_In pos, const char * name);

void drawMiscObjects(dd::ContextHandle ctx);

void drawFrustum(dd::ContextHandle ctx);

void drawText(dd::ContextHandle ctx);
}

//================================================================================
// File loading
//================================================================================

// Loads an entire file into an allocated memory block.
memtype_t* load_file(const char* pstrName, u32& rLengthOut, const u32 kAlignment, const u32 kZeroPadding);

// Release a previously allocated block.
void release_loaded_file(memtype_t* ptr);



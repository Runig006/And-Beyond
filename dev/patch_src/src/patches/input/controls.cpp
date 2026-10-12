// Modern controls and soft auto-aim (PATCH OPTIONS > controls: modern, auto aim). The look rows of the control screen
// (any key, pad input or mouse movement; patches/input/gamepad.cpp, controls_menu.cpp) move the camera; while firing Buzz faces the camera and side-steps (the game's L1 / R1 mode), shots go through the
// reticle and bend onto an enemy near it. Hooks PadToPlayerButtons (004331c0), UpdateFollowCamera (00403ef0),
// UpdateGameCamera (00405020), UpdatePlayerAnim (004014a0) and FireWeapon (004a5030). Vehicles keep the game's
// controls with our camera (vehicle aim / vehicle camera); transport rides stay original.
#include "render.h"
#include "config.h"
#include "gamepad.h"
#include "mouse.h"
#include <math.h>

// Game data
#define g_playerAddr A(0x00535c10)
#define g_playerX (*(int *)(g_playerAddr + 0x00))
#define g_playerY (*(int *)(g_playerAddr + 0x04))                    // grows downwards
#define g_playerZ (*(int *)(g_playerAddr + 0x08))
#define g_playerYaw (*(unsigned short *)(g_playerAddr + 0x0e))       // 4096 = one turn
#define g_playerFlags (*(unsigned short *)(g_playerAddr + 0x40))
#define g_playerFacing (*(short *)(g_playerAddr + 0x48))             // wanted heading; PlayerSteer turns yaw to it
#define g_playerButtons (*(unsigned short *)A(0x0052dde8))               // actions of this tick (PadToPlayerButtons)
#define g_frameStep (*(int *)A(0x00535be4))
#define g_padButtons (*(unsigned short *)A(0x0088995c))                  // pad bits of this frame (8 = Start)
#define g_gameState (*(short *)A(0x0053163c))
#define g_introTimer (*(short *)A(0x00535bec))                          // UpdateGameFrame locks the pad while > 0x3c
#define g_cameraScript (*(unsigned short *)A(0x00531676))               // bit 0: intro / cut scene camera (UpdateGameCamera)
#define g_cameraBlend (*(int *)A(0x00508870))                           // > 0 while easing from the previous drawn camera
#define g_drawnCamera ((int *)A(0x0052f620))                            // x y z, then pitch (short; more = looks down), yaw
#define g_moveFlags (*(unsigned int *)A(0x00541160))
#define g_transportRide (*(int *)A(0x00541048))
#define g_strafeHold (*(int *)A(0x00541154))                            // ticks the side-step mode lingers
#define g_aimConeHalfAngle (*(int *)A(0x00503168))                      // FindAimTargets cone
#define g_aimTargets ((int **)A(0x0088d418))                            // FindAimTargets: up to 3, nearest first, 8 bytes apart
#define g_paused (*(short *)A(0x00531678))
#define g_cheatsOn (*(int *)A(0x0088d940))                              // what typing ZEBNEBULA turns on; nothing turns it off
#define g_viewMatrix ((float *)A(0x00a5aa78))                           // what SetCameraMatrices gave the renderer
#define g_projectionMatrix ((float *)A(0x00a5a938))

enum {
	BTN_UP = 0x0010, BTN_RIGHT = 0x0020, BTN_DOWN = 0x0040, BTN_LEFT = 0x0080,
	BTN_STEP_LEFT = 0x0400, BTN_STEP_RIGHT = 0x0800, // L1 / R1
	ACT_FIRE = 0x8000,
	MOVE_VEHICLES = 0x1c, // wings, bike, surf board
	PLAYER_LOCKED = 0x0004,
};

static const float kCameraTurn = 0x24;       // camera turn per tick with a look input fully pushed (4096 = turn)
static const float kCameraTilt = 0x14;       // same for looking up / down
static const float kMouseTurn = 2.0f;        // camera turn per mouse count (800 dpi: a turn in 6.5 cm)
static const float kLookNeutral = 0x80;      // 11 degrees above the game's view (~34 degrees down), which looked too low
static const float kLookUpMax = 0x280;       // from the game's view: 56 degrees up
static const float kLookDownMax = 0x100;     // 22.5 degrees down
static const int kAutoCameraDelay = 150;    // ticks (2.5 s) without firing or looking before the auto camera
static const int kAimCone = 0x40;            // auto-aim: half angle of the cone (0x40 = 5.6 degrees)
static const int kAssistCone = 0x155;        // camera assist: enemies this close to the aim pull the camera (30 degrees)
static const float kAssistPull = 0.05f;      // camera assist: part of the way to the enemy turned per tick
static const float kAssistMaxTurn = 0x10;    // camera assist: at most this much per tick (1.4 degrees)
static const int kAimRange = 0x8000;         // auto-aim: distance, in FindAimTargets units (position >> 5)
static const float kReticleX = 0.5f, kReticleY = 0.42f; // reticle on the screen (fraction of width / height)
static const int kAimBeyond = 0x18000;        // aim point: this far past Buzz along the camera ray (game units)
static const double kAngleToRadians = 6.283185307 / 4096;
static const float kCameraSpeed[3] = {0.6f, 1, 1.5f}; // PATCH OPTIONS > camera speed

typedef int(__cdecl *FindAimTargetsFn)(int *pos, int unused, int range);
typedef int(__cdecl *Atan2AngleFn)(int dx, int dz);
static const FindAimTargetsFn FindAimTargets = (FindAimTargetsFn)A(0x004a71c0);
static const Atan2AngleFn Atan2Angle = (Atan2AngleFn)A(0x004558b0);

static void (*g_padToPlayerOriginal)();
static void (*g_followCameraOriginal)(int *cam);
typedef void(__cdecl *FireWeaponFn)(unsigned angle, int pitch, int *pos, int *offset, int useTargets, int weapon,
                                    int ammoWeapon, int stateFlags);
static FireWeaponFn g_fireOriginal;

static bool g_aiming;
static bool g_mouseMoved;          // the mouse moved the camera this tick
static bool g_reticleVisible;      // a tick on foot with modern controls ran since the last frame
static bool g_vehicleMouse;        // a tick on a vehicle with modern controls ran since the last frame
static int *g_reticleTarget;       // enemy the auto-aim would hit, or null
static int g_gunHeight = -0x1800;  // height of Buzz's shots above his position (measured on each shot)
static bool g_haveCamera;          // g_cameraYaw is valid
static unsigned short g_cameraYaw; // the camera direction we keep (same units and meaning as the player yaw)
static unsigned short g_cameraYawWritten;
static float g_lookPitch = kLookNeutral; // view raised (+) or lowered (-) from the game's camera, 4096 = one turn
static unsigned short g_aimYaw;    // from the camera as drawn (not its target, which runs ahead) towards Buzz
static void (*g_gameCameraOriginal)();

#define g_gameOptions (*(unsigned char *)A(0x005358a8)) // bit 0x40: directional control (else rotational, the tank)

// Modern controls build on the directional scheme: choosing rotational (pause menu) turns them off
static bool Modern()
{
	return g_config.modernControls && (g_gameOptions & 0x40);
}

static bool OnFoot()
{
	return !(g_moveFlags & MOVE_VEHICLES) && !g_transportRide;
}

// PATCH OPTIONS > vehicle camera: steer. On the bike and the board (g_moveFlags & 0xc) the look left / right inputs
// turn the vehicle, and the camera is the game's.
static bool VehicleSteer()
{
	return Modern() && g_config.vehicleSteer && (g_moveFlags & 0xc) && !g_transportRide;
}

// The camera is ours: on foot and on the vehicles, not on transport rides
static bool CameraOurs()
{
	return Modern() && !g_transportRide && !VehicleSteer();
}

// PATCH OPTIONS > vehicle aim: camera. On the vehicles (with our camera) the shots go where the camera looks, through
// the reticle, as on foot; "original" keeps the game's (straight ahead of the vehicle).
static bool VehicleAim()
{
	return CameraOurs() && g_config.vehicleAim && !OnFoot();
}

// The game is driving: intro, cut scene camera, or the pad locked (same test as UpdateGameFrame)
static bool GameDriving()
{
	return (g_cameraScript & 1) || (g_playerFlags & PLAYER_LOCKED) || (g_gameState == 0 && g_introTimer > 0x3c);
}

static void AimPoint(int out[3]);

// Vertical direction from Buzz's gun to the aim point, in FireWeapon's pitch units (vertical per 0x1000 horizontal;
// y grows downwards)
static int AimPitch()
{
	int p[3];
	AimPoint(p);
	double dx = p[0] - g_playerX, dz = p[2] - g_playerZ, flat = sqrt(dx * dx + dz * dz);
	return flat > 1 ? (int)((p[1] - (g_playerY + g_gunHeight)) * 4096.0 / flat) : 0;
}

// Look rows from pad and keys (-1..1) as camera movement per tick: squared, so small pushes move it slowly
static float StickSpeed(float s, float full)
{
	return s * (s < 0 ? -s : s) * full * kCameraSpeed[g_config.cameraSpeed] * g_frameStep;
}

// Mouse movement through the look rows given a mouse movement: counts turning right and looking up
static void MouseLook(int dx, int dy, int *right, int *up)
{
	*right = *up = 0;
	for (int i = 0; i < 4; i++) {
		int m = g_config.mouseBind[BIND_FIRST_LOOK + i];
		int c = m == MI_MOVE_LEFT ? -dx : m == MI_MOVE_RIGHT ? dx : m == MI_MOVE_UP ? -dy : m == MI_MOVE_DOWN ? dy : 0;
		c = c > 0 ? c : 0;
		int bind = BIND_FIRST_LOOK + i;
		*right += bind == BIND_LOOK_RIGHT ? c : bind == BIND_LOOK_LEFT ? -c : 0;
		*up += bind == BIND_LOOK_UP ? c : bind == BIND_LOOK_DOWN ? -c : 0;
	}
}

// Mouse movement as camera movement (whole counts, already per frame)
static float MouseSpeed(int counts)
{
	return counts * kMouseTurn * kCameraSpeed[g_config.cameraSpeed];
}

// Nearest enemy close to the aim (horizontal cone from FindAimTargets, vertical cone checked here), or null.
// FindAimTargets looks along g_player.yaw: the caller makes sure it is the aim direction.
static int *FindAimTarget()
{
	int cone = g_aimConeHalfAngle;
	g_aimConeHalfAngle = kAimCone;
	int found = FindAimTargets((int *)g_playerAddr, 0, kAimRange);
	g_aimConeHalfAngle = cone;
	double aim = atan(AimPitch() / 4096.0), limit = kAimCone * kAngleToRadians * 1.5;
	for (int i = 0; i < found && i < 3; i++) {
		int *t = g_aimTargets[i * 2];
		if (!t) {
			break;
		}
		double dx = t[0] - g_playerX, dy = t[1] - g_playerY, dz = t[2] - g_playerZ;
		double flat = sqrt(dx * dx + dz * dz);
		if (flat > 1 && fabs(atan(dy / flat) - aim) < limit) {
			return t;
		}
	}
	return nullptr;
}

// While firing, the enemy nearest the aim (within kAssistCone) pulls the camera gently towards it; looking
// weakens the pull. Call with g_player.yaw = g_aimYaw (FindAimTargets looks along it).
static void CameraAssist()
{
	int cone = g_aimConeHalfAngle;
	g_aimConeHalfAngle = kAssistCone;
	int found = FindAimTargets((int *)g_playerAddr, 0, kAimRange);
	g_aimConeHalfAngle = cone;
	double aimUp = -atan(AimPitch() / 4096.0); // radians, + = above
	int bestTurn = 0;
	double bestUp = 0, best = 1e9;
	for (int i = 0; i < found && i < 3; i++) {
		int *t = g_aimTargets[i * 2];
		if (!t) {
			break;
		}
		double dx = t[0] - g_playerX, dz = t[2] - g_playerZ, flat = sqrt(dx * dx + dz * dz);
		if (flat <= 1) {
			continue;
		}
		int turn = (((Atan2Angle((int)dx >> 5, (int)dz >> 5) - g_aimYaw) + 0x800) & 0xfff) - 0x800;
		// Up / down from the gun to the target as high above its feet as the gun is above Buzz's: the gun height cancels
		double up = -atan((t[1] - g_playerY) / flat) - aimUp;
		double off = fabs(turn * kAngleToRadians) + fabs(up);
		if (fabs(up) < kAssistCone * kAngleToRadians && off < best) {
			best = off;
			bestTurn = turn;
			bestUp = up;
		}
	}
	if (best > 1e8) {
		return;
	}
	float room = g_mouseMoved ? 0 : 1 - fabsf(g_padExtra.lookX) - fabsf(g_padExtra.lookY);
	if (room <= 0) {
		return;
	}
	float pull = kAssistPull * g_frameStep * room, maxTurn = kAssistMaxTurn * g_frameStep;
	float turn = bestTurn * pull, up = (float)(bestUp / kAngleToRadians) * pull;
	turn = turn > maxTurn ? maxTurn : turn < -maxTurn ? -maxTurn : turn;
	up = up > maxTurn ? maxTurn : up < -maxTurn ? -maxTurn : up;
	static float turnFraction;
	turn += turnFraction;
	int whole = (int)turn;
	turnFraction = turn - whole;
	g_cameraYaw = (unsigned short)((g_cameraYaw + whole) & 0xfff);
	g_lookPitch += up;
	if (g_lookPitch > kLookUpMax) {
		g_lookPitch = kLookUpMax;
	}
	if (g_lookPitch < -kLookDownMax) {
		g_lookPitch = -kLookDownMax;
	}
}

// PadToPlayerButtons (004331c0): turns the pad into this tick's actions; runs before UpdatePlayer
static void PadToPlayerButtons_Modern()
{
	g_padToPlayerOriginal();
	// Esc pauses, like Start (UpdateGameFrame looks at the pad bits right after this; patches/input/gamepad.cpp)
	if (g_escPressed) {
		g_padButtons |= 0x0008;
		g_escPressed = false;
	}
	bool wasAiming = g_aiming;
	g_aiming = false;
	if (!Modern() || GameDriving()) {
		return;
	}
	if (VehicleSteer()) {
		int mouseX, mouseY, mouseRight, mouseUp;
		MouseTakeMove(&mouseX, &mouseY);
		MouseLook(mouseX, mouseY, &mouseRight, &mouseUp);
		static float fraction;
		float turn = StickSpeed(g_padExtra.lookX, kCameraTurn) + MouseSpeed(mouseRight) + fraction;
		int whole = (int)turn;
		fraction = turn - whole;
		g_playerYaw = (unsigned short)((g_playerYaw + whole) & 0xfff);
		g_playerFacing = (short)((g_playerFacing + whole) & 0xfff);
		g_vehicleMouse = true;
	}
	if (!OnFoot()) {
		return; // vehicles keep the original controls otherwise
	}
	if (!(g_playerButtons & ACT_FIRE) || !g_haveCamera) {
		if (wasAiming) {
			g_strafeHold = 0; // back to the classic movement now, not after the side-step mode lingers
		}
		return;
	}

	// Face the camera direction; the stick walks and side-steps instead of turning
	g_aiming = true;
	g_playerYaw = g_aimYaw;
	g_playerFacing = g_aimYaw;
	// From this tick's direction bits (keys, pad and stick past the game's threshold), not g_stickX / g_stickY: the
	// game never resets those when the stick is let go, they keep the last push.
	unsigned short dirs = g_playerButtons;
	unsigned short b = dirs & ~(BTN_LEFT | BTN_RIGHT | BTN_STEP_LEFT | BTN_STEP_RIGHT);
	if (dirs & BTN_RIGHT) {
		b |= BTN_STEP_RIGHT;
	}
	if (dirs & BTN_LEFT) {
		b |= BTN_STEP_LEFT;
	}
	g_playerButtons = b;
	g_strafeHold = 0x1e; // keep the side-step mode on while aiming, even standing still
}

// UpdatePlayerAnim (004014a0) side-steps when g_moveFlags has 0x80 or 0x100 alone. While aiming, a diagonal push
// sets both for the call, so it runs or walks back instead; only a pure sideways push side-steps.
static void (*g_playerAnimOriginal)();
static void UpdatePlayerAnim_Modern()
{
	unsigned int flags = g_moveFlags;
	if (g_aiming && (g_playerButtons & (BTN_UP | BTN_DOWN)) && (flags & 0x180)) {
		g_moveFlags |= 0x180;
	}
	g_playerAnimOriginal();
	g_moveFlags = (g_moveFlags & ~0x180u) | (flags & 0x180);
}

// UpdateFollowCamera(int *cam) (00403ef0) turns cam yaw (+0x28) towards g_player.yaw: for the call the player yaw is
// our camera yaw. While firing the tank bit and "down" are hidden too, so backing away does not bring the camera
// low and close. Up / down look: UpdateGameCamera_Modern.
static void __cdecl UpdateFollowCamera_Modern(int *cam)
{
	unsigned short &camYaw = *(unsigned short *)((char *)cam + 0x28);
	if (!CameraOurs() || GameDriving()) {
		g_followCameraOriginal(cam);
		g_haveCamera = false;
		return;
	}
	if (!g_haveCamera || camYaw != g_cameraYawWritten) { // first time, or the game moved it (level start, cut scene)
		g_cameraYaw = camYaw;
		g_lookPitch = kLookNeutral;
	}
	g_haveCamera = true;

	// Auto camera after a while without firing or looking: the game's camera, view back to level
	int mouseX, mouseY, mouseRight, mouseUp;
	MouseTakeMove(&mouseX, &mouseY);
	MouseLook(mouseX, mouseY, &mouseRight, &mouseUp);
	g_mouseMoved = mouseRight || mouseUp;
	static int idleTicks = kAutoCameraDelay;
	if (g_aiming || g_padExtra.lookX || g_padExtra.lookY || g_mouseMoved) {
		idleTicks = 0;
	} else if (idleTicks < kAutoCameraDelay) {
		idleTicks += g_frameStep;
	}
	bool autoCamera = idleTicks >= kAutoCameraDelay;

	static float yawFraction; // keeps slow turns smooth
	float turn = StickSpeed(g_padExtra.lookX, kCameraTurn) + MouseSpeed(mouseRight) + yawFraction;
	int whole = (int)turn;
	yawFraction = turn - whole;
	g_cameraYaw = (unsigned short)((g_cameraYaw + whole) & 0xfff);
	g_lookPitch += StickSpeed(g_config.invertLook ? -g_padExtra.lookY : g_padExtra.lookY, kCameraTilt);
	g_lookPitch += MouseSpeed(g_config.invertLook ? -mouseUp : mouseUp);
	if (autoCamera) {
		float back = g_frameStep / 30.f;
		g_lookPitch -= (g_lookPitch - kLookNeutral) * (back > 1 ? 1 : back);
	}
	if (g_lookPitch > kLookUpMax) {
		g_lookPitch = kLookUpMax;
	}
	if (g_lookPitch < -kLookDownMax) {
		g_lookPitch = -kLookDownMax;
	}

	unsigned short playerYaw = g_playerYaw;
	unsigned int moveFlags = g_moveFlags;
	if (!autoCamera) {
		camYaw = g_cameraYaw;
		g_playerYaw = g_cameraYaw;
	}
	unsigned short buttons = g_playerButtons;
	if (g_aiming) {
		g_moveFlags &= ~1u;
		g_playerButtons &= ~BTN_DOWN;
	}
	g_followCameraOriginal(cam);
	g_moveFlags = moveFlags;
	g_playerButtons = buttons;
	g_cameraYaw = g_cameraYawWritten = camYaw; // in auto mode the game turned it: take over from there
	g_playerYaw = playerYaw;
	if (!OnFoot()) { // vehicles: the camera, and the aim only if they fire where it looks
		g_vehicleMouse = true;
		if (!VehicleAim()) {
			return;
		}
	}

	// Aim from the camera as it is now (UpdateGameCamera copies it to the drawn camera right after this)
	g_aimYaw = (unsigned short)(Atan2Angle((g_playerX - cam[0]) >> 5, (g_playerZ - cam[2]) >> 5) & 0xfff);
	g_playerYaw = g_aimYaw; // FindAimTargets looks along the player yaw
	g_reticleTarget = g_config.autoAim ? FindAimTarget() : nullptr;
	if (g_config.autoAim && g_aiming) {
		CameraAssist();
	}
	g_playerYaw = playerYaw;
	g_reticleVisible = true;
}

// The camera ray through the reticle, from the matrices of the last frame drawn: origin and direction in game units
static bool CameraRay(double org[3], double dir[3])
{
	// Inverse of view x projection (row vectors, as the game builds them), then unproject the reticle at depth 0 and 1
	double m[4][8];
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			double s = 0;
			for (int k = 0; k < 4; k++) {
				s += (double)g_viewMatrix[r * 4 + k] * g_projectionMatrix[k * 4 + c];
			}
			m[r][c] = s;
			m[r][c + 4] = r == c;
		}
	}
	for (int c = 0; c < 4; c++) { // Gauss-Jordan
		int p = c;
		for (int r = c + 1; r < 4; r++) {
			if (fabs(m[r][c]) > fabs(m[p][c])) {
				p = r;
			}
		}
		if (fabs(m[p][c]) < 1e-12) {
			return false;
		}
		for (int k = 0; k < 8; k++) {
			double tmp = m[c][k];
			m[c][k] = m[p][k];
			m[p][k] = tmp;
		}
		double d = m[c][c];
		for (int k = 0; k < 8; k++) {
			m[c][k] /= d;
		}
		for (int r = 0; r < 4; r++) {
			if (r != c) {
				double f = m[r][c];
				for (int k = 0; k < 8; k++) {
					m[r][k] -= f * m[c][k];
				}
			}
		}
	}
	double nx = kReticleX * 2 - 1, ny = 1 - kReticleY * 2, pts[2][3];
	for (int z = 0; z < 2; z++) {
		double v[4] = {nx, ny, (double)z, 1}, w[4] = {};
		for (int c = 0; c < 4; c++) {
			for (int k = 0; k < 4; k++) {
				w[c] += v[k] * m[k][c + 4];
			}
		}
		if (fabs(w[3]) < 1e-12) {
			return false;
		}
		for (int c = 0; c < 3; c++) {
			pts[z][c] = w[c] / w[3] * 32; // the renderer gets positions / 32
		}
	}
	double len = 0;
	for (int c = 0; c < 3; c++) {
		org[c] = pts[0][c];
		dir[c] = pts[1][c] - pts[0][c];
		len += dir[c] * dir[c];
	}
	len = sqrt(len);
	if (len < 1e-6) {
		return false;
	}
	for (int c = 0; c < 3; c++) {
		dir[c] /= len;
	}
	return true;
}

// Where the shots go: on the camera ray through the reticle, kAimBeyond past Buzz (straight ahead if no frame yet)
static void AimPoint(int out[3])
{
	double org[3], dir[3];
	if (!CameraRay(org, dir)) {
		double a = g_aimYaw * kAngleToRadians;
		out[0] = g_playerX + (int)(sin(a) * kAimBeyond);
		out[1] = g_playerY + g_gunHeight;
		out[2] = g_playerZ + (int)(cos(a) * kAimBeyond);
		return;
	}
	double toBuzz = (g_playerX - org[0]) * dir[0] + (g_playerY - org[1]) * dir[1] + (g_playerZ - org[2]) * dir[2];
	double d = (toBuzz > 0 ? toBuzz : 0) + kAimBeyond;
	for (int c = 0; c < 3; c++) {
		out[c] = (int)(org[c] + dir[c] * d);
	}
}

// UpdateGameCamera (00405020) ends by copying the follow camera to the drawn camera. Up / down look changes only that
// copy: it orbits around Buzz's head by g_lookPitch and tilts as much; near the ground it only tilts.
static void UpdateGameCamera_Modern()
{
	g_gameCameraOriginal();
	if (!CameraOurs() || GameDriving() || !g_haveCamera || g_cameraBlend > 0) {
		return;
	}
	double tilt = g_lookPitch * kAngleToRadians; // + = look up = camera lower
	double pivotY = g_playerY - 0x3000;          // what the follow camera looks at (Buzz's head)
	double vx = g_drawnCamera[0] - g_playerX, vy = g_drawnCamera[1] - pivotY, vz = g_drawnCamera[2] - g_playerZ;
	double flat = sqrt(vx * vx + vz * vz), dist = sqrt(flat * flat + vy * vy);
	if (flat < 1 || dist < 1) {
		return;
	}
	double elevation = atan2(-vy, flat); // above the head > 0 (y grows downwards)
	double lowest = -5 * 3.14159265 / 180;
	double orbit = tilt;
	if (elevation - orbit < lowest) {
		orbit = elevation - lowest > 0 ? elevation - lowest : 0;
	}
	double e = elevation - orbit, k = dist * cos(e) / flat;
	g_drawnCamera[0] = g_playerX + (int)(vx * k);
	g_drawnCamera[2] = g_playerZ + (int)(vz * k);
	g_drawnCamera[1] = (int)(pivotY - dist * sin(e));
	short &pitch = *(short *)&g_drawnCamera[3];
	pitch = (short)((pitch - (int)g_lookPitch) & 0xfff);
}

void DrawReticleAt(float cx, float cy, bool locked); // patches/hud/reticle.cpp

// Aiming reticle, drawn on the finished frame where the shots go
static void DrawReticle()
{
	bool show = g_reticleVisible && g_haveCamera;
	g_reticleVisible = false; // set again by the next tick on foot
	bool vehicle = g_vehicleMouse;
	g_vehicleMouse = false;
	MouseCapture((show || vehicle) && !g_paused); // the pointer stays in the window while the mouse moves the camera or fires
	if (!show) {
		return;
	}
	DrawReticleAt(kReticleX * g_render.Width(), kReticleY * g_render.Height(), g_reticleTarget != nullptr);
}

// FireWeapon (004a5030). Only Buzz's own straight shots (called from the player's weapon update, 004a3350..004a4589,
// without homing targets) are aimed: up / down with the camera, and bent onto a target near the reticle.
static void __cdecl FireWeapon_Aimed(unsigned angle, int pitch, int *pos, int *offset, int useTargets, int weapon,
                                     int ammoWeapon, int stateFlags)
{
	DWORD caller = (DWORD)_ReturnAddress();
	bool fromBuzz = caller >= A(0x004a3350) && caller < A(0x004a4589);
	if (fromBuzz) {
		g_gunHeight = pos[1] + offset[1] - g_playerY;
	}
	if (fromBuzz && !useTargets) {
		bool vehicleAim = VehicleAim();
		if (g_aiming || vehicleAim) {
			// From the gun (right hand, not Buzz's centre) to the point under the reticle, so the shot goes through it
			int p[3];
			AimPoint(p);
			int dx = p[0] - (pos[0] + offset[0]), dy = p[1] - (pos[1] + offset[1]), dz = p[2] - (pos[2] + offset[2]);
			double flat = sqrt((double)dx * dx + (double)dz * dz);
			if (flat > 1) {
				angle = Atan2Angle(dx >> 5, dz >> 5) & 0xfff;
				pitch = (int)(dy * 4096.0 / flat);
			}
		}
		unsigned short yaw = g_playerYaw; // FindAimTarget looks along it: Buzz faces the aim when g_aiming,
		if (vehicleAim) {                   // a vehicle does not
			g_playerYaw = g_aimYaw;
		}
		int *t = g_config.autoAim ? FindAimTarget() : nullptr;
		g_playerYaw = yaw;
		if (t) { // from the gun to the target, as high above its feet as the gun is above Buzz's
			int dx = t[0] - (pos[0] + offset[0]), dz = t[2] - (pos[2] + offset[2]);
			int dy = t[1] + g_gunHeight - (pos[1] + offset[1]);
			double flat = sqrt((double)dx * dx + (double)dz * dz);
			if (flat > 1) {
				angle = Atan2Angle(dx >> 5, dz >> 5) & 0xfff;
				pitch = (int)(dy * 4096.0 / flat);
			}
		}
		if (pitch > 0x1800) {
			pitch = 0x1800;
		}
		if (pitch < -0x1800) {
			pitch = -0x1800;
		}
	}
	g_fireOriginal(angle, pitch, pos, offset, useTargets, weapon, ammoWeapon, stateFlags);
}

extern "C" void InstallControls(void)
{
	if (g_config.cheats) {
		g_cheatsOn = 1;
		LogLine("cheats on");
	}
	static const unsigned char padBytes[] = {0xa0, 0xa8, 0x58, 0x53, 0x00};    // mov al, [005358a8]
	static const unsigned char cameraBytes[] = {0xa0, 0x60, 0x11, 0x54, 0x00}; // mov al, [00541160]
	static const unsigned char fireBytes[] = {0x8b, 0x44, 0x24, 0x10, 0x8b, 0x4c, 0x24, 0x0c}; // mov eax/ecx, [esp+..]
	g_padToPlayerOriginal = (void (*)())HookTrampoline(A(0x004331c0), (const void *)PadToPlayerButtons_Modern, padBytes,
	                                                   5, "modern controls (PadToPlayerButtons)");
	static const unsigned char gameCameraBytes[] = {0x0f, 0xbf, 0x05, 0x74, 0x16, 0x53, 0x00}; // movsx eax, [00531674]
	g_gameCameraOriginal = (void (*)())HookTrampoline(A(0x00405020), (const void *)UpdateGameCamera_Modern,
	                                                  gameCameraBytes, 7, "camera look up / down (UpdateGameCamera)");
	g_followCameraOriginal = (void (*)(int *))HookTrampoline(A(0x00403ef0), (const void *)UpdateFollowCamera_Modern,
	                                                         cameraBytes, 5, "modern camera (UpdateFollowCamera)");
	static const unsigned char animBytes[] = {0x51, 0x53, 0x55, 0x56, 0x57}; // push ecx/ebx/ebp/esi/edi
	g_playerAnimOriginal = (void (*)())HookTrampoline(A(0x004014a0), (const void *)UpdatePlayerAnim_Modern, animBytes, 5,
	                                                  "side-step animation only sideways (UpdatePlayerAnim)");
	g_render.AddBeforePresent(DrawReticle);
	g_fireOriginal = (FireWeaponFn)HookTrampoline(A(0x004a5030), (const void *)FireWeapon_Aimed, fireBytes, 8,
	                                              "aimed shots (FireWeapon)");
}

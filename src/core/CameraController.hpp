#pragma once

#include "ITickable.hpp"
#include "input/KeyboardHandler.hpp"
#include "utils/Macro.hpp"

class KeyboardHandler;
class CursorPositionHandler;
class Camera;

class CameraController : public ITickable
{
public:
	CameraController() = default;
	CameraController(Camera* camera, KeyboardHandler* khandler, CursorPositionHandler* chandler);
	OnlyMovable(CameraController)
	~CameraController();
	void init(Camera* camera, KeyboardHandler* keyboard_handler, CursorPositionHandler* cursor_handler);
	void tick() override;
private:
	void handle_cursor_move(double newx, double newy, double oldx, double oldy);
private:
	// from where we get input callbacks
	CursorPositionHandler* m_cursor_handler = nullptr;
	KeyboardHandler* m_keyboard_handler = nullptr;
	Camera* m_camera = nullptr;
	void* m_cursor_pos_listener = nullptr;
};

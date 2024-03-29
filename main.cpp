#include <Windows.h>
#include <thread>
#include <cassert>
//#include "video_render_util.h"
#include "websocket_server.h"
#include <future>
#include "display_manager/layout_manager.h"
#include "display_manager/window_creater.h"
#include "json_parser.h"
#include "display_manager/win32_window.h"
#include <map>

constexpr const wchar_t* WNDCLASS_NAME = L"Sample Window Class";
const std::wstring child_id{ L"test" };
#define ID_CLOSE 1101
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace grt {
	bool show_window(HWND hwnd, bool to_show) {
		assert(hwnd);
		const auto flags = to_show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
		const auto second_flag = to_show ? HWND_TOPMOST : HWND_NOTOPMOST;
		const auto ret = SetWindowPos(hwnd,
			second_flag,
			0,
			0,
			0,
			0,
			SWP_NOSIZE | flags
		);
		assert(ret);
		if (to_show)
			PostMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		return ret;
	}
}//namespace grt

std::vector<std::string> 
id_generator(const int n) {
	std::vector<std::string> ret(n);
	assert(ret.size() == n);
	std::generate(ret.begin(), ret.end(), []() {
		static int i = 0;
		i++;
		return std::string{ "random" } +std::to_string(i);
		
	});

	return ret;
}

std::vector<display::window*> 
get_windows(display::window_creator* creator, std::vector<std::string> ids) {
	assert(creator);
	std::vector<display::window*> windows;
	std::transform(ids.begin(), ids.end(), std::back_inserter(windows), [creator](std::string id) {
		return creator->create_window(id);
	});
	assert(windows.size() == ids.size());
	return windows;
}

//constexpr const char* REC_ID = "3245";
#if 1

struct windows_store {
	std::vector<display::window*> windows_;
	~windows_store() {
		std::transform(windows_.begin(), windows_.end(), windows_.begin(), [](auto ptr) {
			delete ptr;
			ptr = nullptr;
			return ptr;
		});

	}
	windows_store() = default;
	windows_store(std::vector<display::window*> windows):windows_{windows}{}

	void add(display::window* wind) {
		windows_.push_back(wind);
	}

	bool remove(display::window* wnd) {
		auto iter = std::find(windows_.begin(), windows_.end(), wnd);
		assert(iter != windows_.end());
		windows_.erase(iter);
		return true;
	}

	display::window* get_window() {
		//returns first available window
		if (windows_.empty()) return nullptr;
		display::window* r = windows_.front();
		windows_.erase(windows_.begin());
		return r;
	}

};

HWND create_leave_window(HWND parent_wnd) {
	auto leave_window = ::CreateWindowEx(WS_EX_TOPMOST, L"Button", L"Leave", WS_CHILD | WS_TABSTOP, 0, 0, 0, 0,
		parent_wnd, reinterpret_cast<HMENU>(ID_CLOSE), GetModuleHandle(NULL), NULL);
	return leave_window;
}

util::func_thread_handler* client_ = nullptr;

void set_server_client_handle(util::func_thread_handler* client) {
	assert(client_ == nullptr);
	client_ = client;
	assert(client_);
}

void send_message_to_client(std::string const& m) {
	assert(client_);
	client_->dispatch(UI_SERVER_ID, m);
}

class server_handler : public grt::parser_callback {
private:
	 util::func_thread_handler func_object_;
	 std::future<void> handle_server_;
	 std::unique_ptr<display::window_creator> main_wnd_;	
	 windows_store availabl_wnds_;
	 windows_store used_wnds_;
	 display::win32_window leave_btn_;
	 display::layout_manager layout_{ display::get_desktop_width(),display::get_desktop_height(),
		 display::get_desktop_width(),display::get_desktop_height() };
	 std::map<std::string, display::window*> window_map_;
	 const int leave_btn_x_{ 0 };
	 const int leave_btn_y_{ 0 };
	 std::unique_ptr<display::window> ui_wnd_;
public:
	explicit server_handler(std::unique_ptr<display::window_creator> main_wnd_, int child_wnd_count);
	void start_server(unsigned short port);

	void message_from_server(std::string msg);

	//parser callback interface
	void on_message(grt::message_type, absl::any msg) override;

};

server_handler::server_handler(std::unique_ptr<display::window_creator> main_wnd_, int child_wnd_count)
	:main_wnd_{ std::move(main_wnd_) }, 
	availabl_wnds_{ get_windows(this->main_wnd_.get(), id_generator(child_wnd_count)) },
	leave_btn_{ create_leave_window(this->main_wnd_.get()->get_handle()),"Leave"},
	leave_btn_x_{ display::get_desktop_width() / 2 - 30 },
	leave_btn_y_{ display::get_desktop_height() - 120 },
	ui_wnd_{ this->main_wnd_.get()->create_window("ui_wnd") }{
	
	set_server_client_handle(&func_object_);
	layout_.add(ui_wnd_.get());
	

}

void server_handler::start_server(unsigned short port) {
	func_object_.register_id(
		REC_ID,
		std::bind(&server_handler::message_from_server, this, std::placeholders::_1)
	);
	
	handle_server_ = std::async(std::launch::async, &grt::start_server_block, port, 1, &func_object_, REC_ID);
}

void server_handler::message_from_server(std::string msg) {
	//assert(false);
	grt::async_parse_message(msg, this);

}

void server_handler::on_message(grt::message_type type, absl::any msg) {
	switch (type) {
	case grt::message_type::wnd_create_req:
	{
		const auto id = absl::any_cast<std::string>(msg);
		auto ptr = availabl_wnds_.get_window();
		if (ptr == nullptr || window_map_.find(id) != window_map_.end()) {
			//todo return failure
			const auto m = grt::make_render_wnd_req_res(false, std::string{}, std::string{}, std::string{}, id);
			func_object_.dispatch(UI_SERVER_ID, m);
			if(ptr) availabl_wnds_.add(ptr);//add back window
			return;
		}
		assert(ptr);
		
		used_wnds_.add(ptr);
		layout_.add(ptr);
		leave_btn_.reposition(leave_btn_x_, leave_btn_y_, 70, 20);
		//availabl_wnds_.remove(ptr);

		{
			const auto r = window_map_.emplace(id, ptr);
			assert(r.second);//no element should exist before insertion for the same id.
		}

		std::wstring class_name{ display::get_default_class_name() };
		std::wstring wnd_name{ display::get_default_wnd_name() };
		const auto m = grt::make_render_wnd_req_res(true, std::string{ class_name.begin(), class_name.end() },
			std::string{ wnd_name.begin(), wnd_name.end() },
			ptr->get_window_name(), id );
		func_object_.dispatch(UI_SERVER_ID, m);

		}
	break;

	case grt::message_type::wnd_close_req:
	{
		const auto id = absl::any_cast<std::string>(msg);
		auto iter = window_map_.find(id);
		const bool isValidClose = iter != window_map_.end();
		
		if (isValidClose) {
			auto wnd = iter->second;
			layout_.remove(wnd);
			used_wnds_.remove(wnd);
			availabl_wnds_.add(wnd);
			window_map_.erase(id);
			leave_btn_.reposition(leave_btn_x_, leave_btn_y_, 70, 20);
		}
		const auto m = grt::make_render_wnd_close_res(isValidClose, id);
		if(func_object_.is_dispatch_id_exists(UI_SERVER_ID))
			func_object_.dispatch(UI_SERVER_ID, m);
		else {
			//todo: log thisl
			
		}

	}
		break;

	case grt::message_type::wnd_show_hide:
	{
		const auto to_show = absl::any_cast<bool>(msg);
		if (to_show) {
			if(layout_.is_exists(ui_wnd_.get()))
				layout_.remove(ui_wnd_.get());
			leave_btn_.reposition(leave_btn_x_, leave_btn_y_, 70, 20);
		}
		else {
			layout_.add(ui_wnd_.get());
		}
	}
	break;

	}
}

#endif//

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = WNDCLASS_NAME;

	RegisterClass(&wc);

	auto creator = display::get_wnd_creator(hInstance, WNDCLASS_NAME);
	auto hwnd = creator->get_handle();
	
	//
	if (hwnd == NULL) {
		return 0;
	}
	server_handler server_handler_{ std::move(creator), 6 };

	//auto got_hwd = FindWindow(display::get_default_class_name(), display::get_default_wnd_name());
	//assert(hwnd == got_hwd);

	//auto p = creator->create_window( L"Anil");
	//assert(p);
	//auto child_wnd = find_child_window(display::get_default_class_name(), display::get_default_wnd_name(), L"anil");
	//assert(child_wnd);
	
	
	server_handler_.start_server(8002);

	ShowWindow(hwnd, nCmdShow);

	//p->reposition(24, 50, 800, 600);

	// Run the message loop.

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == ID_CLOSE) {
			//MessageBoxA(0, "Close Button Clicked...", "	Clicked... ", 0);
			const auto m = grt::make_user_session_leave_req();
			send_message_to_client(m);
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
	}
	return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
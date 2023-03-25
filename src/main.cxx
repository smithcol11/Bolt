#include <fmt/core.h>

#include "browser.hxx"
#include "browser/app.hxx"
#include "browser/client.hxx"
#include "browser/handler/render_process_handler.hxx"
#include "browser/handler/request_handler.hxx"
#include "include/cef_render_process_handler.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(CEF_X11)
#include <X11/Xlib.h>

int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
	fmt::print(
		"X error received: type {}, serial {}, error_code {}, request_code {}, minor_code {}\n",
		event->type,
		event->serial,
		event->error_code,
		event->request_code,
		event->minor_code
	);
	return 0;
}

int XIOErrorHandlerImpl(Display* display) {
	return 0;
}
#endif

int main(int argc, char* argv[]) {
	// Provide CEF with command-line arguments
	cef_main_args_t main_args;
	main_args.argc = argc;
	main_args.argv = argv;

	// Set up our app struct
	Browser::RenderProcessHandler rph_;
	CefRefPtr<CefRenderProcessHandler> rph = & rph_;
	Browser::App cef_app_(rph);
	CefRefPtr<CefApp> cef_app = &cef_app_;

	// CEF applications have multiple sub-processes (render, GPU, etc) that share the same executable.
	// This function checks the command-line and, if this is a sub-process, executes the appropriate logic.
	int exit_code = CefExecuteProcess(main_args, cef_app, nullptr);
	if (exit_code >= 0) {
		return exit_code;
	}

#if defined(CEF_X11)
	// X11 error handlers
	XSetErrorHandler(XErrorHandlerImpl);
	XSetIOErrorHandler(XIOErrorHandlerImpl);
#endif

	// Parse command-line arguments
	CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
#if defined(OS_WIN)
	command_line->InitFromString(::GetCommandLineW());
#else
	command_line->InitFromArgv(argc, argv);
#endif

	// CEF settings - only set the ones we're interested in
	CefSettings settings = CefSettings();
	settings.log_severity = LOGSEVERITY_WARNING; // Print warnings and errors only
	//settings.external_message_pump = true;     // Allows us to integrate CEF's event loop into our own
	settings.command_line_args_disabled = true;  // We don't want our command-line to configure CEF's windows
	settings.uncaught_exception_stack_size = 8;  // Number of call stack frames given in unhandled exception events

	// Initialize CEF
	exit_code = CefInitialize(main_args, settings, cef_app, nullptr);
	if (exit_code == 0) {
		fmt::print("Exiting with error: cef_initialize exit_code {}\n", exit_code);
		return exit_code;
	}

	// Our CEF client and the various things it needs pointers to
	Browser::LifeSpanHandler life_span_handler_;
	CefRefPtr<CefLifeSpanHandler> life_span_handler = &life_span_handler_;
	Browser::RequestHandler request_handler_;
	CefRefPtr<CefRequestHandler> request_handler = &request_handler_;
	Browser::Client client_(life_span_handler, request_handler);
	CefRefPtr<CefClient> client = &client_;

	// Spawn a window using the "views" pipeline
	Browser::Details details = {
		.min_width = 800,
		.min_height = 608,
		.max_width = 800,
		.max_height = 608,
		.preferred_width = 800,
		.preferred_height = 608,
		.startx = 100,
		.starty = 100,
		.resizeable = false,
		.frame = true,
		.controls_overlay = true,
	};
	Browser::Window win(client, details);

	// Run the CEF message loop
	// TODO: later this will be replaced with an OS-specific event loop capable of calling
	// cef_do_message_loop_work() in response to CEF's "message pump"
	CefRunMessageLoop();

	// Release things we still own, then shut down CEF
	CefShutdown();

	return 0;
}

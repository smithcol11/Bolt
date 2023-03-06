#include "app.hxx"

Browser::App* resolve_app(cef_app_t* app) {
	return reinterpret_cast<Browser::App*>(reinterpret_cast<size_t>(app) - offsetof(Browser::App, cef_app));
}

Browser::App* resolve_base(cef_base_ref_counted_t* base) {
	return reinterpret_cast<Browser::App*>(reinterpret_cast<size_t>(base) - (offsetof(Browser::App, cef_app) + offsetof(cef_app_t, base)));
}

Browser::App::App() {
	this->cef_app.base.size = sizeof(cef_base_ref_counted_t);
	this->cef_app.base.add_ref = Browser::AddRef;
	this->cef_app.base.release = Browser::Release;
	this->cef_app.base.has_one_ref = Browser::HasOneRef;
	this->cef_app.base.has_at_least_one_ref = Browser::HasAnyRefs;
	this->cef_app.on_before_command_line_processing = Browser::OnBeforeCommandLineProcessing;
	this->cef_app.on_register_custom_schemes = Browser::OnRegisterCustomSchemes;
	this->cef_app.get_resource_bundle_handler = Browser::ResourceBundleHandler;
	this->cef_app.get_browser_process_handler = Browser::BrowserProcessHandler;
	this->cef_app.get_render_process_handler = Browser::RenderProcessHandler;
	this->refcount = 0;
}

void Browser::App::Destroy() {
	// Any self-cleanup should be done here
}

cef_app_t* Browser::App::app() {
	this->refcount += 1;
	return &this->cef_app;
}

void Browser::AddRef(cef_base_ref_counted_t* app) {
	resolve_base(app)->refcount += 1;
}

int Browser::Release(cef_base_ref_counted_t* app) {
	Browser::App* _app = resolve_base(app);
	_app->refcount -= 1;

	if (_app->refcount == 0) {
		_app->Destroy();
		return 1;
	}

	return 0;
}

int Browser::HasOneRef(cef_base_ref_counted_t* app) {
	return (resolve_base(app)->refcount == 1) ? 1 : 0;
}

int Browser::HasAnyRefs(cef_base_ref_counted_t* app) {
	return (resolve_base(app)->refcount >= 1) ? 1 : 0;
}

void Browser::OnBeforeCommandLineProcessing(cef_app_t*, const cef_string_t*, cef_command_line_t*) { }

void Browser::OnRegisterCustomSchemes(cef_app_t*, cef_scheme_registrar_t*) { }

cef_resource_bundle_handler_t* Browser::ResourceBundleHandler(cef_app_t*) {
	return nullptr;
}

cef_browser_process_handler_t* Browser::BrowserProcessHandler(cef_app_t*) {
	return nullptr;
}

cef_render_process_handler_t* Browser::RenderProcessHandler(cef_app_t*) {
	return nullptr;
}

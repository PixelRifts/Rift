import "standard.cp";
import "gs.cp";

GS.gs_command_buffer_t cb;
GS.gs_immediate_draw_t imm;

namespace Yeet {
    enum test_enum {
        blah
    }
    
    struct heyo {
        int a;
    }
}

void init() {
	using GS;
	cb = gs_command_buffer_new();
	imm = gs_immediate_draw_new();
}

void update() {
	using GS;
	
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) gs_engine_quit();
    
	gsi_camera2D(&imm);
	gsi_rectv(&imm, gs_v2(50.f, 50.f), gs_v2(150.f, 150.f), gs_color(255 * 0.8f, 255 * 0.2f, 255 * 0.3f, 255 * 1.0f), GraphicsPrimitive.GS_GRAPHICS_PRIMITIVE_TRIANGLES);
	gsi_render_pass_submit(&imm, &cb, gs_color(10, 10, 10, 255));
    
	gs_graphics_submit_command_buffer(&cb);
}

int main() {
	using GS;
	gs_app_desc_t app; // Fill this with whatever your app needs
	app.init = init;
	app.update = update;
    
    gs_engine_create(app); // Create instance of engine for framework and run
	while (gs_engine_app()->is_running) {
	    gs_engine_frame();
	}
	return 0;
}
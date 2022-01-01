cinsert "#define GS_NO_HIJACK_MAIN";
cinsert "#define GS_IMPL";
cinclude "gs/gs.h";

cinsert "#define GS_IMMEDIATE_DRAW_IMPL";
cinclude "gs/util/gs_idraw.h";

namespace GS {
	native struct gs_app_desc_t {
		^void() init;
		^void() update;
		bool is_running;
	}
    
	native struct gs_command_buffer_t;
	native struct gs_immediate_draw_t;
    
	native struct gs_vec2    { float x; float y; }
	native struct gs_vec3    { float x; float y; float z; }
	native struct gs_vec4    { float x; float y; float z; float w; }
	native struct gs_color_t { float r; float g; float b; float a; }
    
	native gs_vec2 gs_v2(float x, float y);
	native gs_vec3 gs_v3(float x, float y, float z);
	native gs_vec4 gs_v4(float x, float y, float z, float w);
	native gs_color_t gs_color(float r, float g, float b, float a);
    
	// This is wack
	int GS_KEYCODE_ESC = 51;
    native enum GraphicsPrimitive {
        _gs_gs_graphics_primitive_type_default,
        GS_GRAPHICS_PRIMITIVE_LINES,
        GS_GRAPHICS_PRIMITIVE_TRIANGLES,
        GS_GRAPHICS_PRIMITIVE_QUADS,
        _gs_gs_graphics_primitive_type_count,
    }
    
	native void gs_engine_create(gs_app_desc_t app);
	native gs_app_desc_t* gs_engine_app();
	native void gs_engine_frame();
	native bool gs_platform_key_pressed(int key);
	native void gs_engine_quit();
    
	native gs_command_buffer_t gs_command_buffer_new();
	native gs_immediate_draw_t gs_immediate_draw_new();
	native void gs_graphics_submit_command_buffer(gs_command_buffer_t* cb);
    
	native void gsi_camera2D(gs_immediate_draw_t* imm);
	native void gsi_defaults(gs_immediate_draw_t* imm);
    
	native void gsi_rect(gs_immediate_draw_t* imm, float x0, float y0, float x1, float y1, float r, float g, float b, float a, int primitive);
	native void gsi_rectv(gs_immediate_draw_t* imm, gs_vec2 start, gs_vec2 end, gs_color_t color, int primitive);
	native void gsi_render_pass_submit(gs_immediate_draw_t* imm, gs_command_buffer_t* cb, gs_color_t clear);
}

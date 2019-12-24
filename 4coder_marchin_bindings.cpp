/*
4coder_default_bidings.cpp - Supplies the default bindings used for default 4coder behavior.
*/

// TOP

#if !defined(FCODER_MARCHIN_BINDINGS_CPP)
#define FCODER_MARCHIN_BINDINGS_CPP

#include "4coder_default_include.cpp"
#include "4coder_marchin_map.cpp"

void
custom_layer_init(Application_Links *app){
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    async_task_handler_init(app, &global_async_system);
    code_index_init();
    buffer_modified_set_init();
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    initialize_managed_id_metadata(app);
    set_default_color_scheme(app);
    
    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    set_custom_hook(app, HookID_RenderCaller, skytrias_render_caller);
    set_custom_hook(app, HookID_NewFile, custom_new_file);
    mapping_init(tctx, &framework_mapping);
    setup_marchin_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
}

#endif //FCODER_MARCHIN_BINDINGS

// BOTTOM


/*
4coder_default_map.cpp - Instantiate default bindings.
*/
// TOP

static FColor FUNCTION_HIGHLIGHT_COLOR = fcolor_argb(.5f, 0.6f, 0.7f, 1.0f);

// NOTE(Skytrias): custom token coloring
function FColor
skytrias_get_token_color_cpp(Token token){
    Managed_ID color = defcolor_text_default;
    switch (token.kind){
        case TokenBaseKind_Preprocessor:
        {
            color = defcolor_preproc;
        }break;
        case TokenBaseKind_Keyword:
        {
            color = defcolor_keyword;
        }break;
        case TokenBaseKind_Comment:
        {
            color = defcolor_comment;
        }break;
        case TokenBaseKind_LiteralString:
        {
            color = defcolor_str_constant;
        }break;
        case TokenBaseKind_LiteralInteger:
        {
            color = defcolor_int_constant;
        }break;
        case TokenBaseKind_LiteralFloat:
        {
            color = defcolor_float_constant;
        }break;
        default:
        {
            switch (token.sub_kind){
                case TokenCppKind_LiteralTrue:
                case TokenCppKind_LiteralFalse:
                case TokenCppKind_ColonColon:
                //case TokenCppKind_ColonEq:
				{
                    color = defcolor_bool_constant;
                }break;
                case TokenCppKind_LiteralCharacter:
                case TokenCppKind_LiteralCharacterWide:
                case TokenCppKind_LiteralCharacterUTF8:
                case TokenCppKind_LiteralCharacterUTF16:
                case TokenCppKind_LiteralCharacterUTF32:
                {
                    color = defcolor_char_constant;
                }break;
                case TokenCppKind_PPIncludeFile:
                {
                    color = defcolor_include;
                }break;
            }
        }break;
    }
    return(fcolor_id(color));
}

// NOTE(Skytrias): nothing customized, just here since token colors are customized
function void
skytrias_draw_cpp_token_colors(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array){
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    for (;;){
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last){
            break;
        }
        FColor color = skytrias_get_token_color_cpp(*token);
        ARGB_Color argb = fcolor_resolve(color);
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        if (!token_it_inc_all(&it)){
            break;
        }
    }
}

function void
skytrias_paint_functions(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                         i64 pos) {
    i64 keyword_length = 0;
    i64 start_pos = 0;
    i64 end_pos = 0;
    
	Token_Array array = get_token_array_from_buffer(app, buffer);
    if (array.tokens != 0){
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        i64 first_index = token_index_from_pos(&array, visible_range.first);
        Token_Iterator_Array it = token_iterator_index(0, &array, first_index);
        for (;;){
            Token *token = token_it_read(&it);
            if (token->pos >= visible_range.one_past_last){
                break;
            }
            
            // get pos at paren
            if (keyword_length != 0 && token->kind == TokenBaseKind_ParentheticalOpen) {
                end_pos = token->pos;
            }
            
            // search for default text, count up the size
            if (token->kind == TokenBaseKind_Identifier) {
                if (keyword_length == 0) {
                    start_pos = token->pos;
                }
                
                keyword_length += 1;
            } else {
                keyword_length = 0;
            }
            
            // color text 
            if (start_pos != 0 && end_pos != 0) {
                Range_i64 range = { 0 };
                range.start = start_pos;
                range.end = end_pos;
				// NOTE(Skytrias): use your own colorscheme her via fcolor_id(defcolor_*)
				// NOTE(Skytrias): or set the color you'd like to use globally like i do
                paint_text_color(app, text_layout_id, range, fcolor_resolve(FUNCTION_HIGHLIGHT_COLOR));
                
                end_pos = 0;
                start_pos = 0;
            }
            
            if (!token_it_inc_all(&it)){
                break;
            }
        }
    }
}

// default file_bar draw call with macro recording highlighted in red
static void
skytrias_draw_file_bar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar, f32 delta){
    Scratch_Block scratch(app);
    
    // NOTE(Skytrias): when recording, highlight file bar
    if (global_keyboard_macro_is_recording) {
        draw_rectangle_fcolor(app, bar, 0.f, fcolor_blend(fcolor_id(defcolor_bar), 0.5f, f_red, 0.5f));
    } else {
        draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
    }
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    
    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));
    
    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;
        
        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;
        
        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }
    
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        u8 space[3];
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }
    
    // NOTE(Skytrias): push the string REC to the file bar
    if (global_keyboard_macro_is_recording) {
        push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" REC"));
    }
    
    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

static void
skytrias_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                       Buffer_ID buffer, Text_Layout_ID text_layout_id,
                       Rect_f32 rect, Frame_Info frame_info){
    ProfileScope(app, "[Skytrias] render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        skytrias_draw_cpp_token_colors(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
        }
    }
    else{
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight){
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
#if 0
    // NOTE(rjf): Brace highlight
    {
        ARGB_Color colors[] =
        {
            0xff8ffff2,
        };
        
        Fleury4RenderBraceHighlight(app, buffer, text_layout_id, cursor_pos,
                                    colors, sizeof(colors)/sizeof(colors[0]));
    }
    #endif

    if (global_config.use_error_highlight || global_config.use_jump_highlight){
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight){
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight){
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(allen): Color parens
    if (global_config.use_paren_helper){
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(Skytrias): word highlight before braces ()
    skytrias_paint_functions(app, buffer, text_layout_id, cursor_pos);
    
    // NOTE(allen): Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number,
                            fcolor_id(defcolor_highlight_cursor_line));
    }
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = (metrics.normal_advance*0.5f)*0.9f;
    f32 mark_thickness = 2.f;
    
    // NOTE(allen): Cursor
    switch (fcoder_mode){
        case FCoderMode_Original:
        {
            //skytrias_render_cursor(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness, frame_info);
            draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        }break;
        case FCoderMode_NotepadLike:
        {
            //draw_notepad_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
            draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
        }break;
    }
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
#if 0
    // NOTE(rjf): Brace annotations
    {
        Fleury4RenderCloseBraceAnnotation(app, buffer, text_layout_id, cursor_pos);
    }
    
    // NOTE(rjf): Brace lines
    {
        Fleury4RenderBraceLines(app, buffer, view_id, text_layout_id, cursor_pos);
    }
    
    // NOTE(rjf): Draw code peek
    if(global_code_peek_open && is_active_view)
    {
        Fleury4RenderRangeHighlight(app, view_id, text_layout_id, global_code_peek_token_range);
        skytrias_render_code_peek(app, active_view, face_id, buffer, frame_info);
    }
#endif
    
    draw_set_clip(app, prev_clip);
}

// NOTE(Skytrias): pretty much just custom scroll speed and render_buffer
function void
skytrias_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, "skytrias render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        skytrias_draw_file_bar(app, view_id, buffer, face_id, pair.min, frame_info.animation_dt);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    // NOTE(Skytrias): faster scrolling like alpha
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt * 1.125f, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
            for (i32 i = 0; i < query_bars.count; i += 1){
                Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
                region = pair.max;
            }
        }
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        wregion = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
    //default_render_buffer(app, view_id, face_id, buffer, text_layout_id, region, frame_info);
    skytrias_render_buffer(app, view_id, face_id, buffer, text_layout_id, region, frame_info);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

BUFFER_HOOK_SIG(custom_new_file) {
    Scratch_Block scratch(app);
    String_Const_u8 file_name = push_buffer_base_name(app, scratch, buffer_id);
    if (!string_match(string_postfix(file_name, 2), string_u8_litexpr(".h"))) {
        return(0);
    }
    
    List_String_Const_u8 guard_list = {};
    for (u64 i = 0; i < file_name.size; ++i) {
        u8 c[2] = {};
        u64 c_size = 1;
        u8 ch = file_name.str[i];
        if (ch == '.') {
            c[0] = '_';
        } else if (ch >= 'A' && ch <= 'Z') {
            c_size = 2;
            c[0] = '_';
            c[1] = ch;
        } else if (ch >= 'a' && ch <= 'z') {
            c[0] = ch - ('a' - 'A');
        }
        String_Const_u8 part = push_string_copy(scratch, SCu8(c, c_size));
        string_list_push(scratch, &guard_list, part);
    }
    String_Const_u8 guard = string_list_flatten(scratch, guard_list);
    
    Buffer_Insertion insert = begin_buffer_insertion_at_buffered(app, buffer_id, 0, scratch, KB(16));
    insertf(&insert,
            "#ifndef %.*s\n"
            "#define %.*s\n"
            "\n"
            "#endif //%.*s\n",
            string_expand(guard),
            string_expand(guard),
            string_expand(guard));
    end_buffer_insertion(&insert);
    
    return(0);
}

CUSTOM_COMMAND_SIG(backspace_alpha_numeric_token_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric token boundary to the left.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Backward,
                                 push_boundary_list(scratch, boundary_token, boundary_alpha_numeric_camel));
}

CUSTOM_COMMAND_SIG(delete_alpha_numeric_token_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric token boundary to the right.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward,
                                 push_boundary_list(scratch, boundary_token, boundary_alpha_numeric_camel));
}

CUSTOM_COMMAND_SIG(backspace_whitespace_token_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric token boundary to the left.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Backward,
                                 push_boundary_list(scratch, boundary_token, boundary_non_whitespace));
}

CUSTOM_COMMAND_SIG(delete_whitespace_token_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric token boundary to the right.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward,
                                 push_boundary_list(scratch, boundary_token, boundary_non_whitespace));
}

CUSTOM_COMMAND_SIG(move_up_5)
CUSTOM_DOC("Moves the cursor up five lines.")
{
    move_vertical_lines(app, -5);
}

CUSTOM_COMMAND_SIG(move_down_5)
CUSTOM_DOC("Moves the cursor down five lines.")
{
    move_vertical_lines(app, 5);
}

CUSTOM_COMMAND_SIG(insert_new_line_above)
{
    seek_beginning_of_line(app);
    write_string(app, string_u8_litexpr("\n"));
    auto_indent_line_at_cursor(app);
    move_up(app);
}

CUSTOM_COMMAND_SIG(insert_new_line_below)
{
    seek_end_of_line(app);
    write_string(app, string_u8_litexpr("\n"));
    auto_indent_line_at_cursor(app);
}

CUSTOM_COMMAND_SIG(arrow_operator)
{
    write_string(app, string_u8_litexpr("->"));
}

CUSTOM_COMMAND_SIG(copy_line)
{
    View_ID active_view = get_active_view(app, Access_Always);
    i64 pos = view_get_cursor_pos(app, active_view);
    seek_beginning_of_line(app);
    set_mark(app);
    seek_end_of_line(app);
    copy(app);
    view_set_cursor(app, active_view,seek_pos(pos));
}

CUSTOM_COMMAND_SIG(cut_line)
{
    seek_beginning_of_line(app);
    set_mark(app);
    seek_end_of_line(app);
    copy(app);
    delete_line(app);
}

CUSTOM_COMMAND_SIG(paste_line)
{
    insert_new_line_below(app);
    paste_and_indent(app);
}

CUSTOM_COMMAND_SIG(delete_rest_of_line)
{
    set_mark(app);
    seek_end_of_line(app);
    delete_range(app);
}

function void
setup_marchin_mapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id){
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    BindCore(default_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    Bind(keyboard_macro_start_recording , KeyCode_U, KeyCode_Control);
    Bind(keyboard_macro_finish_recording, KeyCode_U, KeyCode_Control, KeyCode_Shift);
    Bind(keyboard_macro_replay,           KeyCode_U, KeyCode_Alt);
    Bind(change_active_panel,           KeyCode_Period, KeyCode_Control);
    Bind(change_active_panel_backwards, KeyCode_Comma, KeyCode_Control);
    Bind(interactive_new,               KeyCode_N, KeyCode_Control);
    Bind(interactive_open_or_new,       KeyCode_O, KeyCode_Control);
    Bind(open_in_other,                 KeyCode_O, KeyCode_Alt);
    Bind(interactive_kill_buffer,       KeyCode_K, KeyCode_Control);
    Bind(interactive_switch_buffer,     KeyCode_I, KeyCode_Control);
    Bind(project_go_to_root_directory,  KeyCode_H, KeyCode_Control);
    Bind(open_panel_hsplit,  KeyCode_H, KeyCode_Alt);
    Bind(open_panel_vsplit,  KeyCode_V, KeyCode_Alt);
    Bind(save_all_dirty_buffers,        KeyCode_S, KeyCode_Control, KeyCode_Shift);
    Bind(change_to_build_panel,         KeyCode_Period, KeyCode_Alt);
    Bind(close_build_panel,             KeyCode_Comma, KeyCode_Alt);
    Bind(goto_next_jump,                KeyCode_N, KeyCode_Alt);
    Bind(goto_prev_jump,                KeyCode_N, KeyCode_Alt, KeyCode_Shift);
    Bind(build_in_build_panel,          KeyCode_M, KeyCode_Alt);
    Bind(goto_first_jump,               KeyCode_M, KeyCode_Alt, KeyCode_Shift);
    Bind(toggle_filebar,                KeyCode_B, KeyCode_Alt);
    Bind(execute_any_cli,               KeyCode_Z, KeyCode_Alt);
    Bind(execute_previous_cli,          KeyCode_Z, KeyCode_Alt, KeyCode_Shift);
    Bind(command_lister,                KeyCode_P, KeyCode_Control);
    Bind(project_command_lister,        KeyCode_P, KeyCode_Control, KeyCode_Shift);
    Bind(list_all_functions_current_buffer, KeyCode_I, KeyCode_Control, KeyCode_Shift);
    Bind(project_fkey_command, KeyCode_F1);
    Bind(project_fkey_command, KeyCode_F2);
    Bind(project_fkey_command, KeyCode_F3);
    Bind(project_fkey_command, KeyCode_F4);
    Bind(project_fkey_command, KeyCode_F5);
    Bind(project_fkey_command, KeyCode_F6);
    Bind(project_fkey_command, KeyCode_F7);
    Bind(project_fkey_command, KeyCode_F8);
    Bind(project_fkey_command, KeyCode_F9);
    Bind(project_fkey_command, KeyCode_F10);
    Bind(project_fkey_command, KeyCode_F11);
    Bind(project_fkey_command, KeyCode_F12);
    Bind(project_fkey_command, KeyCode_F13);
    Bind(project_fkey_command, KeyCode_F14);
    Bind(project_fkey_command, KeyCode_F15);
    Bind(project_fkey_command, KeyCode_F16);
    Bind(toggle_fullscreen,          KeyCode_F11, KeyCode_Control);
    Bind(exit_4coder,          KeyCode_F12, KeyCode_Control);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
    Bind(arrow_operator, KeyCode_ForwardSlash, KeyCode_Control);
    
    SelectMap(file_id);
    ParentMap(global_id);
    BindTextInput(write_text_input);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    Bind(delete_char,            KeyCode_Delete);
    Bind(backspace_char,         KeyCode_Backspace);
    Bind(move_up,                KeyCode_Up);
    Bind(move_down,              KeyCode_Down);
    Bind(move_left,              KeyCode_Left);
    Bind(move_right,             KeyCode_Right);
    Bind(seek_end_of_line,       KeyCode_End);
    Bind(seek_beginning_of_line, KeyCode_Home);
    Bind(page_up,                KeyCode_PageUp);
    Bind(page_down,              KeyCode_PageDown);
    Bind(goto_beginning_of_file, KeyCode_Home, KeyCode_Control);
    Bind(goto_end_of_file,       KeyCode_End, KeyCode_Control);
    Bind(move_up_to_blank_line_end,        KeyCode_Up, KeyCode_Control, KeyCode_Shift);
    Bind(move_up_5,        KeyCode_Up, KeyCode_Control);
    Bind(move_down_5,        KeyCode_Down, KeyCode_Control);
    Bind(move_down_to_blank_line_end,      KeyCode_Down, KeyCode_Control, KeyCode_Shift);
    Bind(wmove_left_whitespace_or_token_boundary,    KeyCode_Left, KeyCode_Control);
    Bind(move_right_whitespace_or_token_boundary,   KeyCode_Right, KeyCode_Control);
    Bind(move_line_up,                     KeyCode_Up, KeyCode_Alt);
    Bind(move_line_down,                   KeyCode_Down, KeyCode_Alt);
    Bind(set_mark,                    KeyCode_Space, KeyCode_Control);
    Bind(replace_in_range,            KeyCode_A, KeyCode_Control);
    Bind(copy,                        KeyCode_C, KeyCode_Control);
    Bind(copy_line,                   KeyCode_C, KeyCode_Control, KeyCode_Shift);
    Bind(delete_range,                KeyCode_Delete, KeyCode_Shift);
    Bind(delete_line,                 KeyCode_L, KeyCode_Control);
    Bind(center_view,                 KeyCode_E, KeyCode_Control);
    Bind(left_adjust_view,            KeyCode_E, KeyCode_Control, KeyCode_Shift);
    Bind(search,                      KeyCode_F, KeyCode_Control);
    Bind(list_all_locations,          KeyCode_F, KeyCode_Control, KeyCode_Shift);
    Bind(list_all_substring_locations_case_insensitive, KeyCode_F, KeyCode_Alt);
    Bind(goto_line,                   KeyCode_G, KeyCode_Control);
    Bind(list_all_locations_of_selection,  KeyCode_G, KeyCode_Control, KeyCode_Shift);
    Bind(snippet_lister,              KeyCode_J, KeyCode_Control);
    Bind(kill_buffer,                 KeyCode_K, KeyCode_Control, KeyCode_Shift);
    Bind(duplicate_line,              KeyCode_D, KeyCode_Control);
    Bind(cursor_mark_swap,            KeyCode_M, KeyCode_Control);
    Bind(reopen,                      KeyCode_O, KeyCode_Control, KeyCode_Shift);
    Bind(query_replace,               KeyCode_Q, KeyCode_Control);
    Bind(query_replace_identifier,    KeyCode_Q, KeyCode_Control, KeyCode_Shift);
    Bind(query_replace_selection,     KeyCode_Q, KeyCode_Alt);
    Bind(reverse_search,              KeyCode_R, KeyCode_Control);
    Bind(save,                        KeyCode_S, KeyCode_Control);
    Bind(save_all_dirty_buffers,      KeyCode_S, KeyCode_Control, KeyCode_Shift);
    Bind(search_identifier,           KeyCode_T, KeyCode_Control);
    Bind(list_all_locations_of_identifier, KeyCode_T, KeyCode_Control, KeyCode_Shift);
    Bind(paste_and_indent,            KeyCode_V, KeyCode_Control);
    Bind(paste_line,                  KeyCode_V, KeyCode_Control, KeyCode_Shift);
    Bind(cut,                         KeyCode_X, KeyCode_Control);
    Bind(cut_line,                    KeyCode_X, KeyCode_Control, KeyCode_Shift);
    Bind(undo,                        KeyCode_Z, KeyCode_Control);
    Bind(redo,                        KeyCode_Z, KeyCode_Control, KeyCode_Shift);
    Bind(redo,                        KeyCode_Z, KeyCode_Control, KeyCode_Shift);
    Bind(delete_rest_of_line,         KeyCode_D, KeyCode_Control, KeyCode_Shift);
    Bind(view_buffer_other_panel,     KeyCode_1, KeyCode_Control);
    Bind(swap_panels,                 KeyCode_2, KeyCode_Control);
    Bind(if_read_only_goto_position,  KeyCode_Return);
    Bind(if_read_only_goto_position_same_panel, KeyCode_Return, KeyCode_Shift);
    Bind(view_jump_list_with_lister,  KeyCode_Period, KeyCode_Control, KeyCode_Shift);
    Bind(backspace_whitespace_token_boundary, KeyCode_Backspace, KeyCode_Control);
    Bind(delete_whitespace_token_boundary,    KeyCode_Delete, KeyCode_Control);
    Bind(backspace_alpha_numeric_token_boundary, KeyCode_Backspace, KeyCode_Control, KeyCode_Shift);
    Bind(delete_alpha_numeric_token_boundary,    KeyCode_Delete, KeyCode_Control, KeyCode_Shift);
    Bind(insert_new_line_below, KeyCode_Return, KeyCode_Control);
    Bind(insert_new_line_above, KeyCode_Return, KeyCode_Control, KeyCode_Shift);
    Bind(close_panel, KeyCode_W, KeyCode_Control);
    
    SelectMap(code_id);
    ParentMap(file_id);
    BindTextInput(write_text_and_auto_indent);
    Bind(move_left_alpha_numeric_or_camel_boundary,  KeyCode_Left, KeyCode_Control, KeyCode_Shift);
    Bind(move_right_alpha_numeric_or_camel_boundary, KeyCode_Right, KeyCode_Control, KeyCode_Shift);
    Bind(comment_line_toggle,        KeyCode_Semicolon, KeyCode_Control);
    Bind(word_complete,              KeyCode_Tab);
    Bind(auto_indent_range,          KeyCode_Tab, KeyCode_Control, KeyCode_Shift);
    Bind(auto_indent_line_at_cursor, KeyCode_Tab, KeyCode_Shift);
    Bind(word_complete_drop_down,    KeyCode_Tab, KeyCode_Control);
    Bind(write_block,                KeyCode_R, KeyCode_Alt);
    Bind(write_todo,                 KeyCode_T, KeyCode_Alt);
    Bind(write_note,                 KeyCode_Y, KeyCode_Alt);
    Bind(list_all_locations_of_type_definition,               KeyCode_D, KeyCode_Alt);
    Bind(list_all_locations_of_type_definition_of_identifier, KeyCode_T, KeyCode_Alt, KeyCode_Shift);
    Bind(open_long_braces,           KeyCode_LeftBracket, KeyCode_Control);
    Bind(open_long_braces_semicolon, KeyCode_LeftBracket, KeyCode_Control, KeyCode_Shift);
    Bind(open_long_braces_break,     KeyCode_RightBracket, KeyCode_Control, KeyCode_Shift);
    Bind(select_surrounding_scope,   KeyCode_LeftBracket, KeyCode_Alt);
    Bind(select_surrounding_scope_maximal, KeyCode_LeftBracket, KeyCode_Alt, KeyCode_Shift);
    Bind(select_prev_scope_absolute, KeyCode_RightBracket, KeyCode_Alt);
    Bind(select_prev_top_most_scope, KeyCode_RightBracket, KeyCode_Alt, KeyCode_Shift);
    Bind(select_next_scope_absolute, KeyCode_Quote, KeyCode_Alt);
    Bind(select_next_scope_after_current, KeyCode_Quote, KeyCode_Alt, KeyCode_Shift);
    Bind(place_in_scope,             KeyCode_ForwardSlash, KeyCode_Alt);
    Bind(delete_current_scope,       KeyCode_Minus, KeyCode_Alt);
    Bind(if0_off,                    KeyCode_0, KeyCode_Alt);
    Bind(open_file_in_quotes,        KeyCode_1, KeyCode_Alt);
    Bind(open_matching_file_cpp,     KeyCode_2, KeyCode_Alt);
    Bind(write_zero_struct,          KeyCode_0, KeyCode_Control);
}

// BOTTOM


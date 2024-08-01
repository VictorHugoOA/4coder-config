String_ID shared_mode;
String_ID normal_mode;
String_ID insert_mode;

void set_current_mapid(Application_Links* app,
                       Command_Map_ID mapid){
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr = mapid;
}

CUSTOM_COMMAND_SIG(hugovhoa_normal_mode) {
    set_current_mapid(app, normal_mode);
    
    active_color_table.arrays[ defcolor_margin_active ].vals[ 0 ] = 0xff00ff00;
}

CUSTOM_COMMAND_SIG(hugovhoa_insert_mode) {
    set_current_mapid(app, insert_mode);
}

CUSTOM_COMMAND_SIG(hugovhoa_cut_and_enter_insert_mode){
    cut_view(app);
    set_current_mapid(app, insert_mode);
}

CUSTOM_COMMAND_SIG(hugovhoa_cut_character){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    if(!if_view_has_highlighted_range_delete_range(app, view)){
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        i64 start = view_get_cursor_pos(app, view);
        i64 buffer_size = buffer_get_size(app, buffer);
        if(0 <= start && start < buffer_size){
            Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(start));
            i64 character = view_relative_character_from_pos(app, view, cursor.line, cursor.pos);
            i64 end = view_pos_from_relative_character(app, view, cursor.line, character + 1);
            Range_i64 cut_char = Ii64(start, end);
            if(clipboard_post_buffer_range(app, 0, buffer, cut_char)){
                buffer_replace_range(app, buffer, cut_char, string_u8_empty);
            }
        }
    }
    
}

CUSTOM_COMMAND_SIG(hugovhoa_cut_line){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
    range.end += 1;
    i32 size = (i32)buffer_get_size(app, buffer);
    range.end = clamp_top(range.end, size);
    if(range_size(range) == 0 ||
       buffer_get_char(app, buffer, range.end - 1) != '\n'){
        range.start -= 1;
        range.first = clamp_bot(0, range.first);
    }
    if(clipboard_post_buffer_range(app, 0, buffer, range)){
        buffer_replace_range(app, buffer, range, string_u8_empty);
    }
}

CUSTOM_COMMAND_SIG(hugovhoa_add_line_before_cursor_and_insert){
    char text[] = "\n";
    i32 size = sizeof(text) - 1;
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
    
    buffer_replace_range(app, buffer, Ii64(range.start), SCu8(text, size));
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.start));
    auto_indent_buffer(app, buffer, Ii64_size(range.start, size));
    move_past_lead_whitespace(app, view, buffer);
    set_current_mapid(app, insert_mode);
}

CUSTOM_COMMAND_SIG(hugovhoa_add_line_after_cursor_and_insert){
    char text[] = "\n";
    i32 size = sizeof(text) - 1;
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
    range.end += 1;
    i32 buffer_size = (i32)buffer_get_size(app, buffer);
    range.end = clamp_top(range.end, buffer_size);
    buffer_replace_range(app, buffer, Ii64(range.end), SCu8(text, size));
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.end));
    auto_indent_buffer(app, buffer, Ii64_size(range.end, size));
    move_past_lead_whitespace(app, view, buffer);
    view_enqueue_command_function(app, view, hugovhoa_insert_mode);
}

CUSTOM_COMMAND_SIG(hugovhoa_jump_forward_to_word){
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    i64 index = token_index_from_pos(&token_array, cursor_pos);
    Token_Iterator_Array it = token_iterator_index(0, &token_array, index);
    if(token_it_inc(&it)){
        Token *token  = token_it_read(&it);
        view_set_mark(app, view, seek_pos(token->pos));
        view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos + token->size));
    }
}

CUSTOM_COMMAND_SIG(hugovhoa_jump_forward_to_word_no_mark){
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    i64 index = token_index_from_pos(&token_array, cursor_pos);
    Token_Iterator_Array it = token_iterator_index(0, &token_array, index);
    if(token_it_inc(&it)){
        Token *token  = token_it_read(&it);
        view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos + token->size));
    }
}

CUSTOM_COMMAND_SIG(hugovhoa_jump_backward_to_word){
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    i64 index = token_index_from_pos(&token_array, cursor_pos);
    Token_Iterator_Array it = token_iterator_index(0, &token_array, index);
    if(token_it_dec(&it)){
        Token *token  = token_it_read(&it);
        view_set_mark(app, view, seek_pos(token->pos+token->size));
        view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos));
    }
}

CUSTOM_COMMAND_SIG(hugovhoa_jump_backward_to_word_no_mark){
    View_ID view = get_active_view(app, 0);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    i64 index = token_index_from_pos(&token_array, cursor_pos);
    Token_Iterator_Array it = token_iterator_index(0, &token_array, index);
    if(token_it_dec(&it)){
        Token *token  = token_it_read(&it);
        view_set_cursor_and_preferred_x(app, view, seek_pos(token->pos));
    }
}

CUSTOM_COMMAND_SIG(hugovhoa_yank_line){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
    range.end += 1;
    i32 size = (i32)buffer_get_size(app, buffer);
    range.end = clamp_top(range.end, size);
    if(range_size(range) == 0 ||
       buffer_get_char(app, buffer, range.end - 1) != '\n'){
        range.start -= 1;
        range.first = clamp_bot(0, range.first);
    }
    clipboard_post_buffer_range(app, 0, buffer, range);
}

#if OS_MAC
#define SUPER_KEYCODE KeyCode_Command
#else
#define SUPER_KEYCODE KeyCode_Alt
#endif

internal void
setup_my_mapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id){
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    
    SelectMap(shared_mode);
    BindCore(default_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    Bind(hugovhoa_normal_mode, KeyCode_Escape);
    Bind(move_left, KeyCode_Left);
    Bind(move_right, KeyCode_Right);
    Bind(move_up, KeyCode_Up);
    Bind(move_down, KeyCode_Down);
    
    SelectMap(normal_mode);
    ParentMap(shared_mode);
    Bind(hugovhoa_insert_mode, KeyCode_I);
    Bind(hugovhoa_insert_mode, KeyCode_A);
    
    Bind(keyboard_macro_start_recording , KeyCode_Q);
    Bind(keyboard_macro_finish_recording, KeyCode_Q, KeyCode_Shift);
    Bind(keyboard_macro_replay,           KeyCode_Q, KeyCode_Control);
    
    Bind(change_active_panel,           KeyCode_W, KeyCode_Control);
    
    Bind(interactive_open_or_new,       KeyCode_O, KeyCode_Control);
    
    Bind(open_in_other,                 KeyCode_O, SUPER_KEYCODE);
    
    Bind(move_left, KeyCode_H);
    Bind(move_right, KeyCode_L);
    Bind(move_up, KeyCode_K);
    Bind(move_down, KeyCode_J);
    
    Bind(hugovhoa_jump_forward_to_word, KeyCode_W);
    Bind(hugovhoa_jump_forward_to_word_no_mark, KeyCode_W, KeyCode_Shift);
    
    Bind(hugovhoa_jump_backward_to_word, KeyCode_B);
    Bind(hugovhoa_jump_backward_to_word_no_mark, KeyCode_B, KeyCode_Shift);
    
    Bind(change_to_build_panel,         KeyCode_Period, SUPER_KEYCODE);
    Bind(close_build_panel,             KeyCode_Comma, SUPER_KEYCODE, KeyCode_Shift);
    
    Bind(build_in_build_panel,          KeyCode_M, SUPER_KEYCODE);
    Bind(goto_first_jump,               KeyCode_M, KeyCode_Control, KeyCode_Shift);
    
    Bind(execute_any_cli,               KeyCode_Z, KeyCode_Shift);
    Bind(execute_previous_cli,          KeyCode_Z, KeyCode_Control, KeyCode_Shift);
    
    Bind(command_lister,                KeyCode_Period, KeyCode_Shift);
    Bind(project_command_lister,        KeyCode_Period, KeyCode_Control, KeyCode_Shift);
    
    Bind(interactive_switch_buffer,             KeyCode_1);
    Bind(list_all_functions_current_buffer, KeyCode_Down, SUPER_KEYCODE);
    
    Bind(exit_4coder,          KeyCode_Q, SUPER_KEYCODE);
    Bind(seek_beginning_of_line, KeyCode_0);
    Bind(seek_end_of_line, KeyCode_4, KeyCode_Shift);
    Bind(goto_beginning_of_file, KeyCode_G);
    Bind(goto_end_of_file, KeyCode_G, KeyCode_Shift);
    Bind(page_up, KeyCode_Up, KeyCode_Shift);
    Bind(hugovhoa_cut_character, KeyCode_X);
    Bind(hugovhoa_cut_line, KeyCode_X, KeyCode_Shift);
    Bind(page_down, KeyCode_Down, KeyCode_Shift);
    Bind(set_mark, KeyCode_Space);
    Bind(hugovhoa_cut_and_enter_insert_mode, KeyCode_C);
    Bind(copy, KeyCode_Y);
    Bind(hugovhoa_yank_line, KeyCode_Y, KeyCode_Shift);
    Bind(cut, KeyCode_D);
    Bind(hugovhoa_cut_line, KeyCode_D, KeyCode_Shift);
    Bind(center_view, KeyCode_L, KeyCode_Shift);
    Bind(search, KeyCode_7, KeyCode_Shift);
    Bind(reverse_search, '\'');
    Bind(list_all_locations, KeyCode_T);
    
    Bind(move_up_to_blank_line_end, KeyCode_K, KeyCode_Shift);
    Bind(move_down_to_blank_line_end, KeyCode_J, KeyCode_Shift);
    
    Bind(cursor_mark_swap, KeyCode_Space, KeyCode_Shift);
    
    Bind(query_replace, KeyCode_R, KeyCode_Shift);
    
    Bind(paste, KeyCode_P);
    Bind(paste_next_and_indent, KeyCode_P, KeyCode_Shift);
    
    Bind(redo, KeyCode_U, KeyCode_Shift);
    Bind(undo, KeyCode_U);
    
    Bind(write_block, KeyCode_Return, KeyCode_Shift);
    
    Bind(backspace_alpha_numeric_boundary, KeyCode_Backspace, KeyCode_Shift);
    Bind(delete_alpha_numeric_boundary,    KeyCode_Delete, KeyCode_Shift);
    
    Bind(swap_panels, KeyCode_W, KeyCode_Shift, SUPER_KEYCODE);
    Bind(jump_to_definition_at_cursor, KeyCode_W, KeyCode_Command);
    Bind(auto_indent_range, KeyCode_Tab);
    Bind(hugovhoa_add_line_before_cursor_and_insert, KeyCode_O, KeyCode_Shift);
    Bind(hugovhoa_add_line_after_cursor_and_insert, KeyCode_O);
    
    Bind(jump_to_definition_at_cursor, KeyCode_Right, KeyCode_Shift);
    
    SelectMap(insert_mode);
    ParentMap(shared_mode);
    BindTextInput(write_text_and_auto_indent);
    
    Bind(delete_char,            KeyCode_Delete);
    Bind(word_complete_drop_down, KeyCode_Tab, KeyCode_Shift);
    Bind(backspace_char,         KeyCode_Backspace);
    //Bind(search_identifier,           KeyCode_T, KeyCode_Command);
    Bind(list_all_locations_of_identifier, KeyCode_T, KeyCode_Command, KeyCode_Shift);
    //Bind(paste_next_and_indent,       KeyCode_V, KeyCode_Command, KeyCode_Shift);
    //Bind(view_buffer_other_panel,     KeyCode_1, KeyCode_Command);
    //Bind(if_read_only_goto_position,  KeyCode_Return);
    //Bind(if_read_only_goto_position_same_panel, KeyCode_Return, KeyCode_Shift);
    //Bind(view_jump_list_with_lister,  KeyCode_Period, KeyCode_Command, KeyCode_Shift);
    
    //SelectMap(code_id);
    //Bind(move_left_alpha_numeric_boundary,           KeyCode_Left, KeyCode_Command);
    //Bind(move_right_alpha_numeric_boundary,          KeyCode_Right, KeyCode_Command);
    //Bind(move_left_alpha_numeric_or_camel_boundary,  KeyCode_Left, KeyCode_Control);
    //Bind(move_right_alpha_numeric_or_camel_boundary, KeyCode_Right, KeyCode_Control);
    //Bind(comment_line_toggle,        KeyCode_Semicolon, KeyCode_Command);
    //Bind(write_block,                KeyCode_R, KeyCode_Control);
    //Bind(write_todo,                 KeyCode_T, KeyCode_Control);
    //Bind(write_note,                 KeyCode_Y, KeyCode_Control);
    //Bind(list_all_locations_of_type_definition,               KeyCode_D, KeyCode_Control);
    //Bind(list_all_locations_of_type_definition_of_identifier, KeyCode_T, KeyCode_Control, KeyCode_Shift);
    //Bind(open_long_braces,           KeyCode_LeftBracket, KeyCode_Command);
    //Bind(open_long_braces_semicolon, KeyCode_LeftBracket, KeyCode_Command, KeyCode_Shift);
    //Bind(open_long_braces_break,     KeyCode_RightBracket, KeyCode_Command, KeyCode_Shift);
    //Bind(select_surrounding_scope,   KeyCode_LeftBracket, KeyCode_Control);
    //Bind(select_surrounding_scope_maximal, KeyCode_LeftBracket, KeyCode_Control, KeyCode_Shift);
    //Bind(select_prev_scope_absolute, KeyCode_RightBracket, KeyCode_Control);
    //Bind(select_prev_top_most_scope, KeyCode_RightBracket, KeyCode_Control, KeyCode_Shift);
    //Bind(select_next_scope_absolute, KeyCode_Quote, KeyCode_Control);
    //Bind(select_next_scope_after_current, KeyCode_Quote, KeyCode_Control, KeyCode_Shift);
    //Bind(place_in_scope,             KeyCode_ForwardSlash, KeyCode_Control);
    //Bind(delete_current_scope,       KeyCode_Minus, KeyCode_Control);
    //Bind(if0_off,                    KeyCode_I, KeyCode_Control);
    //Bind(open_file_in_quotes,        KeyCode_1, KeyCode_Control);
    //Bind(open_matching_file_cpp,     KeyCode_2, KeyCode_Control);
    //Bind(write_zero_struct,          KeyCode_0, KeyCode_Command);
    //
    
    SelectMap(file_id);
    ParentMap(normal_mode);
    
    SelectMap(code_id);
    
    ParentMap(normal_mode);
    
}

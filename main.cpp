// ArcaneText - Core Skeleton with Smart Mapping UI

#include <gtk/gtk.h>
#include <json-c/json.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>
#include <algorithm>
#include <map>

static GtkWidget *text_view;
static std::unordered_map<uint8_t, uint8_t> encode_map;
static std::unordered_map<uint8_t, uint8_t> decode_map;
static std::vector<uint8_t> last_loaded_raw_bytes;

struct MappingRow {
    GtkWidget* char_label;
    GtkWidget* hex_label;
    GtkWidget* entry;
};

std::unordered_map<uint8_t, std::vector<uint8_t>> current_mappings;
std::unordered_map<std::string, uint8_t> reverse_map; // for decoding
static std::map<uint8_t, MappingRow> grid_rows;  // Keep ordered keys

void rebuild_encode_decode_maps() {
    encode_map.clear();
    decode_map.clear();
    for (const auto& [char_byte, mapped_hex_str] : current_mappings) {
        unsigned int mapped_byte = 0;
        if (!mapped_hex_str.empty())
        mapped_byte = mapped_hex_str[0]; // use first byte of mapping
        encode_map[char_byte] = (uint8_t)mapped_byte;
        decode_map[(uint8_t)mapped_byte] = char_byte;
    }
}

std::vector<uint8_t> parse_hex_bytes(const std::string& hex_str) {
    std::vector<uint8_t> bytes;
    for (size_t i = 2; i < hex_str.length(); i += 2) {
        std::string byte_str = hex_str.substr(i, 2);
        uint8_t byte = (uint8_t)std::stoul(byte_str, nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}



void load_mapping_from_json(const char* filename) {
    json_object* root = json_object_from_file(filename);
    if (!root) return;

    json_object_object_foreach(root, key, val) {
        int original = std::stoi(key, nullptr, 16);
        const char* mapped_hex = json_object_get_string(val);
        current_mappings[(uint8_t)original] = parse_hex_bytes(mapped_hex);
    }

    json_object_put(root);
}

void save_mapping_to_json(const char* filename) {
    json_object* root = json_object_new_object();

    for (auto& [key, val] : current_mappings) {
        std::stringstream hex_val;
        hex_val << "0x";
        for (uint8_t b : val) {
            hex_val << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        }

        std::stringstream original_key;
        original_key << std::hex << std::setw(2) << std::setfill('0') << (int)key;

        json_object_object_add(root,
                               original_key.str().c_str(),
                               json_object_new_string(hex_val.str().c_str()));
    }

    json_object_to_file(filename, root);
    json_object_put(root);
}

void refresh_grid(GtkWidget* grid, const std::vector<uint8_t>& unique_chars) {
    // Clear grid rows if needed, or destroy grid and recreate

    // Remove all children from grid
    GList* children = gtk_container_get_children(GTK_CONTAINER(grid));
    for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    grid_rows.clear();

    int row = 0;
    for (uint8_t c : unique_chars) {
        std::stringstream hex_ss;
        hex_ss << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;

        char display_char = (c >= 32 && c <= 126) ? (char)c : '.';
        std::string char_label_text = std::string(1, display_char);

        GtkWidget* char_label = gtk_label_new(char_label_text.c_str());
        GtkWidget* hex_label = gtk_label_new(hex_ss.str().c_str());
        GtkWidget* entry = gtk_entry_new();

        // Pre-fill with current mapping or default original hex
        auto it = current_mappings.find(c);
        if (it != current_mappings.end()) {
            std::stringstream ss;
            ss << "0x";
            for (uint8_t b : it->second)
                ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)b;

            gtk_entry_set_text(GTK_ENTRY(entry), ss.str().c_str());
        } else {
            gtk_entry_set_text(GTK_ENTRY(entry), hex_ss.str().c_str());
            current_mappings[c] = parse_hex_bytes(hex_ss.str());
        }

        gtk_grid_attach(GTK_GRID(grid), char_label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), hex_label, 1, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry, 2, row, 1, 1);

        grid_rows[c] = {char_label, hex_label, entry};
        row++;
    }

    gtk_widget_show_all(grid);
}

void on_import_clicked(GtkButton* button, gpointer user_data) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Import JSON Mapping",
                                                    GTK_WINDOW(user_data),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.json");
    gtk_file_filter_set_name(filter, "JSON Mapping Files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        load_mapping_from_json(filename);
        rebuild_encode_decode_maps();
        g_free(filename);

        // After loading mappings, refresh grid to show updated mappings but keep old keys
        // You'll need to keep unique_chars updated or re-scan text with merging
        // For demo, just refresh grid with current keys:
        std::vector<uint8_t> keys;
        for (auto& pair : current_mappings) keys.push_back(pair.first);
        std::sort(keys.begin(), keys.end());
        GtkWidget* grid = GTK_WIDGET(user_data);
        refresh_grid(grid, keys);
    }
    gtk_widget_destroy(dialog);
}

void on_export_clicked(GtkButton* button, gpointer user_data) {

    GtkWidget* dialog = gtk_file_chooser_dialog_new("Export JSON Mapping",
                                                    GTK_WINDOW(user_data),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.json");
    gtk_file_filter_set_name(filter, "JSON Mapping Files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        save_mapping_to_json(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void sync_grid_to_current_mappings() {
    for (auto& [byte, row] : grid_rows) {
        const gchar* text = gtk_entry_get_text(GTK_ENTRY(row.entry));
        current_mappings[byte] = parse_hex_bytes(text);
    }
}

void apply_decode_to_buffer(const std::vector<uint8_t>& raw_bytes) {
    // Build reverse_map (sequence of hex string → decoded byte)
    reverse_map.clear();
    for (const auto& [ch, sequence] : current_mappings) {
        std::stringstream key;
        for (uint8_t b : sequence) {
            key << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        }
        reverse_map[key.str()] = ch;
    }

    std::string decoded_text;
    size_t i = 0;

    while (i < raw_bytes.size()) {
        bool matched = false;

        // Try longest (3-byte) match first
        for (int len = 3; len >= 1; --len) {
            if (i + len <= raw_bytes.size()) {
                std::stringstream ss;
                for (int j = 0; j < len; ++j)
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)raw_bytes[i + j];

                auto it = reverse_map.find(ss.str());
                if (it != reverse_map.end()) {
                    decoded_text += static_cast<char>(it->second);
                    i += len;
                    matched = true;
                    break;
                }
            }
        }

        if (!matched) {
            decoded_text += static_cast<char>(raw_bytes[i]);
            i++;
        }
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, decoded_text.c_str(), decoded_text.size());
}

std::vector<uint8_t> apply_encode_from_buffer() {
    // Encode text buffer to raw bytes using encode_map
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    std::vector<uint8_t> encoded_bytes;
    for (const char* p = text; *p; ++p) {
        uint8_t c = (uint8_t)*p;
        auto it = current_mappings.find(c);
        if (it != current_mappings.end()) {
            encoded_bytes.insert(encoded_bytes.end(), it->second.begin(), it->second.end());
        } else {
            encoded_bytes.push_back(c);
        }
    }
    g_free(text);
    return encoded_bytes;
}


void show_smart_mapping_dialog(GtkWidget* parent) {
    // (1) Get current text unique chars - merge with current_mappings keys to preserve previous entries

    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    std::set<uint8_t> seen;
    std::vector<uint8_t> unique_chars;

    for (const char* p = text; *p; ++p) {
        uint8_t byte = (uint8_t)*p;
        if (seen.insert(byte).second) {
            unique_chars.push_back(byte);
        }
    }

    // Merge keys from current_mappings that might not be in unique_chars
    for (auto& pair : current_mappings) {
        if (seen.insert(pair.first).second) {
            unique_chars.push_back(pair.first);
        }
    }

    std::sort(unique_chars.begin(), unique_chars.end(), [](uint8_t a, uint8_t b) {
        char ca = (a >= 32 && a <= 126) ? std::tolower(a) : 0;
        char cb = (b >= 32 && b <= 126) ? std::tolower(b) : 0;
        return ca < cb;
    });

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Smart Mapping Generator",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        "_Import", GTK_RESPONSE_ACCEPT,
        "_Export", GTK_RESPONSE_APPLY,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, 400, 300);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

    refresh_grid(grid, unique_chars);

    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);
    gtk_container_add(GTK_CONTAINER(content_area), scrolled_window);
    gtk_widget_show_all(dialog);

    int response;
    do {
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response == GTK_RESPONSE_ACCEPT) { // Import
            on_import_clicked(NULL, grid);
        } else if (response == GTK_RESPONSE_APPLY) { // Export
            sync_grid_to_current_mappings();   // <-- Sync user-edited values into memory
            on_export_clicked(NULL, NULL);
        } else if (response == GTK_RESPONSE_CLOSE) {
            break;
        }

        // On Import, grid is refreshed by on_import_clicked
    } while (true);


    encode_map.clear();
    decode_map.clear();
    for (auto& [byte, row] : grid_rows) {
        const gchar* map_text = gtk_entry_get_text(GTK_ENTRY(row.entry));
        unsigned int val = 0;
        sscanf(map_text, "%x", &val);
        uint8_t mapped = (uint8_t)val;

        encode_map[byte] = mapped;
        decode_map[mapped] = byte;

        current_mappings[byte] = parse_hex_bytes(map_text);
    }

    // Decode only if we have loaded bytes
    if (!last_loaded_raw_bytes.empty()) {
        apply_decode_to_buffer(last_loaded_raw_bytes);
    }

    gtk_widget_destroy(dialog);
    g_free(text);
}


void on_open_file(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
        GTK_WINDOW(user_data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::ifstream file(filename, std::ios::binary);
        if (file) {
            last_loaded_raw_bytes = std::vector<uint8_t>(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

            // 🔄 REBUILD maps and DECODE here
            rebuild_encode_decode_maps();            // <-- Rebuild from current_mappings
            apply_decode_to_buffer(last_loaded_raw_bytes);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_save_file(GtkWidget* widget, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File",
        GTK_WINDOW(user_data),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::vector<uint8_t> encoded_bytes = apply_encode_from_buffer();
        std::ofstream file(filename, std::ios::binary);
        file.write(reinterpret_cast<char*>(encoded_bytes.data()), encoded_bytes.size());
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

GtkWidget* build_menu(GtkWidget *window) {
    GtkWidget *menu_bar = gtk_menu_bar_new();
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *tools_menu = gtk_menu_new();

    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    GtkWidget *tools_item = gtk_menu_item_new_with_label("Tools");

    GtkWidget *smart_map_item = gtk_menu_item_new_with_label("Generate Smart Mapping");

    GtkWidget *open_file_item = gtk_menu_item_new_with_label("Open File");
    GtkWidget *save_file_item = gtk_menu_item_new_with_label("Save File");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_file_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_file_item);

    g_signal_connect(open_file_item, "activate", G_CALLBACK(on_open_file), window);
    g_signal_connect(save_file_item, "activate", G_CALLBACK(on_save_file), window);

    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), smart_map_item);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tools_item), tools_menu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), tools_item);

    g_signal_connect(smart_map_item, "activate", G_CALLBACK(show_smart_mapping_dialog), window);

    return menu_bar;
}

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "ArcaneText");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *menu_bar = build_menu(window);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

    GtkWidget *scrolled_text = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_text),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_text, TRUE);
    gtk_widget_set_hexpand(scrolled_text, TRUE);

    text_view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_text), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_text, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.arcane.text", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

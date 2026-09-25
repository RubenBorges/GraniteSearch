#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <future>
#include <sstream>
#include <string>
#include <vector>

#include <granitesearch.hpp>

namespace {

constexpr int window_width = 1280;
constexpr int window_height = 820;
constexpr int margin = 32;
constexpr int field_height = 42;
constexpr Color background{20, 24, 31, 255};
constexpr Color panel{31, 38, 49, 255};
constexpr Color field_background{42, 51, 64, 255};
constexpr Color border{78, 92, 111, 255};
constexpr Color accent{73, 166, 255, 255};
constexpr Color text{235, 240, 247, 255};
constexpr Color muted{157, 171, 190, 255};

struct TextField {
	Rectangle bounds{};
	std::string label;
	std::string value;
	bool multiline = false;
	bool focused = false;
	bool backspace_was_down = false;
	float backspace_elapsed = 0.0f;
	bool replace_on_next_input = false;
	size_t cursor_position = 0;
	float cursor_blink_elapsed = 0.0f;
};

struct Button {
	Rectangle bounds{};
	const char* label = "";
};

std::vector<std::string> split_lines(const std::string& text_value) {
	std::vector<std::string> lines;
	std::stringstream stream(text_value);
	std::string line;
	while (std::getline(stream, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		lines.push_back(line);
	}
	return lines;
}

void reset_cursor_blink(TextField& field) {
	field.cursor_blink_elapsed = 0.0f;
}

void insert_text(TextField& field, const std::string& inserted_text) {
	if (field.replace_on_next_input) {
		field.value.clear();
		field.cursor_position = 0;
		field.replace_on_next_input = false;
	}
	field.value.insert(field.cursor_position, inserted_text);
	field.cursor_position += inserted_text.size();
	reset_cursor_blink(field);
}

void move_cursor_vertical(TextField& field, int direction) {
	const size_t line_start_marker = field.value.rfind(
		'\n', field.cursor_position == 0 ? 0 : field.cursor_position - 1);
	const size_t line_start = line_start_marker == std::string::npos
		? 0 : line_start_marker + 1;
	const size_t column = field.cursor_position - line_start;

	if (direction < 0) {
		if (line_start == 0) {
			field.cursor_position = 0;
			return;
		}
		const size_t previous_end = line_start - 1;
		const size_t previous_marker = field.value.rfind(
			'\n', previous_end == 0 ? 0 : previous_end - 1);
		const size_t previous_start = previous_marker == std::string::npos
			? 0 : previous_marker + 1;
		field.cursor_position = previous_start +
			std::min(column, previous_end - previous_start);
		return;
	}

	const size_t next_marker = field.value.find('\n', field.cursor_position);
	if (next_marker == std::string::npos) {
		field.cursor_position = field.value.size();
		return;
	}
	const size_t next_end = field.value.find('\n', next_marker + 1);
	const size_t next_line_end = next_end == std::string::npos
		? field.value.size() : next_end;
	field.cursor_position = next_marker + 1 +
		std::min(column, next_line_end - next_marker - 1);
}

void draw_field(const TextField& field) {
	DrawText(field.label.c_str(), static_cast<int>(field.bounds.x),
			 static_cast<int>(field.bounds.y - 24), 18, muted);
	DrawRectangleRec(field.bounds, field_background);
	DrawRectangleLinesEx(field.bounds, field.focused ? 2.0f : 1.0f,
						 field.focused ? accent : border);

	const int text_x = static_cast<int>(field.bounds.x + 12);
	const int text_y = static_cast<int>(field.bounds.y + 11);
	if (!field.multiline) {
		DrawText(field.value.c_str(), text_x, text_y, 18, text);
		if (field.focused && field.cursor_blink_elapsed < 0.6f) {
			const std::string prefix = field.value.substr(0, field.cursor_position);
			const int cursor_x = text_x + MeasureText(prefix.c_str(), 18);
			DrawLine(cursor_x, text_y - 2, cursor_x, text_y + 20, accent);
		}
		return;
	}

	const std::vector<std::string> lines = split_lines(field.value);
	for (size_t index = 0; index < lines.size() && index < 4; ++index) {
		DrawText(lines[index].c_str(), text_x,
				 text_y + static_cast<int>(index * 24), 17, text);
	}
	if (field.focused && field.cursor_blink_elapsed < 0.6f) {
		const size_t line_marker = field.value.rfind(
			'\n', field.cursor_position == 0 ? 0 : field.cursor_position - 1);
		const size_t line_start = line_marker == std::string::npos
			? 0 : line_marker + 1;
		const size_t line_number = std::count(field.value.begin(),
			field.value.begin() + field.cursor_position, '\n');
		const std::string prefix = field.value.substr(
			line_start, field.cursor_position - line_start);
		const int cursor_x = text_x + MeasureText(prefix.c_str(), 17);
		const int cursor_y = text_y + static_cast<int>(line_number * 24);
		DrawLine(cursor_x, cursor_y - 2, cursor_x, cursor_y + 19, accent);
	}
}

void update_field(TextField& field, bool& consumed_mouse, float delta_time) {
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
		CheckCollisionPointRec(GetMousePosition(), field.bounds)) {
		field.focused = true;
		field.cursor_position = field.value.size();
		reset_cursor_blink(field);
		consumed_mouse = true;
	}

	if (!field.focused) {
		return;
	}
	field.cursor_position = std::min(field.cursor_position, field.value.size());
	field.cursor_blink_elapsed += delta_time;

	if (IsKeyPressed(KEY_LEFT)) {
		field.cursor_position = field.cursor_position == 0 ? 0 : field.cursor_position - 1;
		reset_cursor_blink(field);
	}
	if (IsKeyPressed(KEY_RIGHT)) {
		field.cursor_position = std::min(field.cursor_position + 1, field.value.size());
		reset_cursor_blink(field);
	}
	if (IsKeyPressed(KEY_HOME)) {
		const size_t marker = field.value.rfind(
			'\n', field.cursor_position == 0 ? 0 : field.cursor_position - 1);
		field.cursor_position = marker == std::string::npos ? 0 : marker + 1;
		reset_cursor_blink(field);
	}
	if (IsKeyPressed(KEY_END)) {
		const size_t marker = field.value.find('\n', field.cursor_position);
		field.cursor_position = marker == std::string::npos
			? field.value.size() : marker;
		reset_cursor_blink(field);
	}
	if (field.multiline && IsKeyPressed(KEY_UP)) {
		move_cursor_vertical(field, -1);
		reset_cursor_blink(field);
	}
	if (field.multiline && IsKeyPressed(KEY_DOWN)) {
		move_cursor_vertical(field, 1);
		reset_cursor_blink(field);
	}

	const bool control_down = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	if (control_down && IsKeyPressed(KEY_C)) {
		SetClipboardText(field.value.c_str());
	}
	if (control_down && IsKeyPressed(KEY_A)) {
		field.replace_on_next_input = true;
	}
	if (control_down && IsKeyPressed(KEY_V)) {
		const char* clipboard_text = GetClipboardText();
		if (clipboard_text != nullptr) {
			insert_text(field, clipboard_text);
		}
	}

	if (IsKeyDown(KEY_BACKSPACE)) {
		field.backspace_elapsed += delta_time;
		const bool initial_delete = !field.backspace_was_down;
		const bool repeat_delete = field.backspace_elapsed >= 0.05f;
		if (initial_delete || repeat_delete) {
			if (field.replace_on_next_input) {
				field.value.clear();
				field.cursor_position = 0;
				field.replace_on_next_input = false;
			} else if (field.cursor_position > 0) {
				field.value.erase(field.cursor_position - 1, 1);
				--field.cursor_position;
			}
			field.backspace_elapsed = 0.0f;
			reset_cursor_blink(field);
		}
		field.backspace_was_down = true;
	} else {
		field.backspace_was_down = false;
		field.backspace_elapsed = 0.0f;
	}

	int character = GetCharPressed();
	while (character > 0) {
		const bool allowed = character >= 32 && character <= 126;
		if (allowed) {
			insert_text(field, std::string(1, static_cast<char>(character)));
		}
		character = GetCharPressed();
	}

	if (field.multiline && IsKeyPressed(KEY_ENTER)) {
		insert_text(field, "\n");
	}
	if (IsKeyPressed(KEY_DELETE)) {
		if (field.replace_on_next_input) {
			field.value.clear();
			field.cursor_position = 0;
			field.replace_on_next_input = false;
		} else if (field.cursor_position < field.value.size()) {
			field.value.erase(field.cursor_position, 1);
		}
		reset_cursor_blink(field);
	}
}

bool clicked(const Button& button) {
	return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
		CheckCollisionPointRec(GetMousePosition(), button.bounds);
}

void focus_only(TextField& selected, std::vector<TextField*>& fields) {
	for (TextField* field : fields) {
		field->focused = field == &selected;
	}
}

} // namespace

int main(int argc, char* argv[]) {
	const std::filesystem::path executable_path =
		std::filesystem::absolute(argc > 0 ? argv[0] : "GraniteUI");
	const std::filesystem::path executable_directory = executable_path.parent_path();
	std::filesystem::path data_directory = executable_directory;
	if (!std::filesystem::exists(data_directory / "ibm-granite30m")) {
		data_directory = std::filesystem::current_path() / "dist";
	}
	const std::string default_model_path =
		(data_directory / "ibm-granite30m/ibm-granite").string();

	TextField model_path{
		{margin, 82, window_width - 2 * margin, field_height},
		"Model path", default_model_path};
	TextField query{
		{margin, 164, window_width - 2 * margin, field_height},
		"Query", "What does Linda like to do in the backyard?"};
	TextField documents{
		{margin, 246, window_width - 2 * margin, 116},
		"Document paths (one per line)",
		(data_directory / "docs/ai_programming.txt").string() + "\n" +
		(data_directory / "docs/baking_bread.txt").string() + "\n" +
		(data_directory / "docs/cpp_mechanics.txt").string() + "\n" +
		(data_directory / "docs/linda_the_dog.txt").string(), true};

	Button run_button{{margin, 390, 150, 44}, "Run search"};
	Button reset_button{{margin + 166, 390, 150, 44}, "Reset"};
	Button exit_button{{window_width - margin - 150, 390, 150, 44}, "Exit"};

	std::vector<TextField*> fields{&model_path, &query, &documents};
	std::future<std::string> pending_search;
	std::string output = "Enter search values and press Run search.";
	bool running = false;

	InitWindow(window_width, window_height, "GraniteSearch");
	Image icon = LoadImage((data_directory / "granite-search.png").string().c_str());
	if (icon.data != nullptr) {
		SetWindowIcon(icon);
		UnloadImage(icon);
	}

	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		if (IsKeyPressed(KEY_TAB)) {
			size_t selected = 0;
			for (size_t index = 0; index < fields.size(); ++index) {
				if (fields[index]->focused) {
					selected = (index + 1) % fields.size();
					break;
				}
			}
			focus_only(*fields[selected], fields);
		}
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			for (TextField* field : fields) {
				if (CheckCollisionPointRec(GetMousePosition(), field->bounds)) {
					focus_only(*field, fields);
					break;
				}
			}
		}

		bool consumed_mouse = false;
		for (TextField* field : fields) {
			update_field(*field, consumed_mouse, GetFrameTime());
		}

		if (!running && clicked(run_button)) {
			focus_only(model_path, fields);
			std::error_code file_error;
			const bool model_exists = std::filesystem::is_regular_file(model_path.value, file_error);
			const auto model_size = model_exists
				? std::filesystem::file_size(model_path.value, file_error)
				: 0;
			if (file_error || !model_exists || model_size < 1'000'000) {
				output = "Model file is missing or invalid: " + model_path.value +
					"\nRestore the real GGUF model before running the search.";
			} else {
				running = true;
				output = "Running GraniteSearch...";
				pending_search = std::async(std::launch::async, run_semantic_search,
									model_path.value, query.value, split_lines(documents.value));
			}
		}

		if (clicked(reset_button)) {
			model_path.value = default_model_path;
			query.value = "What does Linda like to do in the backyard?";
			documents.value = (data_directory / "docs/ai_programming.txt").string() + "\n" +
				(data_directory / "docs/baking_bread.txt").string() + "\n" +
				(data_directory / "docs/cpp_mechanics.txt").string() + "\n" +
				(data_directory / "docs/linda_the_dog.txt").string();
			output = "Enter search values and press Run search.";
		}

		if (clicked(exit_button)) {
			break;
		}

		if (running && pending_search.wait_for(std::chrono::milliseconds(0)) ==
						   std::future_status::ready) {
			output = pending_search.get();
			running = false;
		}

		BeginDrawing();
		ClearBackground(background);
		DrawText("GraniteSearch", margin, 24, 30, text);
		DrawText("Semantic document search", margin + 220, 32, 18, muted);
		draw_field(model_path);
		draw_field(query);
		draw_field(documents);

		DrawRectangleRec(run_button.bounds, running ? border : accent);
		DrawText(running ? "Running..." : run_button.label,
				 static_cast<int>(run_button.bounds.x + 18),
				 static_cast<int>(run_button.bounds.y + 13), 18, text);
		DrawRectangleRec(reset_button.bounds, border);
		DrawText(reset_button.label, static_cast<int>(reset_button.bounds.x + 38),
				 static_cast<int>(reset_button.bounds.y + 13), 18, text);
		DrawRectangleRec(exit_button.bounds, Color{174, 74, 82, 255});
		DrawText(exit_button.label, static_cast<int>(exit_button.bounds.x + 48),
				 static_cast<int>(exit_button.bounds.y + 13), 18, text);

		DrawText("Results", margin, 466, 22, text);
		DrawRectangleRec({margin, 500, window_width - 2 * margin, 285}, panel);
		const std::vector<std::string> output_lines = split_lines(output);
		const size_t first_line = output_lines.size() > 16 ? output_lines.size() - 16 : 0;
		for (size_t index = first_line; index < output_lines.size(); ++index) {
			std::string line = output_lines[index];
			if (line.size() > 125) {
				line.resize(125);
			}
			DrawText(line.c_str(), margin + 14,
					 514 + static_cast<int>((index - first_line) * 16), 14, text);
		}
		EndDrawing();
	}

	if (running) {
		pending_search.wait();
	}
	CloseWindow();
	return 0;
}
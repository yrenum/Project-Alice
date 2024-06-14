#pragma once

#include "freetype/freetype.h"
#include "freetype/ftglyph.h"
#include "unordered_dense.h"
#include "hb.h"

namespace sys {
struct state;
}

namespace text {

inline constexpr uint32_t max_texture_layers = 256;
inline constexpr int magnification_factor = 4;
inline constexpr int dr_size = 64 * magnification_factor;

enum class font_selection {
	body_font,
	header_font,
	map_font
};

uint16_t name_into_font_id(sys::state& state, std::string_view text);
int32_t size_from_font_id(uint16_t id);
bool is_black_from_font_id(uint16_t id);
font_selection font_index_from_font_id(sys::state& state, uint16_t id);

struct glyph_sub_offset {
	float x = 0.0f;
	float y = 0.0f;
	float x_advance = 0.0f;
	uint16_t texture_slot = 0;
};

class font_manager;

enum class font_feature {
	none, small_caps
};

class font;



struct stored_glyphs {
	std::vector<hb_glyph_info_t> glyph_info;
	std::vector<hb_glyph_position_t> glyph_pos;
	unsigned int glyph_count = 0;

	stored_glyphs() = default;
	stored_glyphs(stored_glyphs const& other) noexcept = default;
	stored_glyphs(stored_glyphs&& other) noexcept = default;
	stored_glyphs(sys::state& state, font_selection type, std::string const& s);
	stored_glyphs(stored_glyphs& other, uint32_t offset, uint32_t count);

	void set_text(sys::state& state, font_selection type, std::string const& s);
	void clear() {
		glyph_info.clear();
		glyph_pos.clear();
		glyph_count = 0;
	}
};

struct stored_text : public stored_glyphs {
	std::string base_text;

	stored_text() = default;
	stored_text(stored_text const& other) noexcept = default;
	stored_text(stored_text&& other) noexcept = default;
	stored_text(sys::state& state, font_selection type, std::string const& s);
	stored_text(sys::state& state, font_selection type, std::string&& s);

	void set_text(sys::state& state, font_selection type, std::string const& s);
	void set_text(sys::state& state, font_selection type, std::string&& s);
	void clear() {
		base_text.clear();
		stored_glyphs::clear();
	}
};

class font {
private:
	font(font const&) = delete;
	font& operator=(font const&) = delete;
public:
	font() = default;

	std::string file_name;
	FT_Face font_face = nullptr;
	hb_font_t* hb_font_face = nullptr;
	hb_buffer_t* hb_buf = nullptr;

	float internal_line_height = 0.0f;
	float internal_ascender = 0.0f;
	float internal_descender = 0.0f;
	float internal_top_adj = 0.0f;
	ankerl::unordered_dense::map<char32_t, glyph_sub_offset> glyph_positions;
	ankerl::unordered_dense::map<uint16_t, uint32_t> texture_slots;

	std::unique_ptr<FT_Byte[]> file_data;

	~font();

	char codepoint_to_alnum(char32_t codepoint);
	bool can_display(char32_t ch_in) const;
	std::string get_conditional_indicator(bool v) const;
	void make_glyph(char32_t ch_in);
	float base_glyph_width(char32_t ch_in);
	float line_height(int32_t size) const;
	float ascender(int32_t size) const;
	float descender(int32_t size) const;
	float top_adjustment(int32_t size) const;
	float text_extent(sys::state& state, stored_glyphs const& txt, uint32_t starting_offset, uint32_t count, int32_t size);
	void remake_cache(sys::state& state, font_selection type, stored_glyphs& txt, std::string const& source);

	friend class font_manager;

	font(font&& o) noexcept : file_name(std::move(o.file_name)), texture_slots(std::move(o.texture_slots)), glyph_positions(std::move(o.glyph_positions)), file_data(std::move(o.file_data)) {
		font_face = o.font_face;
		o.font_face = nullptr;
		hb_font_face = o.hb_font_face;
		o.hb_font_face = nullptr;
		hb_buf = o.hb_buf;
		o.hb_buf = nullptr;
		internal_line_height = o.internal_line_height;
		internal_ascender = o.internal_ascender;
		internal_descender = o.internal_descender;
		internal_top_adj = o.internal_top_adj;
	}
	font& operator=(font&& o) noexcept {
		file_name = std::move(o.file_name);
		file_data = std::move(o.file_data);
		glyph_positions = std::move(o.glyph_positions);
		texture_slots = std::move(o.texture_slots);
		font_face = o.font_face;
		o.font_face = nullptr;
		hb_font_face = o.hb_font_face;
		o.hb_font_face = nullptr;
		hb_buf = o.hb_buf;
		o.hb_buf = nullptr;
		internal_line_height = o.internal_line_height;
		internal_ascender = o.internal_ascender;
		internal_descender = o.internal_descender;
		internal_top_adj = o.internal_top_adj;
	}
};

class font_manager {
public:
	font_manager();
	~font_manager();

	ankerl::unordered_dense::map<uint16_t, dcon::text_key> font_names;
	FT_Library ft_library;
private:
	std::vector<font> font_array;
	dcon::locale_id current_locale;
public:
	bool map_font_is_black = false;

	dcon::locale_id get_current_locale() const {
		return current_locale;
	}
	void change_locale(sys::state& state, dcon::locale_id l);
	font& get_font(sys::state& state, font_selection s = font_selection::body_font);
	void load_font(font& fnt, char const* file_data, uint32_t file_size);
};

} // namespace text

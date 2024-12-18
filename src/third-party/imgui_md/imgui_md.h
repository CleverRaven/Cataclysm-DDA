/*
 * imgui_md: Markdown for Dear ImGui using MD4C
 * (http://https://github.com/mekhontsev/imgui_md)
 *
 * Copyright (c) 2021 Dmitry Mekhontsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef IMGUI_MD_H
#define IMGUI_MD_H

#include "md4c/md4c.h"
#include "imgui/imgui.h"
#include <string>
#include <vector>

#ifdef __cplusplus
namespace imgui_md {
    using namespace md4c;
#endif

struct imgui_md
{
	imgui_md();
	virtual ~imgui_md() {};

	//returns 0 on success
	int print(const char* str, const char* str_end);

	//for example, these flags can be changed in div callback

	//draw border
	bool m_table_border = true;
	//render header in a different way than other rows
	bool m_table_header_highlight = true;

protected:

	virtual void BLOCK_DOC(bool);
	virtual void BLOCK_QUOTE(bool);
	virtual void BLOCK_UL(const MD_BLOCK_UL_DETAIL*, bool);
	virtual void BLOCK_OL(const MD_BLOCK_OL_DETAIL*, bool);
	virtual void BLOCK_LI(const MD_BLOCK_LI_DETAIL*, bool);
	virtual void BLOCK_HR(bool e);
	virtual void BLOCK_H(const MD_BLOCK_H_DETAIL* d, bool e);
	virtual void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL*, bool);
	virtual void BLOCK_HTML(bool);
	virtual void BLOCK_P(bool);
	virtual void BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL*, bool);
	virtual void BLOCK_THEAD(bool);
	virtual void BLOCK_TBODY(bool);
	virtual void BLOCK_TR(bool);
	virtual void BLOCK_TH(const MD_BLOCK_TD_DETAIL*, bool);
	virtual void BLOCK_TD(const MD_BLOCK_TD_DETAIL*, bool);

	virtual void SPAN_EM(bool e);
	virtual void SPAN_STRONG(bool e);
	virtual void SPAN_A(const MD_SPAN_A_DETAIL* d, bool e);
	virtual void SPAN_IMG(const MD_SPAN_IMG_DETAIL*, bool);
	virtual void SPAN_CODE(bool);
	virtual void SPAN_DEL(bool);
	virtual void SPAN_LATEXMATH(bool);
	virtual void SPAN_LATEXMATH_DISPLAY(bool);
	virtual void SPAN_WIKILINK(const MD_SPAN_WIKILINK_DETAIL*, bool);
	virtual void SPAN_U(bool);

	////////////////////////////////////////////////////////////////////////////

	struct image_info
	{
		ImTextureID	texture_id;
		ImVec2	size;
		ImVec2	uv0;
		ImVec2	uv1;
		ImVec4	col_tint;
		ImVec4	col_border;
	};

	//use m_href to identify image
	virtual bool get_image(image_info& nfo) const;

	virtual ImFont* get_font() const;
	virtual ImVec4 get_color() const;


	//url == m_href
	virtual void open_url() const;

	//returns true if the term has been processed
	virtual bool render_entity(const char* str, const char* str_end);

	//returns true if the term has been processed
	virtual bool check_html(const char* str, const char* str_end);

	//called when '\n' in source text where it is not semantically meaningful
	virtual void soft_break();

	//e==true : enter
	//e==false : leave
	virtual void html_div(const std::string& dclass, bool e);
	////////////////////////////////////////////////////////////////////////////

	//current state
	std::string m_href;//empty if no link/image

	bool m_is_underline=false;
	bool m_is_strikethrough = false;
	bool m_is_em = false;
	bool m_is_strong = false;
	bool m_is_table_header = false;
	bool m_is_table_body = false;
	bool m_is_image = false;
	bool m_is_code = false;
	unsigned m_hlevel=0;//0 - no heading

private:

	int text(MD_TEXTTYPE type, const char* str, const char* str_end);
	int block(MD_BLOCKTYPE type, void* d, bool e);
	int span(MD_SPANTYPE type, void* d, bool e);

	void render_text(const char* str, const char* str_end);

	void set_font(bool e);
	void set_color(bool e);
	void set_href(bool e, const MD_ATTRIBUTE& src);

	static void line(ImColor c, bool under);

	//table state
	int m_table_next_column = 0;
	ImVec2 m_table_last_pos;
	std::vector<float> m_table_col_pos;
	std::vector<float> m_table_row_pos;

	//list state
	struct list_info
	{
		unsigned	cur_ol;
		char		delim;
		bool		is_ol;
	};
	std::vector<list_info> m_list_stack;

	std::vector<std::string> m_div_stack;

	MD_PARSER m_md;
};

#ifdef __cplusplus
}
#endif

#endif  /* IMGUI_MD_H */

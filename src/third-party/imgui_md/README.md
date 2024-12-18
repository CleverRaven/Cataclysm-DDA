# imgui_md
Markdown renderer for [Dear ImGui](https://github.com/ocornut/imgui) using [MD4C](https://github.com/mity/md4c) parser.

C++11 or above

imgui_md currently supports the following markdown functionality:

  * Wrapped text
  * Headers 
  * Emphasis
  * Ordered and unordered list, sub-lists
  * Link
  * Image
  * Horizontal rule	
  * Table
  * Underline
  * Strikethrough
  * HTML elements: \<br\> \<hr\> \<u\> \<div\> \&nbsp;
  * Backslash Escapes

Most format tags can be mixed!

Current tables limitations:
  * Width of the columns are defined by header
  * Cells are always left aligned

## Usage

Add imgui_md.h imgui_md.cpp md4c.h md4c.c to your project and use the following code:

```cpp
#include "imgui_md.h"

//Fonts and images (ImTextureID) must be loaded in other place
//see https://github.com/ocornut/imgui/blob/master/docs/FONTS.md
extern ImFont* g_font_regular;
extern ImFont* g_font_bold;
extern ImFont* g_font_bold_large;
extern ImTextureID g_texture1;

struct my_markdown : public imgui_md 
{
	ImFont* get_font() const override
	{
		if (m_is_table_header) {
			return g_font_bold;
		}

		switch (m_hlevel)
		{
		case 0:
			return m_is_strong ? g_font_bold : g_font_regular;
		case 1:
			return g_font_bold_large;
		default:
			return g_font_bold;
		}
	};

	void open_url() const override
	{
		//platform dependent code
		SDL_OpenURL(m_href.c_str());
	}

	bool get_image(image_info& nfo) const override
	{
		//use m_href to identify images
		nfo.texture_id = g_texture1;
		nfo.size = {40,20};
		nfo.uv0 = { 0,0 };
		nfo.uv1 = {1,1};
		nfo.col_tint = { 1,1,1,1 };
		nfo.col_border = { 0,0,0,0 };
		return true;
	}
	
	void html_div(const std::string& dclass, bool e) override
	{
		if (dclass == "red") {
			if (e) {
				m_table_border = false;
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			} else {
				ImGui::PopStyleColor();
				m_table_border = true;
			}
		}
	}
};


//call this function to render your markdown
void markdown(const char* str, const char* str_end)
{
	static my_markdown s_printer;
	s_printer.print(str, str_end);
}

```

## Examples

```markdown
# Table

Name &nbsp; &nbsp; &nbsp; &nbsp; | Multiline &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;<br>header  | [Link&nbsp;](#link1)
:------|:-------------------|:--
Value-One | Long <br>explanation <br>with \<br\>\'s|1
~~Value-Two~~ | __text auto wrapped__\: long explanation here |25 37 43 56 78 90
**etc** | [~~Some **link**~~](https://github.com/mekhontsev/imgui_md)|3
```
<img src="https://user-images.githubusercontent.com/2968582/115061289-30fcc900-9f13-11eb-9d7a-8f32f51e11b3.png" width="677" />

```markdown
# List

1. First ordered list item
2. Another item
   * Unordered sub-list 1.
   * Unordered sub-list 2.
1. Actual numbers don't matter, just that it's a number
   1. **Ordered** sub-list 1
   2. **Ordered** sub-list 2
4. And another item with minuses.
   - __sub-list with underline__
   - sub-list with escapes: \[looks like\]\(a link\)
5. ~~Item with pluses and strikethrough~~.
   + sub-list 1
   + sub-list 2
   + [Just a link](https://github.com/mekhontsev/imgui_md).
      * Item with [link1](#link1)
      * Item with bold [**link2**](#link1)
```

<img src="https://user-images.githubusercontent.com/2968582/115061306-38bc6d80-9f13-11eb-9fb4-1fee53ffed96.png" width="676" />

```markdown
<div class = "red">

This table | is inside an | HTML div
--- | --- | ---
Still | ~~renders~~ | __nicely__
Border | **is not** | visible

</div>
```

<img src="https://user-images.githubusercontent.com/2968582/115060481-1b3ad400-9f12-11eb-87d7-4a1e0eb8ea93.png" width="464" />


## Credits
[imgui_markdown for ideas](https://github.com/juliettef/imgui_markdown)

[Martin Mitáš for MD4C](https://github.com/mity/md4c)

[Omar Cornut for Dear ImGui](https://github.com/ocornut/imgui)  


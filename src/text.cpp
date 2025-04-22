#include <cstddef>
#include <initializer_list>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#undef IMGUI_DEFINE_MATH_OPERATORS

#include "color.h"
#include "text.h"

static uint32_t u32_from_color( nc_color color )
{
    return ImGui::ColorConvertFloat4ToU32( color );
}

namespace cataimgui
{
Segment::Segment( ) = default;
Segment::Segment( std::string_view( sv ), uint32_t c )
    : str( sv )
    , color( c )
{}
Segment::Segment( std::string_view( sv ), nc_color c )
    : str( sv )
    , color( u32_from_color( c ) )
{}

std::shared_ptr<Paragraph> parse_colored_text( std::string_view str, nc_color default_color )
{
    std::shared_ptr<Paragraph> p = std::make_shared<Paragraph>();
    p->append_colored_text( str, default_color );
    return p;
}

Paragraph::Paragraph( )
    : segs( {} )
{}
Paragraph::Paragraph( std::vector<Segment> &s )
    : segs( s )
{}
Paragraph::Paragraph( std::string_view str )
{
    append_colored_text( str, 0 );
}

Paragraph *Paragraph::append( std::string_view str, uint32_t color )
{
    segs.emplace_back( str, color );
    return this;
}

Paragraph *Paragraph::append( std::string_view str, nc_color color )
{
    return append( str, u32_from_color( color ) );
}

Paragraph *Paragraph::append_colored_text( std::string_view str, nc_color default_color )
{
    return append_colored_text( str, u32_from_color( default_color ) );
}

Paragraph *Paragraph::append_colored_text( std::string_view str, uint32_t default_color )
{
    if( str.empty() ) {
        return this;
    }

    size_t s = 0;
    size_t e = 0;
    std::vector<uint32_t> colors = std::vector<uint32_t>( 1, default_color );
    while( std::string::npos != ( e = str.find( '<', s ) ) ) {
        append( str.substr( s, e - s ), colors.back() );
        size_t ce = str.find( '>', e );
        if( std::string::npos != ce ) {
            color_tag_parse_result tag = get_color_from_tag( str.substr( e, ce - e + 1 ) );
            switch( tag.type ) {
                case color_tag_parse_result::open_color_tag:
                    colors.push_back( u32_from_color( tag.color ) );
                    break;
                case color_tag_parse_result::close_color_tag:
                    if( colors.size() > 1 ) {
                        colors.pop_back();
                    }
                    break;
                case color_tag_parse_result::non_color_tag:
                    append( str.substr( e, ce - e + 1 ), colors.back() );
                    break;
            }
            s = ce + 1;
        } else {
            break;
        }
    }
    append( str.substr( s, std::string::npos ), colors.back() );
    return this;
}

Paragraph *Paragraph::black( std::string_view str )
{
    append( str, c_black );
    return this;
}

Paragraph *Paragraph::white( std::string_view str )
{
    append( str, c_white );
    return this;
}

Paragraph *Paragraph::light_gray( std::string_view str )
{
    append( str, c_light_gray );
    return this;
}

Paragraph *Paragraph::dark_gray( std::string_view str )
{
    append( str, c_dark_gray );
    return this;
}

Paragraph *Paragraph::red( std::string_view str )
{
    append( str, c_red );
    return this;
}

Paragraph *Paragraph::green( std::string_view str )
{
    append( str, c_green );
    return this;
}

Paragraph *Paragraph::blue( std::string_view str )
{
    append( str, c_blue );
    return this;
}

Paragraph *Paragraph::cyan( std::string_view str )
{
    append( str, c_cyan );
    return this;
}

Paragraph *Paragraph::magenta( std::string_view str )
{
    append( str, c_magenta );
    return this;
}

Paragraph *Paragraph::brown( std::string_view str )
{
    append( str, c_brown );
    return this;
}

Paragraph *Paragraph::light_red( std::string_view str )
{
    append( str, c_light_red );
    return this;
}

Paragraph *Paragraph::light_green( std::string_view str )
{
    append( str, c_light_green );
    return this;
}

Paragraph *Paragraph::light_blue( std::string_view str )
{
    append( str, c_light_blue );
    return this;
}

Paragraph *Paragraph::light_cyan( std::string_view str )
{
    append( str, c_light_cyan );
    return this;
}

Paragraph *Paragraph::pink( std::string_view str )
{
    append( str, c_pink );
    return this;
}

Paragraph *Paragraph::yellow( std::string_view str )
{
    append( str, c_yellow );
    return this;
}

Paragraph *Paragraph::spaced()
{
    if( !segs.empty() && segs.back().str.back() != ' ' ) {
        append( " ", 0 );
    }
    return this;
}

Paragraph *Paragraph::separated()
{
    separator_before = true;
    return this;
}

static void TextEx( std::string_view str, float wrap_width, uint32_t color )
{
    if( str.empty() ) {
        return;
    }
    ImFont *Font = ImGui::GetFont();
    const char *textStart = str.data();
    const char *textEnd = textStart + str.length();

    do {
        float widthRemaining = ImGui::CalcWrapWidthForPos( ImGui::GetCursorScreenPos(), wrap_width );
        const char *drawEnd = Font->CalcWordWrapPositionA( 1.0f, textStart, textEnd, widthRemaining );
        if( textStart == drawEnd ) {
            ImGui::NewLine();
            // NewLine assumes that we are done with one item and are
            // adding another so it advances CursorPos.y by
            // ItemSpacing.y. Since we know that this is just part of
            // a paragraph, undo just that small extra spacing.
            ImGui::GetCurrentWindow()->DC.CursorPos.y -= ImGui::GetStyle().ItemSpacing.y;
            drawEnd = Font->CalcWordWrapPositionA( 1.0f, textStart, textEnd, widthRemaining );
        } else {
            if( color ) {
                ImGui::PushStyleColor( ImGuiCol_Text, color );
            }
            ImGui::TextUnformatted( textStart, textStart == drawEnd ? nullptr : drawEnd );
            // see above
            ImGui::GetCurrentWindow()->DC.CursorPos.y -= ImGui::GetStyle().ItemSpacing.y;
            if( color ) {
                ImGui::PopStyleColor();
            }
        }

        if( textStart == drawEnd || drawEnd == textEnd ) {
            ImGui::SameLine( 0.0f, 0.0f );
            break;
        }

        textStart = drawEnd;

        while( textStart < textEnd ) {
            const char c = *textStart;
            if( ImCharIsBlankA( c ) ) {
                textStart++;
            } else if( c == '\n' ) {
                textStart++;
                break;
            } else {
                break;
            }
        }
    } while( true );
}

static void TextSegmentEx( const Segment &seg, float wrap_width, bool styled )
{
    TextEx( seg.str, wrap_width, styled ? seg.color : 0 );
}

static void TextParagraphEx( std::shared_ptr<Paragraph> &para, float wrap_width, bool styled )
{
    for( const Segment &seg : para->segs ) {
        TextSegmentEx( seg, wrap_width, styled );
    }
    ImGui::NewLine();
}

void TextStyled( std::shared_ptr<Paragraph> para, float wrap_width )
{
    TextParagraphEx( para, wrap_width, true );
}

void TextUnstyled( std::shared_ptr<Paragraph> para, float wrap_width )
{
    TextParagraphEx( para, wrap_width, false );
}

void TextParagraph( nc_color color, std::string_view para, float wrap_width )
{
    TextEx( para, wrap_width, u32_from_color( color ) );
}

void TextColoredParagraph( nc_color default_color, std::string_view str,
                           std::optional<Segment> value, float wrap_width )
{
    if( str.empty() ) {
        return;
    }

    size_t s = 0;
    size_t e = 0;
    std::vector<uint32_t> colors = std::vector<uint32_t>( 1, u32_from_color( default_color ) );
    while( std::string::npos != ( e = str.find( '<', s ) ) ) {
        TextEx( str.substr( s, e - s ), wrap_width, colors.back() );
        size_t ce = str.find_first_of( "<>", e + 1 );
        if( std::string::npos != ce ) {
            // back up by one if we find the start of another tag instead of the end of this one
            // an annoying example from help.json: "<color_light_green>^>v<</color>"
            if( str[ ce ] == '<' ) {
                ce -= 1;
            }
            size_t len = ce - e + 1;
            std::string_view tagname = str.substr( e, len );
            if( "<num>" == tagname && value.has_value() ) {
                TextSegmentEx( value.value(), wrap_width, true );
            } else {
                color_tag_parse_result tag = get_color_from_tag( tagname );
                switch( tag.type ) {
                    case color_tag_parse_result::open_color_tag:
                        colors.push_back( u32_from_color( tag.color ) );
                        break;
                    case color_tag_parse_result::close_color_tag:
                        if( colors.size() > 1 ) {
                            colors.pop_back();
                        }
                        break;
                    case color_tag_parse_result::non_color_tag:
                        TextEx( tagname, wrap_width, colors.back() );
                        break;
                }
            }
            s = ce + 1;
        } else {
            break;
        }
    }
    TextEx( str.substr( s, std::string::npos ), wrap_width, colors.back() );
}

} // namespace cataimgui

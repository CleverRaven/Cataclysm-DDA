#pragma once
#ifndef CATA_SRC_TEXT_H
#define CATA_SRC_TEXT_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class nc_color;

// This is an experimental text API. Where possible, it is better to
// use TextParagraph or TextColoredParagraph to wrap text rather than
// allocating Segments and Paragraphs.

namespace cataimgui
{
struct Segment {
    std::string str;
    uint32_t color;

    Segment( );
    // NOLINTBEGIN(google-explicit-constructor)
    Segment( const std::string_view( sv ), uint32_t c = 0 );
    Segment( const std::string_view( sv ), nc_color c );
    // NOLINTEND(google-explicit-constructor)
};

struct Paragraph {
    std::vector<Segment> segs;
    bool separator_before = false;

    Paragraph( );
    explicit Paragraph( std::vector<Segment> &s );
    explicit Paragraph( std::string_view str );
    Paragraph *append( std::string_view str, uint32_t color );
    Paragraph *append( std::string_view str, nc_color color );
    Paragraph *append_colored_text( std::string_view str, uint32_t default_color );
    Paragraph *append_colored_text( std::string_view str, nc_color default_color );

    Paragraph *black( std::string_view str );
    Paragraph *white( std::string_view str );
    Paragraph *light_gray( std::string_view str );
    Paragraph *dark_gray( std::string_view str );
    Paragraph *red( std::string_view str );
    Paragraph *green( std::string_view str );
    Paragraph *blue( std::string_view str );
    Paragraph *cyan( std::string_view str );
    Paragraph *magenta( std::string_view str );
    Paragraph *brown( std::string_view str );
    Paragraph *light_red( std::string_view str );
    Paragraph *light_green( std::string_view str );
    Paragraph *light_blue( std::string_view str );
    Paragraph *light_cyan( std::string_view str );
    Paragraph *pink( std::string_view str );
    Paragraph *yellow( std::string_view str );

    Paragraph *spaced();
    Paragraph *separated();
};

std::shared_ptr<Paragraph> parse_colored_text( std::string_view str, nc_color default_color );
void TextStyled( std::shared_ptr<Paragraph> para, float wrap_width = 0.0f );
void TextUnstyled( std::shared_ptr<Paragraph> para, float wrap_width = 0.0f );
void TextParagraph( nc_color color, std::string_view para, float wrap_width = 0.0f );
void TextColoredParagraph( nc_color color, std::string_view str,
                           std::optional<Segment> value = std::nullopt, float wrap_width = 0.0f );

} // namespace cataimgui

#endif // CATA_SRC_TEXT_H

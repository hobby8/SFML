////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2015 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/MyText.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <cmath>


namespace sf
{
////////////////////////////////////////////////////////////
MyText::MyText() :
m_string            (),
m_font              (NULL),
m_characterSize     (30),
m_style             (Regular),
m_color             (255, 255, 255),
m_bounds            (),
m_geometryNeedUpdate(false)
{

}


////////////////////////////////////////////////////////////
MyText::MyText(const String& string, const MyFont& font, unsigned int characterSize) :
m_string            (string),
m_font              (&font),
m_characterSize     (characterSize),
m_style             (Regular),
m_color             (255, 255, 255),
m_bounds            (),
m_geometryNeedUpdate(true)
{

}


////////////////////////////////////////////////////////////
void MyText::setString(const String& string)
{
    if (m_string != string)
    {
        m_string = string;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
void MyText::setFont(const MyFont& font)
{
    if (m_font != &font)
    {
        m_font = &font;
        m_geometryNeedUpdate = true;

        // MyGlyph textures will change, so delete all VertexArray instances
        m_verticesMap.clear();
    }
}


////////////////////////////////////////////////////////////
void MyText::setCharacterSize(unsigned int size)
{
    if (m_characterSize != size)
    {
        m_characterSize = size;
        m_geometryNeedUpdate = true;

        // MyGlyph textures will change, so delete all VertexArray instances
        m_verticesMap.clear();
    }
}


////////////////////////////////////////////////////////////
void MyText::setStyle(Uint32 style)
{
    if (m_style != style)
    {
        m_style = style;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
void MyText::setColor(const Color& color)
{
    if (color != m_color)
    {
        m_color = color;

        // Change vertex colors directly, no need to update whole geometry
        // (if geometry is updated anyway, we can skip this step)
        if (!m_geometryNeedUpdate)
        {
            for (VertexArrayMap::iterator it = m_verticesMap.begin(); it != m_verticesMap.end(); ++it)
            {
                VertexArray& vertices = it->second;
                for (std::size_t i = 0; i < vertices.getVertexCount(); ++i)
                    vertices[i].color = m_color;
            }
        }
    }
}


////////////////////////////////////////////////////////////
const String& MyText::getString() const
{
    return m_string;
}


////////////////////////////////////////////////////////////
const MyFont* MyText::getFont() const
{
    return m_font;
}


////////////////////////////////////////////////////////////
unsigned int MyText::getCharacterSize() const
{
    return m_characterSize;
}


////////////////////////////////////////////////////////////
Uint32 MyText::getStyle() const
{
    return m_style;
}


////////////////////////////////////////////////////////////
const Color& MyText::getColor() const
{
    return m_color;
}


////////////////////////////////////////////////////////////
Vector2f MyText::findCharacterPos(std::size_t index) const
{
    // Make sure that we have a valid font
    if (!m_font)
        return Vector2f();

    // Adjust the index if it's out of range
    if (index > m_string.getSize())
        index = m_string.getSize();

    // Precompute the variables needed by the algorithm
    bool  bold   = (m_style & Bold) != 0;
    float hspace = static_cast<float>(m_font->getGlyph(L' ', m_characterSize, bold).advance);
    float vspace = static_cast<float>(m_font->getLineSpacing(m_characterSize));

    // Compute the position
    Vector2f position;
    Uint32 prevChar = 0;
    for (std::size_t i = 0; i < index; ++i)
    {
        Uint32 curChar = m_string[i];

        // Apply the kerning offset
        position.x += static_cast<float>(m_font->getKerning(prevChar, curChar, m_characterSize));
        prevChar = curChar;

        // Handle special characters
        switch (curChar)
        {
            case ' ':  position.x += hspace;                 continue;
            case '\t': position.x += hspace * 4;             continue;
            case '\n': position.y += vspace; position.x = 0; continue;
        }

        // For regular characters, add the advance offset of the glyph
        position.x += static_cast<float>(m_font->getGlyph(curChar, m_characterSize, bold).advance);
    }

    // Transform the position to global coordinates
    position = getTransform().transformPoint(position);

    return position;
}


////////////////////////////////////////////////////////////
FloatRect MyText::getLocalBounds() const
{
    ensureGeometryUpdate();

    return m_bounds;
}


////////////////////////////////////////////////////////////
FloatRect MyText::getGlobalBounds() const
{
    return getTransform().transformRect(getLocalBounds());
}


////////////////////////////////////////////////////////////
void MyText::draw(RenderTarget& target, RenderStates states) const
{
    if (m_font)
    {
        ensureGeometryUpdate();

        states.transform *= getTransform();

        for (VertexArrayMap::iterator it = m_verticesMap.begin(); it != m_verticesMap.end(); ++it)
        {
            if (it->second.getVertexCount() > 0)
            {
                states.texture = it->first;
                target.draw(it->second, states);
            }
        }
    }
}


////////////////////////////////////////////////////////////
void MyText::ensureGeometryUpdate() const
{
    // Do nothing, if geometry has not changed
    if (!m_geometryNeedUpdate)
        return;

    // Mark geometry as updated
    m_geometryNeedUpdate = false;

    // Clear the previous geometry but keep all VertexArray instances so they can reuse their allocated memory
    for (VertexArrayMap::iterator it = m_verticesMap.begin(); it != m_verticesMap.end(); ++it)
        it->second.clear();
    m_bounds = FloatRect();

    // No font: nothing to draw
    if (!m_font)
        return;

    // No text: nothing to draw
    if (m_string.isEmpty())
        return;

    // Compute values related to the text style
    bool  bold               = (m_style & Bold) != 0;
    bool  underlined         = (m_style & Underlined) != 0;
    bool  strikeThrough      = (m_style & StrikeThrough) != 0;
    float italic             = (m_style & Italic) ? 0.208f : 0.f; // 12 degrees
    float underlineOffset    = m_font->getUnderlinePosition(m_characterSize);
    float underlineThickness = m_font->getUnderlineThickness(m_characterSize);

    // Compute the location of the strike through dynamically
    // We use the center point of the lowercase 'x' glyph as the reference
    // We reuse the underline thickness as the thickness of the strike through as well
    FloatRect xBounds = m_font->getGlyph(L'x', m_characterSize, bold).bounds;
    float strikeThroughOffset = xBounds.top + xBounds.height / 2.f;

    // Precompute the variables needed by the algorithm
    float hspace = static_cast<float>(m_font->getGlyph(L' ', m_characterSize, bold).advance);
    float vspace = static_cast<float>(m_font->getLineSpacing(m_characterSize));
    float x      = 0.f;
    float y      = static_cast<float>(m_characterSize);

    // Create one quad for each character
    float minX = static_cast<float>(m_characterSize);
    float minY = static_cast<float>(m_characterSize);
    float maxX = 0.f;
    float maxY = 0.f;
    Uint32 prevChar = 0;
    for (std::size_t i = 0; i < m_string.getSize(); ++i)
    {
        Uint32 curChar = m_string[i];

        // Apply the kerning offset
        x += static_cast<float>(m_font->getKerning(prevChar, curChar, m_characterSize));
        prevChar = curChar;

        // If we're using the underlined style and there's a new line, draw a line
        if (underlined && (curChar == L'\n'))
        {
            float top = std::floor(y + underlineOffset - (underlineThickness / 2) + 0.5f);
            float bottom = top + std::floor(underlineThickness + 0.5f);

            VertexArray& vertices = m_verticesMap[NULL];
            vertices.append(Vertex(Vector2f(0, top),    m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(x, bottom), m_color, Vector2f(1, 1)));
        }

        // If we're using the strike through style and there's a new line, draw a line across all characters
        if (strikeThrough && (curChar == L'\n'))
        {
            float top = std::floor(y + strikeThroughOffset - (underlineThickness / 2) + 0.5f);
            float bottom = top + std::floor(underlineThickness + 0.5f);

            VertexArray& vertices = m_verticesMap[NULL];
            vertices.append(Vertex(Vector2f(0, top),    m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
            vertices.append(Vertex(Vector2f(x, bottom), m_color, Vector2f(1, 1)));
        }

        // Handle special characters
        if ((curChar == ' ') || (curChar == '\t') || (curChar == '\n'))
        {
            // Update the current bounds (min coordinates)
            minX = std::min(minX, x);
            minY = std::min(minY, y);

            switch (curChar)
            {
                case ' ':  x += hspace;        break;
                case '\t': x += hspace * 4;    break;
                case '\n': y += vspace; x = 0; break;
            }

            // Update the current bounds (max coordinates)
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);

            // Next glyph, no need to create a quad for whitespace
            continue;
        }

        // Extract the current glyph's description
        const MyGlyph& glyph = m_font->getGlyph(curChar, m_characterSize, bold);

        float left   = glyph.bounds.left;
        float top    = glyph.bounds.top;
        float right  = glyph.bounds.left + glyph.bounds.width;
        float bottom = glyph.bounds.top  + glyph.bounds.height;

        float u1 = static_cast<float>(glyph.textureRect.left);
        float v1 = static_cast<float>(glyph.textureRect.top);
        float u2 = static_cast<float>(glyph.textureRect.left + glyph.textureRect.width);
        float v2 = static_cast<float>(glyph.textureRect.top  + glyph.textureRect.height);

        // Add a quad for the current character
        VertexArray& vertices = m_verticesMap[glyph.texture];
        vertices.append(Vertex(Vector2f(x + left  - italic * top,    y + top),    m_color, Vector2f(u1, v1)));
        vertices.append(Vertex(Vector2f(x + right - italic * top,    y + top),    m_color, Vector2f(u2, v1)));
        vertices.append(Vertex(Vector2f(x + left  - italic * bottom, y + bottom), m_color, Vector2f(u1, v2)));
        vertices.append(Vertex(Vector2f(x + left  - italic * bottom, y + bottom), m_color, Vector2f(u1, v2)));
        vertices.append(Vertex(Vector2f(x + right - italic * top,    y + top),    m_color, Vector2f(u2, v1)));
        vertices.append(Vertex(Vector2f(x + right - italic * bottom, y + bottom), m_color, Vector2f(u2, v2)));

        // Update the current bounds
        minX = std::min(minX, x + left - italic * bottom);
        maxX = std::max(maxX, x + right - italic * top);
        minY = std::min(minY, y + top);
        maxY = std::max(maxY, y + bottom);

        // Advance to the next character
        x += glyph.advance;
    }

    // If we're using the underlined style, add the last line
    if (underlined)
    {
        float top = std::floor(y + underlineOffset - (underlineThickness / 2) + 0.5f);
        float bottom = top + std::floor(underlineThickness + 0.5f);

        VertexArray& vertices = m_verticesMap[NULL];
        vertices.append(Vertex(Vector2f(0, top),    m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(x, bottom), m_color, Vector2f(1, 1)));
    }

    // If we're using the strike through style, add the last line across all characters
    if (strikeThrough)
    {
        float top = std::floor(y + strikeThroughOffset - (underlineThickness / 2) + 0.5f);
        float bottom = top + std::floor(underlineThickness + 0.5f);

        VertexArray& vertices = m_verticesMap[NULL];
        vertices.append(Vertex(Vector2f(0, top),    m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(0, bottom), m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(x, top),    m_color, Vector2f(1, 1)));
        vertices.append(Vertex(Vector2f(x, bottom), m_color, Vector2f(1, 1)));
    }

    // Update the bounding rectangle
    m_bounds.left = minX;
    m_bounds.top = minY;
    m_bounds.width = maxX - minX;
    m_bounds.height = maxY - minY;

    // Finally, set type of primitives to triangles
    for (VertexArrayMap::iterator it = m_verticesMap.begin(); it != m_verticesMap.end(); ++it)
        it->second.setPrimitiveType(Triangles);
}

} // namespace sf

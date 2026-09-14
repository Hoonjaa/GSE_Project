#include "stdafx.h"
#include "Renderer.h"
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <limits>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Rasterize Windows Korean glyphs once, then draw their cached coverage in OpenGL.
// The bounded atlas can be recycled after pending vertices have been flushed.
struct FontAtlas
{
    enum
    {
        Size = 1024,
        Cell = 64,
        Columns = Size / Cell,
        Capacity = Columns * Columns
    };

    struct Glyph
    {
        float advance = 0, left = 0, top = 0, width = 0, height = 0;
        float u = 0, v = 0, U = 0, V = 0;
    };

    HDC dc = nullptr;
    HFONT font = nullptr;
    HGDIOBJ previousFont = nullptr;
    GLuint texture = 0;
    int ascent = 28;
    std::map<wchar_t, Glyph> glyphs;

    FontAtlas()
    {
        dc = CreateCompatibleDC(nullptr);
        if (!dc)
        {
            return;
        }
        font = CreateFontW(
            -28,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            HANGEUL_CHARSET,
            OUT_TT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Malgun Gothic");
        if (!font)
        {
            return;
        }
        previousFont = SelectObject(dc, font);
        if (!previousFont || previousFont == HGDI_ERROR)
        {
            return;
        }
        TEXTMETRICW metrics = {};
        if (!GetTextMetricsW(dc, &metrics))
        {
            return;
        }
        ascent = metrics.tmAscent;
        // Detect absent Korean coverage instead of silently displaying empty boxes.
        WORD index = 0xffff;
        if (GetGlyphIndicesW(dc, L"가", 1, &index, GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR ||
            index == 0xffff)
        {
            return;
        }
        glGenTextures(1, &texture);
        if (!texture)
        {
            return;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Reset();
    }

    ~FontAtlas()
    {
        if (texture)
        {
            glDeleteTextures(1, &texture);
        }
        if (dc && previousFont && previousFont != HGDI_ERROR)
        {
            SelectObject(dc, previousFont);
        }
        if (font)
        {
            DeleteObject(font);
        }
        if (dc)
        {
            DeleteDC(dc);
        }
    }

    void Reset()
    {
        glyphs.clear();
        std::vector<unsigned char> empty(Size * Size, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, Size, Size, 0, GL_RED, GL_UNSIGNED_BYTE, empty.data());
    }

    bool Full(wchar_t ch) const
    {
        return glyphs.size() >= Capacity && glyphs.find(ch) == glyphs.end();
    }

    Glyph Get(wchar_t ch)
    {
        auto found = glyphs.find(ch);
        if (found != glyphs.end())
        {
            return found->second;
        }
        const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
        GLYPHMETRICS metrics = {};
        DWORD count = GetGlyphOutlineW(dc, ch, GGO_GRAY8_BITMAP, &metrics, 0, nullptr, &identity);
        if (count == GDI_ERROR || metrics.gmBlackBoxX > Cell - 2 || metrics.gmBlackBoxY > Cell - 2)
        {
            if (ch != L'?')
            {
                return Get(L'?');
            }
            Glyph missing;
            missing.advance = 28;
            return missing;
        }
        std::vector<unsigned char> pixels(Cell * Cell, 0);
        if (count > 0)
        {
            std::vector<unsigned char> bitmap(count);
            if (GetGlyphOutlineW(dc, ch, GGO_GRAY8_BITMAP, &metrics, count, bitmap.data(), &identity) ==
                GDI_ERROR)
            {
                return ch != L'?' ? Get(L'?') : Glyph{};
            }
            const DWORD stride = (metrics.gmBlackBoxX + 3) & ~3u;
            for (DWORD y = 0; y < metrics.gmBlackBoxY; ++y)
            {
                for (DWORD x = 0; x < metrics.gmBlackBoxX; ++x)
                {
                    pixels[(y + 1) * Cell + x + 1] = static_cast<unsigned char>(
                        (std::min)(255u, static_cast<unsigned int>(bitmap[y * stride + x]) * 255u / 64u));
                }
            }
        }
        const int slot = static_cast<int>(glyphs.size());
        const int x = slot % Columns * Cell, y = slot / Columns * Cell;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, Cell, Cell, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        Glyph glyph;
        glyph.advance = static_cast<float>(metrics.gmCellIncX);
        glyph.left = static_cast<float>(metrics.gmptGlyphOrigin.x);
        glyph.top = static_cast<float>(ascent - metrics.gmptGlyphOrigin.y);
        glyph.width = static_cast<float>(metrics.gmBlackBoxX);
        glyph.height = static_cast<float>(metrics.gmBlackBoxY);
        glyph.u = (x + 1.f) / Size;
        glyph.v = (y + 1.f) / Size;
        glyph.U = (x + 1.f + glyph.width) / Size;
        glyph.V = (y + 1.f + glyph.height) / Size;
        glyphs.emplace(ch, glyph);
        return glyph;
    }
};

namespace
{
    constexpr float Pi = 3.14159265359f;

    Color Shade(Color c, float s)
    {
        return {c.r * s, c.g * s, c.b * s};
    }

    std::string ShaderSource(const wchar_t* name)
    {
        wchar_t executable[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
        if (!length || length >= MAX_PATH)
        {
            return {};
        }
        const std::wstring path(executable, length);
        const auto folder = path.substr(0, path.find_last_of(L"/\\") + 1);
        const std::wstring shaderPath = folder + L"Shaders\\" + name;
        std::ifstream file(shaderPath.c_str(), std::ios::binary);
        if (!file)
        {
            std::cerr << "Shader file missing beside executable. Rebuild to copy Shaders.\n";
            return {};
        }
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    GLuint Compile(GLenum type, const std::string& source)
    {
        if (source.empty())
        {
            return 0;
        }
        GLuint shader = glCreateShader(type);
        if (!shader)
        {
            return 0;
        }
        const char* text = source.c_str();
        glShaderSource(shader, 1, &text, nullptr);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[4096] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << log << '\n';
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint LoadProgram(const wchar_t* vertexFile, const wchar_t* fragmentFile)
    {
        const GLuint vs = Compile(GL_VERTEX_SHADER, ShaderSource(vertexFile));
        const GLuint fs = Compile(GL_FRAGMENT_SHADER, ShaderSource(fragmentFile));
        GLuint result = 0;
        if (vs && fs)
        {
            GLuint program = glCreateProgram();
            if (program)
            {
                glAttachShader(program, vs);
                glAttachShader(program, fs);
                glLinkProgram(program);
                GLint success = GL_FALSE;
                glGetProgramiv(program, GL_LINK_STATUS, &success);
                if (success)
                {
                    result = program;
                }
                else
                {
                    char log[4096] = {};
                    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
                    std::cerr << log << '\n';
                    glDeleteProgram(program);
                }
            }
        }
        if (vs)
        {
            glDeleteShader(vs);
        }
        if (fs)
        {
            glDeleteShader(fs);
        }
        return result;
    }

    float LinearColor(float value)
    {
        return value <= .04045f ? value / 12.92f : std::pow((value + .055f) / 1.055f, 2.4f);
    }

}

namespace Geometry
{
    void Triangle(Mesh& m, Vec3 a, Vec3 b, Vec3 c, Color col)
    {
        m.push_back({a, col});
        m.push_back({b, col});
        m.push_back({c, col});
    }

    void Quad(Mesh& m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color col)
    {
        Triangle(m, a, b, c, col);
        Triangle(m, a, c, d, col);
    }

    void Box(Mesh& m, Vec3 p, Vec3 s, Color c)
    {
        float x = p.x, y = p.y, z = p.z, X = x + s.x, Y = y + s.y, Z = z + s.z;
        Quad(m, {x, Y, z}, {X, Y, z}, {X, Y, Z}, {x, Y, Z}, c);
        Quad(m, {x, y, z}, {X, y, z}, {X, Y, z}, {x, Y, z}, Shade(c, .72f));
        Quad(m, {x, y, Z}, {X, y, Z}, {X, Y, Z}, {x, Y, Z}, Shade(c, .88f));
        Quad(m, {x, y, z}, {x, y, Z}, {x, Y, Z}, {x, Y, z}, Shade(c, .65f));
        Quad(m, {X, y, z}, {X, y, Z}, {X, Y, Z}, {X, Y, z}, Shade(c, .8f));
    }

    void Cone(Mesh& m, Vec3 p, float r, float h, Color c, int sides)
    {
        for (int i = 0; i < sides; ++i)
        {
            float a = 2 * Pi * i / sides, b = 2 * Pi * (i + 1) / sides;
            Triangle(
                m,
                {p.x + r * std::cos(a), p.y, p.z + r * std::sin(a)},
                {p.x, p.y + h, p.z},
                {p.x + r * std::cos(b), p.y, p.z + r * std::sin(b)},
                Shade(c, .75f + .25f * (.5f + .5f * std::cos(a))));
        }
    }

    void Disc(Mesh& m, Vec3 p, float r, Color c, int sides)
    {
        for (int i = 0; i < sides; ++i)
        {
            float a = 2 * Pi * i / sides, b = 2 * Pi * (i + 1) / sides;
            Triangle(
                m,
                p,
                {p.x + r * std::cos(a), p.y, p.z + r * std::sin(a)},
                {p.x + r * std::cos(b), p.y, p.z + r * std::sin(b)},
                c);
        }
    }
}

Renderer::Renderer(int w, int h) : m_width(w), m_height(h)
{
    m_program = LoadProgram(L"SolidRect.vs", L"SolidRect.fs");
    m_worldProgram = LoadProgram(L"World.vs", L"World.fs");
    if (!m_program || !m_worldProgram)
    {
        return;
    }
    m_worldOffset = glGetUniformLocation(m_worldProgram, "u_Offset");
    m_worldRotation = glGetUniformLocation(m_worldProgram, "u_Rotation");
    m_worldProjection = glGetUniformLocation(m_worldProgram, "u_ProjectionScale");
    m_worldHdr = glGetUniformLocation(m_worldProgram, "u_HdrScene");
    m_worldIntensity = glGetUniformLocation(m_worldProgram, "u_SceneIntensity");
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, color)));
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, uv)));
    m_font.reset(new FontAtlas());
    m_fontReady = m_font->texture != 0;
    if (!m_fontReady)
    {
        std::cerr << "Korean font initialization failed. Install the Windows Malgun Gothic font.\n";
    }
    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "u_Font"), 0);
    m_post.reset(new PostProcessing(
        LoadProgram(L"PostProcess.vs", L"PostProcess.fs"),
        LoadProgram(L"PostProcess.vs", L"Blur.fs")));
    glUseProgram(m_program);
    m_batch.reserve(300000);
    Resize(w, h);
}

Renderer::~Renderer()
{
    for (const auto& entry : m_meshCache)
    {
        glDeleteBuffers(1, &entry.second.vbo);
        glDeleteVertexArrays(1, &entry.second.vao);
    }
    if (m_worldProgram)
    {
        glDeleteProgram(m_worldProgram);
    }
    m_font.reset();
    m_post.reset();
    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_program)
    {
        glDeleteProgram(m_program);
    }
}

void Renderer::Resize(int w, int h)
{
    m_width = w > 0 ? w : 1;
    m_height = h > 0 ? h : 1;
    if (m_post)
    {
        m_post->Resize(m_width, m_height);
    }
    glViewport(0, 0, m_width, m_height);
}

void Renderer::SetPostProcessing(const PostProcessingSettings& settings)
{
    m_postSettings = settings;
    auto bounded = [](float value, float fallback, float low, float high)
    {
        if (!std::isfinite(value))
        {
            value = fallback;
        }
        return (std::max)(low, (std::min)(high, value));
    };
    m_postSettings.exposure = bounded(settings.exposure, 1.f, .1f, 4.f);
    m_postSettings.sceneIntensity = bounded(settings.sceneIntensity, 2.f, .1f, 8.f);
    m_postSettings.vignetteStrength = bounded(settings.vignetteStrength, .24f, 0.f, .8f);
    m_postSettings.blurRadius = bounded(settings.blurRadius, 3.f, 0.f, 12.f);
    m_postSettings.blurStrength = bounded(settings.blurStrength, .85f, 0.f, 1.f);
    m_postSettings.blurStart = bounded(settings.blurStart, .42f, 0.f, .95f);
    m_postSettings.blurEnd = bounded(settings.blurEnd, .98f, m_postSettings.blurStart + .01f, 1.5f);
}

void Renderer::Begin(float zoom)
{
    // Unloaded chunks must not retain GPU memory in an endlessly growing world.
    for (auto it = m_meshCache.begin(); it != m_meshCache.end();)
    {
        if (it->first.expired())
        {
            glDeleteBuffers(1, &it->second.vbo);
            glDeleteVertexArrays(1, &it->second.vao);
            it = m_meshCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_zoom = zoom;
    m_batch.clear();
    m_triangleCount = 0;
    m_overlay = false;
    m_hdrScene = m_postSettings.enabled && PostProcessingAvailable();
    glDisable(GL_FRAMEBUFFER_SRGB);
    if (m_hdrScene)
    {
        m_post->BeginScene();
        glClearColor(
            LinearColor(.055f) * m_postSettings.sceneIntensity,
            LinearColor(.40f) * m_postSettings.sceneIntensity,
            LinearColor(.51f) * m_postSettings.sceneIntensity,
            1);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);
        glClearColor(.055f, .40f, .51f, 1);
    }
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void Renderer::Submit(const Mesh& mesh, Vec3 o, float rotation)
{
    const float c = std::cos(rotation), s = std::sin(rotation);
    for (const Vertex& v : mesh)
    {
        const float x = v.position.x * c - v.position.z * s + o.x;
        const float z = v.position.x * s + v.position.z * c + o.z;
        const float y = v.position.y + o.y;
        // Orthographic isometric basis. Relative coordinates avoid float drift.
        Vec3 clip = {
            (x - z) * .70710678f * m_zoom * 2 / m_width,
            (y * .81649658f - (x + z) * .40824829f) * m_zoom * 2 / m_height,
            -(x + y + z) * .57735027f / 2048.f};
        m_batch.push_back({clip, v.color});
    }
}

void Renderer::Submit(const CachedMesh& mesh, Vec3 offset, float rotation)
{
    if (!mesh.m_vertices || mesh.m_vertices->empty())
    {
        return;
    }
    Flush();
    const MeshKey key(mesh.m_vertices);
    auto found = m_meshCache.find(key);
    if (found == m_meshCache.end())
    {
        GpuMesh gpu;
        gpu.count = static_cast<GLsizei>(mesh.m_vertices->size());
        glGenVertexArrays(1, &gpu.vao);
        glGenBuffers(1, &gpu.vbo);
        if (!gpu.vao || !gpu.vbo)
        {
            glDeleteBuffers(1, &gpu.vbo);
            glDeleteVertexArrays(1, &gpu.vao);
            Submit(*mesh.m_vertices, offset, rotation);
            return;
        }
        glBindVertexArray(gpu.vao);
        glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.m_vertices->size() * sizeof(Vertex)),
            mesh.m_vertices->data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, position)));
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, color)));
        found = m_meshCache.emplace(key, gpu).first;
    }
    glUseProgram(m_worldProgram);
    glUniform3f(m_worldOffset, offset.x, offset.y, offset.z);
    glUniform2f(m_worldRotation, std::cos(rotation), std::sin(rotation));
    glUniform2f(m_worldProjection, m_zoom * 2 / m_width, m_zoom * 2 / m_height);
    glUniform1i(m_worldHdr, m_hdrScene && !m_overlay);
    glUniform1f(m_worldIntensity, m_postSettings.sceneIntensity);
    glBindVertexArray(found->second.vao);
    glDrawArrays(GL_TRIANGLES, 0, found->second.count);
    m_triangleCount += found->second.count / 3;
}

Vec2 Renderer::Project(Vec3 p) const
{
    return {
        m_width * .5f + (p.x - p.z) * .70710678f * m_zoom,
        m_height * .5f - (p.y * .81649658f - (p.x + p.z) * .40824829f) * m_zoom};
}

Vec3 Renderer::ScreenGround(int x, int y, float height) const
{
    const float horizontal = (x - m_width * .5f) / m_zoom;
    const float vertical = (m_height * .5f - y) / m_zoom;
    const float sum = (height * .81649658f - vertical) / .40824829f;
    const float difference = horizontal / .70710678f;
    return {(sum + difference) * .5f, height, (sum - difference) * .5f};
}

void Renderer::Flush()
{
    if (m_batch.empty())
    {
        return;
    }
    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "u_HdrScene"), m_hdrScene && !m_overlay);
    glUniform1f(glGetUniformLocation(m_program, "u_SceneIntensity"), m_postSettings.sceneIntensity);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_font ? m_font->texture : 0);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_batch.size() * sizeof(Vertex)),
        m_batch.data(),
        GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_batch.size()));
    m_triangleCount += m_batch.size() / 3;
    m_batch.clear();
}

void Renderer::PresentScene()
{
    if (m_overlay)
    {
        return;
    }
    Flush();
    if (m_hdrScene)
    {
        m_post->Composite(m_postSettings);
    }
    m_overlay = true;
}

void Renderer::BeginOverlay()
{
    PresentScene();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::Panel(float x, float y, float w, float h, Color c)
{
    const float l = 2 * x / m_width - 1, r = 2 * (x + w) / m_width - 1;
    const float t = 1 - 2 * y / m_height, b = 1 - 2 * (y + h) / m_height;
    Geometry::Quad(m_batch, {l, t, 0}, {r, t, 0}, {r, b, 0}, {l, b, 0}, c);
}

void Renderer::Text(float x, float y, const std::string& text, Color c, float scale, float maxWidth)
{
    if (!m_fontReady || text.empty() || scale <= 0 ||
        text.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
    {
        return;
    }
    const int length = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0);
    if (length <= 0)
    {
        std::cerr << "Text requires valid UTF-8 input.\n";
        return;
    }
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        &wide[0],
        length);
    const float factor = scale * 9.f / 28.f;
    const float left = x,
                right = maxWidth > 0 ? (std::min)(x + maxWidth, float(m_width - 8)) : float(m_width - 8);
    for (size_t i = 0; i < wide.size(); ++i)
    {
        wchar_t ch = wide[i];
        if (ch == L'\r')
        {
            continue;
        }
        if (ch == L'\n')
        {
            x = left;
            y += 12 * scale;
            continue;
        }
        if (ch == L'\t')
        {
            ch = L' ';
        }
        // The prototype supports Korean/BMP glyphs; replace one supplementary code point once.
        if (ch >= 0xd800 && ch <= 0xdbff)
        {
            if (i + 1 < wide.size() && wide[i + 1] >= 0xdc00 && wide[i + 1] <= 0xdfff)
            {
                ++i;
            }
            ch = L'?';
        }
        if (m_font->Full(ch))
        {
            Flush();
            m_font->Reset();
        }
        const auto glyph = m_font->Get(ch);
        const float advance = glyph.advance * factor;
        if (x + advance > right)
        {
            if (maxWidth <= 0 || advance > right - left)
            {
                break;
            }
            x = left;
            y += 12 * scale;
        }
        if (y + 9 * scale > m_height)
        {
            break;
        }
        if (glyph.width > 0 && glyph.height > 0)
        {
            const float l = 2 * (x + glyph.left * factor) / m_width - 1;
            const float r = l + 2 * glyph.width * factor / m_width;
            const float t = 1 - 2 * (y + glyph.top * factor) / m_height;
            const float b = t - 2 * glyph.height * factor / m_height;
            const Vertex a = {{l, t, 0}, c, {glyph.u, glyph.v}}, d = {{l, b, 0}, c, {glyph.u, glyph.V}};
            const Vertex e = {{r, t, 0}, c, {glyph.U, glyph.v}}, f = {{r, b, 0}, c, {glyph.U, glyph.V}};
            m_batch.insert(m_batch.end(), {a, e, f, a, f, d});
        }
        x += advance;
    }
}

void Renderer::End()
{
    PresentScene();
    Flush();
}

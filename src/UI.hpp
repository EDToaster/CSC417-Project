#pragma once

#include <cstdint>
#include <stdexcept>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Types.hpp"

#define FONTS_DIR "../assets/fonts/"

#define PI 3.141592653589793238462643383279502884

namespace UI {

    void ScreenToOpenGL(const glm::ivec2& disp_res, i64 x, i64 y, GLfloat& glx, GLfloat& gly) {
        glx = 2 * x / double(disp_res.x) - 1.0;
        gly = 2 * (disp_res.y - y) / double(disp_res.y) - 1.0;
    }

    void SimToScreen(const glm::vec2& disp_res, const glm::vec2& ren_scale, float px, float py, float& sx, float& sy) {
        sx = px * ren_scale.x;
        sy = disp_res.y - py * ren_scale.y;
    }

    void DrawCircle(const glm::ivec2& disp_res, i64 sx, i64 sy, i64 srx, i64 sry) {

        GLfloat x, y, rx, ry;
        UI::ScreenToOpenGL(disp_res, sx, sy, x, y);
        UI::ScreenToOpenGL(disp_res, srx, sry, rx, ry);

        int i;
        int triangleAmount = 40; //# of triangles used to draw circle

        //GLfloat radius = 0.8f; //radius
        GLfloat twicePi = 2.0f * PI;

        glBegin(GL_LINE_LOOP);
        for (i = 0; i <= triangleAmount; i++) {
            glVertex2f(
                x + ((rx + 1) * cos(i * twicePi / triangleAmount)),
                y + ((ry - 1) * sin(i * twicePi / triangleAmount))
            );
        }
        glEnd();
    }

    void DrawRect(const glm::ivec2& disp_res, i64 s1x, i64 s1y, i64 s2x, i64 s2y) {
        GLfloat sc1x, sc1y, sc2x, sc2y;
        ScreenToOpenGL(disp_res, s1x, s1y, sc1x, sc1y);
        ScreenToOpenGL(disp_res, s2x, s2y, sc2x, sc2y);
        glRectd(sc1x, sc1y, sc2x, sc2y);
    }

    class Text {
    public:
        Text(std::string t) : t(t) {};
        
        std::string t;
        i64 x, y;
    };


    class Display {
    public:
        Display(const i64 id, i64 x, i64 y, i64 w, i64 h, i64 o, glm::vec3 fill_col) : id(id), x(x), y(y), w(w), h(h), o(o), fill_col(fill_col) {};
        const i64 id, x, y, w, h, o;
        glm::vec3 fill_col;
    };

    struct GlyphChar {
        FT_Bitmap bitmap;

    };

    class UIRenderer {
    public:
        glm::ivec2 disp_res;	
        FT_Library ft;
        FT_Face face;

        GlyphChar glyphs[256];

        UIRenderer(glm::ivec2 disp_res) : disp_res(disp_res) {
            std::cout << "[INFO] Initializing UIRenderer." << std::endl;
            // initialize text
            if (FT_Init_FreeType(&ft)) {
                std::cout << "[ERROR] Could not initialize the FreeType library." << std::endl;
                throw std::runtime_error("Could not initialize the FreeType library.");
            }

            if (FT_New_Face(ft, FONTS_DIR "Langar-Regular.ttf", 0, &face)) {
                std::cout << "[ERROR] Could not load the font." << std::endl;                
                throw std::runtime_error("Could not load the font.");
            }
            std::cout << "[INFO] Done loading fonts, creating bitmaps." << std::endl;

            FT_Set_Pixel_Sizes(face, 0, 48);

            // set glyphs
            for (unsigned int i = 0; i < 256; i++) {
                unsigned char c = (unsigned char)i;
                FT_GlyphSlot slot = face->glyph;
                FT_UInt glyph_index = FT_Get_Char_Index(face, c);

                /* load glyph image into the slot (erase previous one) */
                if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))
                    continue;  /* ignore errors */

                    /* convert to an anti-aliased bitmap */
                if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
                    continue;

                glyphs[c].bitmap = slot->bitmap;
            }
        };

        std::vector<Display> displays;
        void AddDisplay(Display d) {
            displays.push_back(d);
        }

        void ClearDisplays() {
            displays.clear();
        }

        std::vector<Text> texts;
        void AddText(Text t) {
            texts.push_back(t);
        }

        void ClearTexts() {
            texts.clear();
        }

        GLubyte rasters[24] = {
           0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00,
           0xff, 0x00, 0xff, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00,
           0xff, 0xc0, 0xff, 0xc0 };

        void Render(int selectedid) {
            // draw displays
            for (auto d : displays) {
                if (d.id == selectedid) {
                    glColor3d(1, 1, 1);
                }
                else {
                    glColor3d(0, 0, 0);
                }

                DrawRect(disp_res, d.x - d.o, d.y - d.o, d.x + d.w + d.o, d.y + d.h + d.o);

                glColor3d(d.fill_col.x, d.fill_col.y, d.fill_col.z);
                DrawRect(disp_res, d.x, d.y, d.x + d.w, d.y + d.h);

            }

            // draw text
            for (auto t : texts) {
                for (unsigned char c : t.t) {
                    GlyphChar& glyphchar = glyphs[c];
                    FT_Bitmap& bitmap = glyphchar.bitmap;

                    //glClear(GL_COLOR_BUFFER_BIT);
                    glColor3f(1.0, 1.0, 1.0);
                    glRasterPos2i(20, 20);
                    glBitmap(10, 12, 0.0, 0.0, 11.0, 0.0, rasters);
                    glBitmap(10, 12, 0.0, 0.0, 11.0, 0.0, rasters);
                    glBitmap(10, 12, 0.0, 0.0, 11.0, 0.0, rasters);
                    //glFlush();
                }
            }
        }
    };
}
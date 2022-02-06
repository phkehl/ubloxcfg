// flipflip's glmatrix experiment
//
// Copyright (c) 2022 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/glmatrix
//
// Based on xscreensaver glmatrix.c, Copyright (c) 1992-2018 Jamie Zawinski <jwz@jwz.org>
//
// Permission to use, copy, modify, distribute, and sell this software and its documentation for any purpose is hereby
// granted without fee, provided that the above copyright notice appear in all copies and that both that copyright
// notice and this permission notice appear in supporting documentation.  No representations are made about the
// suitability of this software for any purpose.  It is provided "as is" without express or implied warranty.
//
// GLMatrix -- simulate the text scrolls from the movie "The Matrix".
//
// This program does a 3D rendering of the dropping characters that appeared in the title sequences of the movies. See
// also `xmatrix' for a simulation of what the computer monitors actually *in* the movie did.

#ifndef __GLMATRIX_HPP__
#define __GLMATRIX_HPP__

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

#include <glm/glm.hpp>

/* ****************************************************************************************************************** */

class GlMatrixRenderer;
class GlMatrixAnimator;

class GlMatrix
{
    public:

        GlMatrix();
       ~GlMatrix();

        struct Options
        {
            Options() :
                speed{1.0f}, density{20.0f}, doClock{false}, timeFmt{" %H:%M "},
                doFog{true}, doWaves{true}, doRotate{true}, mode{MATRIX},
                debugGrid{false}, debugFrames{false} {}
            float       speed;        //!< Animation speed factor (how fast the glyphs move)
            float       density;      //!< Strip density [%]
            bool        doClock;      //!< Show time in some strips
            std::string timeFmt;      //!< Time format (strftime() style)
            bool        doFog;        //!< Dim far away glyphs
            bool        doWaves;      //!< Brightness waves
            bool        doRotate;     //!< Slowly tilt and rotate view
            enum Mode_e { MATRIX, DNA, BINARY, HEXADECIMAL, DECIMAL };
            enum Mode_e mode;         //!< Glyph selection
            bool        debugGrid;    //!< Render debug grid
            bool        debugFrames;  //!< Render debug frames around glyphs
        };

        bool Init(const Options &options = Options());
        void Destroy();

        void Animate();
        void Render(const int width, const int height);

    private:
        std::unique_ptr<GlMatrixRenderer> _renderer;
        std::unique_ptr<GlMatrixAnimator> _animator;
};

/* ****************************************************************************************************************** */
#endif // __GLMATRIX_HPP__
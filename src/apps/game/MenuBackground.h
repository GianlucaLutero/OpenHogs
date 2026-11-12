// MenuBackground.h
//
// What is behind the menus: the original front end's background picture,
// "pigbkpc1" (a blurred view of brass machinery, 640 x 480) from
// FEbmps/FEBMP.MAD, stretched to cover the screen.

#pragma once

#include "FrontEndPictures.h"

#include "openhogs/render/UiRenderer.h"

namespace openhogs::gameapp {

class MenuBackground {
public:
    explicit MenuBackground(FrontEndPictures& pictures);

    // Draws the picture over the whole canvas (cut at the top and bottom on
    // screens wider than 4:3); black if it is missing.
    void draw(render::UiRenderer& ui) const;

private:
    const render::UiImage* picture_ = nullptr;
};

} // namespace openhogs::gameapp

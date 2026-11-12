// MenuBackground.cpp - see MenuBackground.h for an overview.

#include "MenuBackground.h"

#include <algorithm>

namespace openhogs::gameapp {

namespace {
const char* const kBackground = "pigbkpc1";
}

MenuBackground::MenuBackground(FrontEndPictures& pictures) : picture_(pictures.picture(kBackground)) {}

void MenuBackground::draw(render::UiRenderer& ui) const {
    ui.drawRect(0.0f, 0.0f, ui.canvasWidth(), render::kCanvasHeight, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    if (!picture_) return;
    // As small as possible while covering the whole canvas.
    const float scale = std::max(ui.canvasWidth() / picture_->width(), render::kCanvasHeight / picture_->height());
    const float width = picture_->width() * scale;
    const float height = picture_->height() * scale;
    ui.drawImage(*picture_, (ui.canvasWidth() - width) / 2.0f, (render::kCanvasHeight - height) / 2.0f, width, height);
}

} // namespace openhogs::gameapp

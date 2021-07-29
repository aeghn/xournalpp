#include "PopplerGlibPage.h"
#include <poppler.h>
#include <poppler-page.h>
#include "cairo.h"


PopplerGlibPage::PopplerGlibPage(PopplerPage* page): page(page) {
    if (page != nullptr) {
        g_object_ref(page);
    }
}

PopplerGlibPage::PopplerGlibPage(const PopplerGlibPage& other): page(other.page) {
    if (page != nullptr) {
        g_object_ref(page);
    }
}

PopplerGlibPage::~PopplerGlibPage() {
    if (page) {
        g_object_unref(page);
        page = nullptr;
    }
}

PopplerGlibPage& PopplerGlibPage::operator=(const PopplerGlibPage& other) {
    if (&other == this) {
        return *this;
    }
    if (page) {
        g_object_unref(page);
        page = nullptr;
    }

    page = other.page;
    if (page != nullptr) {
        g_object_ref(page);
    }
    return *this;
}

auto PopplerGlibPage::getWidth() -> double {
    double width = 0;
    poppler_page_get_size(page, &width, nullptr);

    return width;
}

auto PopplerGlibPage::getHeight() -> double {
    double height = 0;
    poppler_page_get_size(page, nullptr, &height);

    return height;
}

void PopplerGlibPage::render(cairo_t* cr, bool forPrinting)  // NOLINT(google-default-arguments)
{
    if (forPrinting) {
        poppler_page_render_for_printing(page, cr);
    } else {
        poppler_page_render(page, cr);
    }
}

auto PopplerGlibPage::getPageId() -> int { return poppler_page_get_index(page); }

auto PopplerGlibPage::findText(std::string& text) -> std::vector<XojPdfRectangle> {
    std::vector<XojPdfRectangle> findings;

    double height = getHeight();
    GList* matches = poppler_page_find_text(page, text.c_str());

    for (GList* l = matches; l && l->data; l = g_list_next(l)) {
        auto* rect = static_cast<PopplerRectangle*>(l->data);

        findings.emplace_back(rect->x1, height - rect->y1, rect->x2, height - rect->y2);

        poppler_rectangle_free(rect);
    }
    g_list_free(matches);

    return findings;
}

auto PopplerGlibPage::selectText(XojPdfRectangle* points) -> std::string {
    PopplerRectangle rec = {
        .x1 = points->x1,
        .y1 = points->y1,
        .x2 = points->x2,
        .y2 = points->y2,
    };

    return poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &rec);
}

auto PopplerGlibPage::selectTextInArea(XojPdfRectangle* points) -> std::string {
    PopplerRectangle rec = {
        .x1 = points->x1,
        .y1 = points->y1,
        .x2 = points->x2,
        .y2 = points->y2,
    };

    return poppler_page_get_text_for_area(page, &rec);
}

auto PopplerGlibPage::selectTextRegion(XojPdfRectangle* rec, gdouble scale) -> std::vector<XojPdfRectangle> {
    std::vector<XojPdfRectangle> recs;

    PopplerRectangle rec2 = {
        .x1 = rec->x1,
        .y1 = rec->y1,
        .x2 = rec->x2,
        .y2 = rec->y2,
    };

    GList* region = poppler_page_get_selection_region(page, 1.0, POPPLER_SELECTION_GLYPH, &rec2);

    GList *l;
	for (l = region; l; l = g_list_next (l)) {
		PopplerRectangle *rec = (PopplerRectangle *)l->data;
        
        recs.emplace_back(std::min(rec->x1, rec->x2),
            std::min(rec->y1, rec->y2),
            std::max(rec->x1, rec->x2),
            std::max(rec->y1, rec->y2));
    }


    return recs;
}

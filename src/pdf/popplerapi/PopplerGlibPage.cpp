#include <sstream>
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

auto PopplerGlibPage::selectText(XojPdfRectangle& points) -> std::string {
    PopplerRectangle rec = {
        .x1 = points.x1,
        .y1 = points.y1,
        .x2 = points.x2,
        .y2 = points.y2,
    };

    return poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &rec);
}

auto PopplerGlibPage::selectTextInArea(XojPdfRectangle& points) -> std::string {
    auto recs = this->selectTextRegionInArea(points, 1);

    for (size_t i = 0; i < recs.size() - 1; i++) {
        for (size_t j = i + 1; j < recs.size(); j++) {
            auto& r1 = recs.at(i);
            auto& r2 = recs.at(j);
            // TODO need to consider other conditions
            if (r1.y2 > r2.y1) {
                auto t = r1.y2;
                r1.y2 = r2.y1 - 1;
                r2.y1 = t + 1;
            }
        }
    }

    std::ostringstream oss;

    for (auto &item : recs) {
        oss << this->selectText(item) << "\n";
    }

    recs.clear();
    return oss.str();
}

auto PopplerGlibPage::selectTextRegion(XojPdfRectangle& rec, gdouble scale) -> std::vector<XojPdfRectangle> {
    std::vector<XojPdfRectangle> recs;

    PopplerRectangle rec2 = {
        .x1 = rec.x1,
        .y1 = rec.y1,
        .x2 = rec.x2,
        .y2 = rec.y2,
    };

    GList* region = poppler_page_get_selection_region(page, 1.0, POPPLER_SELECTION_GLYPH, &rec2);

    GList *l = nullptr;
	for (l = region; l; l = g_list_next (l)) {
		auto * r = (PopplerRectangle *)l->data;
        
        recs.emplace_back(std::min(r->x1, r->x2),
            std::min(r->y1, r->y2),
            std::max(r->x1, r->x2),
            std::max(r->y1, r->y2));
    }

	poppler_page_selection_region_free(region);

    return recs;
}

auto PopplerGlibPage::selectTextRegionInArea(XojPdfRectangle& rec, double scale) -> std::vector<XojPdfRectangle> {
    double aX = std::min(rec.x1, rec.x2);
    double bX = std::max(rec.x1, rec.x2);
    double aY = std::min(rec.y1, rec.y2);
    double bY = std::max(rec.y1, rec.y2);

    auto rec2 = XojPdfRectangle(aX, aY, bX, bY);
    auto recs = this->selectTextRegion(rec2, scale);
    // todo
    /* delete &rec2; */

    for (auto r = recs.begin(); r != recs.end(); ) {
        // this rectangle is not intersecting with original selection box.
        if (r->x1 > bX || r->x2 < aX || r->y1 > bY || r->y2 < aY) {
            r = recs.erase(r);
            continue;
        }

        r->x1 = std::max(r->x1, aX);
        r->x2 = std::min(r->x2, bX);
        r++;
    }

    return recs;
}
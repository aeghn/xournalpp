#include "PopplerGlibPage.h"

#include <sstream>

#include <poppler-page.h>
#include <poppler.h>

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

auto PopplerGlibPage::selectHeadTailText(const XojPdfRectangle& points) -> std::string {
    PopplerRectangle rec = {
        .x1 = points.x1,
        .y1 = points.y1,
        .x2 = points.x2,
        .y2 = points.y2,
    };

    return poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &rec);
}

auto PopplerGlibPage::selectHeadTailTextRegion(const XojPdfRectangle& rec) -> cairo_region_t* {
    PopplerRectangle rec2 = {
        .x1 = rec.x1,
        .y1 = rec.y1,
        .x2 = rec.x2,
        .y2 = rec.y2,
    };

    return poppler_page_get_selected_region(page, 1.0, POPPLER_SELECTION_GLYPH, &rec2);
}

void PopplerGlibPage::selectHeadTailFinally(const XojPdfRectangle& rec, cairo_region_t** region,
    std::vector<XojPdfRectangle>* recs, std::string* text) {
    recs->clear();
    text->clear();

    *region = this->selectHeadTailTextRegion(rec);
    if (cairo_region_is_empty(*region)) return;

    *text = this->selectHeadTailText(rec);
    PopplerRectangle area = {
        .x1 = rec.x1,
        .y1 = rec.y1,
        .x2 = rec.x2,
        .y2 = rec.y2,
    };

    // [2021-08-18] this part is come from:
    // https://gitlab.freedesktop.org/poppler/poppler/-/blob/master/glib/demo/annots.c
    PopplerRectangle* pRects;
    guint rectNums;
    if (! poppler_page_get_text_layout_for_area(this->page, &area, &pRects, &rectNums)) return;
    auto r = PopplerRectangle {G_MAXDOUBLE, G_MAXDOUBLE, G_MINDOUBLE, G_MINDOUBLE};
    
    for (int i = 0; i < rectNums; i++) {
        /* Check if the rectangle belongs to the same line.
           On a new line, start a new target rectangle.
           On the same line, make an union of rectangles at
           the same line */
        if (std::abs(r.y2 - pRects[i].y2) > 0.0001) {
            if (i > 0)
                recs->emplace_back(r.x1, r.y1, r.x2, r.y2);
            r.x1 = pRects[i].x1;
            r.y1 = pRects[i].y1;
            r.x2 = pRects[i].x2;
            r.y2 = pRects[i].y2;
        } else {
            r.x1 = std::min(r.x1, pRects[i].x1);
            r.y1 = std::min(r.y1, pRects[i].y1);
            r.x2 = std::max(r.x2, pRects[i].x2);
            r.y2 = std::max(r.y2, pRects[i].y2);
        }
    }
    recs->emplace_back(r.x1, r.y1, r.x2, r.y2);
}

void PopplerGlibPage::selectAreaFinally(const XojPdfRectangle& rec, cairo_region_t** region,
    std::vector<XojPdfRectangle>* recs, std::string* text) {
    recs->clear();
    text->clear();

    std::ostringstream oss;

    auto x1Box = std::min(rec.x1, rec.x2);
    auto x2Box = std::max(rec.x1, rec.x2);
    auto y1Box = std::min(rec.y1, rec.y2);
    auto y2Box = std::max(rec.y1, rec.y2);

    PopplerRectangle* pRects;
    guint rectNums;
    if (! poppler_page_get_text_layout(this->page, &pRects, &rectNums)) return;

    auto chars = poppler_page_get_text(this->page);
    auto r = PopplerRectangle {G_MAXDOUBLE, G_MAXDOUBLE, G_MINDOUBLE, G_MINDOUBLE};

    int startIndex = -1;
    for (int i = 0; i < rectNums; i++) {
        if (pRects[i].x1 > x2Box || pRects[i].y1 > y2Box || pRects[i].x2 < x1Box || pRects[i].y2 < y1Box)
            continue;
        startIndex = i + 1;

        r.x1 = pRects[i].x1;
        r.y1 = pRects[i].y1;
        r.x2 = pRects[i].x2;
        r.y2 = pRects[i].y2;

        break;
    }

    if (startIndex == -1 || startIndex >= rectNums) return;

    for (int i = startIndex; i < rectNums; i++) {
        if (pRects[i].x1 > x2Box || pRects[i].y1 > y2Box || pRects[i].x2 < x1Box || pRects[i].y2 < y1Box)
            continue;

        /* Check if the rectangle belongs to the same line.
           On a new line, start a new target rectangle.
           On the same line, make an union of rectangles at
           the same line */
        if (pRects[i].y2 - r.y2 > (pRects[i].y2 - pRects[i].y1)/2) {
            recs->emplace_back(r.x1, r.y1, r.x2, r.y2);
            r.x1 = pRects[i].x1;
            r.y1 = pRects[i].y1;
            r.x2 = pRects[i].x2;
            r.y2 = pRects[i].y2;
            oss << "\n" << chars[i];
        } else {
            r.x1 = std::min(r.x1, pRects[i].x1);
            r.y1 = std::min(r.y1, pRects[i].y1);
            r.x2 = std::max(r.x2, pRects[i].x2);
            r.y2 = std::max(r.y2, pRects[i].y2);
            oss << chars[i];
        }
    }
    recs->emplace_back(r.x1, r.y1, r.x2, r.y2);

    auto tmpRegion = cairo_region_create();
    cairo_rectangle_int_t cRect;
    for (const auto& item: *recs) {
        cRect.x = static_cast<int>(item.x1);
        cRect.y = static_cast<int>(item.y1);
        cRect.width = static_cast<int>(item.x2 - item.x1);
        cRect.height = static_cast<int>(item.y2 - item.y1);
        cairo_region_union_rectangle(tmpRegion, &cRect);
    }

    *region = tmpRegion;
    *text = oss.str();
}

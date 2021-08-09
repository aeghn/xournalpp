/*
 * Xournal++
 *
 * Handles text selection on a PDF page and in Xournal Texts
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string_view>
#include <vector>
#include <cairo.h>
#include "model/PageRef.h"
#include "pdf/base/XojPdfPage.h"
#include "gui/PageView.h"
#include "tools/StrokeHandler.h"

class PDFTextSelectControl {
public:
    PDFTextSelectControl(XojPageView* view, cairo_t* cr, double x, double y);
    virtual ~PDFTextSelectControl();

public:
    bool currentPos(double x2, double y2);

    bool selectPdfText();
    bool selectPdfRecs();

    void paint(cairo_t* cr, double scale_x, double scale_y);
    void repaint(double zoom);
    void rerender();

    bool finalize(double x, double y);
    void popMenu(double x, double y);

    void copyText();
    void draw(int style, int depth, int pos);
    void drawHighlight();
    void drawUnderline();
    void drawStrikethrough();

private:
    void freeSelectResult();
    cairo_region_t * createRegionFromRecs(std::vector<XojPdfRectangle> xojRecs, gdouble xscale, gdouble yscale);

private:
    GtkWidget* widget = nullptr;

    XojPageView* view;
    PageRef page;
    XojPdfPageSPtr pdf;
    XojPdfRectangle* se;
    cairo_t* cr;

    std::vector<XojPdfRectangle> selectTextRecs;
    std::string selectedText;
    
    bool isTap;
    double zoom;
};

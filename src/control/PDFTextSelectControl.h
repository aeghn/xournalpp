#pragma once

#include <string_view>
#include <vector>

#include <cairo.h>

#include "gui/PageView.h"
#include "model/PageRef.h"
#include "pdf/base/XojPdfPage.h"
#include "tools/StrokeHandler.h"

class PdfFloatingToolbox;
enum PdfTextSelectType {
    SELECT_IN_AREA = 0,
    SELECT_HEAD_TAIL
};

class PDFTextSelectControl {
public:
    PDFTextSelectControl(XojPageView* view, cairo_t* cr, double x, double y);
    virtual ~PDFTextSelectControl();

public:
    bool currentPos(double x2, double y2);

    bool selectPdfText();
    bool selectPdfRecs();

    void paint(cairo_t* cr, double scale_x, double scale_y);
    void reselect();
    void repaintPage();
    void rerenderPage();

    bool finalize(double x, double y);
    void popMenu(double x, double y);

    void copyText();
    void draw(int style, int depth, int pos);
    void drawHighlight();
    void drawUnderline();
    void drawStrikethrough();
    void setSelectType(PdfTextSelectType selectType);

private:
    void selectPdfTextInArea();
    void selectPdfTextHeadTail();
    void selectPdfRecsInArea();
    void selectPdfRecsHeadTail();

    void freeSelectResult();
    static cairo_region_t* createRegionFromRecs(const std::vector<XojPdfRectangle>& xojRecs);

private:
    GtkWidget* widget = nullptr;

    XojPageView* view;
    PageRef page;
    XojPdfPageSPtr pdf;
    XojPdfRectangle* se;
    cairo_t* cr;

    std::vector<XojPdfRectangle> selectTextRecs;
    std::string selectedText;

    PdfTextSelectType selectType;
    
    bool isTap;
    double zoom;
    void rerenderBox(const std::vector<XojPdfRectangle>& xojRecs);
};

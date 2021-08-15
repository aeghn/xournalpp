#pragma once

#include <string>
#include <vector>

#include "gui/PageView.h"
#include "gui/Redrawable.h"
#include "model/Element.h"
#include "model/PageRef.h"
#include "pdf/base/XojPdfPage.h"

enum PdfMarkerStyle {
    POS_TEXT_BOTTOM = 0,
    POS_TEXT_MIDDLE,
    POS_TEXT_TOP,

    WIDTH_TEXT_LINE,
    WIDTH_TEXT_HEIGHT
};

class PdfTextSelection {
public:
    PdfTextSelection(double x, double y, XojPageView* view);
    virtual ~PdfTextSelection();

    bool isFinalized;
    bool isFinished;

public:
    virtual bool finalize(PageRef page);
    virtual void paint(cairo_t* cr, GdkRectangle* rect, double zoom);
    virtual void currentPos(double x, double y);

    bool selectPdfText();
    bool selectPdfRecs();
    void popMenu();
    void clearSelection();

    const std::vector<XojPdfRectangle>& getSelectedTextRecs() const;
    const std::string& getSelectedText() const;
    XojPageView* getPageView() const;

private:
    XojPageView* view = nullptr;
    XojPdfPageSPtr pdf;

    std::vector<XojPdfRectangle> selectedTextRecs;
    std::string selectedText;

    double sx;
    double sy;
    double ex;
    double ey;
};

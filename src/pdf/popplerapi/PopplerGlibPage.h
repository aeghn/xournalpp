/*
 * Xournal++
 *
 * PDF Page GLib Implementation
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <poppler.h>

#include "pdf/base/XojPdfPage.h"


class PopplerGlibPage: public XojPdfPage {
public:
    PopplerGlibPage(PopplerPage* page);
    PopplerGlibPage(const PopplerGlibPage& other);
    virtual ~PopplerGlibPage();
    PopplerGlibPage& operator=(const PopplerGlibPage& other);

public:
    virtual double getWidth();
    virtual double getHeight();

    virtual void render(cairo_t* cr, bool forPrinting = false);  // NOLINT(google-default-arguments)

    virtual std::vector<XojPdfRectangle> findText(std::string& text);

    virtual std::string selectHeadTailText(const XojPdfRectangle& rec);
    virtual cairo_region_t* selectHeadTailTextRegion(const XojPdfRectangle& rec);
    
    virtual void selectHeadTailFinally(const XojPdfRectangle& se, cairo_region_t** region,
                                       std::vector<XojPdfRectangle>* recs, std::string* text);

    void selectAreaFinally(const XojPdfRectangle& rec, cairo_region_t** region, std::vector<XojPdfRectangle>* recs,
                           std::string* text);

    virtual int getPageId();

private:
    PopplerPage* page;
};

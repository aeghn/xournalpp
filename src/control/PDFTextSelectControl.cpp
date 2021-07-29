#include "PDFTextSelectControl.h"

#include <climits>
#include <utility>
#include <cairo.h>

#include "model/Layer.h"
#include "model/Text.h"
#include "view/TextView.h"
#include "control/Control.h"
#include "gui/XournalView.h"


using std::string;

PDFTextSelectControl::PDFTextSelectControl(XojPageView* view, cairo_t* cr):
        isTap(true), view(view), rec(new XojPdfRectangle()), cr(cr) {
    auto xournal = view->getXournal();
    this->widget = xournal->getWidget();
    this->page = view->getPage();

    auto pNr = this->page->getPdfPageNr();
    if (pNr != npos) {
        Document* doc = xournal->getControl()->getDocument();

        doc->lock();
        pdf = doc->getPdfPage(pNr);
        doc->unlock();
        this->zoom = xournal->getZoom();
        this->pdf = std::move(pdf);
    }

    this->x1Box = 0;
    this->y1Box = 0;
    this->x2Box = 0;
    this->y2Box = 0;
}

PDFTextSelectControl::~PDFTextSelectControl() {
    freeSelectResult();
}

void PDFTextSelectControl::freeSelectResult() {
    this->selectedText.clear();
    this->selectTextRecs.clear();
}

auto PDFTextSelectControl::selectPdfText() -> bool {
    this->selectedText.clear();
    if (this->pdf) {
        this->selectedText = this->pdf->selectText(this->rec);
    } else {
        return false;
    }

    return !this->selectedText.empty();
}

auto PDFTextSelectControl::selectPdfRecs() -> bool {
    this->selectTextRecs.clear();
    if (this->pdf)
        this->selectTextRecs = this->pdf->selectTextRegion(this->rec, 1);
    else
        return false;

    return !this->selectTextRecs.empty();
}

auto PDFTextSelectControl::currentPos(double x1, double y1, double x2, double y2) -> bool {
    if (this->isTap) this->isTap = false;

    this->rec->x1 = std::min(x1, x2);
    this->rec->x2 = std::max(x1, x2);

    this->rec->y1 = std::min(y1, y2);
    this->rec->y2 = std::max(y1, y2);

    this->repaint(1);

    return true;
}

auto PDFTextSelectControl::createRegionFromRecs(std::vector<XojPdfRectangle> xojRecs, gdouble xscale, gdouble yscale) -> cairo_region_t * {
    cairo_region_t *retval = nullptr;

    retval = cairo_region_create();

    for(auto & xojRec: xojRecs) {
        cairo_rectangle_int_t crRect;

        int aX = std::min(xojRec.x1, xojRec.x2);
        int bX = std::max(xojRec.x1, xojRec.x2);

        int aY = std::min(xojRec.y1, xojRec.y2);
        int bY = std::max(xojRec.y1, xojRec.y2);

        crRect.x = (gint) ((aX * xscale) + 0.5);
        crRect.y = (gint) ((aY * yscale) + 0.5);
        crRect.width  = (gint) ((bX * xscale) + 0.5) - crRect.x;
        crRect.height = (gint) ((bY * yscale) + 0.5) - crRect.y;

        if (aX < this->x1Box) this->x1Box = aX;
        if (aY < this->y1Box) this->y1Box = aY;
        if (bX > this->x2Box) this->x2Box = bX;
        if (bY > this->y2Box) this->y2Box = bY;

        cairo_region_union_rectangle (retval, &crRect);
    }

    return retval;
}

void PDFTextSelectControl::paint(cairo_t* cr, double scale_x, double scale_y) {
    if (! this->selectPdfRecs()) return;

    auto region = createRegionFromRecs(this->selectTextRecs, 1, 1);

    cairo_save (cr);
    cairo_scale (cr, scale_x, scale_y);
    GdkRGBA selectionColor = view->getSelectionColor();
    auto applied = GdkRGBA{selectionColor.red, selectionColor.green, selectionColor.blue, 0.3};
    gdk_cairo_region (cr, region);
    gdk_cairo_set_source_rgba(cr, &applied);
    cairo_set_operator (cr, CAIRO_OPERATOR_MULTIPLY);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_fill (cr);
    cairo_restore (cr);
}

void PDFTextSelectControl::repaint(double zoom) {
    this->view->repaintPage();
}

auto PDFTextSelectControl::finalize(double x, double y) -> bool {
    if (this->isTap) return false;


    double aX = std::min(this->rec->x1, this->rec->x2);
    double bX = std::max(this->rec->x1, this->rec->x2);

    double aY = std::min(this->rec->y1, this->rec->y2);
    double bY = std::max(this->rec->y1, this->rec->y2);

    this->rec->x1 = aX;
    this->rec->x2 = bX;
    this->rec->y1 = aY;
    this->rec->y2 = bY;

    if (!this->selectPdfText()) return true;
    this->paint(this->cr, zoom, zoom);

    this->popMenu(x, y);

    return true;
}

void PDFTextSelectControl::draw(int style, int depth, int pos) {
    if (this->selectTextRecs.empty()) return;

    auto strokeHandler = new StrokeHandler(this->view->getXournal(), this->view, this->view->getPage());
    Stroke* stroke = nullptr;

    auto p = new PositionInputData();
    p->pressure = -1;
    p->state = GDK_RELEASE_MASK;

    for (XojPdfRectangle rect: this->selectTextRecs) {
        double midY = (rect.y1 + rect.y2) /2;
        if (pos == 0) { midY = std::max(rect.y1, rect.y2); }

        double width = abs(rect.y1 - rect.y2);
        if (style == 0) { width = 1; }

        p->x = rect.x1;
        p->y = midY;

        if (! strokeHandler->getStroke() || stroke == nullptr) {
            strokeHandler->createStroke(Point(rect.x1, midY, -1));
            stroke = strokeHandler->getStroke();
            stroke->setFill(depth);
            stroke->setToolType(STROKE_TOOL_HIGHLIGHTER);
            stroke->setWidth(width);
        }

        strokeHandler->onButtonPressEvent(*p);
        stroke->addPoint(Point(rect.x2, midY, -1));

        auto points = stroke->getPointVector();

        strokeHandler->onButtonReleaseEvent(*p);
    }

    delete p;
}

void PDFTextSelectControl::copyText() {
    GtkClipboard* clipboard = gtk_widget_get_clipboard(this->widget, GDK_SELECTION_CLIPBOARD);

    const gchar* text = this->selectedText.c_str();

    gtk_clipboard_set_text(clipboard, text, -1);
    this->view->rerenderPage();
}

void PDFTextSelectControl::drawHighlight() {
    this->repaint(this->zoom);
    this->draw(1, 60, 1);
    this->view->rerenderPage();
}

static void highlightCb(GtkWidget *button, PDFTextSelectControl* ptc) {
    ptc->drawHighlight();
}

void PDFTextSelectControl::drawUnderline() {
    this->repaint(this->zoom);
    this->draw(0, 230, 0);
    this->view->rerenderPage();
}

void PDFTextSelectControl::drawStrikethrough() {
    this->repaint(this->zoom);
    this->draw(0, 230, 1);
    this->view->rerenderPage();
}

void PDFTextSelectControl::rerender() {
    this->repaint(this->zoom);
    this->view->rerenderPage();
}

void PDFTextSelectControl::popMenu(double x, double y) {
    if (this->selectTextRecs.empty()) {
        return;
    }

    gint wx = 0, wy = 0;
    GtkWidget* widget = view->getXournal()->getWidget();
    gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &wx, &wy);
    wx += std::lround(x * zoom + view->getX());
    wy += std::lround(y * zoom + view->getY());

    this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->show(wx, wy, this);
}

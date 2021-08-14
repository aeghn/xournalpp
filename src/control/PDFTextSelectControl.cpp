#include "PDFTextSelectControl.h"

#include <utility>
#include <cairo.h>

#include "view/TextView.h"
#include "control/Control.h"
#include "gui/XournalView.h"

PDFTextSelectControl::PDFTextSelectControl(XojPageView* view, cairo_t* cr, double x, double y):
    isTap(true), view(view), cr(cr) {
    auto xournal = view->getXournal();
    this->widget = xournal->getWidget();
    this->page = view->getPage();
    this->selectType = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->getSelectType();

    auto pNr = this->page->getPdfPageNr();
    if (pNr != npos) {
        Document* doc = xournal->getControl()->getDocument();

        doc->lock();
        pdf = doc->getPdfPage(pNr);
        doc->unlock();
        this->zoom = xournal->getZoom();
        this->pdf = std::move(pdf);
    }

    this->se = new XojPdfRectangle(x, y, x, y);
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
        if (this->selectType == PdfTextSelectType::SELECT_HEAD_TAIL) {
            this->selectPdfTextHeadTail();
        } else {
            this->selectPdfTextInArea();
        }
    } else {
        return false;
    }

    return !this->selectedText.empty();
}

void PDFTextSelectControl::selectPdfTextInArea() {
    this->selectedText = this->pdf->selectTextInArea(this->se);
}

void PDFTextSelectControl::selectPdfTextHeadTail() {
    this->selectedText = this->pdf->selectText(this->se);
}

auto PDFTextSelectControl::selectPdfRecs() -> bool {
    this->selectTextRecs.clear();
    if (this->pdf) {
        if (this->selectType == PdfTextSelectType::SELECT_HEAD_TAIL) {
            this->selectPdfRecsHeadTail();
        } else {
            this->selectPdfRecsInArea();
        }
    }  else
        return false;

    return !this->selectTextRecs.empty();
}

void PDFTextSelectControl::selectPdfRecsInArea() {
    this->selectTextRecs = this->pdf->selectTextRegionInArea(this->se, 1);
}

void PDFTextSelectControl::selectPdfRecsHeadTail() {
    this->selectTextRecs = this->pdf->selectTextRegion(this->se, 1);
}

auto PDFTextSelectControl::currentPos(double x2, double y2) -> bool {
    if (this->isTap) this->isTap = false;

    this->se->x2 = x2;
    this->se->y2 = y2;

    this->repaintPage();

    return true;
}

auto PDFTextSelectControl::createRegionFromRecs(const std::vector<XojPdfRectangle>& xojRecs) -> cairo_region_t * {
    cairo_region_t *retval = cairo_region_create();

    for(auto & xojRec: xojRecs) {
        cairo_rectangle_int_t crRect;

        int aX = std::min(xojRec.x1, xojRec.x2);
        int bX = std::max(xojRec.x1, xojRec.x2);

        int aY = std::min(xojRec.y1, xojRec.y2);
        int bY = std::max(xojRec.y1, xojRec.y2);

        crRect.x = (gint) ((aX * 1) + 0.5);
        crRect.y = (gint) ((aY * 1) + 0.5);
        crRect.width  = (gint) ((bX * 1) + 0.5) - crRect.x;
        crRect.height = (gint) ((bY * 1) + 0.5) - crRect.y;

        cairo_region_union_rectangle (retval, &crRect);
    }

    return retval;
}

void PDFTextSelectControl::paint(cairo_t* cr, double scale_x, double scale_y) {
    if (! this->selectPdfRecs()) return;

    auto region = createRegionFromRecs(this->selectTextRecs);

    cairo_save (cr);
    cairo_scale (cr, scale_x, scale_y);
    GdkRGBA selectionColor = view->getSelectionColor();
    auto applied = GdkRGBA{selectionColor.red, selectionColor.green, selectionColor.blue, 0.3};
    gdk_cairo_region (cr, region);
    gdk_cairo_set_source_rgba(cr, &applied);
    cairo_set_operator (cr, CAIRO_OPERATOR_MULTIPLY);
    cairo_fill (cr);
    cairo_region_destroy(region);

    cairo_restore (cr);
}

void PDFTextSelectControl::repaintPage() {
    this->view->repaintPage();
}

auto PDFTextSelectControl::finalize(double x, double y) -> bool {
    if (this->isTap) return false;

    if (!this->selectPdfText()) return false;
    this->paint(this->cr, this->zoom, this->zoom);
    this->repaintPage();
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
    this->repaintPage();
    this->draw(1, 60, 1);
    this->view->rerenderPage();
}

void PDFTextSelectControl::drawUnderline() {
    this->repaintPage();
    this->draw(0, 230, 0);
    this->view->rerenderPage();
}

void PDFTextSelectControl::drawStrikethrough() {
    this->repaintPage();
    this->draw(0, 230, 1);
    this->view->rerenderPage();
}

void PDFTextSelectControl::rerenderPage() {
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

void PDFTextSelectControl::rerenderBox(const std::vector<XojPdfRectangle>& xojRecs) {
    auto aX = xojRecs.at(0).x1;
    auto aY = xojRecs.at(0).y1;
    auto bX = xojRecs.at(0).x2;
    auto bY = xojRecs.at(0).y2;

    for (const auto& item: xojRecs) {
        aX = std::min(item.x1, aX);
        aY = std::min(item.y1, aY);
        bX = std::max(item.x2, bX);
        bY = std::max(item.y2, bY);
    }

    this->view->rerenderRect(aX, aY, bX-aX, bY-aY);
}

void PDFTextSelectControl::reselect() {
    auto pftb = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox;
    if (pftb->getSelectType() == PdfTextSelectType::SELECT_IN_AREA) {
        this->rerenderBox(this->selectTextRecs);
    }
    this->repaintPage();
    this->paint(this->cr, this->zoom, this->zoom);
    this->selectPdfText();
}
void PDFTextSelectControl::setSelectType(PdfTextSelectType selectType) {
    PDFTextSelectControl::selectType = selectType;
}
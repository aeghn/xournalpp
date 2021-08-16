#include "PdfTextSelection.h"
#include "control/Control.h"

PdfTextSelection::PdfTextSelection(double x, double y, XojPageView* view) {
    this->sx = x;
    this->sy = y;
    this->ex = x;
    this->ey = y;

    this->view = view;

    this->isFinalized = false;
    this->isFinished = false;

    auto xournal = view->getXournal();
    auto pNr = this->view->getPage()->getPdfPageNr();
    if (pNr != npos) {
        Document* doc = xournal->getControl()->getDocument();

        doc->lock();
        pdf = doc->getPdfPage(pNr);
        doc->unlock();
        this->pdf = std::move(pdf);
    }
}

PdfTextSelection::~PdfTextSelection() = default;

auto PdfTextSelection::finalize(PageRef page) -> bool {
    this->isFinalized = true;

    if (this->selectPdfRecs()) {
        this->popMenu();
        this->view->rerenderPage();
        return true;
    }

    return false;
}

void PdfTextSelection::popMenu() {
    if (this->selectedTextRecs.empty()) {
        return;
    }
    auto zoom = this->view->getXournal()->getZoom();
    int wx = 0, wy = 0;
    GtkWidget* widget = this->view->getXournal()->getWidget();
    gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &wx, &wy);
    wx += std::lround(this->ex * zoom + view->getX());
    wy += std::lround(this->ey * zoom + view->getY());

    this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->show(wx, wy, this);
}

void PdfTextSelection::paint(cairo_t* cr, GdkRectangle* rect, double zoom) {
    if (this->selectedTextRecs.empty()) return;

    cairo_region_t *region = cairo_region_create();

    for(auto & xojRec: this->selectedTextRecs) {
        cairo_rectangle_int_t crRect;

        int aX = std::min(xojRec.x1, xojRec.x2);
        int bX = std::max(xojRec.x1, xojRec.x2);

        int aY = std::min(xojRec.y1, xojRec.y2);
        int bY = std::max(xojRec.y1, xojRec.y2);

        crRect.x = (gint) ((aX * 1) + 0.5);
        crRect.y = (gint) ((aY * 1) + 0.5);
        crRect.width  = (gint) ((bX * 1) + 0.5) - crRect.x;
        crRect.height = (gint) ((bY * 1) + 0.5) - crRect.y;

        cairo_region_union_rectangle (region, &crRect);
    }

    GdkRGBA selectionColor = view->getSelectionColor();
    auto applied = GdkRGBA{selectionColor.red, selectionColor.green, selectionColor.blue, 0.3};
    gdk_cairo_region(cr, region);
    gdk_cairo_set_source_rgba(cr, &applied);
    cairo_fill(cr);
    cairo_region_destroy(region);
}

void PdfTextSelection::currentPos(double x, double y) {
    this->ex = x;
    this->ey = y;

    this->selectPdfRecs();

    if (this->selectedTextRecs.empty()) return;

    double x1Box = this->selectedTextRecs.at(0).x1;
    double x2Box = this->selectedTextRecs.at(0).x2;
    double y1Box = this->selectedTextRecs.at(0).y1;
    double y2Box = this->selectedTextRecs.at(0).y2;
    for (const auto& item: this->selectedTextRecs) {
        x1Box = std::min(item.x1, x1Box);
        x2Box = std::max(item.x2, x2Box);
        y1Box = std::min(item.y1, y1Box);
        y2Box = std::max(item.y2, y2Box);
    }
    
    this->view->repaintArea(x1Box - 20, y1Box - 20, x2Box + 20, y2Box + 20);
}

auto PdfTextSelection::selectPdfText() -> bool {
    this->selectedText.clear();

    auto se = XojPdfRectangle{sx, sy, ex, ey};

    if (this->pdf) {
        auto type = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->getSelectType();
        if (type == PdfTextSelectType::SELECT_HEAD_TAIL) {
            this->selectedText = this->pdf->selectText(se);
        } else {
            this->selectedText = this->pdf->selectTextInArea(se);
        }
    } else {
        return false;
    }

    return !this->selectedText.empty();
}

auto PdfTextSelection::selectPdfRecs() -> bool {
    this->selectedTextRecs.clear();

    auto se = XojPdfRectangle{sx, sy, ex, ey};

    if (this->pdf) {
        auto type = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->getSelectType();
        if (type == PdfTextSelectType::SELECT_HEAD_TAIL) {
            this->selectedTextRecs = this->pdf->selectTextRegion(se, 1);
        } else {
            this->selectedTextRecs = this->pdf->selectTextRegionInArea(se, 1);
        }
    }

    return !this->selectedTextRecs.empty();
}

const std::vector<XojPdfRectangle>& PdfTextSelection::getSelectedTextRecs() const { return selectedTextRecs; }
const std::string& PdfTextSelection::getSelectedText() const { return selectedText; }
XojPageView* PdfTextSelection::getPageView() const { return view; }
void PdfTextSelection::clearSelection() {
    this->selectedTextRecs.clear();
    this->selectedText.clear();
}

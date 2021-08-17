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

    this->select();

    if (!this->selectedTextRecs.empty()) {
        this->popMenu();
        this->view->rerenderPage();
        return true;
    }

    return false;
}

void PdfTextSelection::select() {
    auto se = XojPdfRectangle{sx, sy, ex, ey};
    auto type = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->getSelectType();
    if (type == PdfTextSelectType::SELECT_HEAD_TAIL) {
        this->pdf->selectHeadTailFinally(se, &this->selectedTextRegion, &this->selectedTextRecs, &this->selectedText);
    } else {
        this->pdf->selectAreaFinally(se, &this->selectedTextRegion, &this->selectedTextRecs, &this->selectedText);
    }
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
    if (this->pdf) {
        GdkRGBA selectionColor = view->getSelectionColor();
        auto applied = GdkRGBA{selectionColor.red, selectionColor.green, selectionColor.blue, 0.3};

        auto type = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->getSelectType();
        if (this->isFinalized || type == PdfTextSelectType::SELECT_HEAD_TAIL) {
            if (!this->selectedTextRegion || cairo_region_is_empty(this->selectedTextRegion))
                return;

            gdk_cairo_region(cr, this->selectedTextRegion);
            gdk_cairo_set_source_rgba(cr, &applied);
            cairo_fill(cr);

            // cairo_region_destroy(this->selectedTextRegion);
            // this->selectedTextRegion = nullptr;
        } else {
            cairo_set_line_width(cr, 1 / zoom);
            gdk_cairo_set_source_rgba(cr, &selectionColor);

            int aX = std::min(this->sx, this->ex);
            int bX = std::max(this->sx, this->ex);

            int aY = std::min(this->sy, this->ey);
            int bY = std::max(this->sy, this->ey);

            cairo_move_to(cr, aX, aY);
            cairo_line_to(cr, bX, aY);
            cairo_line_to(cr, bX, bY);
            cairo_line_to(cr, aX, bY);
            cairo_close_path(cr);
            cairo_stroke_preserve(cr);
            gdk_cairo_set_source_rgba(cr, &applied);
            cairo_fill(cr);
        }
    }
}

void PdfTextSelection::currentPos(double x, double y) {
    this->ex = x;
    this->ey = y;

    if (this->pdf) {
        auto type = this->view->getXournal()->getControl()->getWindow()->pdfFloatingToolBox->getSelectType();
        if (type == PdfTextSelectType::SELECT_HEAD_TAIL) {
            this->selectHeadTailTextRegion();
        } else {
            // TODO maybe we should do something here
        }
    }

    this->view->repaintPage();
}

auto PdfTextSelection::selectHeadTailTextRegion() -> bool {
    auto se = XojPdfRectangle{sx, sy, ex, ey};
    this->selectedTextRegion = this->pdf->selectHeadTailTextRegion(se);

    return !cairo_region_is_empty(this->selectedTextRegion);
}

const std::vector<XojPdfRectangle>& PdfTextSelection::getSelectedTextRecs() const { return selectedTextRecs; }
const std::string& PdfTextSelection::getSelectedText() const { return selectedText; }
XojPageView* PdfTextSelection::getPageView() const { return view; }
void PdfTextSelection::clearSelection() {
    this->selectedTextRecs.clear();
    this->selectedText.clear();
}

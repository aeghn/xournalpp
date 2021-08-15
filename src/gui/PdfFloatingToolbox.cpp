#include <control/tools/StrokeHandler.h>

#include "GladeGui.h"
#include "MainWindow.h"

PdfFloatingToolbox::PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay) {
    this->theMainWindow = theMainWindow;
    this->floatingToolbox = theMainWindow->get("pdfFloatingToolbox");

    this->selectType = PdfTextSelectType::SELECT_HEAD_TAIL;

    gtk_overlay_add_overlay(overlay, this->floatingToolbox);
    gtk_overlay_set_overlay_pass_through(overlay, this->floatingToolbox, true);

    g_signal_connect(overlay, "get-child-position", G_CALLBACK(this->getOverlayPosition), this);

    g_signal_connect(theMainWindow->get("pdfTbHighlight"), "clicked", G_CALLBACK(this->highlightCb), this);
    g_signal_connect(theMainWindow->get("pdfTbCopyText"), "clicked", G_CALLBACK(this->copyTextCb), this);
    g_signal_connect(theMainWindow->get("pdfTbUnderline"), "clicked", G_CALLBACK(this->underlineCb), this);
    g_signal_connect(theMainWindow->get("pdfTbStrikethrough"), "clicked", G_CALLBACK(this->strikethroughCb), this);
    g_signal_connect(theMainWindow->get("pdfTbDoNothing"), "clicked", G_CALLBACK(this->closeCb), this);
    g_signal_connect(theMainWindow->get("pdfTbChangeType"), "clicked", G_CALLBACK(this->switchSelectTypeCb), this);

    this->hide();
}

PdfFloatingToolbox::~PdfFloatingToolbox() = default;

void PdfFloatingToolbox::show(int x, int y, PdfTextSelection* pdfTextSelection) {
    if (this->pdfTextSelection && this->pdfTextSelection != pdfTextSelection) {
        delete this->pdfTextSelection;
        this->pdfTextSelection = nullptr;
    }
    this->pdfTextSelection = pdfTextSelection;
    this->floatingToolboxX = x;
    this->floatingToolboxY = y;
    this->show();
}

void PdfFloatingToolbox::hide() {
    gtk_widget_hide(this->floatingToolbox);
}

auto PdfFloatingToolbox::getOverlayPosition(GtkOverlay* overlay, GtkWidget* widget, GdkRectangle* allocation,
                                            PdfFloatingToolbox* self) -> gboolean {
    if (widget == self->floatingToolbox) {
        gtk_widget_get_allocation(widget, allocation);  // get existing width and height

        GtkRequisition natural;
        gtk_widget_get_preferred_size(widget, nullptr, &natural);
        allocation->width = natural.width;
        allocation->height = natural.height;

        allocation->x = self->floatingToolboxX;
        allocation->y = self->floatingToolboxY;
        return true;
    }

    return false;
}

void PdfFloatingToolbox::postAction() {
    this->pdfTextSelection->isFinished = true;
    auto view = this->pdfTextSelection->getPageView();

    this->hide();
    this->pdfTextSelection->clearSelection();

    delete this->pdfTextSelection;
    this->pdfTextSelection = nullptr;
    view->pdfTextSelection = nullptr;

    view->rerenderPage();
}

void PdfFloatingToolbox::highlightCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->createStrokesForHighlight();
    pft->postAction();
}

void PdfFloatingToolbox::copyTextCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->copyText();
    pft->postAction();
}

void PdfFloatingToolbox::underlineCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->createStrokesForUnderline();
    pft->postAction();
}

void PdfFloatingToolbox::strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->createStrokesForStrikethrough();
    pft->postAction();
}

void PdfFloatingToolbox::closeCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->postAction();
}

void PdfFloatingToolbox::show() {
    gtk_widget_hide(this->floatingToolbox);  // force showing in new position
    gtk_widget_show_all(this->floatingToolbox);
}

void PdfFloatingToolbox::setSelectType(PdfTextSelectType type) {
    this->selectType = type;
}

PdfTextSelectType PdfFloatingToolbox::getSelectType() {
    return this->selectType;
}

void PdfFloatingToolbox::switchSelectTypeCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->switchSelectType();
    pft->pdfTextSelection->selectPdfRecs();
    pft->pdfTextSelection->getPageView()->rerenderPage();
}

void PdfFloatingToolbox::switchSelectType() {
    if (this->selectType != PdfTextSelectType::SELECT_IN_AREA) {
        this->selectType = PdfTextSelectType::SELECT_IN_AREA;
    } else {
        this->selectType = PdfTextSelectType::SELECT_HEAD_TAIL;
    }
}

void PdfFloatingToolbox::copyText() {
    GtkClipboard* clipboard = gtk_widget_get_clipboard(this->theMainWindow->getWindow(), GDK_SELECTION_CLIPBOARD);

    this->pdfTextSelection->selectPdfText();
    const gchar* text = this->pdfTextSelection->getSelectedText().c_str();

    gtk_clipboard_set_text(clipboard, text, -1);
}

void PdfFloatingToolbox::createStrokes(PdfMarkerStyle position, PdfMarkerStyle width, int markerOpacity) {
    if (this->pdfTextSelection->getSelectedTextRecs().empty()) return;

    auto strokeHandler = new StrokeHandler(this->pdfTextSelection->getPageView()->getXournal(),
                                           this->pdfTextSelection->getPageView(),
                                           this->pdfTextSelection->getPageView()->getPage());
    Stroke* stroke = nullptr;

    auto p = new PositionInputData();
    p->pressure = -1;
    p->state = GDK_RELEASE_MASK;

    for (XojPdfRectangle rect: this->pdfTextSelection->getSelectedTextRecs()) {
        // the center line position of stroke
        double h = position == PdfMarkerStyle::POS_TEXT_BOTTOM
                   ? std::max(rect.y1, rect.y2)
                   : position == PdfMarkerStyle::POS_TEXT_MIDDLE
                     ? (rect.y1 + rect.y2) /2
                     : std::min(rect.y1, rect.y2);

        // the width of stroke
        double w = width == PdfMarkerStyle::WIDTH_TEXT_LINE ? 1 : std::abs(rect.y2 - rect.y1);

        if (! strokeHandler->getStroke() || stroke == nullptr) {
            strokeHandler->createStroke(Point(rect.x1, h, -1));
            stroke = strokeHandler->getStroke();
            stroke->setFill(markerOpacity);
            stroke->setToolType(STROKE_TOOL_HIGHLIGHTER);
            stroke->setWidth(w);
        }

        strokeHandler->onButtonPressEvent(*p);

        stroke->addPoint(Point(rect.x2, h, -1));

        strokeHandler->onButtonReleaseEvent(*p);
    }

    delete p;
}

void PdfFloatingToolbox::createStrokesForHighlight() {
    this->createStrokes(PdfMarkerStyle::POS_TEXT_MIDDLE, PdfMarkerStyle::WIDTH_TEXT_HEIGHT, 60);
}

void PdfFloatingToolbox::createStrokesForUnderline() {
    this->createStrokes(PdfMarkerStyle::POS_TEXT_BOTTOM, PdfMarkerStyle::WIDTH_TEXT_LINE, 230);
}

void PdfFloatingToolbox::createStrokesForStrikethrough() {
    this->createStrokes(PdfMarkerStyle::POS_TEXT_MIDDLE, PdfMarkerStyle::WIDTH_TEXT_LINE, 230);
}

#include "GladeGui.h"
#include "MainWindow.h"

PdfFloatingToolbox::PdfFloatingToolbox(MainWindow* theMainWindow, GtkOverlay* overlay) {
    this->selectType = PdfTextSelectType::SELECT_HEAD_TAIL;

    this->floatingToolbox = theMainWindow->get("pdfFloatingToolbox");

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

void PdfFloatingToolbox::show(int x, int y, PDFTextSelectControl* pdfTextSelectControl) {
    delete this->pdfTextSelectControl;
    this->pdfTextSelectControl = pdfTextSelectControl;
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


void PdfFloatingToolbox::highlightCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->drawHighlight();
}

void PdfFloatingToolbox::copyTextCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->copyText();
}

void PdfFloatingToolbox::underlineCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->drawUnderline();
}

void PdfFloatingToolbox::strikethroughCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->drawStrikethrough();
}

void PdfFloatingToolbox::closeCb(GtkButton* button, PdfFloatingToolbox* pft) {
    pft->hide();
    pft->pdfTextSelectControl->rerenderPage();
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
    pft->pdfTextSelectControl->reselect();
}

void PdfFloatingToolbox::switchSelectType() {
    if (this->selectType != PdfTextSelectType::SELECT_IN_AREA) {
        this->selectType = PdfTextSelectType::SELECT_IN_AREA;
    } else {
        this->selectType = PdfTextSelectType::SELECT_HEAD_TAIL;
    }

    this->pdfTextSelectControl->setSelectType(this->selectType);
}

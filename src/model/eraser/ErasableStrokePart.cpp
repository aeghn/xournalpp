#include "ErasableStrokePart.h"

ErasableStrokePart::ErasableStrokePart(Point a, Point b) {
    addPoint(a);
    addPoint(b);
    this->width = a.z;

    this->splitSize = 0;

    calcSize();
}

ErasableStrokePart::ErasableStrokePart(double width) {
    this->points = nullptr;
    this->width = width;
    this->splitSize = 0;

    calcSize();
}

ErasableStrokePart::~ErasableStrokePart() {
    for (GList* l = this->points; l != nullptr; l = l->next) {
        auto* p = static_cast<Point*>(l->data);
        delete p;
    }
    g_list_free(this->points);
    this->points = nullptr;
}

void ErasableStrokePart::calcSize() {
    if (this->points == nullptr) {
        this->x = 0;
        this->y = 0;
        this->elementWidth = 0;
        this->elementHeight = 0;
        return;
    }

    double x1 = (static_cast<Point*>(g_list_first(this->points)->data))->x;
    double y1 = (static_cast<Point*>(g_list_first(this->points)->data))->y;
    double x2 = (static_cast<Point*>(g_list_first(this->points)->data))->x;
    double y2 = (static_cast<Point*>(g_list_first(this->points)->data))->y;

    for (GList* l = this->points; l != nullptr; l = l->next) {
        auto* p = static_cast<Point*>(l->data);
        x1 = std::min(x1, p->x);
        x2 = std::max(x2, p->x);
        y1 = std::min(y1, p->y);
        y2 = std::max(y2, p->y);
    }

    this->x = x1;
    this->y = y1;
    this->elementWidth = x2 - x1;
    this->elementHeight = y2 - y1;
}

auto ErasableStrokePart::clone() -> ErasableStrokePart* {
    auto* part = new ErasableStrokePart(this->width);

    for (GList* l = this->points; l != nullptr; l = l->next) {
        auto* p = static_cast<Point*>(l->data);
        part->addPoint(*p);
    }

    part->splitSize = this->splitSize;

    return part;
}

auto ErasableStrokePart::getX() const -> double { return this->x; }

auto ErasableStrokePart::getY() const -> double { return this->y; }

auto ErasableStrokePart::getElementWidth() const -> double { return this->elementWidth; }

auto ErasableStrokePart::getElementHeight() const -> double { return this->elementHeight; }

void ErasableStrokePart::addPoint(Point p) {
    calcSize();

    this->points = g_list_append(this->points, new Point(p));
}

auto ErasableStrokePart::getWidth() const -> double { return this->width; }

auto ErasableStrokePart::getPoints() -> GList* { return this->points; }

void ErasableStrokePart::clearSplitData() {
    for (GList* l = this->points->next; l->next != nullptr;) {
        auto* p = static_cast<Point*>(l->data);
        delete p;
        GList* link = l;
        l = l->next;

        this->points = g_list_delete_link(this->points, link);
    }
}

void ErasableStrokePart::splitFor(double halfEraserSize) {
    if (halfEraserSize == this->splitSize) {
        return;
    }

    this->splitSize = halfEraserSize;

    auto* a = static_cast<Point*>(g_list_first(this->points)->data);
    auto* b = static_cast<Point*>(g_list_last(this->points)->data);

    // nothing to do, the size is enough small
    if (a->lineLengthTo(*b) <= halfEraserSize) {
        return;
    }

    clearSplitData();

    double len = a->lineLengthTo(*b);
    halfEraserSize /= 2;

    while (len > halfEraserSize) {
        this->points = g_list_insert(this->points, new Point(a->lineTo(*b, len)), 1);
        len -= halfEraserSize;
    }
}

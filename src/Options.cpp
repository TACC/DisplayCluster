#include "Options.h"

Options::Options()
{
    showWindowBorders_ = true;
    showTestPattern_ = false;
}

bool Options::getShowWindowBorders()
{
    return showWindowBorders_;
}

bool Options::getShowTestPattern()
{
    return showTestPattern_;
}

void Options::setShowWindowBorders(bool set)
{
    showWindowBorders_ = set;

    emit(updated());
}

void Options::setShowTestPattern(bool set)
{
    showTestPattern_ = set;

    emit(updated());
}

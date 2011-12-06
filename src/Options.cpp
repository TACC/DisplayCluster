#include "Options.h"

Options::Options()
{
    showWindowBorders_ = true;
}

bool Options::getShowWindowBorders()
{
    return showWindowBorders_;
}

void Options::setShowWindowBorders(bool set)
{
    showWindowBorders_ = set;

    emit(updated());
}

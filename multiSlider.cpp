#include "qt.pc.h"
#include "multiSlider.h"


void MultiSlider::update()
{
    std::sort(std::begin(handles_), std::end(handles_));

    QSlider::update();
}

void MultiSlider::addHandle(handle::Value value)
{
    if (this->isValid(value))
    {
        auto handle{ handles_.emplace_back(this, value) };
    }
}

void MultiSlider::removeHandle(handle::Value value)
{
    if (std::erase_if(handles_, Handle::has(value)))
    {
        emit valueChanged(value);
    }
}

handle::Values MultiSlider::values() const
{
    handle::Values values;

    for (auto& handle : handles_)
    {
        values.emplace_back(handle.value());
    }
    return values;
}

bool MultiSlider::contains(handle::Value value) const
{
    return std::any_of(std::begin(handles_), std::end(handles_), Handle::has(value));
}

bool MultiSlider::isValid(handle::Value value) const
{
    return this->minimum() <= value and value <= this->maximum()
        and (not this->contains(value)
            or handleBehavior_ == handle::Behavior::Stack);
}

int MultiSlider::length() const
{
    QStyleOptionSlider options{};
    this->initStyleOption(&options);

    auto margin{
        style()->proxy()->pixelMetric(QStyle::PM_SliderLength, &options, this)
    };

    switch (this->orientation())
    {
    default:
    case Qt::Orientation::Horizontal: return this->width() - margin;
    case Qt::Orientation::Vertical: return this->height() - margin;
    }
}

handle::Value MultiSlider::roundedValue(double rawValue) const
{
    return static_cast<handle::Value>(rawValue / this->tickInterval()) * this->tickInterval();
}

QPoint MultiSlider::positionFromValue(handle::Value value) const
{
    QStyleOptionSlider options{};
    this->initStyleOption(&options);

    bool ticksAbove{
        this->tickPosition() == TickPosition::TicksAbove
        or this->tickPosition() == TickPosition::TicksBothSides
    };
    auto margin{
        ticksAbove ? style()->proxy()->pixelMetric(QStyle::PM_SliderTickmarkOffset, &options, this) : 0
    };

    auto sliderPosition{ QStyle::sliderPositionFromValue(
        this->minimum(), this->maximum(), value, this->length())
    };

    switch (this->orientation())
    {
    default:
    case Qt::Orientation::Horizontal: return{ sliderPosition, margin };
    case Qt::Orientation::Vertical: return{ margin, sliderPosition };
    }
}

handle::Value MultiSlider::valueFromPosition(QPoint position) const
{
    auto sliderPosition{ [this, &position]
    {
        switch (this->orientation())
        {
        default:
        case Qt::Orientation::Horizontal: return position.x();
        case Qt::Orientation::Vertical: return position.y();
        }
    }() };
    return QStyle::sliderValueFromPosition(
        this->minimum(), this->maximum(), sliderPosition, this->length());
}

void MultiSlider::paintEvent(QPaintEvent*)
{
    QPainter painter{ this };

    // Draw default slider groove and ticks
    QStyleOptionSlider options{};
    this->initStyleOption(&options);
    options.subControls = QStyle::SC_SliderGroove;
    if (this->tickPosition() != NoTicks)
        options.subControls |= QStyle::SC_SliderTickmarks;
    this->style()->drawComplexControl(QStyle::CC_Slider, &options, &painter, this);

    // Load default handle area
    QRect handleArea = style()->subControlRect(QStyle::CC_Slider, &options, QStyle::SC_SliderHandle, this);

    // Draw handles
    for (auto& handle : handles_)
    {
        handleArea.moveTopLeft(this->positionFromValue(handle.value()));
        painter.setPen({});
        painter.setBrush(handle.color());
        painter.drawRect(handleArea);
    }
}

void MultiSlider::mousePressEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->buttons() & Qt::LeftButton)
    {
        // Load default handle area
        QStyleOptionSlider options{};
        this->initStyleOption(&options);
        QRect handleArea = style()->subControlRect(QStyle::CC_Slider, &options, QStyle::SC_SliderHandle, this);

        for (auto& handle : handles_)
        {
            // Check if handle area contains mouse position
            if (handleArea.moveTopLeft(this->positionFromValue(handle.value()));
                handleArea.contains(mouseEvent->pos()))
            {
                handle.hold();
                this->update();
                break;
            }
        }
    }
}

void MultiSlider::mouseMoveEvent(QMouseEvent* mouseEvent)
{
    auto value{ roundedValue(this->valueFromPosition(mouseEvent->pos())) };

    QStyleOptionSlider options{};
    this->initStyleOption(&options);
    QRect handleArea = style()->subControlRect(QStyle::CC_Slider, &options, QStyle::SC_SliderHandle, this);

    for (auto& handle : handles_)
    {
        if (mouseEvent->buttons() & Qt::LeftButton
            and handle.isPressed())
        {
            if (not this->contains(value) or handleBehavior_ == handle::Behavior::Stack)
            {
                handle.setValue(value);
            }
        }
        else if (handleArea.moveTopLeft(this->positionFromValue(handle.value()));
            handleArea.contains(mouseEvent->pos())
            and not handle.isPressed())
        {
            handle.hover();
        }
        else
        {
            handle.release();
        }
    }
    this->update();
}

void MultiSlider::mouseReleaseEvent(QMouseEvent*)
{
    for (auto& handle : handles_)
    {
        handle.release();
    }
    this->update();
}

void MultiSlider::Handle::setValue(double rawValue)
{
    value_ = slider_->roundedValue(rawValue);
    emit slider_->valueChanged(value_);
}

constexpr QColor MultiSlider::Handle::color() const
{
    switch (interaction_)
    {
    default:
    case Interaction::None: return { 0, 120, 215 };
    case Interaction::Hovered: return { 23, 23, 23 };
    case Interaction::Pressed: return { 204, 204, 204 };
    }
}

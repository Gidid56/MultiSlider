#pragma once

namespace handle
{
    enum class Behavior{ Stack, Jump };

    using Value = int;
    using Values = std::vector<Value>;
}

class MultiSlider : public QSlider
{
public:
    class Handle;
    using Handles = std::vector<Handle>;

    explicit MultiSlider(QWidget* parent = nullptr)
        : QSlider{ parent }
    {
        this->setMouseTracking(true);
    }

    void update();

    void addHandle(handle::Value);
    void removeHandle(handle::Value);

    void clear() { handles_.clear(); }

    handle::Values values() const;

    bool contains(handle::Value) const;
    bool isValid(handle::Value) const;

    int length() const;

    handle::Value roundedValue(double) const;

    QPoint positionFromValue(handle::Value) const;
    handle::Value valueFromPosition(QPoint) const;

    auto handleBehavior() const { return handleBehavior_; }

    auto size() const { return std::size(handles_); }

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    handle::Behavior handleBehavior_{ handle::Behavior::Jump };
    Handles handles_;
};

class MultiSlider::Handle
{
public:
    enum class Interaction{ None, Hovered, Pressed };

    explicit Handle(MultiSlider* slider, handle::Value value)
        : slider_{ slider }, value_{ value }
    {}

    auto value() const { return value_; }
    void setValue(double rawValue);

    bool isPressed() const { return interaction_ == Interaction::Pressed; }
    void hold() { interaction_ = Interaction::Pressed; }
    void hover() { interaction_ = Interaction::Hovered; }
    void release() { interaction_ = Interaction::None; }

    constexpr QColor color() const;

    constexpr auto operator <=> (Handle const& other) const {
        return value_ <=> other.value_;
    }

    static constexpr auto has(handle::Value value) {
        return [value](auto const& handle) {
            return handle.value() == value;
        };
    }

private:
    MultiSlider* slider_;
    handle::Value value_;
    Interaction interaction_{ Interaction::None };
};
#pragma once

#include <memory>

#include <QVBoxLayout>
#include <QWidget>

class ParamsWidget : public QWidget
{
	Q_OBJECT
public:
	ParamsWidget(QWidget* parent = nullptr)
	    : QWidget(parent)
	    , _layout(this)
	{
		_layout.setSpacing(0);
		_layout.setMargin(0);
		_layout.addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
	}

	void setParamSubWidget(std::unique_ptr<QWidget>&& subWidget)
	{
		_paramsSubWidget = std::move(subWidget);
		_layout.insertWidget(0, _paramsSubWidget.get());
	}

private:
	std::unique_ptr<QWidget> _paramsSubWidget;
	QVBoxLayout _layout;
};


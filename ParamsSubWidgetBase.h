#pragma once

#include <QWidget>
#include <QFormLayout>

#include "Common.h"

class ParamsSubWidgetBase : public QWidget
{
	Q_OBJECT
public:
	// memory is managed by ParamsWidget, therefore initialize QWidget with nullptr
	ParamsSubWidgetBase() : QWidget(nullptr), _layout(this)
	{
		_layout.setMargin(3);
		_layout.setSpacing(1);
	}
	virtual ~ParamsSubWidgetBase() {}
protected:
	QFormLayout _layout;
signals:
	void settingsChanged(const BeesBookCommon::Stage stage);
};


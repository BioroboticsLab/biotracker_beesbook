#pragma once

#include <QWidget>

#include "Common.h"

class ParamsSubWidgetBase : public QWidget
{
	Q_OBJECT
public:
	// memory is managed by ParamsWidget, therefore initialize QWidget with nullptr
	ParamsSubWidgetBase() : QWidget(nullptr) {}
	virtual ~ParamsSubWidgetBase() {}
signals:
	void settingsChanged(const BeesBookCommon::Stage stage);
};


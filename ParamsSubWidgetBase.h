#pragma once

#include <QWidget>

#include "Common.h"

class ParamsSubWidgetBase : public QWidget
{
	Q_OBJECT
public:
	ParamsSubWidgetBase(QWidget* parent = nullptr) : QWidget(parent) {}
	virtual ~ParamsSubWidgetBase() {}
signals:
	void settingsChanged(const BeesBookCommon::Stage stage);
};


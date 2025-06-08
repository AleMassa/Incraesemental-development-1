#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Concrete_Reinforcement_Front.h"
#include "RebarCalc.h"

class Concrete_Reinforcement_Front : public QMainWindow
{
    Q_OBJECT

public:
    Concrete_Reinforcement_Front(QWidget* parent = nullptr);
    ~Concrete_Reinforcement_Front();
    void initUI();

public slots:
    void on_pushButton_autoGenerate_clicked();
    void on_pushButton_genPic_clicked();
    void on_pushButton_genPic2_clicked();

private:
    void handleDesignRequest(bool isCrossSection);

    Ui::Concrete_Reinforcement_FrontClass ui;
    RebarCalc rebarCalc;
};
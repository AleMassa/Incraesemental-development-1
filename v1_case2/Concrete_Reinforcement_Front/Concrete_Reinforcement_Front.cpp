#include "stdafx.h"
#include "Concrete_Reinforcement_Front.h"
#include "RebarCalc.h"
#include <opencv2/opencv.hpp>
#include <string>

Concrete_Reinforcement_Front::Concrete_Reinforcement_Front(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    initUI();
    statusBar()->showMessage("Ready. Please enter the bridge parameters.");
}

Concrete_Reinforcement_Front::~Concrete_Reinforcement_Front() {}

void Concrete_Reinforcement_Front::initUI()
{
    connect(ui.pushButton_autoGenerate, &QPushButton::clicked, this, &Concrete_Reinforcement_Front::on_pushButton_autoGenerate_clicked);
    connect(ui.pushButton_genPic, &QPushButton::clicked, this, &Concrete_Reinforcement_Front::on_pushButton_genPic_clicked);
    connect(ui.pushButton_genPic_2, &QPushButton::clicked, this, &Concrete_Reinforcement_Front::on_pushButton_genPic2_clicked);
    setWindowTitle("Concrete Rebar Simulator");
}

void Concrete_Reinforcement_Front::on_pushButton_autoGenerate_clicked()
{
    qInfo() << "Auto-generating geometric parameters...";

    double span = ui.lineEdit_span->text().toDouble();
    if (span <= 0) {
        statusBar()->showMessage("Error: Span must be greater than zero for auto-generation.");
        return;
    }

    double width, height;
    autoGeoParams(span, width, height);

    qInfo() << "width: " << width << " height: " << height;

    ui.lineEdit_width->setText(QString::number(width));
    ui.lineEdit_height->setText(QString::number(height));
    statusBar()->showMessage("Geometric parameters auto-generated.");
}

void Concrete_Reinforcement_Front::handleDesignRequest(bool isCrossSection) {
    double span = ui.lineEdit_span->text().toDouble();
    double width = ui.lineEdit_width->text().toDouble();
    double height = ui.lineEdit_height->text().toDouble();
    double weight = ui.lineEdit_weight->text().toDouble();
    double wheelSpan = ui.lineEdit_wheelSpan->text().toDouble();
    // Leggi il nuovo valore
    double girderSpacing = ui.lineEdit_girderSpacing->text().toDouble();

    if (span <= 0 || width <= 0 || height <= 0 || weight <= 0 || girderSpacing <= 0) {
        statusBar()->showMessage("Error: Please enter valid values greater than zero in all fields.");
        ui.label_totalCostValue->setText("---");
        ui.label_concreteCostValue->setText("---");
        ui.label_steelCostValue->setText("---");
        ui.label_laborCostValue->setText("---");
        return;
    }

    statusBar()->showMessage("Calculating...");

    // Passa il nuovo valore alla funzione di calcolo
    bool success = rebarCalc.runDesign(span, width, height, weight, wheelSpan, girderSpacing);
    const RebarDesign& design = rebarCalc.getDesignResults();

    cv::Mat image;
    if (isCrossSection) {
        image = rebarCalc.generateCrossSectionImage();
    }
    else {
        image = rebarCalc.generateLongitudinalSectionImage();
    }

    if (success) {
        ui.label_totalCostValue->setText(QString::number(design.totalCost, 'f', 2) + " Yuan");
        ui.label_concreteCostValue->setText(QString::number(design.concreteCost, 'f', 2) + " Yuan");
        ui.label_steelCostValue->setText(QString::number(design.steelCost, 'f', 2) + " Yuan");
        ui.label_laborCostValue->setText(QString::number(design.laborCost, 'f', 2) + " Yuan");

        if (isCrossSection) statusBar()->showMessage("Design complete. Cross-section displayed.");
        else statusBar()->showMessage("Design complete. Longitudinal section displayed.");
    }
    else {
        ui.label_totalCostValue->setText("---");
        ui.label_concreteCostValue->setText("---");
        ui.label_steelCostValue->setText("---");
        ui.label_laborCostValue->setText("---");
        statusBar()->showMessage(QString::fromStdString(design.errorMessage));
    }

    if (image.empty()) {
        statusBar()->showMessage("Error: Image generation failed.");
        return;
    }

    QImage qimg(image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    QGraphicsScene* scene = new QGraphicsScene(this);
    scene->addPixmap(QPixmap::fromImage(qimg.rgbSwapped()));
    ui.graphicsView->setScene(scene);
    ui.graphicsView->show();
}

void Concrete_Reinforcement_Front::on_pushButton_genPic_clicked()
{
    handleDesignRequest(true);
}

void Concrete_Reinforcement_Front::on_pushButton_genPic2_clicked()
{
    handleDesignRequest(false);
}
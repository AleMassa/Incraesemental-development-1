#pragma once

#ifndef REBARCALC_H
#define REBARCALC_H

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <limits>
#include <map>
#include <opencv2/opencv.hpp>

// Use M_PI from cmath for better precision
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Concrete_Reinforcement_Front;

// --- Engineering and Design Constants ---
constexpr double CONCRETE_COVER = 30.0;
constexpr double CONCRETE_UNIT_WEIGHT = 2.5e-5; // N/mm^3
constexpr double CONCRETE_FC = 32.4;            // C50 Concrete Compressive Strength (MPa)
constexpr double CONCRETE_FT = 2.65;            // C50 Concrete Tensile Strength (MPa)
constexpr double STEEL_FY = 400.0;              // HRB400 Steel Yield Strength (MPa)
constexpr double STEEL_DENSITY = 7.85e-6;       // ton/mm^3
constexpr double XI_B_LIMIT = 0.518;            // Ductile failure limit

// --- Bridge Structural Parameters ---
struct BridgeParams
{
    double span;
    double width;
    double height;
    double h0; // Effective height
    double wheelSpan;
    double girderSpacing;
};

// --- Reinforcement Design Results ---
struct RebarDesign
{
    int rebarRows = 0;
    int rebarCountRow1 = 0;
    int rebarCountRow2 = 0;
    double flexureRebarDiameter = 0;
    int stirrupLegs = 0;
    double stirrupDiameter = 0;
    double stirrupSpacing = 0;
    bool bentRebarsUsed = false;
    int bentRebarCount = 0;
    double totalCost = 0;
    double concreteCost = 0;
    double steelCost = 0;
    double laborCost = 0;
    double maxMoment = 0; // N*mm
    double maxShear = 0;  // N
    bool designPossible = true;
    std::string errorMessage;
};

// --- Main Calculation Class ---
class RebarCalc
{
public:
    RebarCalc();
    ~RebarCalc();
    bool runDesign(double span, double width, double height, double totalVehicleLoad, double wheelSpan, double girderSpacing);
    cv::Mat generateCrossSectionImage();
    cv::Mat generateLongitudinalSectionImage();
    const RebarDesign& getDesignResults() const { return design; }

private:
    // Member variables store the FINAL optimized design
    BridgeParams params;
    RebarDesign design;
    std::map<double, cv::Scalar> rebarColorMap;

    void initializeColorMap();
    void resetDesign();
    void calculateMaxForces(double vehicleLoadForMoment, double vehicleLoadForShear);

    // Core optimization function
    bool findOptimalDesign();

    // Helper functions for calculations on temporary data
    void designShearForIteration(RebarDesign& tempDesign, const BridgeParams& tempParams) const;
    void calculateTotalCostForIteration(RebarDesign& tempDesign, const BridgeParams& tempParams) const;
    int calculateMaxBarsPerRow(double diameter, const BridgeParams& tempParams) const;
};


// --- Function Implementations ---

inline RebarCalc::RebarCalc() {
    initializeColorMap();
    resetDesign();
}

inline void RebarCalc::initializeColorMap() {
    rebarColorMap[14] = cv::Scalar(255, 0, 0); rebarColorMap[16] = cv::Scalar(0, 128, 0); rebarColorMap[18] = cv::Scalar(0, 255, 255); rebarColorMap[20] = cv::Scalar(0, 165, 255); rebarColorMap[22] = cv::Scalar(255, 0, 255); rebarColorMap[25] = cv::Scalar(128, 0, 128); rebarColorMap[28] = cv::Scalar(42, 42, 165);
}

inline RebarCalc::~RebarCalc() {}

inline void RebarCalc::resetDesign() {
    params = {};
    design = {};
    design.designPossible = true;
}

inline bool RebarCalc::runDesign(double span, double width, double height, double totalVehicleLoad, double wheelSpan, double girderSpacing)
{
    resetDesign();

    params.span = span;
    params.width = width;
    params.height = height;
    params.wheelSpan = wheelSpan * 1000.0; // Convert m to mm
    params.girderSpacing = girderSpacing;   // Keep in mm for cost calc

    constexpr double AASHTO_DIVISOR_MOMENT = 1.7;
    constexpr double AASHTO_DIVISOR_SHEAR = 1.4;
    double girderSpacing_m = girderSpacing / 1000.0;

    double loadFactor_moment = (girderSpacing_m > 0) ? (girderSpacing_m / AASHTO_DIVISOR_MOMENT) : 1.0;
    double loadFactor_shear = (girderSpacing_m > 0) ? (girderSpacing_m / AASHTO_DIVISOR_SHEAR) : 1.0;

    double effectiveLoad_moment = totalVehicleLoad * loadFactor_moment;
    double effectiveLoad_shear = totalVehicleLoad * loadFactor_shear;

    calculateMaxForces(effectiveLoad_moment * 1000.0, effectiveLoad_shear * 1000.0);

    return findOptimalDesign();
}

inline void RebarCalc::calculateMaxForces(double vehicleLoadForMoment, double vehicleLoadForShear)
{
    double L = params.span;
    double q = params.width * params.height * CONCRETE_UNIT_WEIGHT;
    double maxDeadMoment = q * L * L / 8.0;

    double P_moment_axle = vehicleLoadForMoment / 2.0;
    double d_wheel = params.wheelSpan;
    double x_crit_moment = L / 2.0 - d_wheel / 4.0;
    double Reaction_A_forMoment = (P_moment_axle * (L - x_crit_moment) + P_moment_axle * (L - x_crit_moment - d_wheel)) / L;
    double absMaxLiveMoment = Reaction_A_forMoment * x_crit_moment;

    design.maxMoment = maxDeadMoment + absMaxLiveMoment;

    double maxDeadShear = q * L / 2.0;
    double P_shear_axle = vehicleLoadForShear / 2.0;
    double maxLiveShear = (P_shear_axle * L + P_shear_axle * (L - d_wheel)) / L;

    design.maxShear = maxDeadShear + maxLiveShear;
}

// =================================================================================
// == RE-ARCHITECTED CORE LOGIC ==
// =================================================================================
inline bool RebarCalc::findOptimalDesign()
{
    std::vector<double> standardDiameters = { 14, 16, 18, 20, 22, 25, 28 };
    double bestCost = std::numeric_limits<double>::max();
    bool solutionFound = false;

    for (double currentDiameter : standardDiameters) {
        RebarDesign tempDesign = {};
        BridgeParams tempParams = this->params;

        // --- 1. FLEXURAL DESIGN FOR CURRENT DIAMETER ---
        tempDesign.flexureRebarDiameter = currentDiameter;
        double area_per_bar = M_PI * pow(currentDiameter / 2.0, 2);

        // Tentatively assume one row to calculate initial h0
        tempParams.h0 = tempParams.height - CONCRETE_COVER - 8.0 - currentDiameter / 2.0;
        if (tempParams.h0 <= 0) continue;

        double requiredArea = this->design.maxMoment / (STEEL_FY * 0.9 * tempParams.h0);
        double minRebarArea = 0.45 * (CONCRETE_FT / STEEL_FY) * tempParams.width * tempParams.height;
        if (requiredArea < minRebarArea) requiredArea = minRebarArea;

        int totalBars = std::ceil(requiredArea / area_per_bar);
        if (totalBars < 2) totalBars = 2;

        int max_bars_per_row = calculateMaxBarsPerRow(currentDiameter, tempParams);
        if (max_bars_per_row == 0) continue;

        if (totalBars <= max_bars_per_row) {
            tempDesign.rebarRows = 1;
            tempDesign.rebarCountRow1 = totalBars;
        }
        else {
            tempDesign.rebarRows = 2;
            tempDesign.rebarCountRow1 = std::ceil((double)totalBars / 2.0);
            tempDesign.rebarCountRow2 = totalBars - tempDesign.rebarCountRow1;

            if (tempDesign.rebarCountRow1 > max_bars_per_row || tempDesign.rebarCountRow2 > max_bars_per_row) {
                continue;
            }
            // Recalculate h0 for two rows
            double y1 = CONCRETE_COVER + 8.0 + currentDiameter / 2.0;
            double y2 = y1 + currentDiameter + 25.0;
            double As1 = tempDesign.rebarCountRow1 * area_per_bar;
            double As2 = tempDesign.rebarCountRow2 * area_per_bar;
            tempParams.h0 = tempParams.height - ((As1 * y1 + As2 * y2) / (As1 + As2));
        }

        double finalArea = totalBars * area_per_bar;
        double x_comp = (finalArea * STEEL_FY) / (1.0 * CONCRETE_FC * tempParams.width);
        double xi = x_comp / tempParams.h0;
        if (xi >= XI_B_LIMIT) {
            continue;
        }

        // --- 2. SHEAR DESIGN FOR THIS VALID FLEXURAL LAYOUT ---
        designShearForIteration(tempDesign, tempParams);

        // --- 3. TRUE COST CALCULATION ---
        calculateTotalCostForIteration(tempDesign, tempParams);

        // --- 4. OPTIMIZATION CHECK ---
        if (tempDesign.totalCost < bestCost) {
            solutionFound = true;
            bestCost = tempDesign.totalCost;
            this->design = tempDesign; // Store the best design
            this->params.h0 = tempParams.h0; // And its corresponding effective depth
        }
    }

    if (!solutionFound) {
        this->design.designPossible = false;
        this->design.errorMessage = "Error: No valid reinforcement combination found.\nIncrease beam dimensions.";
    }

    return solutionFound;
}

inline void RebarCalc::designShearForIteration(RebarDesign& tempDesign, const BridgeParams& tempParams) const
{
    tempDesign.bentRebarsUsed = false;
    tempDesign.bentRebarCount = 0;
    tempDesign.stirrupDiameter = 8;
    tempDesign.stirrupLegs = 2;

    double Vd = this->design.maxShear;
    double Vc = 0.20 * CONCRETE_FT * tempParams.width * tempParams.h0;
    double Vs_required = Vd - Vc;

    if (Vs_required <= 0) {
        tempDesign.stirrupSpacing = 200;
        return;
    }

    double stirrupAreaPerLeg = M_PI * pow(tempDesign.stirrupDiameter / 2.0, 2);
    double stirrupTotalArea = tempDesign.stirrupLegs * stirrupAreaPerLeg;
    double spacing_if_stirrups_only = (stirrupTotalArea * STEEL_FY * tempParams.h0) / Vs_required;

    if (spacing_if_stirrups_only < 100.0 && tempDesign.rebarCountRow1 >= 2) {
        tempDesign.bentRebarsUsed = true;
        tempDesign.bentRebarCount = 2;
        double singleBentBarArea = M_PI * pow(tempDesign.flexureRebarDiameter / 2.0, 2);
        double Vsb = 0.75 * STEEL_FY * (tempDesign.bentRebarCount * singleBentBarArea) * sin(M_PI / 4.0);
        Vs_required -= Vsb;
    }

    if (Vs_required > 0) {
        tempDesign.stirrupSpacing = (stirrupTotalArea * STEEL_FY * tempParams.h0) / Vs_required;
        tempDesign.stirrupSpacing = std::floor(tempDesign.stirrupSpacing / 25.0) * 25.0;
        if (tempDesign.stirrupSpacing > 200) tempDesign.stirrupSpacing = 200;
        if (tempDesign.stirrupSpacing < 100) tempDesign.stirrupSpacing = 100;
    }
    else {
        tempDesign.stirrupSpacing = 200;
    }
}

inline void RebarCalc::calculateTotalCostForIteration(RebarDesign& tempDesign, const BridgeParams& tempParams) const
{
    double numGirders = 1;
    if (tempParams.girderSpacing > 0) {
        numGirders = 10000.0 / tempParams.girderSpacing;
        if (numGirders < 2) numGirders = 2;
    }

    double cost_concrete_per_m3 = 600.0;
    double cost_rebar_base_per_ton = 3500.0;
    double cost_per_rebar_tied = 300.0;

    double volume_m3 = (tempParams.span * tempParams.width * tempParams.height) / 1e9;
    tempDesign.concreteCost = volume_m3 * cost_concrete_per_m3 * numGirders;

    double total_longitudinal_bars = tempDesign.rebarCountRow1 + tempDesign.rebarCountRow2;
    double extra_length_for_bends = 0;
    if (tempDesign.bentRebarsUsed) {
        extra_length_for_bends = tempDesign.bentRebarCount * (tempParams.height * 0.5);
    }
    double flex_rebar_volume = (M_PI * pow(tempDesign.flexureRebarDiameter / 2.0, 2)) * (total_longitudinal_bars * tempParams.span + extra_length_for_bends);
    double flex_rebar_weight_ton = flex_rebar_volume * STEEL_DENSITY;
    double diameter_cost_factor_flex = 1.0 + (tempDesign.flexureRebarDiameter - 14.0) * 0.025;
    double flexuralSteelCost = flex_rebar_weight_ton * (cost_rebar_base_per_ton * diameter_cost_factor_flex);

    double stirrup_length = 2 * (tempParams.width + tempParams.height);
    double num_stirrups = (tempDesign.stirrupSpacing > 0) ? (tempParams.span / tempDesign.stirrupSpacing) : 0;
    double stirrup_total_area = M_PI * pow(tempDesign.stirrupDiameter / 2.0, 2);
    double stirrup_volume = tempDesign.stirrupLegs * stirrup_total_area * stirrup_length * num_stirrups;
    double stirrup_weight_ton = stirrup_volume * STEEL_DENSITY;
    double diameter_cost_factor_stirrup = 1.0 + (tempDesign.stirrupDiameter - 14.0) * 0.025;
    double stirrupSteelCost = stirrup_weight_ton * (cost_rebar_base_per_ton * diameter_cost_factor_stirrup);

    tempDesign.steelCost = (flexuralSteelCost + stirrupSteelCost) * numGirders;
    tempDesign.laborCost = (total_longitudinal_bars + num_stirrups) * cost_per_rebar_tied * numGirders;
    tempDesign.totalCost = tempDesign.concreteCost + tempDesign.steelCost + tempDesign.laborCost;
}

inline int RebarCalc::calculateMaxBarsPerRow(double diameter, const BridgeParams& tempParams) const {
    double available_width = tempParams.width - (2 * CONCRETE_COVER) - (2 * 8.0);
    double min_spacing = std::max(25.0, diameter);
    if (available_width < diameter) return 0;
    return 1 + std::floor((available_width - diameter) / (diameter + min_spacing));
}
// =================================================================================
// == END RE-ARCHITECTED LOGIC ==
// =================================================================================

inline cv::Mat RebarCalc::generateCrossSectionImage()
{
    int img_w = 600, img_h = 600;
    cv::Mat image(img_h, img_w, CV_8UC3, cv::Scalar(255, 255, 255));

    if (!design.designPossible) {
        cv::putText(image, "DESIGN FAILED", cv::Point(50, img_h / 2 - 20), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 0, 255), 3);
        cv::putText(image, design.errorMessage, cv::Point(50, img_h / 2 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 1);
        return image;
    }

    double scale = std::min((double)img_w * 0.8 / params.width, (double)img_h * 0.8 / params.height);
    int rect_w = params.width * scale;
    int rect_h = params.height * scale;
    int rect_x = (img_w - rect_w) / 2;
    int rect_y = (img_h - rect_h) / 2;

    cv::rectangle(image, cv::Point(rect_x, rect_y), cv::Point(rect_x + rect_w, rect_y + rect_h), cv::Scalar(211, 211, 211), -1);
    double stirrup_offset = CONCRETE_COVER * scale;
    cv::rectangle(image, cv::Point(rect_x + stirrup_offset, rect_y + stirrup_offset), cv::Point(rect_x + rect_w - stirrup_offset, rect_y + rect_h - stirrup_offset), cv::Scalar(0, 0, 0), 2);

    double rebar_radius_scaled = (design.flexureRebarDiameter / 2.0) * scale;
    cv::Scalar rebar_color = rebarColorMap.at(design.flexureRebarDiameter);
    cv::Scalar bent_rebar_color = cv::Scalar(0, 0, 255);

    double x_start = rect_x + stirrup_offset + (8 * scale) + rebar_radius_scaled;
    double x_end = rect_x + rect_w - stirrup_offset - (8 * scale) - rebar_radius_scaled;

    int straight_bars_in_row1 = design.rebarCountRow1 - design.bentRebarCount;
    int bars_in_row2 = (design.rebarRows > 1) ? design.rebarCountRow2 : 0;

    int bottom_row_bar_count = std::max(straight_bars_in_row1, bars_in_row2);
    int second_row_bar_count = std::min(straight_bars_in_row1, bars_in_row2);

    if (bottom_row_bar_count > 0) {
        double y_pos_bottom_row = rect_y + rect_h - stirrup_offset - (8 * scale) - rebar_radius_scaled;
        for (int i = 0; i < bottom_row_bar_count; ++i) {
            double t = (bottom_row_bar_count > 1) ? (double)i / (bottom_row_bar_count - 1) : 0.5;
            cv::circle(image, cv::Point(x_start + t * (x_end - x_start), y_pos_bottom_row), rebar_radius_scaled, rebar_color, -1);
        }
        std::string row1_text = std::to_string(bottom_row_bar_count) + " x d" + std::to_string((int)design.flexureRebarDiameter);
        cv::putText(image, row1_text, cv::Point(rect_x + rect_w + 10, y_pos_bottom_row + 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }

    if (second_row_bar_count > 0) {
        double y_pos_bottom_row = rect_y + rect_h - stirrup_offset - (8 * scale) - rebar_radius_scaled;
        double y_pos_second_row = y_pos_bottom_row - (rebar_radius_scaled * 2) - (25.0 * scale);
        for (int i = 0; i < second_row_bar_count; ++i) {
            double t = (second_row_bar_count > 1) ? (double)i / (second_row_bar_count - 1) : 0.5;
            cv::circle(image, cv::Point(x_start + t * (x_end - x_start), y_pos_second_row), rebar_radius_scaled, rebar_color, -1);
        }
        std::string row2_text = std::to_string(second_row_bar_count) + " x d" + std::to_string((int)design.flexureRebarDiameter);
        cv::putText(image, row2_text, cv::Point(rect_x + rect_w + 10, y_pos_second_row + 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }

    if (design.bentRebarsUsed && design.bentRebarCount > 0) {
        double y_pos_top = rect_y + stirrup_offset + (8 * scale) + rebar_radius_scaled;
        for (int i = 0; i < design.bentRebarCount; ++i) {
            double t = (design.bentRebarCount > 1) ? (double)i / (design.bentRebarCount - 1) : 0.5;
            cv::circle(image, cv::Point(x_start + t * (x_end - x_start), y_pos_top), rebar_radius_scaled, bent_rebar_color, -1);
        }
        std::string bent_text = std::to_string(design.bentRebarCount) + " x d" + std::to_string((int)design.flexureRebarDiameter) + " (Bent)";
        cv::putText(image, bent_text, cv::Point(rect_x + rect_w + 10, y_pos_top + 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }

    cv::putText(image, std::to_string((int)params.width) + "mm", cv::Point(rect_x, rect_y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
    cv::putText(image, std::to_string((int)params.height) + "mm", cv::Point(rect_x - 100, rect_y + rect_h / 2), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

    return image;
}

inline cv::Mat RebarCalc::generateLongitudinalSectionImage() {
    int img_w = 1200, img_h = 400;
    cv::Mat image(img_h, img_w, CV_8UC3, cv::Scalar(255, 255, 255));

    if (params.span <= 0 || params.height <= 0) return image;

    if (!design.designPossible) {
        cv::putText(image, "DESIGN FAILED", cv::Point(50, img_h / 2 - 20), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 0, 255), 3);
        cv::putText(image, design.errorMessage, cv::Point(50, img_h / 2 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 1);
        return image;
    }

    double scale_x = (img_w * 0.9) / params.span;
    double scale_y = (img_h * 0.6) / params.height;
    int rect_w = params.span * scale_x;
    int rect_h = params.height * scale_y;
    int rect_x = (img_w - rect_w) / 2;
    int rect_y = (img_h - rect_h) / 2;

    cv::rectangle(image, cv::Point(rect_x, rect_y), cv::Point(rect_x + rect_w, rect_y + rect_h), cv::Scalar(211, 211, 211), -1);
    cv::rectangle(image, cv::Point(rect_x, rect_y), cv::Point(rect_x + rect_w, rect_y + rect_h), cv::Scalar(0, 0, 0), 1);

    if (design.stirrupSpacing > 0) {
        for (double x_mm = design.stirrupSpacing; x_mm < params.span; x_mm += design.stirrupSpacing) {
            cv::line(image, cv::Point(rect_x + (x_mm * scale_x), rect_y), cv::Point(rect_x + (x_mm * scale_x), rect_y + rect_h), cv::Scalar(128, 128, 128), 1);
        }
    }

    double y_bottom_row1 = rect_y + rect_h - (CONCRETE_COVER * scale_y) - (8.0 * scale_y) - (design.flexureRebarDiameter / 2.0 * scale_y);
    double y_bottom_row2 = y_bottom_row1 - (design.flexureRebarDiameter * scale_y) - (25.0 * scale_y);
    double y_top_section = rect_y + (CONCRETE_COVER * scale_y) + (8.0 * scale_y);
    cv::Scalar rebar_color = rebarColorMap.at(design.flexureRebarDiameter);
    cv::Scalar bent_rebar_color = cv::Scalar(0, 0, 255);

    if (design.rebarCountRow2 > 0) {
        cv::line(image, cv::Point(rect_x, y_bottom_row2), cv::Point(rect_x + rect_w, y_bottom_row2), rebar_color, 2);
    }

    int bent_bar_count = design.bentRebarsUsed ? design.bentRebarCount : 0;
    int straight_bars_in_row1 = design.rebarCountRow1 - bent_bar_count;

    if (straight_bars_in_row1 > 0) {
        cv::line(image, cv::Point(rect_x, y_bottom_row1), cv::Point(rect_x + rect_w, y_bottom_row1), rebar_color, 2);
    }

    if (bent_bar_count > 0) {
        int bend_point_bottom_left_x = rect_x + rect_w / 4;
        int bend_point_bottom_right_x = rect_x + rect_w * 3 / 4;

        double delta_y = y_bottom_row1 - y_top_section;
        double delta_x = delta_y;

        cv::Point p_bottom_left(bend_point_bottom_left_x, y_bottom_row1);
        cv::Point p_top_left(bend_point_bottom_left_x - delta_x, y_top_section);
        cv::Point p_bottom_right(bend_point_bottom_right_x, y_bottom_row1);
        cv::Point p_top_right(bend_point_bottom_right_x + delta_x, y_top_section);

        cv::line(image, cv::Point(rect_x, y_top_section), p_top_left, bent_rebar_color, 3);
        cv::line(image, p_top_left, p_bottom_left, bent_rebar_color, 3);
        cv::line(image, p_bottom_left, p_bottom_right, bent_rebar_color, 3);
        cv::line(image, p_bottom_right, p_top_right, bent_rebar_color, 3);
        cv::line(image, p_top_right, cv::Point(rect_x + rect_w, y_top_section), bent_rebar_color, 3);
    }

    return image;
}

// Auxiliary function to generate geometric parameters based on the span.
inline void autoGeoParams(double span, double& width, double& height) {
    if (span <= 0) return;
    height = std::round(span / 15 / 50) * 50;
    width = std::round(height / 2 / 50) * 50;
    if (height < 400) height = 400;
    if (width < 200) width = 200;
}


#endif // REBARCALC_H
